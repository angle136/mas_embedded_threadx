/*
 * robot_control.c
 *
 * F103C8T6 单板测试 — 达妙电机云台控制
 * 不含遥控器/PS2 逻辑, 云台初始零转矩保持
 */

#include "app_init.h"
#include "robot_func.h"
#include "gimbal_func.h"

#define LOG_TAG "Robot"
#define LOG_LVL LOG_LVL_INFO
#include "ulog_def.h"

static Gimbal_Ctrl_Cmd_t gimbal_ctrl_cmd = {0};
static uint16_t          yaw_ecd         = 0;

void robot_control_init(void)
{
    gimbal_init();
    LOG_I("robot control init done");
}

void robot_control_loop(void)
{
    /* 获取遥控器命令 (后续添加) */
    // RemoteControlSet(NULL, NULL, &gimbal_ctrl_cmd);

    /* 执行云台控制 */
    gimbal_func(&gimbal_ctrl_cmd, &yaw_ecd);

    (void)yaw_ecd;
}

void robot_control_exit(void)
{
    gimbal_ctrl_cmd.gimbal_mode = gimbal_zero_force;
    gimbal_func(&gimbal_ctrl_cmd, NULL);
    LOG_I("robot control exit");
}
