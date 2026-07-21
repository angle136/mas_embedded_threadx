/*
 * @Description: 云台功能实现 — F103 测试用单 DM4310 (gimbal yaw)
 *
 *   1. gimbal_init(): 初始化达妙电机 DM4310, MIT 模式
 *   2. gimbal_func(): 根据 gimbal_cmd 设置电机转矩参考
 */

#include "gimbal_func.h"
#include "module_motor.h"
#include "motor_damiao.h"

#define LOG_TAG "Gimbal"
#define LOG_LVL LOG_LVL_INFO
#include "ulog_def.h"

static DM_Motor_t *yaw_motor = NULL;

void gimbal_init(void)
{
    Motor_Init_Config_s cfg = {
        .motor_init_info.motor_type = DM4310,
        .motor_init_info.gear_ratio = 1.0f,
        .motor_init_info.torque_constant = 0.0f,
        .motor_init_info.max_torque = 10.0f,
        .transport = MOTOR_TRANSPORT_CAN,
        .transport_config.can = {
            .hcan = BSP_CAN_HANDLE1,
            .tx_id = 0x001,
            .rx_id = 0x011,
        },
        .setting_init_config = {
            .loop_type = SPEED_LOOP,
            .enableflag = 0,
            .algorithm_type = CONTROL_PID,
            .motor_reverse_flag = 0,
            .feedback_reverse_flag = 0,
            .angle_feedback_source = 0,
            .speed_feedback_source = 0,
        },
        .controller_init_config = {
            .speed_PID = {
                .Kp = 0.5f,
                .Ki = 0.05f,
                .Kd = 0.0f,
                .MaxOut = 10.0f,
                .DeadBand = 0.0f,
                .IntegralLimit = 3.0f,
                .Improve = PID_Integral_Limit,
            },
            .angle_PID = {0},
        },
        .offline_init_config = {
            .name = "gimbal_yaw",
            .timeout_ms = 100,
            .beep_times = 1,
            .enable = 1,
        },
    };

    yaw_motor = Motor_DM_Init(&cfg, DM_MIT_MODE);
    if (yaw_motor != NULL)
    {
        Motor_DM_Start(yaw_motor);
        Motor_DM_SetRef(yaw_motor, 0.0f);
        LOG_I("gimbal yaw DM4310 initialized (CAN1 ID=0x001)");
    }
    else
    {
        LOG_E("gimbal yaw DM4310 init FAILED");
    }
}

void gimbal_func(Gimbal_Ctrl_Cmd_t *gimbal_cmd, uint16_t *yaw_ecd)
{
    if (yaw_motor == NULL) return;

    if (gimbal_cmd == NULL) return;

    switch (gimbal_cmd->gimbal_mode)
    {
    case gimbal_zero_force:
        Motor_DM_SetRef(yaw_motor, 0.0f);
        break;

    case gimbal_gyro_mode:
        /* 根据云台命令的 yaw 角度偏差计算转矩参考 */
        Motor_DM_SetRef(yaw_motor, gimbal_cmd->yaw);
        break;

    case gimbal_auto_mode:
        /* 自动瞄准模式 — 后续实现 */
        Motor_DM_SetRef(yaw_motor, gimbal_cmd->yaw);
        break;

    default:
        Motor_DM_SetRef(yaw_motor, 0.0f);
        break;
    }

    /* 回传当前编码器值 (从电机反馈中读取) */
    if (yaw_ecd != NULL)
    {
        /* DM 电机单圈角度 [0, 2*PI) → 编码器值 0~8191 */
        float rad = yaw_motor->base.measure.single_round_angle;
        if (rad < 0) rad += 6.283185f;
        *yaw_ecd = (uint16_t)(rad / 6.283185f * 8191.0f);
    }
}
