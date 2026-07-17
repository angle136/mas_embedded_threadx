#include "robot_func.h"

#include "module_remote.h"
#include "user_lib.h"

#include <stddef.h>
#include <stdint.h>

#define SBUS_SWITCH_DOWN_THRESHOLD 700
#define SBUS_SWITCH_UP_THRESHOLD   1350
#define SBUS_WHEEL_DEAD_ZONE       160
#define GIMBAL_CHANNEL_SCALE       0.001f

/*
 * Ozone 调试变量在本文件第 30 行：infantry4_remote_debug。
 * 展开 infantry4_remote_debug 后重点观察以下字段：
 * - rc_online：1 表示 SBUS 遥控器在线。
 * - channels[0..15]：SBUS 第 1 到 16 通道数值。
 * - sw1_raw / sw2_raw / wheel_raw：开关或旋钮原始通道值。
 * - sw1_pos / sw2_pos：三段开关归一化结果，0=下，1=中，2=上。
 * - update_count：主控线程运行时应持续递增。
 * 通道约定：
 * - ch1 / ch2：左摇杆，暂时不用，后面给底盘或保留。
 * - ch3 / ch4：右摇杆，控制云台 yaw / pitch。
 * - ch5：主安全拨杆。
 * - ch6：发射/摩擦轮使能拨杆。
 * - ch7：拨弹模式拨杆。
 * - ch8：备用模式拨杆。
 */
volatile Infantry4_Remote_Debug_t infantry4_remote_debug;

static sbus_switch_e sbus_switch_3pos(int16_t raw)
{
    if (raw < SBUS_SWITCH_DOWN_THRESHOLD) return sbus_switch_down;
    if (raw > SBUS_SWITCH_UP_THRESHOLD) return sbus_switch_up;
    return sbus_switch_mid;
}

static int16_t sbus_raw_centered(uint8_t ch)
{
    return Module_Remote_get_channel(ch) - (int16_t)SBUS_CHX_BIAS;
}

static void set_safe_command(Shoot_Ctrl_Cmd_t *shoot_ctrl, Gimbal_Ctrl_Cmd_t *gimbal_ctrl)
{
    gimbal_ctrl->gimbal_mode = gimbal_zero_force;
    gimbal_ctrl->auto_search = 0;

    shoot_ctrl->shoot_mode    = shoot_off;
    shoot_ctrl->friction_mode = friction_off;
    shoot_ctrl->load_mode     = load_stop;
}

static void update_remote_debug(uint8_t status, Shoot_Ctrl_Cmd_t *shoot_ctrl, Gimbal_Ctrl_Cmd_t *gimbal_ctrl)
{
    infantry4_remote_debug.update_count++;
    infantry4_remote_debug.remote_status = status;
    infantry4_remote_debug.rc_online     = (status & 0x01) ? 1U : 0U;

    for (uint8_t i = 0; i < 16; i++)
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
}

void RemoteControlSet(Shoot_Ctrl_Cmd_t *shoot_ctrl, Gimbal_Ctrl_Cmd_t *gimbal_ctrl)
{
    if (shoot_ctrl == NULL || gimbal_ctrl == NULL) return;

    uint8_t status = Module_Remote_get_offline_status();

    if ((status & 0x01) == 0U)
    {
        set_safe_command(shoot_ctrl, gimbal_ctrl);
        update_remote_debug(status, shoot_ctrl, gimbal_ctrl);
        return;
    }

    int16_t       sw1_raw   = Module_Remote_get_channel(5);
    int16_t       wheel_raw = sbus_raw_centered(7);
    sbus_switch_e sw1       = sbus_switch_3pos(sw1_raw);

    gimbal_ctrl->auto_search = 0;

    if (sw1 == sbus_switch_down)
    {
        set_safe_command(shoot_ctrl, gimbal_ctrl);
    }
    else
    {
        gimbal_ctrl->gimbal_mode = gimbal_gyro_mode;
        gimbal_ctrl->yaw -= GIMBAL_CHANNEL_SCALE * (float)Module_Remote_get_channel(3);
        gimbal_ctrl->pitch += GIMBAL_CHANNEL_SCALE * (float)Module_Remote_get_channel(4);
        VAL_LIMIT(gimbal_ctrl->pitch, PITCH_MIN_ANGLE, PITCH_MAX_ANGLE);

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

    update_remote_debug(status, shoot_ctrl, gimbal_ctrl);
}
