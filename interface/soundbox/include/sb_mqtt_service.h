/*================================================================
 * Static QR UPI Soundbox - MQTT Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_MQTT_SERVICE_H
#define SB_MQTT_SERVICE_H

#include "ql_type.h"
#include "sb_config.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_MQTT_TASK_STACK_SIZE_BYTES      (14u * 1024u)
#define SB_MQTT_TASK_PRIORITY              (16u)
#define SB_MQTT_PDP_PROFILE_ID             (1)
#define SB_MQTT_SEND_BUFFER_LEN            (1024u)
#define SB_MQTT_READ_BUFFER_LEN            (1024u)
#define SB_MQTT_PAYLOAD_LEN                (512u)
#define SB_MQTT_TOPIC_LEN                  (SB_CONFIG_TOPIC_LEN)
#define SB_MQTT_COMMAND_TOPIC_LEN          (SB_CONFIG_TOPIC_LEN + 8u)
#define SB_MQTT_INBOUND_QUEUE_DEPTH        (8u)
#define SB_MQTT_OUTBOUND_QUEUE_DEPTH       (8u)
#define SB_MQTT_NETWORK_WAIT_MS            (2000u)
#define SB_MQTT_RECONNECT_BACKOFF_MS       (15000u)
#define SB_MQTT_CONNECT_FAIL_LIMIT        (3u)
#define SB_MQTT_TLS_FAIL_COOLDOWN_MS      (10u * 60u * 1000u)
#define SB_MQTT_YIELD_MS                   (500u)
#define SB_MQTT_TLS_PORT                   (8883u)

typedef enum {
    SB_MQTT_STATE_STOPPED = 0,
    SB_MQTT_STATE_WAIT_NETWORK,
    SB_MQTT_STATE_CONFIG_CHECK,
    SB_MQTT_STATE_CONNECTING,
    SB_MQTT_STATE_CONNECTED,
    SB_MQTT_STATE_BACKOFF
} sb_mqtt_state_t;

typedef enum {
    SB_MQTT_MESSAGE_OTHER = 0,
    SB_MQTT_MESSAGE_PAYMENT,
    SB_MQTT_MESSAGE_COMMAND
} sb_mqtt_message_type_t;

typedef struct {
    sb_mqtt_message_type_t type;
    char topic[SB_MQTT_TOPIC_LEN];
    char payload[SB_MQTT_PAYLOAD_LEN];
    u32 payload_len;
} sb_mqtt_inbound_message_t;

typedef struct {
    sb_mqtt_state_t state;
    int connected;
    int last_error;
    u32 reconnect_count;
    u32 rx_count;
    u32 tx_count;
} sb_mqtt_status_t;

sb_status_t sb_mqtt_service_init(const sb_config_payload_t *config);
sb_status_t sb_mqtt_service_publish(const char *topic, const char *payload, u32 payload_len, u32 qos, u32 retained);
sb_status_t sb_mqtt_service_receive(sb_mqtt_inbound_message_t *message, u32 timeout_ms);
sb_status_t sb_mqtt_get_status(sb_mqtt_status_t *status);
const char *sb_mqtt_state_name(sb_mqtt_state_t state);
const char *sb_mqtt_message_type_name(sb_mqtt_message_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* SB_MQTT_SERVICE_H */
