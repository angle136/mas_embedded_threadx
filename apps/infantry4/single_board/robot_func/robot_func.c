/*
 * DEBUG_INDEX
 * SBUS_SWITCH_DOWN/UP_THRESHOLD: 本文件第 29-30 行
 *   含义：三段开关原始值判定阈值，小于 down 阈值判为下，大于 up 阈值判为上，中间判为中。
 * SBUS_WHEEL_DEAD_ZONE: 本文件第 31 行
 *   含义：ch7 旋钮/拨轮死区，小于该绝对值时认为没有拨动。
 * INFANTRY4_SBUS_USED_CHANNELS: 本文件第 32 行
 *   含义：当前调试页只展示前 10 个 SBUS 通道。
 * infantry4_remote_debug: 本文件第 34 行
 *   含义：遥控调试快照，集中观察通道值、开关位置和命令输出。
 * sw1_raw: 本文件后部的 RemoteControlSet() 内。
 *   含义：ch5 原始值，当前主安全开关。
 * ch3/ch4: 本文件后部的 RemoteControlSet() 内。
 *   含义：分别累计更新 pitch / yaw 目标角度。
 * wheel_raw: 本文件后部的 RemoteControlSet() 内。
 *   含义：ch7 去中心化后的值，当前用于拨弹/旋钮调试。
 * RemoteControlSet(): 本文件后部的核心入口。
 *   含义：本文件核心入口，决定安全停机、yaw/pitch 目标角度累计和发射命令。
 */

#include "robot_func.h"

#include "module_remote.h"
#include "user_lib.h"
#include <stddef.h>
#include <stdint.h>

#define SBUS_SWITCH_DOWN_THRESHOLD 700
#define SBUS_SWITCH_UP_THRESHOLD   1350
#define SBUS_WHEEL_DEAD_ZONE       160
#define INFANTRY4_SBUS_USED_CHANNELS 10U

volatile Infantry4_Remote_Debug_t infantry4_remote_debug;

int16_t CalcOffsetAngle(float getyawangle)
{
    float offset_ecd;

    const float ECD_MAX  = 8191.0f;
    const float ECD_HALF = 4095.5f;

    offset_ecd = getyawangle - YAW_CHASSIS_ALIGN_ECD;

    while (offset_ecd > ECD_HALF) offset_ecd -= ECD_MAX;
    while (offset_ecd < -ECD_HALF) offset_ecd += ECD_MAX;

    return (int16_t)offset_ecd;
}

static sbus_switch_e sbus_switch_3pos(int16_t raw)
{
    if (raw < SBUS_SWITCH_DOWN_THRESHOLD) return sbus_switch_down;
    if (raw > SBUS_SWITCH_UP_THRESHOLD) return sbus_switch_up;
    return sbus_switch_mid;
}

static int16_t sbus_raw_centered(uint8_t ch)
{
    /* Module_Remote_get_channel() 使用 1-based 通道号，这里不要再手动减一。 */
    return Module_Remote_get_channel(ch) - (int16_t)SBUS_CHX_BIAS;
}

static void set_safe_command(Chassis_Ctrl_Cmd_t *chassis_ctrl, Shoot_Ctrl_Cmd_t *shoot_ctrl, Gimbal_Ctrl_Cmd_t *gimbal_ctrl)
{
    gimbal_ctrl->gimbal_mode = gimbal_zero_force;
    gimbal_ctrl->auto_search = 0;

    shoot_ctrl->shoot_mode    = shoot_off;
    shoot_ctrl->friction_mode = friction_off;
    shoot_ctrl->load_mode     = load_stop;

    chassis_ctrl->vx          = 0;
    chassis_ctrl->vy          = 0;
    chassis_ctrl->wz          = 0;
    chassis_ctrl->offset_angle = 0;
    chassis_ctrl->chassis_mode = chassis_zero_force;
}

static void update_remote_debug(uint8_t status, Chassis_Ctrl_Cmd_t *chassis_ctrl, Shoot_Ctrl_Cmd_t *shoot_ctrl, Gimbal_Ctrl_Cmd_t *gimbal_ctrl)
{
    infantry4_remote_debug.update_count++;
    infantry4_remote_debug.remote_status = status;
    infantry4_remote_debug.rc_online     = (status & 0x01) ? 1U : 0U;

    for (uint8_t i = 0; i < INFANTRY4_SBUS_USED_CHANNELS; i++)
    {
        int16_t channel = Module_Remote_get_channel((uint8_t)(i + 1U));
        infantry4_remote_debug.channels[i] = channel;
    }

    infantry4_remote_debug.sw1_raw   = Module_Remote_get_channel(5);
    infantry4_remote_debug.sw2_raw   = Module_Remote_get_channel(6);
    infantry4_remote_debug.wheel_raw = Module_Remote_get_channel(7);
    infantry4_remote_debug.sw1_pos   = (uint8_t)sbus_switch_3pos(infantry4_remote_debug.sw1_raw);
    infantry4_remote_debug.sw2_pos   = (uint8_t)sbus_switch_3pos(infantry4_remote_debug.sw2_raw);

    infantry4_remote_debug.gimbal_yaw    = gimbal_ctrl->yaw;
    infantry4_remote_debug.gimbal_pitch  = gimbal_ctrl->pitch;
    infantry4_remote_debug.gimbal_mode   = (uint8_t)gimbal_ctrl->gimbal_mode;
    infantry4_remote_debug.shoot_mode    = (uint8_t)shoot_ctrl->shoot_mode;
    infantry4_remote_debug.friction_mode = (uint8_t)shoot_ctrl->friction_mode;
    infantry4_remote_debug.load_mode     = (uint8_t)shoot_ctrl->load_mode;

    infantry4_remote_debug.chassis_vx          = chassis_ctrl->vx;
    infantry4_remote_debug.chassis_vy          = chassis_ctrl->vy;
    infantry4_remote_debug.chassis_offset_angle = chassis_ctrl->offset_angle;
    infantry4_remote_debug.chassis_mode        = (uint8_t)chassis_ctrl->chassis_mode;
}

void RemoteControlSet(Chassis_Ctrl_Cmd_t *chassis_ctrl, Shoot_Ctrl_Cmd_t *shoot_ctrl, Gimbal_Ctrl_Cmd_t *gimbal_ctrl)
{
    if (chassis_ctrl == NULL || shoot_ctrl == NULL || gimbal_ctrl == NULL) return;

    uint8_t status = Module_Remote_get_offline_status();

    if ((status & 0x01) == 0U)
    {
        set_safe_command(chassis_ctrl, shoot_ctrl, gimbal_ctrl);
        update_remote_debug(status, chassis_ctrl, shoot_ctrl, gimbal_ctrl);
        return;
    }

    chassis_ctrl->vx = (float)Module_Remote_get_channel(1) / (float)(DT7_CH_VALUE_MAX - DT7_CH_VALUE_MIN);
    chassis_ctrl->vy = (float)Module_Remote_get_channel(2) / (float)(DT7_CH_VALUE_MAX - DT7_CH_VALUE_MIN);
    chassis_ctrl->wz = 0;

    int16_t       sw1_raw   = Module_Remote_get_channel(5);
    int16_t       sw2_raw   = Module_Remote_get_channel(6);
    int16_t       wheel_raw = sbus_raw_centered(7);
    sbus_switch_e sw1       = sbus_switch_3pos(sw1_raw);
    sbus_switch_e sw2       = sbus_switch_3pos(sw2_raw);

    gimbal_ctrl->auto_search = 0;

    if (sw1 == sbus_switch_down)
    {
        set_safe_command(chassis_ctrl, shoot_ctrl, gimbal_ctrl);
    }
    else
    {
        gimbal_ctrl->gimbal_mode = gimbal_gyro_mode;

        if (sw1 == sbus_switch_mid)
        {
            gimbal_ctrl->yaw -= 0.001f * (float)Module_Remote_get_channel(4);
            gimbal_ctrl->pitch += 0.001f * (float)Module_Remote_get_channel(3);
            VAL_LIMIT(gimbal_ctrl->pitch, PITCH_MIN_ANGLE, PITCH_MAX_ANGLE);
        }

        if (sw2 == sbus_switch_mid)
        {
            chassis_ctrl->chassis_mode = chassis_follow_gimbal_yaw;
        }
        else if (sw2 == sbus_switch_down)
        {
            chassis_ctrl->chassis_mode = chassis_manual;
        }
        else
        {
            chassis_ctrl->chassis_mode = chassis_zero_force;
            chassis_ctrl->vx           = 0;
            chassis_ctrl->vy           = 0;
            chassis_ctrl->wz           = 0;
        }

        shoot_ctrl->load_mode = load_stop;
        if (sw1 == sbus_switch_mid)
        {
            shoot_ctrl->shoot_mode    = shoot_off;
            shoot_ctrl->friction_mode = friction_off;
        }
        else
        {
            shoot_ctrl->shoot_mode    = shoot_on;
            shoot_ctrl->friction_mode = friction_on;

            if (wheel_raw > SBUS_WHEEL_DEAD_ZONE)
            {
                shoot_ctrl->load_mode = load_reverse;
            }
            else if (wheel_raw < -SBUS_WHEEL_DEAD_ZONE)
            {
                shoot_ctrl->load_mode = load_burstfire;
            }
        }
    }

    update_remote_debug(status, chassis_ctrl, shoot_ctrl, gimbal_ctrl);
}
