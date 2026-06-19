/*================================================================
 * Static QR UPI Soundbox - Production Logging Skeleton
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include <stdarg.h>
#include "sb_log.h"

extern int printf(const char *format, ...);
extern int vprintf(const char *format, va_list arg);

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
    va_list args;

    if (level > s_log_level) {
        return;
    }

    if (module == 0) {
        module = "core";
    }

    if (fmt == 0) {
        fmt = "";
    }

    printf("[SB][%s][%s] ", sb_log_level_name(level), module);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\r\n");
}
