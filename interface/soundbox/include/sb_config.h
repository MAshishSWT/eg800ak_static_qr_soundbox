/*================================================================
 * Static QR UPI Soundbox - Configuration Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_CONFIG_H
#define SB_CONFIG_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_CONFIG_MAGIC                 (0x53424346u)
#define SB_CONFIG_VERSION               (1u)
#define SB_CONFIG_DEVICE_ID_LEN         (32u)
#define SB_CONFIG_APN_LEN               (48u)
#define SB_CONFIG_MQTT_HOST_LEN         (96u)
#define SB_CONFIG_MQTT_CLIENT_ID_LEN    (64u)
#define SB_CONFIG_TOPIC_LEN             (96u)
#define SB_CONFIG_LANG_CODE_LEN         (8u)

typedef enum {
    SB_CONFIG_SLOT_A = 0,
    SB_CONFIG_SLOT_B = 1
} sb_config_slot_t;

typedef struct {
    char device_id[SB_CONFIG_DEVICE_ID_LEN];
    char apn[SB_CONFIG_APN_LEN];
    char mqtt_host[SB_CONFIG_MQTT_HOST_LEN];
    u32 mqtt_port;
    char mqtt_client_id[SB_CONFIG_MQTT_CLIENT_ID_LEN];
    char mqtt_sub_topic[SB_CONFIG_TOPIC_LEN];
    char mqtt_pub_topic[SB_CONFIG_TOPIC_LEN];
    char language[SB_CONFIG_LANG_CODE_LEN];
    u32 volume_percent;
    u32 sms_recovery_enabled;
    u32 log_level;
    u32 health_interval_sec;
    u32 mqtt_keepalive_sec;
} sb_config_payload_t;

typedef struct {
    u32 active_sequence;
    sb_config_slot_t active_slot;
    int loaded_from_storage;
} sb_config_state_t;

sb_status_t sb_config_service_init(void);
sb_status_t sb_config_get(sb_config_payload_t *config);
sb_status_t sb_config_get_state(sb_config_state_t *state);
sb_status_t sb_config_commit(const sb_config_payload_t *config);
void sb_config_make_defaults(sb_config_payload_t *config);

#ifdef __cplusplus
}
#endif

#endif /* SB_CONFIG_H */
