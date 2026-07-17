#include "robot_control.h"

#include "bsp_def.h"
#include "gimbal_func.h"
#include "robot_func.h"
#include "shoot_func.h"
#include "tx_api.h"

#define LOG_TAG "app_infantry4"
#define LOG_LVL LOG_LVL_INFO
#include "ulog_def.h"

static TX_THREAD                  robot_control_thread;
APPS_STACK_SECTION static uint8_t robot_control_thread_stack[1024];

static Gimbal_Ctrl_Cmd_t gimbal_cmd;
static Shoot_Ctrl_Cmd_t  shoot_cmd;
static uint16_t          yaw_ecd;

static void robot_control_task(ULONG thread_input)
{
    (void)thread_input;

    while (1)
    {
        /* REMOTE module owns the decode thread; this app thread only consumes decoded data. */
        RemoteControlSet(&shoot_cmd, &gimbal_cmd);
        gimbal_func(&gimbal_cmd, &yaw_ecd);
        shoot_func(&shoot_cmd);
        tx_thread_sleep(2);
    }
}

void robot_control_init(void)
{
    gimbal_init();
    shoot_init();

    UINT status = tx_thread_create(&robot_control_thread, "robot_control_thread", robot_control_task, 0, robot_control_thread_stack,
                                   sizeof(robot_control_thread_stack), 30, 30, TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS)
    {
        LOG_E("robot_control_task failed");
        return;
    }

    LOG_I("robot_control init success");
}
