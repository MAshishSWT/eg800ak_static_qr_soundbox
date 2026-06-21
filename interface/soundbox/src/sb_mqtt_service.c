/*================================================================
 * Static QR UPI Soundbox - MQTT Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "MQTTClient.h"
#include "ql_rtos.h"
#include "ql_ssl_hal.h"
#include "sb_business_service.h"
#include "sb_cloud_utils.h"
#include "sb_config.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_log.h"
#include "sb_mqtt_service.h"
#include "sb_network_service.h"

#define SB_MQTT_MODULE_NAME              "mqtt"
#define SB_MQTT_CIPHER_LIST              "ALL"
#define SB_MQTT_COMMAND_SUFFIX           "/cmd"
#define SB_MQTT_HEALTH_TYPE              "health"

typedef struct {
    char topic[SB_MQTT_TOPIC_LEN];
    char payload[SB_MQTT_PAYLOAD_LEN];
    u32 payload_len;
    u32 qos;
    u32 retained;
} sb_mqtt_outbound_message_t;

static char s_mqtt_root_ca_path[] = "U:/certs/mqtt_root_ca.pem";
static ql_task_t s_mqtt_task = 0;
static ql_mutex_t s_mqtt_mutex = 0;
static ql_queue_t s_mqtt_rx_queue = 0;
static ql_queue_t s_mqtt_tx_queue = 0;
static sb_mqtt_inbound_message_t s_mqtt_rx_pool[SB_MQTT_INBOUND_QUEUE_DEPTH];
static u8 s_mqtt_rx_pool_used[SB_MQTT_INBOUND_QUEUE_DEPTH];
static sb_mqtt_outbound_message_t s_mqtt_tx_pool[SB_MQTT_OUTBOUND_QUEUE_DEPTH];
static u8 s_mqtt_tx_pool_used[SB_MQTT_OUTBOUND_QUEUE_DEPTH];
static int s_mqtt_started = 0;
static sb_config_payload_t s_mqtt_config;
static char s_command_topic[SB_MQTT_COMMAND_TOPIC_LEN];
static MQTTClient s_mqtt_client;
static Network s_mqtt_network;
static SSLConfig s_mqtt_ssl_config;
static unsigned char s_mqtt_sendbuf[SB_MQTT_SEND_BUFFER_LEN];
static unsigned char s_mqtt_readbuf[SB_MQTT_READ_BUFFER_LEN];
static sb_mqtt_status_t s_mqtt_status = {
    SB_MQTT_STATE_STOPPED,
    0,
    0,
    0u,
    0u,
    0u
};

static void sb_mqtt_zero(void *ptr, u32 length)
{
    u32 i;
    unsigned char *p;

    if ((ptr == 0) || (length == 0u)) {
        return;
    }

    p = (unsigned char *)ptr;
    for (i = 0u; i < length; i++) {
        p[i] = 0u;
    }
}

static void sb_mqtt_status_set(sb_mqtt_state_t state, int connected, int error)
{
    if (s_mqtt_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_mqtt_mutex, QL_WAIT_FOREVER);
    }

    s_mqtt_status.state = state;
    s_mqtt_status.connected = connected;
    s_mqtt_status.last_error = error;

    if (s_mqtt_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_mqtt_mutex);
    }
}

static void sb_mqtt_status_count_rx(void)
{
    if (s_mqtt_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_mqtt_mutex, QL_WAIT_FOREVER);
    }
    s_mqtt_status.rx_count++;
    if (s_mqtt_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_mqtt_mutex);
    }
}

static void sb_mqtt_status_count_tx(void)
{
    if (s_mqtt_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_mqtt_mutex, QL_WAIT_FOREVER);
    }
    s_mqtt_status.tx_count++;
    if (s_mqtt_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_mqtt_mutex);
    }
}

static void sb_mqtt_status_count_reconnect(void)
{
    if (s_mqtt_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_mqtt_mutex, QL_WAIT_FOREVER);
    }
    s_mqtt_status.reconnect_count++;
    if (s_mqtt_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_mqtt_mutex);
    }
}

static void sb_mqtt_post_event(sb_event_id_t id, s32 status, u32 value, const char *text)
{
    sb_event_t event;

    sb_event_init(&event, id, SB_EVENT_SOURCE_MQTT);
    event.param_s32 = status;
    event.param_u32 = value;
    if (text != 0) {
        (void)sb_event_set_text(&event, text);
    }
    (void)sb_event_post(&event, QL_NO_WAIT);
}

static int sb_mqtt_pool_alloc(u8 *used, u32 depth, u32 *slot)
{
    u32 i;

    if ((used == 0) || (slot == 0)) {
        return 0;
    }

    if (s_mqtt_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_mqtt_mutex, QL_WAIT_FOREVER);
    }

    for (i = 0u; i < depth; i++) {
        if (used[i] == 0u) {
            used[i] = 1u;
            *slot = i;
            if (s_mqtt_mutex != 0) {
                (void)ql_rtos_mutex_unlock(s_mqtt_mutex);
            }
            return 1;
        }
    }

    if (s_mqtt_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_mqtt_mutex);
    }

    return 0;
}

static void sb_mqtt_pool_free(u8 *used, u32 depth, u32 slot)
{
    if ((used == 0) || (slot >= depth)) {
        return;
    }

    if (s_mqtt_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_mqtt_mutex, QL_WAIT_FOREVER);
    }
    used[slot] = 0u;
    if (s_mqtt_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_mqtt_mutex);
    }
}

static void sb_mqtt_build_command_topic(void)
{
    s_command_topic[0] = '\0';
    if (s_mqtt_config.mqtt_sub_topic[0] == '\0') {
        return;
    }

    sb_cloud_copy_string(s_command_topic, (u32)sizeof(s_command_topic), s_mqtt_config.mqtt_sub_topic);
    (void)sb_cloud_append_string(s_command_topic, (u32)sizeof(s_command_topic), SB_MQTT_COMMAND_SUFFIX);
}

static int sb_mqtt_configured(void)
{
    if ((s_mqtt_config.mqtt_host[0] == '\0') ||
        (s_mqtt_config.mqtt_port == 0u) ||
        (s_mqtt_config.mqtt_client_id[0] == '\0') ||
        (s_mqtt_config.mqtt_sub_topic[0] == '\0')) {
        return 0;
    }

    return 1;
}

static int sb_mqtt_network_ready(void)
{
    sb_network_status_t net_status;

    if (sb_network_get_status(&net_status) != SB_STATUS_OK) {
        return 0;
    }

    return (net_status.online != 0) ? 1 : 0;
}

static sb_mqtt_message_type_t sb_mqtt_classify_topic(const char *topic)
{
    if (sb_cloud_text_equal(topic, s_mqtt_config.mqtt_sub_topic) != 0) {
        return SB_MQTT_MESSAGE_PAYMENT;
    }

    if (sb_cloud_text_equal(topic, s_command_topic) != 0) {
        return SB_MQTT_MESSAGE_COMMAND;
    }

    return SB_MQTT_MESSAGE_OTHER;
}

static void sb_mqtt_copy_topic_from_mqtt(char *dst, u32 dst_len, const MQTTString *src)
{
    u32 i;
    u32 len;
    const char *data;

    if ((dst == 0) || (dst_len == 0u)) {
        return;
    }

    dst[0] = '\0';
    if (src == 0) {
        return;
    }

    data = src->lenstring.data;
    len = (u32)src->lenstring.len;
    if (data == 0) {
        return;
    }

    for (i = 0u; (i + 1u < dst_len) && (i < len); i++) {
        dst[i] = data[i];
    }
    dst[i] = '\0';
}

static void sb_mqtt_message_arrived(MessageData *data)
{
    sb_mqtt_inbound_message_t msg;
    u32 i;
    u32 payload_len;
    u32 slot = 0u;
    const char *payload;

    if ((data == 0) || (data->message == 0) || (data->topicName == 0)) {
        return;
    }

    sb_mqtt_zero(&msg, (u32)sizeof(msg));
    sb_mqtt_copy_topic_from_mqtt(msg.topic, (u32)sizeof(msg.topic), data->topicName);
    msg.type = sb_mqtt_classify_topic(msg.topic);

    payload = (const char *)data->message->payload;
    payload_len = (u32)data->message->payloadlen;
    if (payload_len >= SB_MQTT_PAYLOAD_LEN) {
        payload_len = SB_MQTT_PAYLOAD_LEN - 1u;
    }

    if (payload != 0) {
        for (i = 0u; i < payload_len; i++) {
            msg.payload[i] = payload[i];
        }
    }
    msg.payload[payload_len] = '\0';
    msg.payload_len = payload_len;

    if (s_mqtt_rx_queue != 0) {
        if (sb_mqtt_pool_alloc(s_mqtt_rx_pool_used, SB_MQTT_INBOUND_QUEUE_DEPTH, &slot) != 0) {
            s_mqtt_rx_pool[slot] = msg;
            if (ql_rtos_queue_release(s_mqtt_rx_queue, (u32)sizeof(slot), (u8 *)&slot, QL_NO_WAIT) != 0) {
                sb_mqtt_pool_free(s_mqtt_rx_pool_used, SB_MQTT_INBOUND_QUEUE_DEPTH, slot);
            }
        } else {
            sb_mqtt_post_event(SB_EVENT_MQTT_FAULT, (s32)SB_STATUS_QUEUE_ERROR, 0u, "rx_pool_full");
        }
    }

    sb_mqtt_status_count_rx();
    if (msg.type == SB_MQTT_MESSAGE_PAYMENT) {
        sb_mqtt_post_event(SB_EVENT_MQTT_PAYMENT_MESSAGE, SB_STATUS_OK, msg.payload_len, "payment");
    } else if (msg.type == SB_MQTT_MESSAGE_COMMAND) {
        sb_mqtt_post_event(SB_EVENT_MQTT_COMMAND_MESSAGE, SB_STATUS_OK, msg.payload_len, "command");
    } else {
        sb_mqtt_post_event(SB_EVENT_MQTT_MESSAGE, SB_STATUS_OK, msg.payload_len, "other");
    }
}

static void sb_mqtt_configure_ssl(u32 port)
{
    sb_mqtt_zero(&s_mqtt_ssl_config, (u32)sizeof(s_mqtt_ssl_config));
    s_mqtt_ssl_config.en = (port == SB_MQTT_TLS_PORT) ? 1u : 0u;
    if (s_mqtt_ssl_config.en != 0u) {
        s_mqtt_ssl_config.profileIdx = SB_MQTT_PDP_PROFILE_ID;
        s_mqtt_ssl_config.serverName = s_mqtt_config.mqtt_host;
        s_mqtt_ssl_config.serverPort = (unsigned short)port;
        s_mqtt_ssl_config.protocol = 0u;
        s_mqtt_ssl_config.dbgLevel = 0u;
        s_mqtt_ssl_config.sessionReuseEn = 0u;
        s_mqtt_ssl_config.vsn = SSL_VSN_ALL;
        s_mqtt_ssl_config.verify = SSL_VERIFY_MODE_REQUIRED;
        s_mqtt_ssl_config.cert.from = SSL_CERT_FROM_FS;
        s_mqtt_ssl_config.cert.path.rootCA = s_mqtt_root_ca_path;
        s_mqtt_ssl_config.cert.path.clientKey = NULL;
        s_mqtt_ssl_config.cert.path.clientCert = NULL;
        s_mqtt_ssl_config.cert.clientKeyPwd.data = NULL;
        s_mqtt_ssl_config.cert.clientKeyPwd.len = 0;
        s_mqtt_ssl_config.cipherList = SB_MQTT_CIPHER_LIST;
        s_mqtt_ssl_config.CTRDRBGSeed.data = NULL;
        s_mqtt_ssl_config.CTRDRBGSeed.len = 0;
    }
}

static sb_status_t sb_mqtt_connect(void)
{
    MQTTPacket_connectData connect_data = MQTTPacket_connectData_initializer;
    int rc;

    sb_mqtt_configure_ssl(s_mqtt_config.mqtt_port);
    sb_mqtt_zero(&s_mqtt_client, (u32)sizeof(s_mqtt_client));
    sb_mqtt_zero(&s_mqtt_network, (u32)sizeof(s_mqtt_network));

    NetworkInit(&s_mqtt_network, &s_mqtt_ssl_config, SB_MQTT_PDP_PROFILE_ID);
    MQTTClientInit(&s_mqtt_client,
                   &s_mqtt_network,
                   30000u,
                   s_mqtt_sendbuf,
                   (u32)sizeof(s_mqtt_sendbuf),
                   s_mqtt_readbuf,
                   (u32)sizeof(s_mqtt_readbuf));

    rc = NetworkConnect(&s_mqtt_network, s_mqtt_config.mqtt_host, (int)s_mqtt_config.mqtt_port);
    if (rc != 0) {
        SB_LOGW(SB_MQTT_MODULE_NAME, "network connect failed rc=%d", rc);
        MQTTClientDeinit(&s_mqtt_client);
        return SB_STATUS_NETWORK_ERROR;
    }

    connect_data.MQTTVersion = 3;
    connect_data.clientID.cstring = s_mqtt_config.mqtt_client_id;
    connect_data.keepAliveInterval = (unsigned short)s_mqtt_config.mqtt_keepalive_sec;
    connect_data.cleansession = 1;

    rc = MQTTConnect(&s_mqtt_client, &connect_data);
    if (rc != SUCCESS) {
        SB_LOGW(SB_MQTT_MODULE_NAME, "mqtt connect failed rc=%d", rc);
        NetworkDisconnect(&s_mqtt_network);
        MQTTClientDeinit(&s_mqtt_client);
        return SB_STATUS_MQTT_ERROR;
    }

    rc = MQTTSubscribe(&s_mqtt_client, s_mqtt_config.mqtt_sub_topic, QOS1, sb_mqtt_message_arrived);
    if (rc != SUCCESS) {
        SB_LOGW(SB_MQTT_MODULE_NAME, "payment subscribe failed rc=%d", rc);
        (void)MQTTDisconnect(&s_mqtt_client);
        NetworkDisconnect(&s_mqtt_network);
        MQTTClientDeinit(&s_mqtt_client);
        return SB_STATUS_MQTT_ERROR;
    }

    if (s_command_topic[0] != '\0') {
        rc = MQTTSubscribe(&s_mqtt_client, s_command_topic, QOS1, sb_mqtt_message_arrived);
        if (rc != SUCCESS) {
            SB_LOGW(SB_MQTT_MODULE_NAME, "command subscribe failed rc=%d", rc);
            (void)MQTTDisconnect(&s_mqtt_client);
            NetworkDisconnect(&s_mqtt_network);
            MQTTClientDeinit(&s_mqtt_client);
            return SB_STATUS_MQTT_ERROR;
        }
    }

    return SB_STATUS_OK;
}

static void sb_mqtt_disconnect(void)
{
    if (s_mqtt_client.isconnected != 0) {
        (void)MQTTDisconnect(&s_mqtt_client);
    }
    NetworkDisconnect(&s_mqtt_network);
    MQTTClientDeinit(&s_mqtt_client);
}

static sb_status_t sb_mqtt_publish_now(const char *topic, const char *payload, u32 payload_len, u32 qos, u32 retained)
{
    MQTTMessage message;
    int rc;

    if ((topic == 0) || (topic[0] == '\0') || (payload == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    message.qos = (qos == 0u) ? QOS0 : QOS1;
    message.retained = (unsigned char)((retained != 0u) ? 1u : 0u);
    message.dup = 0u;
    message.id = 0u;
    message.payload = (void *)payload;
    message.payloadlen = payload_len;

    rc = MQTTPublish(&s_mqtt_client, topic, &message);
    if (rc != SUCCESS) {
        return SB_STATUS_MQTT_ERROR;
    }

    sb_mqtt_status_count_tx();
    sb_mqtt_post_event(SB_EVENT_MQTT_PUBLISHED, SB_STATUS_OK, payload_len, topic);
    return SB_STATUS_OK;
}

static sb_status_t sb_mqtt_build_health_payload(char *payload, u32 payload_len)
{
    return sb_business_service_build_health_payload(payload, payload_len);
}

static void sb_mqtt_publish_periodic_health(u32 *elapsed_ms)
{
    char payload[SB_MQTT_PAYLOAD_LEN];
    u32 interval_ms;

    if ((elapsed_ms == 0) || (s_mqtt_config.mqtt_pub_topic[0] == '\0')) {
        return;
    }

    interval_ms = s_mqtt_config.health_interval_sec * 1000u;
    if (interval_ms < 60000u) {
        interval_ms = 60000u;
    }

    *elapsed_ms += SB_MQTT_YIELD_MS;
    if (*elapsed_ms < interval_ms) {
        return;
    }
    *elapsed_ms = 0u;

    if (sb_mqtt_build_health_payload(payload, (u32)sizeof(payload)) == SB_STATUS_OK) {
        (void)sb_mqtt_publish_now(s_mqtt_config.mqtt_pub_topic, payload, sb_cloud_str_len(payload), 1u, 0u);
    }
}

static sb_status_t sb_mqtt_drain_publish_queue(void)
{
    sb_mqtt_outbound_message_t msg;
    QlOSStatus ret;
    sb_status_t status = SB_STATUS_OK;
    u32 slot = 0u;

    if (s_mqtt_tx_queue == 0) {
        return SB_STATUS_NOT_READY;
    }

    while (1) {
        ret = ql_rtos_queue_wait(s_mqtt_tx_queue, (u8 *)&slot, (u32)sizeof(slot), QL_NO_WAIT);
        if (ret != 0) {
            break;
        }
        if (slot >= SB_MQTT_OUTBOUND_QUEUE_DEPTH) {
            continue;
        }
        msg = s_mqtt_tx_pool[slot];
        sb_mqtt_pool_free(s_mqtt_tx_pool_used, SB_MQTT_OUTBOUND_QUEUE_DEPTH, slot);
        status = sb_mqtt_publish_now(msg.topic, msg.payload, msg.payload_len, msg.qos, msg.retained);
        if (status != SB_STATUS_OK) {
            sb_mqtt_post_event(SB_EVENT_MQTT_FAULT, (s32)status, 0u, "publish");
            return status;
        }
    }

    return SB_STATUS_OK;
}

static void sb_mqtt_connected_loop(void)
{
    int rc;
    u32 health_elapsed_ms = 0u;

    sb_mqtt_status_set(SB_MQTT_STATE_CONNECTED, 1, 0);
    sb_mqtt_post_event(SB_EVENT_MQTT_READY, SB_STATUS_OK, (u32)s_mqtt_config.mqtt_port, "connected");
    SB_LOGI(SB_MQTT_MODULE_NAME, "connected host=%s port=%u", s_mqtt_config.mqtt_host, s_mqtt_config.mqtt_port);

    while (1) {
        if (sb_mqtt_network_ready() == 0) {
            sb_mqtt_post_event(SB_EVENT_MQTT_DISCONNECTED, (s32)SB_STATUS_NETWORK_ERROR, 0u, "network_lost");
            break;
        }

        if (sb_mqtt_drain_publish_queue() != SB_STATUS_OK) {
            break;
        }

        rc = MQTTYield(&s_mqtt_client, SB_MQTT_YIELD_MS);
        if (rc != SUCCESS) {
            SB_LOGW(SB_MQTT_MODULE_NAME, "yield failed rc=%d", rc);
            sb_mqtt_post_event(SB_EVENT_MQTT_DISCONNECTED, (s32)SB_STATUS_MQTT_ERROR, (u32)rc, "yield");
            break;
        }

        sb_mqtt_publish_periodic_health(&health_elapsed_ms);
    }

    sb_mqtt_status_set(SB_MQTT_STATE_BACKOFF, 0, SB_STATUS_MQTT_ERROR);
    sb_mqtt_disconnect();
}

static void sb_mqtt_task(void *argv)
{
    sb_status_t status;
    int config_fault_reported = 0;

    (void)argv;
    SB_LOGI(SB_MQTT_MODULE_NAME, "task started");

    while (1) {
        sb_mqtt_status_set(SB_MQTT_STATE_WAIT_NETWORK, 0, 0);
        while (sb_mqtt_network_ready() == 0) {
            ql_rtos_task_sleep_ms(SB_MQTT_NETWORK_WAIT_MS);
        }

        sb_mqtt_status_set(SB_MQTT_STATE_CONFIG_CHECK, 0, 0);
        if (sb_mqtt_configured() == 0) {
            if (config_fault_reported == 0) {
                sb_mqtt_post_event(SB_EVENT_MQTT_FAULT, (s32)SB_STATUS_CONFIG_ERROR, 0u, "not_configured");
                config_fault_reported = 1;
            }
            ql_rtos_task_sleep_ms(SB_MQTT_RECONNECT_BACKOFF_MS);
            continue;
        }
        config_fault_reported = 0;

        sb_mqtt_status_set(SB_MQTT_STATE_CONNECTING, 0, 0);
        status = sb_mqtt_connect();
        if (status != SB_STATUS_OK) {
            sb_mqtt_status_count_reconnect();
            sb_mqtt_status_set(SB_MQTT_STATE_BACKOFF, 0, (int)status);
            sb_mqtt_post_event(SB_EVENT_MQTT_FAULT, (s32)status, 0u, "connect");
            ql_rtos_task_sleep_ms(SB_MQTT_RECONNECT_BACKOFF_MS);
            continue;
        }

        sb_mqtt_connected_loop();
        sb_mqtt_status_count_reconnect();
        ql_rtos_task_sleep_ms(SB_MQTT_RECONNECT_BACKOFF_MS);
    }
}

sb_status_t sb_mqtt_service_init(const sb_config_payload_t *config)
{
    QlOSStatus ret;

    if (s_mqtt_started != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }

    if (config == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    s_mqtt_config = *config;
    sb_mqtt_build_command_topic();

    ret = ql_rtos_mutex_create(&s_mqtt_mutex);
    if (ret != 0) {
        return SB_STATUS_NO_MEMORY;
    }

    ret = ql_rtos_queue_create(&s_mqtt_rx_queue, (u32)sizeof(u32), SB_MQTT_INBOUND_QUEUE_DEPTH);
    if (ret != 0) {
        return SB_STATUS_QUEUE_ERROR;
    }

    ret = ql_rtos_queue_create(&s_mqtt_tx_queue, (u32)sizeof(u32), SB_MQTT_OUTBOUND_QUEUE_DEPTH);
    if (ret != 0) {
        return SB_STATUS_QUEUE_ERROR;
    }

    ret = ql_rtos_task_create(&s_mqtt_task,
                              SB_MQTT_TASK_STACK_SIZE_BYTES,
                              SB_MQTT_TASK_PRIORITY,
                              "sb_mqtt",
                              sb_mqtt_task,
                              0);
    if (ret != 0) {
        return SB_STATUS_TASK_ERROR;
    }

    s_mqtt_started = 1;
    return SB_STATUS_OK;
}

sb_status_t sb_mqtt_service_publish(const char *topic, const char *payload, u32 payload_len, u32 qos, u32 retained)
{
    sb_mqtt_outbound_message_t msg;
    u32 copy_len;
    u32 i;
    u32 slot = 0u;

    if ((topic == 0) || (topic[0] == '\0') || (payload == 0) || (s_mqtt_tx_queue == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (sb_mqtt_pool_alloc(s_mqtt_tx_pool_used, SB_MQTT_OUTBOUND_QUEUE_DEPTH, &slot) == 0) {
        return SB_STATUS_QUEUE_ERROR;
    }

    sb_mqtt_zero(&msg, (u32)sizeof(msg));
    sb_cloud_copy_string(msg.topic, (u32)sizeof(msg.topic), topic);
    copy_len = payload_len;
    if (copy_len >= SB_MQTT_PAYLOAD_LEN) {
        copy_len = SB_MQTT_PAYLOAD_LEN - 1u;
    }
    for (i = 0u; i < copy_len; i++) {
        msg.payload[i] = payload[i];
    }
    msg.payload[copy_len] = '\0';
    msg.payload_len = copy_len;
    msg.qos = (qos == 0u) ? 0u : 1u;
    msg.retained = (retained != 0u) ? 1u : 0u;

    s_mqtt_tx_pool[slot] = msg;
    if (ql_rtos_queue_release(s_mqtt_tx_queue, (u32)sizeof(slot), (u8 *)&slot, QL_NO_WAIT) != 0) {
        sb_mqtt_pool_free(s_mqtt_tx_pool_used, SB_MQTT_OUTBOUND_QUEUE_DEPTH, slot);
        return SB_STATUS_QUEUE_ERROR;
    }

    return SB_STATUS_OK;
}

sb_status_t sb_mqtt_service_receive(sb_mqtt_inbound_message_t *message, u32 timeout_ms)
{
    u32 slot = 0u;

    if ((message == 0) || (s_mqtt_rx_queue == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (ql_rtos_queue_wait(s_mqtt_rx_queue, (u8 *)&slot, (u32)sizeof(slot), timeout_ms) != 0) {
        return SB_STATUS_TIMEOUT;
    }

    if (slot >= SB_MQTT_INBOUND_QUEUE_DEPTH) {
        return SB_STATUS_INVALID_STATE;
    }

    *message = s_mqtt_rx_pool[slot];
    sb_mqtt_pool_free(s_mqtt_rx_pool_used, SB_MQTT_INBOUND_QUEUE_DEPTH, slot);
    return SB_STATUS_OK;
}

sb_status_t sb_mqtt_get_status(sb_mqtt_status_t *status)
{
    if (status == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (s_mqtt_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_mqtt_mutex, QL_WAIT_FOREVER);
    }
    *status = s_mqtt_status;
    if (s_mqtt_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_mqtt_mutex);
    }

    return SB_STATUS_OK;
}

const char *sb_mqtt_state_name(sb_mqtt_state_t state)
{
    switch (state) {
    case SB_MQTT_STATE_STOPPED:
        return "STOPPED";
    case SB_MQTT_STATE_WAIT_NETWORK:
        return "WAIT_NETWORK";
    case SB_MQTT_STATE_CONFIG_CHECK:
        return "CONFIG_CHECK";
    case SB_MQTT_STATE_CONNECTING:
        return "CONNECTING";
    case SB_MQTT_STATE_CONNECTED:
        return "CONNECTED";
    case SB_MQTT_STATE_BACKOFF:
        return "BACKOFF";
    default:
        return "UNKNOWN";
    }
}

const char *sb_mqtt_message_type_name(sb_mqtt_message_type_t type)
{
    switch (type) {
    case SB_MQTT_MESSAGE_PAYMENT:
        return "PAYMENT";
    case SB_MQTT_MESSAGE_COMMAND:
        return "COMMAND";
    case SB_MQTT_MESSAGE_OTHER:
    default:
        return "OTHER";
    }
}
