#ifndef _INFANTRY_DEF_H_
#define _INFANTRY_DEF_H_

#include <stdint.h>

#define PITCH_HORIZON_ANGLE 0.0f
#define PITCH_MAX_ANGLE     40.0f
#define PITCH_MIN_ANGLE     -20.0f

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
    float         yaw;
    float         pitch;
    uint8_t       auto_search;
    gimbal_mode_e gimbal_mode;
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
    shoot_mode_e    shoot_mode;
    loader_mode_e   load_mode;
    friction_mode_e friction_mode;
} Shoot_Ctrl_Cmd_t;

#pragma pack()

#endif
