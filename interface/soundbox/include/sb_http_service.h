/*================================================================
 * Static QR UPI Soundbox - HTTP Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_HTTP_SERVICE_H
#define SB_HTTP_SERVICE_H

#include "ql_type.h"
#include "sb_config.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_HTTP_TASK_STACK_SIZE_BYTES       (12u * 1024u)
#define SB_HTTP_TASK_PRIORITY               (17u)
#define SB_HTTP_PDP_PROFILE_ID              (1)
#define SB_HTTP_URL_LEN                     (192u)
#define SB_HTTP_PAYLOAD_LEN                 (512u)
#define SB_HTTP_QUEUE_DEPTH                 (4u)
#define SB_HTTP_NETWORK_WAIT_MS             (2000u)
#define SB_HTTP_MIN_INTERVAL_SEC            (60u)
#define SB_HTTP_TOTAL_TIMEOUT_MS            (15000u)
#define SB_HTTP_TLS_PORT                    (443u)

typedef enum {
    SB_HTTP_POST_HEALTH = 0,
    SB_HTTP_POST_COMMAND_RESPONSE
} sb_http_post_type_t;

typedef struct {
    sb_http_post_type_t type;
    char payload[SB_HTTP_PAYLOAD_LEN];
    u32 payload_len;
} sb_http_post_request_t;

typedef struct {
    int configured;
    int last_status_code;
    int last_error;
    u32 post_count;
    u32 fault_count;
} sb_http_status_t;

sb_status_t sb_http_service_init(const sb_config_payload_t *config);
sb_status_t sb_http_service_post_command_response(const char *payload, u32 payload_len);
sb_status_t sb_http_get_status(sb_http_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* SB_HTTP_SERVICE_H */
