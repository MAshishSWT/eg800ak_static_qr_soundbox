/*================================================================
 * Static QR UPI Soundbox - Production Logging Skeleton
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_LOG_H
#define SB_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SB_LOG_LEVEL_ERROR = 0,
    SB_LOG_LEVEL_WARN  = 1,
    SB_LOG_LEVEL_INFO  = 2,
    SB_LOG_LEVEL_DEBUG = 3
} sb_log_level_t;

void sb_log_init(sb_log_level_t level);
void sb_log_set_level(sb_log_level_t level);
sb_log_level_t sb_log_get_level(void);
void sb_log_write(sb_log_level_t level, const char *module, const char *fmt, ...);

#define SB_LOGE(module, fmt, ...) sb_log_write(SB_LOG_LEVEL_ERROR, (module), (fmt), ##__VA_ARGS__)
#define SB_LOGW(module, fmt, ...) sb_log_write(SB_LOG_LEVEL_WARN,  (module), (fmt), ##__VA_ARGS__)
#define SB_LOGI(module, fmt, ...) sb_log_write(SB_LOG_LEVEL_INFO,  (module), (fmt), ##__VA_ARGS__)
#define SB_LOGD(module, fmt, ...) sb_log_write(SB_LOG_LEVEL_DEBUG, (module), (fmt), ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* SB_LOG_H */
