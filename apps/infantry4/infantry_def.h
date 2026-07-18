/*
 * @Author: angle136 angieleevcd@gmail.com
 * @Date: 2026-07-17 20:15:53
 * @LastEditors: angle136 angieleevcd@gmail.com
 * @LastEditTime: 2026-07-18 10:51:54
 * @FilePath: \1c:\Users\engineer\Desktop\code\mas_embedded_threadx\apps\infantry4\infantry_def.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _INFANTRY_DEF_H_
#define _INFANTRY_DEF_H_

#include <stdint.h>

/* 云台机械限位参数，当前临时直控阶段仅保留定义，后续切正式闭环时继续使用。 */
#define PITCH_HORIZON_ANGLE 0.0f
#define PITCH_MAX_ANGLE     40.0f
#define PITCH_MIN_ANGLE     -20.0f

/* 发射机构机械参数，当前 yaw 联调阶段未实际使用，后续接入 loader/shoot 时复用。 */
#define REDUCTION_RATIO_LOADER 36.0f
#define ONE_BULLET_DELTA_ANGLE 60.0f
#define NUM_PER_CIRCLE         6

#pragma pack(1)

typedef enum
{
    gimbal_zero_force = 0,
    gimbal_gyro_mode,
    gimbal_auto_mode,
} gimbal_mode_e;

typedef struct
{
    float         yaw;         /* 当前 infantry4 临时直控里取值为 -1/0/+1，表示 yaw 左转/停止/右转方向命令。 */
    float         pitch;       /* pitch 目标量；当前阶段固定为 0，后续接 pitch 电机后恢复实际含义。 */
    uint8_t       auto_search; /* 自动瞄准/搜索标志；当前阶段恒为 0。 */
    gimbal_mode_e gimbal_mode; /* 云台模式：zero_force=停机，gyro_mode=允许当前临时直控逻辑运行。 */
} Gimbal_Ctrl_Cmd_t;

typedef enum
{
    shoot_off = 0,
    shoot_on,
} shoot_mode_e;

typedef enum
{
    friction_off = 0,
    friction_on,
} friction_mode_e;

typedef enum
{
    load_stop = 0,
    load_reverse,
    load_1_bullet,
    load_burstfire,
} loader_mode_e;

typedef struct
{
    shoot_mode_e    shoot_mode;    /* 发射总使能。 */
    loader_mode_e   load_mode;     /* 拨弹模式。 */
    friction_mode_e friction_mode; /* 摩擦轮使能。 */
} Shoot_Ctrl_Cmd_t;

typedef enum
{
    sbus_switch_down = 0,
    sbus_switch_mid,
    sbus_switch_up,
} sbus_switch_e;

typedef struct
{
    uint32_t update_count;  /* 主控线程更新次数；持续递增表示 robot_func 正在运行。 */
    uint8_t  remote_status; /* 遥控/图传联合在线状态位；bit0=遥控在线，bit1=图传在线。 */
    uint8_t  rc_online;     /* 仅看遥控器是否在线，1=在线。 */
    /* 当前步兵四只关心前 10 个 SBUS 通道，物理 ch8 对应 channels[7]。 */
    int16_t  channels[10];   /* 调试用原始通道值快照；数组 0-based，对应物理 ch1~ch10。 */
    int16_t  sw1_raw;        /* ch5 原始值，当前主安全开关。 */
    int16_t  sw2_raw;        /* ch6 原始值，当前预留/发射相关开关。 */
    int16_t  wheel_raw;      /* ch7 原始值，当前作为拨弹/旋钮输入。 */
    uint8_t  sw1_pos;        /* sw1 三段归一化结果：0=下，1=中，2=上。 */
    uint8_t  sw2_pos;        /* sw2 三段归一化结果：0=下，1=中，2=上。 */
    float    gimbal_yaw;     /* 当前写入 gimbal_cmd.yaw 的值。 */
    float    gimbal_pitch;   /* 当前写入 gimbal_cmd.pitch 的值。 */
    uint8_t  gimbal_mode;    /* 当前写入 gimbal_cmd.gimbal_mode 的值。 */
    uint8_t  shoot_mode;     /* 当前写入 shoot_cmd.shoot_mode 的值。 */
    uint8_t  friction_mode;  /* 当前写入 shoot_cmd.friction_mode 的值。 */
    uint8_t  load_mode;      /* 当前写入 shoot_cmd.load_mode 的值。 */
} Infantry4_Remote_Debug_t;

#pragma pack()

#endif
