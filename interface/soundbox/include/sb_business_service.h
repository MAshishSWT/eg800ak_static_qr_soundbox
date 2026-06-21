/*================================================================
 * Static QR UPI Soundbox - Business Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_BUSINESS_SERVICE_H
#define SB_BUSINESS_SERVICE_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_BUSINESS_TASK_STACK_SIZE_BYTES      (10u * 1024u)
#define SB_BUSINESS_TASK_PRIORITY              (18u)
#define SB_BUSINESS_POLL_MS                    (1000u)
#define SB_BUSINESS_LONG_PRESS_MS              (2000u)
#define SB_BUSINESS_VOLUME_STEP_PERCENT        (5u)
#define SB_BUSINESS_HEALTH_PAYLOAD_LEN         (512u)

typedef enum {
    SB_BUSINESS_KEY_ACTION_NONE = 0,
    SB_BUSINESS_KEY_ACTION_VOLUME_UP,
    SB_BUSINESS_KEY_ACTION_VOLUME_DOWN,
    SB_BUSINESS_KEY_ACTION_LAST_TRANSACTION,
    SB_BUSINESS_KEY_ACTION_BATTERY,
    SB_BUSINESS_KEY_ACTION_SIGNAL,
    SB_BUSINESS_KEY_ACTION_DAILY_SUMMARY
} sb_business_key_action_t;

sb_status_t sb_business_service_init(void);
sb_status_t sb_business_service_handle_key_edge(u32 key_id, int pressed, u32 timestamp_ticks);
sb_status_t sb_business_service_build_health_payload(char *payload, u32 payload_len);
const char *sb_business_key_action_name(sb_business_key_action_t action);

#ifdef __cplusplus
}
#endif

#endif /* SB_BUSINESS_SERVICE_H */
