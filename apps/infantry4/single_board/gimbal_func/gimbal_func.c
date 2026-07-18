/*
 * DEBUG_INDEX
 * yaw_motor: 本文件后部的静态全局指针。
 *   含义：6020 电机句柄，初始化成功后不应为 NULL。
 * ins: 本文件后部的静态全局指针。
 *   含义：yaw 闭环角度反馈来源，复用 infantry3 的 INS 角度。
 * bmi088_dev: 本文件后部的静态全局指针。
 *   含义：yaw 闭环角速度反馈来源，复用 infantry3 的 BMI088 陀螺仪。
 * yaw_config.transport_config.can.hcan / tx_id: gimbal_init() 内。
 *   含义：6020 所在 CAN 总线与电机 ID。
 * yaw_config.controller_init_config.lqr_init: gimbal_init() 内。
 *   含义：yaw 正式闭环的 LQR 参数，直接复用 infantry3。
 * gimbal_func(): 本文件核心入口。
 *   含义：负责安全停机、离线保护和 yaw 闭环执行。
 */

#include "gimbal_func.h"

#include "module_bmi088.h"
#include "module_ins.h"
#include "module_offline.h"
#include "motor_def.h"
#include "motor_dji.h"
#include "user_lib.h"

#include <stddef.h>

#define LOG_TAG "app_infantry4_gimbal"
#define LOG_LVL LOG_LVL_INFO
#include "ulog_def.h"

static DJI_Motor_t *yaw_motor = NULL;
static const Ins_t           *ins        = NULL;
static const Bmi088_device_t *bmi088_dev = NULL;

void gimbal_init(void)
{
    ins = Module_INS_get();
    if (ins == NULL)
    {
        LOG_E("ins is null");
        return;
    }

    bmi088_dev = Module_BMI088_get_device();
    if (bmi088_dev == NULL)
    {
        LOG_E("bmi088_dev is null");
        return;
    }

    Motor_Init_Config_s yaw_config = {
        .offline_init_config =
            {
                .name       = "6020",
                .timeout_ms = 100,
                .beep_times = 3,
                .enable     = 1,
            },
        .transport = MOTOR_TRANSPORT_CAN,
        .transport_config.can =
            {
                .hcan  = BSP_CAN_HANDLE1,
                .tx_id = 1,
            },
        .controller_init_config =
            {
                .other_angle_feedback_ptr = &ins->YawTotalAngle_rad,
                .other_speed_feedback_ptr = &bmi088_dev->gyro[2],
                .lqr_init =
                    {
                        .K         = {5.47f, 0.56f},
                        .state_dim = 2,
                    },
            },
        .setting_init_config =
            {
                .algorithm_type        = CONTROL_LQR,
                .feedback_reverse_flag = 0,
                .angle_feedback_source = 1,
                .speed_feedback_source = 1,
                .loop_type             = ANGLE_LOOP,
            },
        .motor_init_info = {.motor_type = GM6020_CURRENT, .gear_ratio = 1.0f, .max_torque = 2.223f, .torque_constant = 0.741f},
    };

    yaw_motor = Motor_DJI_Init(&yaw_config);
    if (yaw_motor == NULL)
    {
        LOG_E("yaw_motor init failed");
        return;
    }
}

void gimbal_func(Gimbal_Ctrl_Cmd_t *gimbal_cmd, uint16_t *yaw_ecd)
{
    if (yaw_motor == NULL) return;

    if (gimbal_cmd == NULL)
    {
        Motor_DJI_Stop(yaw_motor);
        return;
    }

    if (Module_Offline_get_device_status(yaw_motor->base.offline_dev) == STATE_OFFLINE)
    {
        Motor_DJI_Stop(yaw_motor);
        return;
    }

    switch (gimbal_cmd->gimbal_mode)
    {
    case gimbal_zero_force:
        Motor_DJI_Stop(yaw_motor);
        break;
    case gimbal_gyro_mode:
    case gimbal_auto_mode:
        Motor_DJI_Start(yaw_motor);
        Motor_DJI_SetRef(yaw_motor, gimbal_cmd->yaw * DEGREE_2_RAD);
        break;
    default:
        Motor_DJI_Stop(yaw_motor);
        break;
    }

    if (yaw_ecd != NULL)
    {
        *yaw_ecd = yaw_motor->measure.ecd;
    }
}
