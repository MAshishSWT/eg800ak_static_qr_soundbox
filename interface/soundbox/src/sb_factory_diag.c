/*================================================================
 * Static QR UPI Soundbox - Factory Diagnostic Dispatcher
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_type.h"
#include "ql_securedata.h"
#include "ql_rtos.h"
#include "sb_audio_service.h"
#include "sb_cloud_utils.h"
#include "sb_config.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_factory_diag.h"
#include "sb_http_service.h"
#include "sb_json.h"
#include "sb_log.h"
#include "sb_mode_service.h"
#include "sb_mqtt_service.h"
#include "sb_network_service.h"
#include "sb_ota_service.h"
#include "sb_storage_fs.h"
#include "sb_transaction_ledger.h"

#define SB_FACTORY_MODULE_NAME "factory"

static int s_factory_diag_ready = 0;

static void sb_factory_zero_buffer(void *buffer, u32 len)
{
    u32 i;
    unsigned char *ptr;

    if (buffer == 0) {
        return;
    }
    ptr = (unsigned char *)buffer;
    for (i = 0u; i < len; i++) {
        ptr[i] = 0u;
    }
}

static void sb_factory_post_event(sb_event_id_t id, sb_status_t status, const char *text)
{
    sb_event_t event;

    sb_event_init(&event, id, SB_EVENT_SOURCE_FACTORY);
    event.param_s32 = (s32)status;
    if (text != 0) {
        (void)sb_event_set_text(&event, text);
    }
    (void)sb_event_post(&event, QL_NO_WAIT);
}

static int sb_factory_hex_value(char c)
{
    if ((c >= '0') && (c <= '9')) {
        return (int)(c - '0');
    }
    if ((c >= 'a') && (c <= 'f')) {
        return (int)(c - 'a' + 10);
    }
    if ((c >= 'A') && (c <= 'F')) {
        return (int)(c - 'A' + 10);
    }
    return -1;
}

static sb_status_t sb_factory_hex_to_bytes(const char *hex, u8 *out, u32 out_len)
{
    u32 i;

    if ((hex == 0) || (out == 0) || (out_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (sb_cloud_str_len(hex) != (out_len * 2u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    for (i = 0u; i < out_len; i++) {
        int hi = sb_factory_hex_value(hex[i * 2u]);
        int lo = sb_factory_hex_value(hex[(i * 2u) + 1u]);
        if ((hi < 0) || (lo < 0)) {
            return SB_STATUS_INVALID_PARAM;
        }
        out[i] = (u8)(((u32)hi << 4u) | (u32)lo);
    }
    return SB_STATUS_OK;
}


static int sb_factory_text_all_zero(const char *text, u32 len)
{
    u32 i;

    if (text == 0) {
        return 1;
    }
    for (i = 0u; i < len; i++) {
        if ((text[i] != '\0') && (text[i] != (char)0xff)) {
            return 0;
        }
    }
    return 1;
}

static sb_status_t sb_factory_store_secure_text(u8 index, const char *text, u32 max_len)
{
    char buffer[SB_FACTORY_SMS_NUMBER_LEN];
    u32 i;

    if ((text == 0) || (max_len == 0u) || (max_len > (u32)sizeof(buffer))) {
        return SB_STATUS_INVALID_PARAM;
    }
    for (i = 0u; i < (u32)sizeof(buffer); i++) {
        buffer[i] = '\0';
    }
    for (i = 0u; (i < (max_len - 1u)) && (text[i] != '\0'); i++) {
        buffer[i] = text[i];
    }
    if (ql_securedata_store(index, (u8 *)buffer, max_len) < 0) {
        return SB_STATUS_SECURITY_ERROR;
    }
    sb_factory_zero_buffer(buffer, (u32)sizeof(buffer));
    return SB_STATUS_OK;
}

int sb_factory_diag_sms_sender_allowed(const char *sender)
{
    char allowed[SB_FACTORY_SMS_NUMBER_LEN];
    u32 i;

    if ((sender == 0) || (sender[0] == '\0')) {
        return 0;
    }
    for (i = 0u; i < (u32)sizeof(allowed); i++) {
        allowed[i] = '\0';
    }
    if (ql_securedata_read(SB_FACTORY_SMS_NUMBER_INDEX, (u8 *)allowed, (u32)sizeof(allowed)) < 0) {
        return 0;
    }
    allowed[SB_FACTORY_SMS_NUMBER_LEN - 1u] = '\0';
    if (sb_factory_text_all_zero(allowed, (u32)sizeof(allowed)) != 0) {
        return 0;
    }
    if (sb_cloud_text_equal(sender, allowed) != 0) {
        sb_factory_zero_buffer(allowed, (u32)sizeof(allowed));
        return 1;
    }
    sb_factory_zero_buffer(allowed, (u32)sizeof(allowed));
    return 0;
}

static sb_status_t sb_factory_reply_status(char *reply, u32 reply_len, const char *status, const char *detail)
{
    if ((reply == 0) || (reply_len == 0u) || (status == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    reply[0] = '\0';
    if (sb_cloud_append_string(reply, reply_len, "{\"status\":") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_json_string(reply, reply_len, status) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (detail != 0) {
        if (sb_cloud_append_string(reply, reply_len, ",\"detail\":") != SB_STATUS_OK) {
            return SB_STATUS_NO_MEMORY;
        }
        if (sb_cloud_append_json_string(reply, reply_len, detail) != SB_STATUS_OK) {
            return SB_STATUS_NO_MEMORY;
        }
    }
    if (sb_cloud_append_string(reply, reply_len, "}") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    return SB_STATUS_OK;
}

static sb_status_t sb_factory_reply_diag(char *reply, u32 reply_len)
{
    sb_network_status_t net;
    sb_mqtt_status_t mqtt;
    sb_http_status_t http;
    sb_ota_status_t ota;
    sb_daily_summary_t daily;
    sb_device_mode_t mode = SB_DEVICE_MODE_PRODUCTION;

    if ((reply == 0) || (reply_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }

    sb_factory_zero_buffer(&net, (u32)sizeof(net));
    sb_factory_zero_buffer(&mqtt, (u32)sizeof(mqtt));
    sb_factory_zero_buffer(&http, (u32)sizeof(http));
    sb_factory_zero_buffer(&ota, (u32)sizeof(ota));
    sb_factory_zero_buffer(&daily, (u32)sizeof(daily));

    (void)sb_network_get_status(&net);
    (void)sb_mqtt_get_status(&mqtt);
    (void)sb_http_get_status(&http);
    (void)sb_ota_get_status(&ota);
    (void)sb_transaction_ledger_get_daily(&daily);
    (void)sb_mode_get(&mode);

    reply[0] = '\0';
    if (sb_cloud_append_string(reply, reply_len, "{\"status\":\"ok\",\"mode\":") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_json_string(reply, reply_len, sb_mode_name(mode)) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(reply, reply_len, ",\"online\":") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_u32(reply, reply_len, (net.online != 0) ? 1u : 0u) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(reply, reply_len, ",\"csq\":") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_u32(reply, reply_len, (net.csq < 0) ? 0u : (u32)net.csq) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(reply, reply_len, ",\"mqtt_connected\":") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_u32(reply, reply_len, (mqtt.connected != 0) ? 1u : 0u) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(reply, reply_len, ",\"http_posts\":") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_u32(reply, reply_len, http.post_count) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(reply, reply_len, ",\"ota_state\":") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_json_string(reply, reply_len, sb_ota_state_name(ota.state)) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(reply, reply_len, ",\"daily_count\":") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_u32(reply, reply_len, daily.count) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(reply, reply_len, "}") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    return SB_STATUS_OK;
}

static int sb_factory_can_provision(sb_factory_channel_t channel, const sb_config_payload_t *config)
{
    if (sb_mode_factory_access_allowed() != 0) {
        return 1;
    }

    if ((channel == SB_FACTORY_CHANNEL_SMS) && (config != 0) && (config->sms_recovery_enabled != 0u)) {
#ifdef SB_ENABLE_INSECURE_SMS_RECOVERY_PROVISIONING
        return 1;
#else
        return 0;
#endif
    }

    return 0;
}

static sb_status_t sb_factory_commit_config_from_json(const char *json,
                                                      sb_factory_channel_t channel,
                                                      char *reply,
                                                      u32 reply_len)
{
    sb_config_payload_t config;
    u64 number = 0u;

    sb_config_make_defaults(&config);
    (void)sb_config_get(&config);

    if (sb_factory_can_provision(channel, &config) == 0) {
        (void)sb_factory_reply_status(reply, reply_len, "rejected", "locked");
        return SB_STATUS_SECURITY_ERROR;
    }

    (void)sb_json_get_string(json, "device_id", config.device_id, SB_CONFIG_DEVICE_ID_LEN);
    (void)sb_json_get_string(json, "apn", config.apn, SB_CONFIG_APN_LEN);
    (void)sb_json_get_string(json, "mqtt_host", config.mqtt_host, SB_CONFIG_MQTT_HOST_LEN);
    (void)sb_json_get_string(json, "mqtt_client_id", config.mqtt_client_id, SB_CONFIG_MQTT_CLIENT_ID_LEN);
    (void)sb_json_get_string(json, "mqtt_sub_topic", config.mqtt_sub_topic, SB_CONFIG_TOPIC_LEN);
    (void)sb_json_get_string(json, "mqtt_pub_topic", config.mqtt_pub_topic, SB_CONFIG_TOPIC_LEN);
    (void)sb_json_get_string(json, "http_base_url", config.http_base_url, SB_CONFIG_HTTP_BASE_URL_LEN);
    (void)sb_json_get_string(json, "language", config.language, SB_CONFIG_LANG_CODE_LEN);
    if (sb_json_get_u64(json, "mqtt_port", &number) == SB_STATUS_OK) {
        config.mqtt_port = (u32)number;
    }
    if (sb_json_get_u64(json, "volume", &number) == SB_STATUS_OK) {
        config.volume_percent = (number > 100u) ? 100u : (u32)number;
        (void)sb_audio_service_set_volume(config.volume_percent);
    }
    if (sb_json_get_u64(json, "sms_recovery_enabled", &number) == SB_STATUS_OK) {
        config.sms_recovery_enabled = (number != 0u) ? 1u : 0u;
    }
    if (sb_json_get_u64(json, "log_level", &number) == SB_STATUS_OK) {
        config.log_level = (u32)number;
    }

    if (sb_config_commit(&config) != SB_STATUS_OK) {
        (void)sb_factory_reply_status(reply, reply_len, "error", "commit");
        return SB_STATUS_FILE_ERROR;
    }

    return sb_factory_reply_status(reply, reply_len, "ok", "config_saved");
}

static sb_status_t sb_factory_store_ota_key(const char *json,
                                            sb_factory_channel_t channel,
                                            char *reply,
                                            u32 reply_len)
{
    sb_config_payload_t config;
    char key_hex[SB_FACTORY_KEY_HEX_LEN];
    u8 key[SB_FACTORY_SECURE_KEY_LEN];
    int ret;

    sb_config_make_defaults(&config);
    (void)sb_config_get(&config);
    sb_factory_zero_buffer(key_hex, (u32)sizeof(key_hex));
    sb_factory_zero_buffer(key, (u32)sizeof(key));
    if (sb_factory_can_provision(channel, &config) == 0) {
        (void)sb_factory_reply_status(reply, reply_len, "rejected", "locked");
        return SB_STATUS_SECURITY_ERROR;
    }
    if (sb_json_get_string(json, "key_hex", key_hex, (u32)sizeof(key_hex)) != SB_STATUS_OK) {
        (void)sb_factory_reply_status(reply, reply_len, "error", "key_hex");
        return SB_STATUS_INVALID_PARAM;
    }
    if (sb_factory_hex_to_bytes(key_hex, key, (u32)sizeof(key)) != SB_STATUS_OK) {
        (void)sb_factory_reply_status(reply, reply_len, "error", "key_hex");
        return SB_STATUS_INVALID_PARAM;
    }
    ret = ql_securedata_store(1u, key, (u32)sizeof(key));
    if (ret < 0) {
        sb_factory_zero_buffer(key, (u32)sizeof(key));
        (void)sb_factory_reply_status(reply, reply_len, "error", "securedata");
        return SB_STATUS_SECURITY_ERROR;
    }
    sb_factory_zero_buffer(key, (u32)sizeof(key));
    return sb_factory_reply_status(reply, reply_len, "ok", "ota_key_saved");
}


static sb_status_t sb_factory_store_sms_auth(const char *json,
                                             sb_factory_channel_t channel,
                                             char *reply,
                                             u32 reply_len)
{
    char phone[SB_FACTORY_SMS_NUMBER_LEN];
    sb_status_t status;

    sb_factory_zero_buffer(phone, (u32)sizeof(phone));

    if ((channel != SB_FACTORY_CHANNEL_SERIAL) || (sb_mode_factory_access_allowed() == 0)) {
        (void)sb_factory_reply_status(reply, reply_len, "rejected", "locked");
        return SB_STATUS_SECURITY_ERROR;
    }
    if (sb_json_get_string(json, "phone", phone, (u32)sizeof(phone)) != SB_STATUS_OK) {
        return sb_factory_reply_status(reply, reply_len, "error", "phone");
    }
    status = sb_factory_store_secure_text(SB_FACTORY_SMS_NUMBER_INDEX, phone, SB_FACTORY_SMS_NUMBER_LEN);
    if (status != SB_STATUS_OK) {
        return sb_factory_reply_status(reply, reply_len, "error", "phone_store");
    }
    return sb_factory_reply_status(reply, reply_len, "ok", "sms_auth_saved");
}

static sb_status_t sb_factory_set_mode(const char *json,
                                       sb_factory_channel_t channel,
                                       char *reply,
                                       u32 reply_len)
{
    char mode_text[16];
    sb_device_mode_t mode;

    if ((channel != SB_FACTORY_CHANNEL_SERIAL) || (sb_mode_factory_access_allowed() == 0)) {
        (void)sb_factory_reply_status(reply, reply_len, "rejected", "locked");
        return SB_STATUS_SECURITY_ERROR;
    }
    if (sb_json_get_string(json, "mode", mode_text, (u32)sizeof(mode_text)) != SB_STATUS_OK) {
        return sb_factory_reply_status(reply, reply_len, "error", "mode");
    }
    if (sb_cloud_text_equal(mode_text, "production") != 0) {
        mode = SB_DEVICE_MODE_PRODUCTION;
    } else if (sb_cloud_text_equal(mode_text, "factory") != 0) {
        mode = SB_DEVICE_MODE_FACTORY;
    } else if (sb_cloud_text_equal(mode_text, "debug") != 0) {
        mode = SB_DEVICE_MODE_DEBUG;
    } else {
        return sb_factory_reply_status(reply, reply_len, "error", "mode");
    }
    if (sb_mode_set(mode) != SB_STATUS_OK) {
        return sb_factory_reply_status(reply, reply_len, "error", "mode_save");
    }
    return sb_factory_reply_status(reply, reply_len, "ok", sb_mode_name(mode));
}

sb_status_t sb_factory_diag_init(void)
{
    if (s_factory_diag_ready != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }
    s_factory_diag_ready = 1;
    sb_factory_post_event(SB_EVENT_FACTORY_READY, SB_STATUS_OK, "ready");
    return SB_STATUS_OK;
}

sb_status_t sb_factory_diag_dispatch_json(const char *json,
                                           sb_factory_channel_t channel,
                                           char *reply,
                                           u32 reply_len)
{
    char cmd[32];
    sb_status_t status;

    if ((json == 0) || (reply == 0) || (reply_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (sb_json_get_string(json, "cmd", cmd, (u32)sizeof(cmd)) != SB_STATUS_OK) {
        (void)sb_factory_reply_status(reply, reply_len, "error", "cmd");
        return SB_STATUS_INVALID_PARAM;
    }

    if ((sb_cloud_text_equal(cmd, "diag") != 0) || (sb_cloud_text_equal(cmd, "status") != 0)) {
        status = sb_factory_reply_diag(reply, reply_len);
    } else if ((sb_cloud_text_equal(cmd, "set_config") != 0) || (sb_cloud_text_equal(cmd, "provision") != 0)) {
        status = sb_factory_commit_config_from_json(json, channel, reply, reply_len);
    } else if (sb_cloud_text_equal(cmd, "set_ota_key") != 0) {
        status = sb_factory_store_ota_key(json, channel, reply, reply_len);
    } else if ((sb_cloud_text_equal(cmd, "set_sms_auth") != 0) ||
               (sb_cloud_text_equal(cmd, "set_sms_recovery_auth") != 0)) {
        status = sb_factory_store_sms_auth(json, channel, reply, reply_len);
    } else if (sb_cloud_text_equal(cmd, "set_mode") != 0) {
        status = sb_factory_set_mode(json, channel, reply, reply_len);
    } else {
        (void)sb_factory_reply_status(reply, reply_len, "rejected", "unsupported");
        status = SB_STATUS_UNSUPPORTED;
    }

    if (status == SB_STATUS_OK) {
        SB_LOGI(SB_FACTORY_MODULE_NAME, "command=%s channel=%u status=OK", cmd, (u32)channel);
    } else {
        SB_LOGW(SB_FACTORY_MODULE_NAME, "command=%s channel=%u status=%s", cmd, (u32)channel, sb_status_to_string(status));
    }
    return status;
}
