#ifndef _ROBOT_FUNC_H_
#define _ROBOT_FUNC_H_

#include "infantry_def.h"

extern volatile Infantry4_Remote_Debug_t infantry4_remote_debug;

int16_t CalcOffsetAngle(float getyawangle);

void RemoteControlSet(Chassis_Ctrl_Cmd_t *chassis_ctrl, Shoot_Ctrl_Cmd_t *shoot_ctrl, Gimbal_Ctrl_Cmd_t *gimbal_ctrl);

#endif
