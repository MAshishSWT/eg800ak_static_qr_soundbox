/*================================================================
 * Static QR UPI Soundbox - HTTPS Registration and Health Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_HTTP_SERVICE_H
#define SB_HTTP_SERVICE_H

#include "sb_config.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_HTTP_TASK_STACK_SIZE_BYTES       (10u * 1024u)
#define SB_HTTP_TASK_PRIORITY               (17u)
#define SB_HTTP_PDP_PROFILE_ID              (1)
#define SB_HTTP_URL_LEN                     (160u)
#define SB_HTTP_BODY_LEN                    (512u)
#define SB_HTTP_RESPONSE_LEN                (512u)
#define SB_HTTP_HEALTH_INTERVAL_MS          (5u * 60u * 1000u)
#define SB_HTTP_BACKOFF_BASE_MS             (15000u)
#define SB_HTTP_BACKOFF_MAX_MS              (5u * 60u * 1000u)
#define SB_HTTP_TLS_PORT                    (443u)
#define SB_HTTP_ROOT_CA_PATH                "U:/certs/mqtt_root_ca.pem"

typedef enum {
    SB_HTTP_STATE_STOPPED = 0,
    SB_HTTP_STATE_WAIT_NETWORK,
    SB_HTTP_STATE_REGISTERING,
    SB_HTTP_STATE_REGISTERED,
    SB_HTTP_STATE_UNREGISTERED,
    SB_HTTP_STATE_BACKOFF
} sb_http_state_t;

typedef struct {
    sb_http_state_t state;
    int registered;
    int last_status_code;
    int last_error;
    u32 request_count;
    u32 fail_count;
} sb_http_status_t;

sb_status_t sb_http_service_init(const sb_config_payload_t *config);
sb_status_t sb_http_get_status(sb_http_status_t *status);
const char *sb_http_state_name(sb_http_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* SB_HTTP_SERVICE_H */
