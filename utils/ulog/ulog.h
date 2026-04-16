/*
 * @Author: laladuduqq 17503181697@163.com
 * @Date: 2026-04-13 12:10:11
 * @LastEditors: laladuduqq 17503181697@163.com
 * @LastEditTime: 2026-04-14 23:20:04
 * @FilePath: \mas\utils\ulog\ulog.h
 * @Description:
 */
#ifndef _ULOG_H_
#define _ULOG_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 ulog 模块。
 *
 * @return int 成功返回 0，失败返回负数。
 *
 * @note 使用 ulog 前必须先调用此函数。
 */
int ulog_init(void);

/**
 * @brief 反初始化 ulog 模块。
 */
void ulog_deinit(void);

/**
 * @brief 输出格式化日志消息。
 *
 * @param level   日志级别（LOG_LVL_ASSERT、LOG_LVL_ERROR 等）
 * @param tag     日志标签字符串
 * @param newline true 添加换行符，false 不添加
 * @param format  printf 风格的格式字符串
 * @param ...     可变参数
 */
void ulog_output(uint32_t level, const char *tag, bool newline, const char *format, ...);

/**
 * @brief 使用 va_list 输出格式化日志消息。
 *
 * @param level   日志级别
 * @param tag     日志标签字符串
 * @param newline true 添加换行符，false 不添加
 * @param format  printf 风格的格式字符串
 * @param args    可变参数列表
 */
void ulog_voutput(uint32_t level, const char *tag, bool newline, const char *format, va_list args);

/**
 * @brief 输出原始字符串（无格式化处理）。
 *
 * @param format  printf 风格的格式字符串
 * @param ...     可变参数
 */
void ulog_raw(const char *format, ...);

/**
 * @brief 以十六进制格式转储数据。
 *
 * @param tag   日志标签字符串
 * @param width 每行字节数（通常为 16 或 32）
 * @param buf   要转储的数据缓冲区
 * @param size  数据缓冲区大小（字节）
 */
void ulog_hexdump(const char *tag, uint32_t width, const uint8_t *buf, uint32_t size);

/**
 * @brief 刷新挂起的日志输出。
 */
void ulog_flush(void);

/**
 * @brief 设置全局日志级别过滤器。
 *
 * @param level 最小输出日志级别（LOG_LVL_ERROR、LOG_LVL_WARNING 等）
 */
void ulog_set_level(uint32_t level);

/**
 * @brief 获取当前全局日志级别过滤器。
 *
 * @return uint32_t 当前日志级别
 */
uint32_t ulog_get_level(void);

#ifdef __cplusplus
}
#endif

#endif /* _ULOG_H_ */
