/*================================================================
 * Static QR UPI Soundbox - Factory Diagnostic Dispatcher
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_type.h"
#include "ql_securedata.h"
#include "ql_rtos.h"
#include "ql_fs.h"
#include "sb_audio_service.h"
#include "sb_cloud_utils.h"
#include "sb_config.h"
#include "sb_crc32.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_factory_diag.h"
#include "sb_json.h"
#include "sb_log.h"
#include "sb_mode_service.h"
#include "sb_mqtt_service.h"
#include "sb_network_service.h"
#include "sb_transaction_ledger.h"

#define SB_FACTORY_MODULE_NAME "factory"
#define SB_FACTORY_FILE_PATH_LEN (128u)
#define SB_FACTORY_FILE_HEX_MAX  (2048u)

typedef struct {
    int active;
    QFILE *fp;
    char path[SB_FACTORY_FILE_PATH_LEN];
    u32 expected_size;
    u32 expected_crc32;
    u32 received_size;
    u32 crc_state;
} sb_factory_file_upload_t;

static int s_factory_diag_ready = 0;
static sb_factory_file_upload_t s_file_upload;

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
        sb_factory_zero_buffer(buffer, (u32)sizeof(buffer));
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
    sb_daily_summary_t daily;
    sb_device_mode_t mode = SB_DEVICE_MODE_PRODUCTION;

    if ((reply == 0) || (reply_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }

    sb_factory_zero_buffer(&net, (u32)sizeof(net));
    sb_factory_zero_buffer(&mqtt, (u32)sizeof(mqtt));
    sb_factory_zero_buffer(&daily, (u32)sizeof(daily));

    (void)sb_network_get_status(&net);
    (void)sb_mqtt_get_status(&mqtt);
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

static sb_status_t sb_factory_hex_to_bytes_partial(const char *hex, u8 *out, u32 out_max, u32 *out_len)
{
    u32 hex_len;
    u32 i;

    if ((hex == 0) || (out == 0) || (out_len == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    hex_len = sb_cloud_str_len(hex);
    if (((hex_len & 1u) != 0u) || ((hex_len / 2u) > out_max)) {
        return SB_STATUS_INVALID_PARAM;
    }
    for (i = 0u; i < (hex_len / 2u); i++) {
        int hi = sb_factory_hex_value(hex[i * 2u]);
        int lo = sb_factory_hex_value(hex[(i * 2u) + 1u]);
        if ((hi < 0) || (lo < 0)) {
            return SB_STATUS_INVALID_PARAM;
        }
        out[i] = (u8)(((u32)hi << 4u) | (u32)lo);
    }
    *out_len = hex_len / 2u;
    return SB_STATUS_OK;
}

static int sb_factory_file_access_allowed(sb_factory_channel_t channel)
{
    if (channel != SB_FACTORY_CHANNEL_SERIAL) {
        return 0;
    }
    if (sb_mode_factory_access_allowed() != 0) {
        return 1;
    }
#ifdef SB_SERIAL_FILE_PROVISIONING_ENABLED
    return 1;
#else
    return 0;
#endif
}

static sb_status_t sb_factory_file_to_ufs_path(const char *input, char *out, u32 out_len)
{
    if ((input == 0) || (out == 0) || (out_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    out[0] = '\0';
    if (sb_cloud_has_prefix(input, "U:/") != 0) {
        if ((sb_cloud_has_prefix(input, "U:/audio/") == 0) && (sb_cloud_text_equal(input, "U:/audio") == 0)) {
            return SB_STATUS_SECURITY_ERROR;
        }
        sb_cloud_copy_string(out, out_len, input);
        return SB_STATUS_OK;
    }
    if (sb_cloud_has_prefix(input, "audio/") != 0) {
        if (sb_cloud_append_string(out, out_len, "U:/") != SB_STATUS_OK) {
            return SB_STATUS_NO_MEMORY;
        }
        return sb_cloud_append_string(out, out_len, input);
    }
    return SB_STATUS_INVALID_PARAM;
}

static void sb_factory_file_make_parent_dirs(const char *path)
{
    char dir[SB_FACTORY_FILE_PATH_LEN];
    u32 i;
    u32 len;

    if (path == 0) {
        return;
    }
    len = sb_cloud_str_len(path);
    if (len >= (u32)sizeof(dir)) {
        return;
    }
    dir[0] = '\0';
    for (i = 0u; i < len; i++) {
        dir[i] = path[i];
        dir[i + 1u] = '\0';
        if ((i > 2u) && (path[i] == '/')) {
            dir[i] = '\0';
            (void)ql_mkdir(dir, 0u);
            dir[i] = '/';
        }
    }
}

static void sb_factory_file_close_upload(void)
{
    if (s_file_upload.fp != 0) {
        (void)ql_fsync(s_file_upload.fp);
        (void)ql_fclose(s_file_upload.fp);
    }
    s_file_upload.fp = 0;
    s_file_upload.active = 0;
}

static sb_status_t sb_factory_file_begin_from_json(const char *json,
                                                   sb_factory_channel_t channel,
                                                   char *reply,
                                                   u32 reply_len)
{
    char path_in[SB_FACTORY_FILE_PATH_LEN];
    char path[SB_FACTORY_FILE_PATH_LEN];
    u64 size = 0u;
    u64 crc32 = 0u;

    if (sb_factory_file_access_allowed(channel) == 0) {
        (void)sb_factory_reply_status(reply, reply_len, "rejected", "locked");
        return SB_STATUS_SECURITY_ERROR;
    }
    sb_factory_zero_buffer(path_in, (u32)sizeof(path_in));
    sb_factory_zero_buffer(path, (u32)sizeof(path));
    if (sb_json_get_string(json, "path", path_in, (u32)sizeof(path_in)) != SB_STATUS_OK) {
        return sb_factory_reply_status(reply, reply_len, "error", "path");
    }
    if ((sb_json_get_u64(json, "size", &size) != SB_STATUS_OK) || (size == 0u)) {
        return sb_factory_reply_status(reply, reply_len, "error", "size");
    }
    (void)sb_json_get_u64(json, "crc32", &crc32);
    if (sb_factory_file_to_ufs_path(path_in, path, (u32)sizeof(path)) != SB_STATUS_OK) {
        return sb_factory_reply_status(reply, reply_len, "error", "path");
    }

    sb_factory_file_close_upload();
    sb_factory_file_make_parent_dirs(path);
    (void)ql_remove(path);
    s_file_upload.fp = ql_fopen(path, "w+");
    if (s_file_upload.fp == 0) {
        return sb_factory_reply_status(reply, reply_len, "error", "open");
    }
    sb_cloud_copy_string(s_file_upload.path, (u32)sizeof(s_file_upload.path), path);
    s_file_upload.expected_size = (u32)size;
    s_file_upload.expected_crc32 = (u32)crc32;
    s_file_upload.received_size = 0u;
    s_file_upload.crc_state = SB_CRC32_INITIAL_VALUE;
    s_file_upload.active = 1;
    SB_LOGI(SB_FACTORY_MODULE_NAME, "file_begin path=%s size=%u crc32=%08x", path, s_file_upload.expected_size, s_file_upload.expected_crc32);
    return sb_factory_reply_status(reply, reply_len, "ok", "file_begin");
}

static sb_status_t sb_factory_file_chunk_from_json(const char *json,
                                                   sb_factory_channel_t channel,
                                                   char *reply,
                                                   u32 reply_len)
{
    static u8 data[1024];
    char data_hex[SB_FACTORY_FILE_HEX_MAX + 1u];
    u64 offset = 0u;
    u32 data_len = 0u;
    int written;

    if (sb_factory_file_access_allowed(channel) == 0) {
        (void)sb_factory_reply_status(reply, reply_len, "rejected", "locked");
        return SB_STATUS_SECURITY_ERROR;
    }
    if ((s_file_upload.active == 0) || (s_file_upload.fp == 0)) {
        return sb_factory_reply_status(reply, reply_len, "error", "inactive");
    }
    if (sb_json_get_u64(json, "offset", &offset) != SB_STATUS_OK) {
        return sb_factory_reply_status(reply, reply_len, "error", "offset");
    }
    if ((u32)offset != s_file_upload.received_size) {
        return sb_factory_reply_status(reply, reply_len, "error", "offset");
    }
    data_hex[0] = '\0';
    if (sb_json_get_string(json, "data_hex", data_hex, (u32)sizeof(data_hex)) != SB_STATUS_OK) {
        if (sb_json_get_string(json, "data", data_hex, (u32)sizeof(data_hex)) != SB_STATUS_OK) {
            return sb_factory_reply_status(reply, reply_len, "error", "data");
        }
    }
    if (sb_factory_hex_to_bytes_partial(data_hex, data, (u32)sizeof(data), &data_len) != SB_STATUS_OK) {
        return sb_factory_reply_status(reply, reply_len, "error", "data");
    }
    if ((s_file_upload.received_size + data_len) > s_file_upload.expected_size) {
        return sb_factory_reply_status(reply, reply_len, "error", "size");
    }
    written = ql_fwrite(data, 1u, data_len, s_file_upload.fp);
    if (written != (int)data_len) {
        sb_factory_file_close_upload();
        return sb_factory_reply_status(reply, reply_len, "error", "write");
    }
    s_file_upload.crc_state = sb_crc32_update(s_file_upload.crc_state, data, data_len);
    s_file_upload.received_size += data_len;

    reply[0] = '\0';
    if (sb_cloud_append_string(reply, reply_len, "{\"status\":\"ok\",\"written\":") != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_cloud_append_u32(reply, reply_len, data_len) != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_cloud_append_string(reply, reply_len, ",\"received\":") != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_cloud_append_u32(reply, reply_len, s_file_upload.received_size) != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_cloud_append_string(reply, reply_len, "}") != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    return SB_STATUS_OK;
}

static sb_status_t sb_factory_file_end_from_json(sb_factory_channel_t channel,
                                                 char *reply,
                                                 u32 reply_len)
{
    u32 crc_final;

    if (sb_factory_file_access_allowed(channel) == 0) {
        (void)sb_factory_reply_status(reply, reply_len, "rejected", "locked");
        return SB_STATUS_SECURITY_ERROR;
    }
    if ((s_file_upload.active == 0) || (s_file_upload.fp == 0)) {
        return sb_factory_reply_status(reply, reply_len, "error", "inactive");
    }
    if (s_file_upload.received_size != s_file_upload.expected_size) {
        sb_factory_file_close_upload();
        return sb_factory_reply_status(reply, reply_len, "error", "size");
    }
    crc_final = s_file_upload.crc_state ^ 0xFFFFFFFFu;
    if ((s_file_upload.expected_crc32 != 0u) && (crc_final != s_file_upload.expected_crc32)) {
        sb_factory_file_close_upload();
        return sb_factory_reply_status(reply, reply_len, "error", "crc");
    }
    (void)ql_fsync(s_file_upload.fp);
    (void)ql_fclose(s_file_upload.fp);
    s_file_upload.fp = 0;
    s_file_upload.active = 0;
    SB_LOGI(SB_FACTORY_MODULE_NAME, "file_end path=%s size=%u crc32=%08x", s_file_upload.path, s_file_upload.received_size, crc_final);
    return sb_factory_reply_status(reply, reply_len, "ok", "file_end");
}

static sb_status_t sb_factory_file_status_reply(char *reply, u32 reply_len)
{
    reply[0] = '\0';
    if (sb_cloud_append_string(reply, reply_len, "{\"status\":\"ok\",\"active\":") != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_cloud_append_u32(reply, reply_len, (s_file_upload.active != 0) ? 1u : 0u) != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_cloud_append_string(reply, reply_len, ",\"path\":") != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_cloud_append_json_string(reply, reply_len, s_file_upload.path) != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_cloud_append_string(reply, reply_len, ",\"expected_size\":") != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_cloud_append_u32(reply, reply_len, s_file_upload.expected_size) != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_cloud_append_string(reply, reply_len, ",\"received_size\":") != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_cloud_append_u32(reply, reply_len, s_file_upload.received_size) != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_cloud_append_string(reply, reply_len, "}") != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    return SB_STATUS_OK;
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
    } else if (sb_cloud_text_equal(cmd, "file_begin") != 0) {
        status = sb_factory_file_begin_from_json(json, channel, reply, reply_len);
    } else if (sb_cloud_text_equal(cmd, "file_chunk") != 0) {
        status = sb_factory_file_chunk_from_json(json, channel, reply, reply_len);
    } else if (sb_cloud_text_equal(cmd, "file_end") != 0) {
        status = sb_factory_file_end_from_json(channel, reply, reply_len);
    } else if (sb_cloud_text_equal(cmd, "file_status") != 0) {
        status = sb_factory_file_status_reply(reply, reply_len);
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
