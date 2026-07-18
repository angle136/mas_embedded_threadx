/*
 * DEBUG_INDEX
 * SBUS_SWITCH_DOWN/UP_THRESHOLD: 本文件第 29-30 行
 *   含义：三段开关原始值判定阈值，小于 down 阈值判为下，大于 up 阈值判为上，中间判为中。
 * SBUS_WHEEL_DEAD_ZONE: 本文件第 31 行
 *   含义：ch7 旋钮/拨轮死区，小于该绝对值时认为没有拨动。
 * TEMP_INFANTRY4_YAW_RC_DEAD_ZONE: 本文件第 32 行
 *   含义：ch3 遥控打 yaw 的死区，小于该绝对值时认为不转。
 * INFANTRY4_SBUS_USED_CHANNELS: 本文件第 33 行
 *   含义：当前调试页只展示前 10 个 SBUS 通道。
 * infantry4_remote_debug: 本文件第 35 行
 *   含义：遥控调试快照，集中观察通道值、开关位置和命令输出。
 * temp_infantry4_set_yaw_direct_rc_cmd(): 本文件第 56 行
 *   含义：临时函数，把 ch3 映射成 gimbal_ctrl->yaw 的 -1/0/+1 三态。
 * sw1_raw: 本文件第 125 行
 *   含义：ch5 原始值，当前主安全开关。
 * wheel_raw: 本文件第 126 行
 *   含义：ch7 去中心化后的值，当前用于拨弹/旋钮调试。
 * RemoteControlSet(): 本文件第 112 行
 *   含义：本文件核心入口，决定安全停机、yaw 临时直控和发射命令。
 */

#include "robot_func.h"

#include "module_remote.h"
#include <stddef.h>
#include <stdint.h>

#define SBUS_SWITCH_DOWN_THRESHOLD 700
#define SBUS_SWITCH_UP_THRESHOLD   1350
#define SBUS_WHEEL_DEAD_ZONE       160
#define TEMP_INFANTRY4_YAW_RC_DEAD_ZONE 160
#define INFANTRY4_SBUS_USED_CHANNELS 10U

volatile Infantry4_Remote_Debug_t infantry4_remote_debug;

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

/*
 * TEMP_INFANTRY4_YAW_DIRECT_RC
 * 临时联调逻辑：在未接入 infantry3 同款 INS/LQR 闭环前，
 * 直接将 SBUS ch3 映射为 yaw 左/右/停止三态命令。
 * 后续切回正式云台实现时删除本函数，并恢复为目标角度语义。
 */
static void temp_infantry4_set_yaw_direct_rc_cmd(Gimbal_Ctrl_Cmd_t *gimbal_ctrl)
{
    int16_t yaw_channel = Module_Remote_get_channel(3);

    if (yaw_channel > TEMP_INFANTRY4_YAW_RC_DEAD_ZONE)
    {
        gimbal_ctrl->yaw = 1.0f;
    }
    else if (yaw_channel < -TEMP_INFANTRY4_YAW_RC_DEAD_ZONE)
    {
        gimbal_ctrl->yaw = -1.0f;
    }
    else
    {
        gimbal_ctrl->yaw = 0.0f;
    }
}

static void set_safe_command(Shoot_Ctrl_Cmd_t *shoot_ctrl, Gimbal_Ctrl_Cmd_t *gimbal_ctrl)
{
    gimbal_ctrl->gimbal_mode = gimbal_zero_force;
    gimbal_ctrl->auto_search = 0;
    gimbal_ctrl->yaw         = 0.0f;
    gimbal_ctrl->pitch       = 0.0f;

    shoot_ctrl->shoot_mode    = shoot_off;
    shoot_ctrl->friction_mode = friction_off;
    shoot_ctrl->load_mode     = load_stop;
}

static void update_remote_debug(uint8_t status, Shoot_Ctrl_Cmd_t *shoot_ctrl, Gimbal_Ctrl_Cmd_t *gimbal_ctrl)
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
        temp_infantry4_set_yaw_direct_rc_cmd(gimbal_ctrl);
        gimbal_ctrl->pitch = 0.0f;

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
