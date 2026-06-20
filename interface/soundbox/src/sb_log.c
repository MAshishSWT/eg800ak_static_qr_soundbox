/*================================================================
 * Static QR UPI Soundbox - Production Logging
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include <stdarg.h>
#include "sb_log.h"

#define SB_LOG_LINE_LEN (192u)

extern int printf(const char *format, ...);
extern int vsnprintf(char *str, unsigned int size, const char *format, va_list arg);

static sb_log_level_t s_log_level = SB_LOG_LEVEL_INFO;

static const char *sb_log_level_name(sb_log_level_t level)
{
    switch (level) {
    case SB_LOG_LEVEL_ERROR:
        return "E";
    case SB_LOG_LEVEL_WARN:
        return "W";
    case SB_LOG_LEVEL_INFO:
        return "I";
    case SB_LOG_LEVEL_DEBUG:
        return "D";
    default:
        return "?";
    }
}

void sb_log_init(sb_log_level_t level)
{
    s_log_level = level;
}

void sb_log_set_level(sb_log_level_t level)
{
    s_log_level = level;
}

sb_log_level_t sb_log_get_level(void)
{
    return s_log_level;
}

void sb_log_write(sb_log_level_t level, const char *module, const char *fmt, ...)
{
    char line[SB_LOG_LINE_LEN];
    va_list args;
    int ret;

    if (level > s_log_level) {
        return;
    }

    if (module == 0) {
        module = "core";
    }

    if (fmt == 0) {
        fmt = "";
    }

    line[0] = '\0';
    va_start(args, fmt);
    ret = vsnprintf(line, (unsigned int)sizeof(line), fmt, args);
    va_end(args);

    if (ret < 0) {
        line[0] = '\0';
    }

    line[sizeof(line) - 1u] = '\0';
    printf("[SB][%s][%s] %s\r\n", sb_log_level_name(level), module, line);
}
