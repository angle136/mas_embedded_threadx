/*
 * @Author: laladuduqq 17503181697@163.com
 * @Date: 2026-04-13 18:08:51
 * @LastEditors: laladuduqq 17503181697@163.com
 * @LastEditTime: 2026-04-15 20:52:37
 * @FilePath: \mas\robot\robot_init.c
 * @Description:
 */

#include "robot_init.h"

#include "utils_init.h"

#define LOG_LVL LOG_LVL_INFO
#define LOG_TAG "Robot_Init"
#include "ulog_def.h"

void Robot_Init(void)
{
    UTILS_Init();

    LOG_I("robot init finished");
}
