/*================================================================
 * Static QR UPI Soundbox - Network/Data Call Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_NETWORK_SERVICE_H
#define SB_NETWORK_SERVICE_H

#include "ql_type.h"
#include "sb_config.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_NETWORK_TASK_STACK_SIZE_BYTES      (10u * 1024u)
#define SB_NETWORK_TASK_PRIORITY              (15u)
#define SB_NETWORK_PDP_PROFILE_ID             (1)
#define SB_NETWORK_IPV4                       (0)
#define SB_NETWORK_REGISTER_TIMEOUT_SEC       (60u)
#define SB_NETWORK_DATACALL_TIMEOUT_MS        (30000u)
#define SB_NETWORK_ONLINE_POLL_PERIOD_MS      (30000u)
#define SB_NETWORK_BACKOFF_MS                 (10000u)
#define SB_NETWORK_APN_LEN                    (SB_CONFIG_APN_LEN)
#define SB_NETWORK_NTP_SERVER_LEN             (64u)

typedef enum {
    SB_NETWORK_STATE_STOPPED = 0,
    SB_NETWORK_STATE_SIM_CHECK,
    SB_NETWORK_STATE_WAIT_REGISTER,
    SB_NETWORK_STATE_START_DATACALL,
    SB_NETWORK_STATE_ONLINE,
    SB_NETWORK_STATE_BACKOFF
} sb_network_state_t;

typedef struct {
    sb_network_state_t state;
    int sim_status;
    int csq;
    int registered;
    int datacall_state;
    int online;
    int last_error;
} sb_network_status_t;

sb_status_t sb_network_service_init(const sb_config_payload_t *config);
sb_status_t sb_network_get_status(sb_network_status_t *status);
sb_status_t sb_network_request_time_sync(void);
const char *sb_network_state_name(sb_network_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* SB_NETWORK_SERVICE_H */
