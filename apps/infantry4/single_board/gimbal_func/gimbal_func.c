/*
 * DEBUG_INDEX
 * TEMP_INFANTRY4_YAW_DIR_SIGN: 本文件第 34 行
 *   含义：yaw 方向符号开关；1 表示当前方向，-1 表示整体反向。
 * TEMP_INFANTRY4_YAW_DIRECT_SPEED_RAD_PER_SEC: 本文件第 35 行
 *   含义：临时直控下的固定 yaw 角速度，值越小越安全。
 * yaw_motor: 本文件第 37 行
 *   含义：6020 电机句柄，初始化成功后不应为 NULL。
 * temp_infantry4_yaw_direct_rc_control(): 本文件第 45 行
 *   含义：临时函数，把 gimbal_cmd->yaw 的方向命令转换成 6020 速度参考。
 * yaw_config.transport_config.can.hcan: 本文件第 71 行
 *   含义：6020 所在 CAN 总线。
 * yaw_config.transport_config.can.tx_id: 本文件第 72 行
 *   含义：6020 的电机 ID。
 * yaw_config.controller_init_config.speed_PID: 本文件第 76-83 行
 *   含义：当前临时直控使用的速度环 PID 参数。
 * gimbal_func(): 本文件第 104 行
 *   含义：本文件核心入口，负责安全停机、离线保护和 yaw 执行。
 */

#include "gimbal_func.h"

#include "module_offline.h"
#include "motor_def.h"
#include "motor_dji.h"

#include <stddef.h>

#define LOG_TAG "app_infantry4_gimbal"
#define LOG_LVL LOG_LVL_INFO
#include "ulog_def.h"

/* 临时联调用较温和但可起转的 yaw 直控速度；若仍不动，再优先看 output/ecd，而不是继续盲目加速。 */
#define TEMP_INFANTRY4_YAW_DIR_SIGN              1.0f
#define TEMP_INFANTRY4_YAW_DIRECT_SPEED_RAD_PER_SEC 6.0f

static DJI_Motor_t *yaw_motor = NULL;

/*
 * TEMP_INFANTRY4_YAW_DIRECT_RC
 * 临时联调逻辑：在未接入 infantry3 同款 INS/LQR 闭环前，
 * 将 gimbal_cmd->yaw 视为 -1/0/+1 的方向命令，直接驱动 6020 做速度闭环。
 * 后续切回正式云台实现时删除本函数，并恢复为目标角度语义。
 */
static void temp_infantry4_yaw_direct_rc_control(const Gimbal_Ctrl_Cmd_t *gimbal_cmd)
{
    float yaw_speed_ref = 0.0f;

    if (gimbal_cmd != NULL)
    {
        yaw_speed_ref = TEMP_INFANTRY4_YAW_DIR_SIGN * gimbal_cmd->yaw * TEMP_INFANTRY4_YAW_DIRECT_SPEED_RAD_PER_SEC;
    }

    Motor_DJI_Start(yaw_motor);
    Motor_DJI_SetRef(yaw_motor, yaw_speed_ref);
}

void gimbal_init(void)
{
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
                .speed_PID =
                    {
                        .Kp       = 0.08f,
                        .Ki       = 0.0f,
                        .Kd       = 0.0f,
                        .MaxOut   = 1.0f,
                        .DeadBand = 0.0f,
                    },
            },
        .setting_init_config =
            {
                .angle_feedback_source = 0,
                .speed_feedback_source = 0,
                .loop_type             = SPEED_LOOP,
                .feedback_reverse_flag = 0,
                .algorithm_type        = CONTROL_PID,
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

    if (gimbal_cmd->gimbal_mode == gimbal_zero_force)
    {
        Motor_DJI_Stop(yaw_motor);
    }
    else
    {
        temp_infantry4_yaw_direct_rc_control(gimbal_cmd);
    }

    if (yaw_ecd != NULL)
    {
        *yaw_ecd = yaw_motor->measure.ecd;
    }
}
