#ifndef _ULOG_DEF_H_
#define _ULOG_DEF_H_

#include <stdbool.h>
#include <stddef.h>

#include "ulog.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =============================================================================
 * 日志级别定义
 * ============================================================================= */

#define LOG_LVL_ASSERT  0
#define LOG_LVL_ERROR   3
#define LOG_LVL_WARNING 4
#define LOG_LVL_INFO    6
#define LOG_LVL_DBG     7

/* =============================================================================
 * 配置宏
 * ============================================================================= */

/* 静态输出日志级别设置 */
#ifndef ULOG_OUTPUT_LVL
#define ULOG_OUTPUT_LVL LOG_LVL_DBG
#endif

/* 每行日志的缓冲区大小 */
#ifndef ULOG_LINE_BUF_SIZE
#define ULOG_LINE_BUF_SIZE 128
#endif

/* 换行符 */
#ifndef ULOG_NEWLINE_SIGN
#define ULOG_NEWLINE_SIGN "\r\n"
#endif

/* =============================================================================
 * 输出内容控制
 * ============================================================================= */

#define ULOG_OUTPUT_TIME        1
#define ULOG_OUTPUT_LEVEL       1
#define ULOG_OUTPUT_TAG         1
#define ULOG_OUTPUT_THREAD_NAME 1

/* =============================================================================
 * 颜色输出配置
 * ============================================================================= */

#define ULOG_USING_COLOR        1
/* CSI（控制序列引导符）
 * 更多信息见 https://en.wikipedia.org/wiki/ANSI_escape_code */
#define CSI_START               "\033["
#define CSI_END                 "\033[0m"

/* 日志颜色 */
#define F_GREEN                 "32m"
#define F_YELLOW                "33m"
#define F_RED                   "31m"
#define F_MAGENTA               "35m"

/* 日志默认颜色定义 */
#ifndef ULOG_COLOR_DEBUG
#define ULOG_COLOR_DEBUG NULL
#endif

#ifndef ULOG_COLOR_INFO
#define ULOG_COLOR_INFO (F_GREEN)
#endif

#ifndef ULOG_COLOR_WARN
#define ULOG_COLOR_WARN (F_YELLOW)
#endif

#ifndef ULOG_COLOR_ERROR
#define ULOG_COLOR_ERROR (F_RED)
#endif

#ifndef ULOG_COLOR_ASSERT
#define ULOG_COLOR_ASSERT (F_MAGENTA)
#endif

/* =============================================================================
 * 断言配置
 * ============================================================================= */

#ifdef ULOG_ASSERT_ENABLE
#define ULOG_ASSERT(EXPR)                                                                                                                            \
    if (!(EXPR))                                                                                                                                     \
    {                                                                                                                                                \
        ulog_output(LOG_LVL_ASSERT, LOG_TAG, true, "(%s) has assert failed at %s:%ld.", #EXPR, __FUNCTION__, __LINE__);                              \
        ulog_flush();                                                                                                                                \
        while (1);                                                                                                                                   \
    }
#else
#define ULOG_ASSERT(EXPR)
#endif

/* 断言 API 定义 */
#if !defined(ASSERT)
#define ASSERT ULOG_ASSERT
#endif

/* =============================================================================
 * LOG_TAG 和 LOG_LVL 默认值
 * ============================================================================= */

#if !defined(LOG_TAG)
#if defined(DBG_TAG)
#define LOG_TAG DBG_TAG
#elif defined(DBG_SECTION_NAME)
#define LOG_TAG DBG_SECTION_NAME
#else
#define LOG_TAG "NO_TAG"
#endif
#endif /* !defined(LOG_TAG) */

#if !defined(LOG_LVL)
#if defined(DBG_LVL)
#define LOG_LVL DBG_LVL
#elif defined(DBG_LEVEL)
#define LOG_LVL DBG_LEVEL
#else
#define LOG_LVL LOG_LVL_DBG
#endif
#endif /* !defined(LOG_LVL) */

/* =============================================================================
 * 日志输出宏（ulog_x 接口）
 * ============================================================================= */

#if (LOG_LVL >= LOG_LVL_DBG) && (ULOG_OUTPUT_LVL >= LOG_LVL_DBG)
#define ulog_d(TAG, ...) ulog_output(LOG_LVL_DBG, TAG, true, __VA_ARGS__)
#else
#define ulog_d(TAG, ...)
#endif

#if (LOG_LVL >= LOG_LVL_INFO) && (ULOG_OUTPUT_LVL >= LOG_LVL_INFO)
#define ulog_i(TAG, ...) ulog_output(LOG_LVL_INFO, TAG, true, __VA_ARGS__)
#else
#define ulog_i(TAG, ...)
#endif

#if (LOG_LVL >= LOG_LVL_WARNING) && (ULOG_OUTPUT_LVL >= LOG_LVL_WARNING)
#define ulog_w(TAG, ...) ulog_output(LOG_LVL_WARNING, TAG, true, __VA_ARGS__)
#else
#define ulog_w(TAG, ...)
#endif

#if (LOG_LVL >= LOG_LVL_ERROR) && (ULOG_OUTPUT_LVL >= LOG_LVL_ERROR)
#define ulog_e(TAG, ...) ulog_output(LOG_LVL_ERROR, TAG, true, __VA_ARGS__)
#else
#define ulog_e(TAG, ...)
#endif

#if (LOG_LVL >= LOG_LVL_DBG) && (ULOG_OUTPUT_LVL >= LOG_LVL_DBG)
#define ulog_hex(TAG, width, buf, size) ulog_hexdump(TAG, width, buf, size)
#else
#define ulog_hex(TAG, width, buf, size)
#endif

/* 对外宏定义使用接口 */

#define LOG_E(...)                      ulog_e(LOG_TAG, __VA_ARGS__)
#define LOG_W(...)                      ulog_w(LOG_TAG, __VA_ARGS__)
#define LOG_I(...)                      ulog_i(LOG_TAG, __VA_ARGS__)
#define LOG_D(...)                      ulog_d(LOG_TAG, __VA_ARGS__)
#define LOG_RAW(...)                    ulog_raw(__VA_ARGS__)
#define LOG_HEX(name, width, buf, size) ulog_hex(name, width, buf, size)

#ifdef __cplusplus
}
#endif

#endif /* _ULOG_DEF_H_ */
