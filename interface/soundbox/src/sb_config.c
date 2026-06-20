/*================================================================
 * Static QR UPI Soundbox - Configuration Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "sb_config.h"
#include "sb_crc32.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_log.h"
#include "sb_storage_fs.h"
#include "sb_storage_nor.h"

#define SB_CONFIG_MODULE_NAME "config"
#define SB_CONFIG_OFFSET_OF(type, member) ((u32)((unsigned long)&(((type *)0)->member)))

typedef struct {
    u32 magic;
    u16 version;
    u16 header_size;
    u32 sequence;
    u32 slot_id;
    u32 payload_size;
    u32 payload_crc;
    u32 header_crc;
    sb_config_payload_t payload;
} sb_config_record_t;

static sb_config_payload_t s_config;
static sb_config_state_t s_state = {0u, SB_CONFIG_SLOT_A, 0};
static ql_mutex_t s_config_mutex = 0;
static int s_config_ready = 0;


static sb_status_t sb_config_mutex_init(void)
{
    if (s_config_mutex != 0) {
        return SB_STATUS_OK;
    }

    if (ql_rtos_mutex_create(&s_config_mutex) != 0) {
        s_config_mutex = 0;
        return SB_STATUS_ERROR;
    }

    return SB_STATUS_OK;
}

static sb_status_t sb_config_lock(void)
{
    if (s_config_mutex == 0) {
        return SB_STATUS_NOT_READY;
    }

    if (ql_rtos_mutex_lock(s_config_mutex, QL_WAIT_FOREVER) != 0) {
        return SB_STATUS_ERROR;
    }

    return SB_STATUS_OK;
}

static void sb_config_unlock(void)
{
    if (s_config_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_config_mutex);
    }
}

static void sb_config_zero(void *ptr, u32 length)
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

static void sb_config_copy(void *dst, const void *src, u32 length)
{
    u32 i;
    unsigned char *d;
    const unsigned char *s;

    if ((dst == 0) || (src == 0) || (length == 0u)) {
        return;
    }

    d = (unsigned char *)dst;
    s = (const unsigned char *)src;
    for (i = 0u; i < length; i++) {
        d[i] = s[i];
    }
}

static void sb_config_copy_string(char *dst, u32 dst_len, const char *src)
{
    u32 i;

    if ((dst == 0) || (dst_len == 0u)) {
        return;
    }

    if (src == 0) {
        dst[0] = '\0';
        return;
    }

    for (i = 0u; i < (dst_len - 1u); i++) {
        dst[i] = src[i];
        if (src[i] == '\0') {
            return;
        }
    }

    dst[dst_len - 1u] = '\0';
}

static void sb_config_sanitize(sb_config_payload_t *config)
{
    if (config == 0) {
        return;
    }

    config->device_id[SB_CONFIG_DEVICE_ID_LEN - 1u] = '\0';
    config->apn[SB_CONFIG_APN_LEN - 1u] = '\0';
    config->mqtt_host[SB_CONFIG_MQTT_HOST_LEN - 1u] = '\0';
    config->mqtt_client_id[SB_CONFIG_MQTT_CLIENT_ID_LEN - 1u] = '\0';
    config->mqtt_sub_topic[SB_CONFIG_TOPIC_LEN - 1u] = '\0';
    config->mqtt_pub_topic[SB_CONFIG_TOPIC_LEN - 1u] = '\0';
    config->http_base_url[SB_CONFIG_HTTP_BASE_URL_LEN - 1u] = '\0';
    config->language[SB_CONFIG_LANG_CODE_LEN - 1u] = '\0';

    if (config->volume_percent > 100u) {
        config->volume_percent = 100u;
    }

    config->sms_recovery_enabled = (config->sms_recovery_enabled != 0u) ? 1u : 0u;

    if (config->health_interval_sec < 30u) {
        config->health_interval_sec = 30u;
    }

    if (config->mqtt_keepalive_sec < 30u) {
        config->mqtt_keepalive_sec = 30u;
    }
}

void sb_config_make_defaults(sb_config_payload_t *config)
{
    if (config == 0) {
        return;
    }

    sb_config_zero(config, (u32)sizeof(*config));
    sb_config_copy_string(config->language, SB_CONFIG_LANG_CODE_LEN, "en");
    config->mqtt_port = 8883u;
    config->volume_percent = 70u;
    config->sms_recovery_enabled = 0u;
    config->log_level = 2u;
    config->health_interval_sec = 300u;
    config->mqtt_keepalive_sec = 60u;
}

static void sb_config_prepare_record(sb_config_record_t *record, const sb_config_payload_t *payload,
                                     sb_config_slot_t slot, u32 sequence)
{
    if ((record == 0) || (payload == 0)) {
        return;
    }

    sb_config_zero(record, (u32)sizeof(*record));
    record->magic = SB_CONFIG_MAGIC;
    record->version = SB_CONFIG_VERSION;
    record->header_size = (u16)SB_CONFIG_OFFSET_OF(sb_config_record_t, payload);
    record->sequence = sequence;
    record->slot_id = (u32)slot;
    record->payload_size = (u32)sizeof(sb_config_payload_t);
    (void)sb_config_copy(&record->payload, payload, sizeof(sb_config_payload_t));
    sb_config_sanitize(&record->payload);
    record->payload_crc = sb_crc32_compute(&record->payload, record->payload_size);
    record->header_crc = sb_crc32_compute(record, (u32)SB_CONFIG_OFFSET_OF(sb_config_record_t, header_crc));
}

static sb_status_t sb_config_validate_record(const sb_config_record_t *record, sb_config_slot_t expected_slot)
{
    u32 header_crc;
    u32 payload_crc;

    if (record == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    if ((record->magic != SB_CONFIG_MAGIC) || (record->version != SB_CONFIG_VERSION)) {
        return SB_STATUS_INVALID_STATE;
    }

    if ((record->header_size != (u16)SB_CONFIG_OFFSET_OF(sb_config_record_t, payload)) ||
        (record->payload_size != (u32)sizeof(sb_config_payload_t)) ||
        (record->slot_id != (u32)expected_slot)) {
        return SB_STATUS_INVALID_STATE;
    }

    header_crc = sb_crc32_compute(record, (u32)SB_CONFIG_OFFSET_OF(sb_config_record_t, header_crc));
    if (header_crc != record->header_crc) {
        return SB_STATUS_CRC_ERROR;
    }

    payload_crc = sb_crc32_compute(&record->payload, record->payload_size);
    if (payload_crc != record->payload_crc) {
        return SB_STATUS_CRC_ERROR;
    }

    return SB_STATUS_OK;
}

static const char *sb_config_path_for_slot(sb_config_slot_t slot)
{
    return (slot == SB_CONFIG_SLOT_A) ? SB_STORAGE_CONFIG_SLOT_A_PATH : SB_STORAGE_CONFIG_SLOT_B_PATH;
}

static sb_status_t sb_config_load_slot(sb_config_slot_t slot, sb_config_record_t *record)
{
    sb_status_t status;

    if (record == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_storage_fs_read_file(sb_config_path_for_slot(slot), record, (u32)sizeof(*record));
    if (status != SB_STATUS_OK) {
        return status;
    }

    return sb_config_validate_record(record, slot);
}

static sb_config_slot_t sb_config_next_slot(sb_config_slot_t slot)
{
    return (slot == SB_CONFIG_SLOT_A) ? SB_CONFIG_SLOT_B : SB_CONFIG_SLOT_A;
}

static sb_status_t sb_config_write_slot(sb_config_slot_t slot, const sb_config_payload_t *payload, u32 sequence)
{
    sb_config_record_t record;

    if (payload == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    sb_config_prepare_record(&record, payload, slot, sequence);
    return sb_storage_fs_write_file_atomic(sb_config_path_for_slot(slot), SB_STORAGE_CONFIG_TEMP_PATH,
                                           &record, (u32)sizeof(record));
}

static sb_status_t sb_config_select_active(sb_config_record_t *active_record, sb_config_slot_t *active_slot)
{
    sb_config_record_t a_record;
    sb_config_record_t b_record;
    sb_status_t a_status;
    sb_status_t b_status;

    a_status = sb_config_load_slot(SB_CONFIG_SLOT_A, &a_record);
    b_status = sb_config_load_slot(SB_CONFIG_SLOT_B, &b_record);

    if ((a_status == SB_STATUS_OK) && (b_status == SB_STATUS_OK)) {
        if (a_record.sequence >= b_record.sequence) {
            (void)sb_config_copy(active_record, &a_record, sizeof(a_record));
            *active_slot = SB_CONFIG_SLOT_A;
        } else {
            (void)sb_config_copy(active_record, &b_record, sizeof(b_record));
            *active_slot = SB_CONFIG_SLOT_B;
        }
        return SB_STATUS_OK;
    }

    if (a_status == SB_STATUS_OK) {
        (void)sb_config_copy(active_record, &a_record, sizeof(a_record));
        *active_slot = SB_CONFIG_SLOT_A;
        return SB_STATUS_OK;
    }

    if (b_status == SB_STATUS_OK) {
        (void)sb_config_copy(active_record, &b_record, sizeof(b_record));
        *active_slot = SB_CONFIG_SLOT_B;
        return SB_STATUS_OK;
    }

    return SB_STATUS_NOT_FOUND;
}

static void sb_config_post_ready_event(int loaded_from_storage)
{
    sb_event_t event;

    sb_event_init(&event, SB_EVENT_CONFIG_READY, SB_EVENT_SOURCE_CONFIG);
    event.param_u32 = s_state.active_sequence;
    event.param_s32 = loaded_from_storage;
    (void)sb_event_post(&event, QL_NO_WAIT);
}

static void sb_storage_post_ready_event(sb_status_t nor_status)
{
    sb_event_t event;

    sb_event_init(&event, SB_EVENT_STORAGE_READY, SB_EVENT_SOURCE_STORAGE);
    event.param_s32 = (s32)nor_status;
    (void)sb_event_post(&event, QL_NO_WAIT);
}

sb_status_t sb_config_service_init(void)
{
    sb_config_record_t active_record;
    sb_config_slot_t active_slot = SB_CONFIG_SLOT_A;
    sb_status_t status;
    sb_status_t nor_status;

    status = sb_config_mutex_init();
    if (status != SB_STATUS_OK) {
        return status;
    }

    status = sb_config_lock();
    if (status != SB_STATUS_OK) {
        return status;
    }

    if (s_config_ready != 0) {
        sb_config_unlock();
        return SB_STATUS_ALREADY_INITIALIZED;
    }

    status = sb_storage_fs_init();
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) {
        SB_LOGE(SB_CONFIG_MODULE_NAME, "fs init status=%s", sb_status_to_string(status));
        sb_config_unlock();
        return status;
    }

    nor_status = sb_storage_nor_init();
    if ((nor_status != SB_STATUS_OK) && (nor_status != SB_STATUS_ALREADY_INITIALIZED)) {
        SB_LOGW(SB_CONFIG_MODULE_NAME, "nor probe status=%s", sb_status_to_string(nor_status));
    }
    sb_storage_post_ready_event(nor_status);

    status = sb_config_select_active(&active_record, &active_slot);
    if (status == SB_STATUS_OK) {
        sb_config_copy(&s_config, &active_record.payload, sizeof(s_config));
        sb_config_sanitize(&s_config);
        s_state.active_sequence = active_record.sequence;
        s_state.active_slot = active_slot;
        s_state.loaded_from_storage = 1;
        s_config_ready = 1;
        sb_config_post_ready_event(1);
        SB_LOGI(SB_CONFIG_MODULE_NAME, "loaded slot=%u seq=%u", (u32)s_state.active_slot, s_state.active_sequence);
        sb_config_unlock();
        return SB_STATUS_OK;
    }

    sb_config_make_defaults(&s_config);
    status = sb_config_write_slot(SB_CONFIG_SLOT_A, &s_config, 1u);
    if (status != SB_STATUS_OK) {
        SB_LOGE(SB_CONFIG_MODULE_NAME, "default commit status=%s", sb_status_to_string(status));
        sb_config_unlock();
        return status;
    }

    s_state.active_sequence = 1u;
    s_state.active_slot = SB_CONFIG_SLOT_A;
    s_state.loaded_from_storage = 0;
    s_config_ready = 1;
    sb_config_post_ready_event(0);
    SB_LOGI(SB_CONFIG_MODULE_NAME, "defaults committed");
    sb_config_unlock();
    return SB_STATUS_OK;
}

sb_status_t sb_config_get(sb_config_payload_t *config)
{
    sb_status_t status;

    if (config == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_config_lock();
    if (status != SB_STATUS_OK) {
        return status;
    }

    if (s_config_ready == 0) {
        sb_config_unlock();
        return SB_STATUS_NOT_READY;
    }

    sb_config_copy(config, &s_config, sizeof(s_config));
    sb_config_unlock();
    return SB_STATUS_OK;
}

sb_status_t sb_config_get_state(sb_config_state_t *state)
{
    sb_status_t status;

    if (state == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_config_lock();
    if (status != SB_STATUS_OK) {
        return status;
    }

    if (s_config_ready == 0) {
        sb_config_unlock();
        return SB_STATUS_NOT_READY;
    }

    sb_config_copy(state, &s_state, sizeof(s_state));
    sb_config_unlock();
    return SB_STATUS_OK;
}

sb_status_t sb_config_commit(const sb_config_payload_t *config)
{
    sb_config_payload_t sanitized;
    sb_config_slot_t slot;
    sb_status_t status;
    u32 sequence;

    if (config == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_config_lock();
    if (status != SB_STATUS_OK) {
        return status;
    }

    if (s_config_ready == 0) {
        sb_config_unlock();
        return SB_STATUS_NOT_READY;
    }

    sb_config_copy(&sanitized, config, sizeof(sanitized));
    sb_config_sanitize(&sanitized);
    slot = sb_config_next_slot(s_state.active_slot);
    sequence = s_state.active_sequence + 1u;

    status = sb_config_write_slot(slot, &sanitized, sequence);
    if (status != SB_STATUS_OK) {
        sb_config_unlock();
        return status;
    }

    sb_config_copy(&s_config, &sanitized, sizeof(s_config));
    s_state.active_sequence = sequence;
    s_state.active_slot = slot;
    s_state.loaded_from_storage = 1;
    sb_config_post_ready_event(1);
    sb_config_unlock();
    return SB_STATUS_OK;
}
