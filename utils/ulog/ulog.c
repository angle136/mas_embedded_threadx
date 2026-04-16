#include <stdarg.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "ulog.h"
#include "ulog_def.h"
#include "SEGGER_RTT.h"

/* HAL_GetTick 外部声明） */
extern uint32_t HAL_GetTick(void);

#include "tx_api.h"

/*
 * 中断检测（ARM Cortex-M）
 * 读取 IPSR（中断程序状态寄存器）检测中断上下文。
 * 返回 0 表示线程模式，非零表示在中断服务程序。
 */
static inline uint32_t ulog_get_ipsr(void)
{
    uint32_t result;
    __asm__ volatile("mrs %0, ipsr" : "=r"(result));
    return result;
}

static inline bool ulog_in_isr(void) { return ulog_get_ipsr() != 0; }

/* 颜色输出信息 */
#if ULOG_LINE_BUF_SIZE < 80
#error "日志行缓冲区大小必须大于 80"
#endif

#ifdef ULOG_OUTPUT_LEVEL
/* 日志级别输出信息 */
static const char *const level_output_info[] = {
    "A/", NULL, NULL, "E/", "W/", NULL, "I/", "D/",
};
#endif /* ULOG_OUTPUT_LEVEL */

#ifdef ULOG_USING_COLOR
/* 颜色输出信息 */
static const char *const color_output_info[] = {
    ULOG_COLOR_ASSERT, NULL, NULL, ULOG_COLOR_ERROR, ULOG_COLOR_WARN, NULL, ULOG_COLOR_INFO, ULOG_COLOR_DEBUG,
};
#endif /* ULOG_USING_COLOR */

/* Ulog 控制块 */
static struct
{
    bool     init_ok;
    uint32_t level;                               /* 全局日志级别过滤器 */
    char     log_buf_th[ULOG_LINE_BUF_SIZE + 1];  /* 线程上下文缓冲区 */
    char     log_buf_isr[ULOG_LINE_BUF_SIZE + 1]; /* 中断上下文缓冲区 */
} ulog = {0};

/* 内部辅助函数 */

/**
 * @brief 根据上下文获取适当的日志缓冲区。
 *
 * @return char* 日志缓冲区指针。
 */
static inline char *get_log_buf(void)
{
    if (ulog_in_isr())
    {
        return ulog.log_buf_isr;
    }
    return ulog.log_buf_th;
}

/**
 * @brief 带长度跟踪的安全字符串拷贝。
 *
 * @param cur_len 目标缓冲区当前长度
 * @param dst     目标缓冲区指针
 * @param src     源字符串指针
 * @return size_t 拷贝的字符数
 */
static size_t ulog_strcpy(size_t cur_len, char *dst, const char *src)
{
    const char *src_old = src;

    if (dst == NULL || src == NULL)
    {
        return 0;
    }

    while (*src != 0)
    {
        /* 确保目标缓冲区有足够空间 */
        if (cur_len++ < ULOG_LINE_BUF_SIZE)
        {
            *dst++ = *src++;
        }
        else
        {
            break;
        }
    }
    return src - src_old;
}

/**
 * @brief 将无符号长整数转换为字符串。
 *
 * @param str 输出缓冲区指针
 * @param num 要转换的数字
 * @return size_t 输出字符串长度
 */
static size_t ulog_ultoa(char *str, unsigned long int num)
{
    size_t left_idx  = 0;
    size_t right_idx = 0;
    size_t len       = 0;
    char   swap;

    if (str == NULL)
    {
        return 0;
    }

    do
    {
        str[len++] = (num % 10) + '0';
    } while (num /= 10);
    str[len] = '\0';

    /* 反转字符串 */
    for (left_idx = 0, right_idx = len - 1; left_idx < right_idx; ++left_idx, --right_idx)
    {
        swap           = str[left_idx];
        str[left_idx]  = str[right_idx];
        str[right_idx] = swap;
    }
    return len;
}

/* 日志格式化函数 */

/**
 * @brief 格式化日志头部（时间戳、级别、标签、线程名、颜色）。
 *
 * @param log_buf 输出缓冲区
 * @param level   日志级别
 * @param tag     日志标签
 * @return size_t 格式化后的头部长度
 */
static size_t ulog_head_formater(char *log_buf, uint32_t level, const char *tag)
{
    size_t log_len = 0;

    if (log_buf == NULL || tag == NULL || level > LOG_LVL_DBG)
    {
        return 0;
    }

#ifdef ULOG_USING_COLOR
    /* 添加 CSI 起始标记和颜色信息 */
    if (color_output_info[level] != NULL)
    {
        log_len += ulog_strcpy(log_len, log_buf + log_len, CSI_START);
        log_len += ulog_strcpy(log_len, log_buf + log_len, color_output_info[level]);
    }
#endif /* ULOG_USING_COLOR */

    log_buf[log_len] = '\0';

#ifdef ULOG_OUTPUT_TIME
    /* 添加时间信息，格式为 [秒.毫秒] */
    {
        uint32_t tick    = HAL_GetTick();
        uint32_t seconds = tick / 1000;
        uint32_t millis  = tick % 1000;
        size_t   tick_len;

        log_buf[log_len]                = '[';
        tick_len                        = ulog_ultoa(log_buf + log_len + 1, seconds);
        log_buf[log_len + 1 + tick_len] = '.';
        /* 将毫秒填充为 3 位 */
        if (millis < 100)
        {
            log_buf[log_len + 2 + tick_len] = '0';
            if (millis < 10)
            {
                log_buf[log_len + 3 + tick_len] = '0';
                tick_len += ulog_ultoa(log_buf + log_len + 4 + tick_len, millis) + 3;
            }
            else
            {
                tick_len += ulog_ultoa(log_buf + log_len + 3 + tick_len, millis) + 2;
            }
        }
        else
        {
            tick_len += ulog_ultoa(log_buf + log_len + 2 + tick_len, millis) + 1;
        }
        log_buf[log_len + 1 + tick_len] = ']';
        log_buf[log_len + 2 + tick_len] = '\0';
        log_len += strlen(log_buf + log_len);
    }
#endif /* ULOG_OUTPUT_TIME */

#ifdef ULOG_OUTPUT_LEVEL
#ifdef ULOG_OUTPUT_TIME
    log_len += ulog_strcpy(log_len, log_buf + log_len, " ");
#endif
    /* 添加级别信息 */
    log_len += ulog_strcpy(log_len, log_buf + log_len, level_output_info[level]);
#endif /* ULOG_OUTPUT_LEVEL */

#ifdef ULOG_OUTPUT_TAG
#if !defined(ULOG_OUTPUT_LEVEL) && defined(ULOG_OUTPUT_TIME)
    log_len += ulog_strcpy(log_len, log_buf + log_len, " ");
#endif
    /* 添加标签信息 */
    log_len += ulog_strcpy(log_len, log_buf + log_len, tag);
#endif /* ULOG_OUTPUT_TAG */

#ifdef ULOG_OUTPUT_THREAD_NAME
    /* 添加线程信息 */
    {
#if defined(ULOG_OUTPUT_TIME) || defined(ULOG_OUTPUT_LEVEL) || defined(ULOG_OUTPUT_TAG)
        log_len += ulog_strcpy(log_len, log_buf + log_len, " ");
#endif
        /* 检查是否处于中断上下文 */
        if (!ulog_in_isr())
        {
            /*
             * tx_thread_identify() 返回当前线程控制块指针。
             * 在内核启动前（tx_application_define 阶段）或从 ISR 调用时返回 TX_NULL。
             * 此处已排除 ISR 路径，故 TX_NULL 意味着内核尚未调度任何线程。
             */
            TX_THREAD *p_thread = tx_thread_identify();
            if (p_thread != TX_NULL)
            {
                const char *thread_name = p_thread->tx_thread_name;
                if (thread_name != NULL && thread_name[0] != '\0')
                {
                    log_len += ulog_strcpy(log_len, log_buf + log_len, thread_name);
                }
                else
                {
                    log_len += ulog_strcpy(log_len, log_buf + log_len, "N/A");
                }
            }
            else
            {
                /* 内核未启动或处于初始化阶段 */
                log_len += ulog_strcpy(log_len, log_buf + log_len, "Pre-Init");
            }
        }
        else
        {
            log_len += ulog_strcpy(log_len, log_buf + log_len, "ISR");
        }
    }
#endif /* ULOG_OUTPUT_THREAD_NAME */

    log_len += ulog_strcpy(log_len, log_buf + log_len, ": ");

    return log_len;
}

/**
 * @brief 格式化日志尾部（换行和颜色重置）。
 *
 * @param log_buf 输出缓冲区
 * @param log_len 当前日志长度
 * @param newline 是否添加换行
 * @param level   日志级别（用于颜色重置）
 * @return size_t 格式化后的总长度
 */
static size_t ulog_tail_formater(char *log_buf, size_t log_len, bool newline, uint32_t level)
{
    size_t newline_len;

    if (log_buf == NULL)
    {
        return 0;
    }

    newline_len = strlen(ULOG_NEWLINE_SIGN);

    /* 溢出检查，为 CSI 结束标记、换行符和空终止符预留空间 */
#ifdef ULOG_USING_COLOR
    size_t reserve = (color_output_info[level] != NULL) ? (sizeof(CSI_END) - 1) : 0;
    if (log_len + reserve + newline_len + 1 > ULOG_LINE_BUF_SIZE)
    {
        log_len = ULOG_LINE_BUF_SIZE - reserve - newline_len - 1;
#else
    if (log_len + newline_len + 1 > ULOG_LINE_BUF_SIZE)
    {
        log_len = ULOG_LINE_BUF_SIZE - newline_len - 1;
#endif
    }

    /* 添加换行符 */
    if (newline)
    {
        log_len += ulog_strcpy(log_len, log_buf + log_len, ULOG_NEWLINE_SIGN);
    }

#ifdef ULOG_USING_COLOR
    /* 添加 CSI 结束标记 */
    if (color_output_info[level] != NULL)
    {
        log_len += ulog_strcpy(log_len, log_buf + log_len, CSI_END);
    }
#endif /* ULOG_USING_COLOR */

    /* 添加字符串结束标记 */
    log_buf[log_len] = '\0';

    return log_len;
}

/**
 * @brief 格式化完整日志消息。
 *
 * @param log_buf 输出缓冲区
 * @param level   日志级别
 * @param tag     日志标签
 * @param newline 是否添加换行
 * @param format  printf 风格的格式字符串
 * @param args    可变参数列表
 * @return size_t 格式化后的总长度
 */
static size_t ulog_formater(char *log_buf, uint32_t level, const char *tag, bool newline, const char *format, va_list args)
{
    size_t log_len;
    int    fmt_result;

    if (log_buf == NULL || format == NULL)
    {
        return 0;
    }

    /* 日志头部 */
    log_len = ulog_head_formater(log_buf, level, tag);

    /* 日志内容 */
    fmt_result = vsnprintf(log_buf + log_len, ULOG_LINE_BUF_SIZE - log_len, format, args);

    /* 计算日志长度 */
    if ((log_len + fmt_result <= ULOG_LINE_BUF_SIZE) && (fmt_result > -1))
    {
        log_len += fmt_result;
    }
    else
    {
        /* 使用最大长度 */
        log_len = ULOG_LINE_BUF_SIZE;
    }

    /* 日志尾部 */
    return ulog_tail_formater(log_buf, log_len, newline, level);
}

/**
 * @brief 格式化十六进制转储日志。
 *
 * @param log_buf 输出缓冲区
 * @param tag     日志标签
 * @param buf     数据缓冲区
 * @param size    数据大小
 * @param width   每行字节数
 * @param addr    显示的起始地址
 * @return size_t 格式化后的总长度
 */
static size_t ulog_hex_formater(char *log_buf, const char *tag, const uint8_t *buf, size_t size, size_t width, uint32_t addr)
{
#define _is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')

    size_t log_len;
    size_t idx;
    int    fmt_result;
    char   dump_string[8];

    if (log_buf == NULL || buf == NULL || tag == NULL)
    {
        return 0;
    }

    /* 日志头部 */
    log_len = ulog_head_formater(log_buf, LOG_LVL_DBG, tag);

    /* 日志内容：地址范围 */
    fmt_result = snprintf(log_buf + log_len, ULOG_LINE_BUF_SIZE, "%04" PRIx32 "-%04" PRIx32 ": ", addr, addr + size);

    if ((fmt_result > -1) && (fmt_result <= ULOG_LINE_BUF_SIZE))
    {
        log_len += fmt_result;
    }
    else
    {
        log_len = ULOG_LINE_BUF_SIZE;
    }

    /* 转储十六进制 */
    for (idx = 0; idx < width; idx++)
    {
        if (idx < size)
        {
            (void)snprintf(dump_string, sizeof(dump_string), "%02X ", buf[idx]);
        }
        else
        {
            (void)strncpy(dump_string, "   ", sizeof(dump_string));
        }
        log_len += ulog_strcpy(log_len, log_buf + log_len, dump_string);
        if ((idx + 1) % 8 == 0)
        {
            log_len += ulog_strcpy(log_len, log_buf + log_len, " ");
        }
    }
    log_len += ulog_strcpy(log_len, log_buf + log_len, "  ");

    /* 转储十六进制对应的字符 */
    for (idx = 0; idx < size; idx++)
    {
        (void)snprintf(dump_string, sizeof(dump_string), "%c", _is_print(buf[idx]) ? buf[idx] : '.');
        log_len += ulog_strcpy(log_len, log_buf + log_len, dump_string);
    }

    /* 日志尾部 */
    return ulog_tail_formater(log_buf, log_len, true, LOG_LVL_DBG);

#undef __is_print
}

/**
 * @brief 将日志缓冲区输出到 SEGGER RTT。
 *
 * @param log_buf 日志缓冲区
 * @param log_len 日志长度
 */
static void do_output(const char *log_buf, size_t log_len)
{
    if (log_buf == NULL || log_len == 0)
    {
        return;
    }

    /* 输出到 SEGGER RTT 通道 0 */
    SEGGER_RTT_Write(0, log_buf, (unsigned)log_len);
}

/* 公共 API 实现 */
/**
 * @brief 使用 va_list 输出日志。
 */
void ulog_voutput(uint32_t level, const char *tag, bool newline, const char *format, va_list args)
{
    char  *log_buf;
    size_t log_len;

    if (!ulog.init_ok)
    {
        return;
    }

    if (tag == NULL || format == NULL)
    {
        return;
    }

    if (level > LOG_LVL_DBG)
    {
        return;
    }

    /* 级别过滤 */
    if (level > ulog.level)
    {
        return;
    }

    /* 获取日志缓冲区 */
    log_buf = get_log_buf();
    if (log_buf == NULL)
    {
        return;
    }

    /* 使用 RTT 锁格式化日志以确保线程安全 */
    SEGGER_RTT_LOCK();
    log_len = ulog_formater(log_buf, level, tag, newline, format, args);
    do_output(log_buf, log_len);
    SEGGER_RTT_UNLOCK();
}

/**
 * @brief 输出日志。
 */
void ulog_output(uint32_t level, const char *tag, bool newline, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    ulog_voutput(level, tag, newline, format, args);
    va_end(args);
}

/**
 * @brief 输出原始字符串（无格式化）。
 */
void ulog_raw(const char *format, ...)
{
    char   *log_buf;
    va_list args;
    int     fmt_result;
    size_t  log_len = 0;

    if (!ulog.init_ok || format == NULL)
    {
        return;
    }

    log_buf = get_log_buf();
    if (log_buf == NULL)
    {
        return;
    }

    va_start(args, format);

    SEGGER_RTT_LOCK();
    fmt_result = vsnprintf(log_buf, ULOG_LINE_BUF_SIZE, format, args);
    va_end(args);

    /* 计算日志长度 */
    if ((fmt_result > -1) && (fmt_result < ULOG_LINE_BUF_SIZE))
    {
        log_len = (size_t)fmt_result;
    }
    else
    {
        log_len = ULOG_LINE_BUF_SIZE;
    }

    do_output(log_buf, log_len);
    SEGGER_RTT_UNLOCK();
}

/**
 * @brief 以十六进制格式转储数据。
 */
void ulog_hexdump(const char *tag, uint32_t width, const uint8_t *buf, uint32_t size)
{
    uint32_t offset;
    size_t   len;
    char    *log_buf;

    if (!ulog.init_ok || tag == NULL || buf == NULL || size == 0 || width == 0)
    {
        return;
    }

    log_buf = get_log_buf();
    if (log_buf == NULL)
    {
        return;
    }

    SEGGER_RTT_LOCK();
    for (offset = 0; offset < size; offset += width, buf += width)
    {
        if (offset + width > size)
        {
            len = size - offset;
        }
        else
        {
            len = width;
        }
        len = ulog_hex_formater(log_buf, tag, buf, len, width, offset);
        do_output(log_buf, len);
    }
    SEGGER_RTT_UNLOCK();
}

/**
 * @brief 刷新挂起的日志输出。
 */
void ulog_flush(void) { /* SEGGER RTT 直接写入缓冲区，无需刷新 */ }

/**
 * @brief 设置全局日志级别。
 */
void ulog_set_level(uint32_t level)
{
    if (level <= LOG_LVL_DBG)
    {
        ulog.level = level;
    }
}

/**
 * @brief 获取全局日志级别。
 */
uint32_t ulog_get_level(void) { return ulog.level; }

/**
 * @brief 初始化 ulog 模块。
 */
int ulog_init(void)
{
    if (ulog.init_ok)
    {
        return 0;
    }

    /* 设置默认级别为输出所有日志 */
    ulog.level = LOG_LVL_DBG;

    /* 初始化 SEGGER RTT */
    SEGGER_RTT_Init();

    ulog.init_ok = true;

    return 0;
}

/**
 * @brief 反初始化 ulog 模块。
 */
void ulog_deinit(void)
{
    if (!ulog.init_ok)
    {
        return;
    }

    ulog.init_ok = false;
}
