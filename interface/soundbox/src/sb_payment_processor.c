/*================================================================
 * Static QR UPI Soundbox - Payment Processor
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "sb_audio_service.h"
#include "sb_audio_types.h"
#include "sb_cloud_utils.h"
#include "sb_config.h"
#include "sb_crc32.h"
#include "sb_demo_profile.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_json.h"
#include "sb_log.h"
#include "sb_payment_processor.h"
#include "sb_time_service.h"

#define SB_PAYMENT_MODULE_NAME       "payment"
#define SB_PAYMENT_ACK_PAYLOAD_LEN   (192u)

static sb_config_payload_t s_payment_config;
static int s_payment_ready = 0;

static void sb_payment_post_event(sb_event_id_t id, s32 status, u32 value, const char *text)
{
    sb_event_t event;
    sb_event_init(&event, id, SB_EVENT_SOURCE_BUSINESS);
    event.param_s32 = status;
    event.param_u32 = value;
    if (text != 0) {
        (void)sb_event_set_text(&event, text);
    }
    (void)sb_event_post(&event, QL_NO_WAIT);
}

sb_audio_provider_t sb_payment_provider_from_text(const char *provider_text)
{
    if (provider_text == 0) {
        return SB_AUDIO_PROVIDER_OTHER;
    }
    if ((sb_cloud_text_equal(provider_text, "paytm") != 0) || (sb_cloud_text_equal(provider_text, "Paytm") != 0)) {
        return SB_AUDIO_PROVIDER_PAYTM;
    }
    if ((sb_cloud_text_equal(provider_text, "phonepe") != 0) || (sb_cloud_text_equal(provider_text, "PhonePe") != 0)) {
        return SB_AUDIO_PROVIDER_PHONEPE;
    }
    if ((sb_cloud_text_equal(provider_text, "gpay") != 0) || (sb_cloud_text_equal(provider_text, "googlepay") != 0)) {
        return SB_AUDIO_PROVIDER_GPAY;
    }
    if ((sb_cloud_text_equal(provider_text, "bhim") != 0) || (sb_cloud_text_equal(provider_text, "BHIM") != 0)) {
        return SB_AUDIO_PROVIDER_BHIM;
    }
    return SB_AUDIO_PROVIDER_OTHER;
}

static sb_status_t sb_payment_build_ack(char *payload, u32 payload_len, const char *tx_id, const char *status)
{
    payload[0] = '\0';
    if (sb_cloud_append_string(payload, payload_len, "{\"type\":\"payment_ack\",\"tx_id\":") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_json_string(payload, payload_len, tx_id) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(payload, payload_len, ",\"status\":") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_json_string(payload, payload_len, status) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    return sb_cloud_append_string(payload, payload_len, "}");
}

static void sb_payment_zero_record(sb_transaction_record_t *record)
{
    u32 i;
    unsigned char *ptr;

    if (record == 0) {
        return;
    }

    ptr = (unsigned char *)record;
    for (i = 0u; i < (u32)sizeof(*record); i++) {
        ptr[i] = 0u;
    }
}

static void sb_payment_publish_ack(const char *tx_id, const char *status)
{
    char payload[SB_PAYMENT_ACK_PAYLOAD_LEN];

    if (s_payment_config.mqtt_pub_topic[0] == '\0') {
        return;
    }
    if (sb_payment_build_ack(payload, (u32)sizeof(payload), tx_id, status) == SB_STATUS_OK) {
        (void)sb_mqtt_service_publish(s_payment_config.mqtt_pub_topic, payload, sb_cloud_str_len(payload), 1u, 0u);
    }
}


static int sb_payment_payload_is_compat_hash(const char *payload)
{
    u32 i;

    if ((payload == 0) || (payload[0] == '\0') || (payload[0] == '{')) {
        return 0;
    }

    for (i = 0u; payload[i] != '\0'; i++) {
        if (payload[i] == '#') {
            return 1;
        }
    }

    return 0;
}

static sb_status_t sb_payment_read_compat_token(const char *payload,
                                                u32 *offset,
                                                char *token,
                                                u32 token_len)
{
    u32 i = 0u;
    u32 pos;

    if ((payload == 0) || (offset == 0) || (token == 0) || (token_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }

    token[0] = '\0';
    pos = *offset;
    while ((payload[pos] != '\0') && (payload[pos] != '#')) {
        if ((i + 1u) >= token_len) {
            token[token_len - 1u] = '\0';
            return SB_STATUS_NO_MEMORY;
        }
        token[i++] = payload[pos++];
    }
    token[i] = '\0';

    if (payload[pos] == '#') {
        pos++;
    }
    *offset = pos;

    return (i > 0u) ? SB_STATUS_OK : SB_STATUS_INVALID_PARAM;
}

static sb_status_t sb_payment_parse_compat_amount_paise(const char *amount_text, u64 *amount_paise)
{
    u32 i;
    u64 rupees = 0u;
    u32 paise = 0u;
    u32 paise_digits = 0u;
    int seen_dot = 0;

    if ((amount_text == 0) || (amount_paise == 0) || (amount_text[0] == '\0')) {
        return SB_STATUS_INVALID_PARAM;
    }

    for (i = 0u; amount_text[i] != '\0'; i++) {
        char c = amount_text[i];
        if (c == '.') {
            if (seen_dot != 0) {
                return SB_STATUS_INVALID_PARAM;
            }
            seen_dot = 1;
            continue;
        }
        if ((c < '0') || (c > '9')) {
            return SB_STATUS_INVALID_PARAM;
        }
        if (seen_dot == 0) {
            rupees = (rupees * 10u) + (u64)(c - '0');
        } else if (paise_digits < 2u) {
            paise = (paise * 10u) + (u32)(c - '0');
            paise_digits++;
        } else {
            return SB_STATUS_INVALID_PARAM;
        }
    }

    if ((seen_dot != 0) && (paise_digits == 1u)) {
        paise *= 10u;
    }

    *amount_paise = (rupees * 100u) + (u64)paise;
    return (*amount_paise > 0u) ? SB_STATUS_OK : SB_STATUS_INVALID_PARAM;
}

static char sb_payment_hex_digit(u32 value)
{
    value &= 0x0Fu;
    return (value < 10u) ? (char)('0' + value) : (char)('a' + (value - 10u));
}

static void sb_payment_u32_to_hex(u32 value, char *out, u32 out_len)
{
    u32 i;

    if ((out == 0) || (out_len < 9u)) {
        return;
    }

    for (i = 0u; i < 8u; i++) {
        out[i] = sb_payment_hex_digit(value >> ((7u - i) * 4u));
    }
    out[8] = '\0';
}

static void sb_payment_make_compat_tx_id(const char *payload, char *tx_id, u32 tx_id_len)
{
    char crc_hex[9];
    char tick_hex[9];
    u32 crc;
    u32 tick;

    if ((payload == 0) || (tx_id == 0) || (tx_id_len == 0u)) {
        return;
    }

    crc = sb_crc32_compute(payload, sb_cloud_str_len(payload));
    tick = ql_rtos_get_systicks();
    sb_payment_u32_to_hex(crc, crc_hex, (u32)sizeof(crc_hex));
    sb_payment_u32_to_hex(tick, tick_hex, (u32)sizeof(tick_hex));

    tx_id[0] = '\0';
    (void)sb_cloud_append_string(tx_id, tx_id_len, "compat-");
    (void)sb_cloud_append_string(tx_id, tx_id_len, crc_hex);
    (void)sb_cloud_append_string(tx_id, tx_id_len, "-");
    (void)sb_cloud_append_string(tx_id, tx_id_len, tick_hex);
}

static sb_status_t sb_payment_record_from_compat_message(const sb_mqtt_inbound_message_t *message,
                                                         sb_transaction_record_t *record,
                                                         sb_audio_language_t *play_language)
{
    char amount_text[24];
    char language_text[SB_CONFIG_LANG_CODE_LEN];
    char provider_text[SB_LEDGER_PROVIDER_LEN];
    u32 offset = 0u;
    sb_time_status_t time_status;
    sb_status_t status;

    if ((message == 0) || (record == 0) || (play_language == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_payment_read_compat_token(message->payload, &offset, amount_text, (u32)sizeof(amount_text));
    if (status != SB_STATUS_OK) {
        return status;
    }

    if ((sb_cloud_text_equal(amount_text, "conf") != 0) ||
        (sb_cloud_text_equal(amount_text, "play") != 0)) {
        return SB_STATUS_UNSUPPORTED;
    }

    status = sb_payment_read_compat_token(message->payload, &offset, language_text, (u32)sizeof(language_text));
    if (status != SB_STATUS_OK) {
        return status;
    }
    status = sb_payment_read_compat_token(message->payload, &offset, provider_text, (u32)sizeof(provider_text));
    if (status != SB_STATUS_OK) {
        return status;
    }

    sb_payment_zero_record(record);
    status = sb_payment_parse_compat_amount_paise(amount_text, &record->amount_paise);
    if (status != SB_STATUS_OK) {
        return status;
    }

    sb_payment_make_compat_tx_id(message->payload, record->tx_id, SB_LEDGER_TX_ID_LEN);
    record->provider = sb_payment_provider_from_text(provider_text);
    *play_language = sb_audio_language_from_code(language_text);

    if (sb_time_get_rtc(&time_status) == SB_STATUS_OK) {
        record->date_yyyymmdd = sb_transaction_date_from_rtc(&time_status.rtc);
        record->time_hhmmss = sb_transaction_time_from_rtc(&time_status.rtc);
    }

    return SB_STATUS_OK;
}

static sb_status_t sb_payment_record_from_message(const sb_mqtt_inbound_message_t *message,
                                                  sb_transaction_record_t *record,
                                                  sb_audio_language_t *play_language)
{
    char provider_text[SB_LEDGER_PROVIDER_LEN];
    sb_time_status_t time_status;
    sb_status_t status;

    if ((message == 0) || (record == 0) || (play_language == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    *play_language = sb_audio_language_from_code(s_payment_config.language);
    if (sb_payment_payload_is_compat_hash(message->payload) != 0) {
        return sb_payment_record_from_compat_message(message, record, play_language);
    }

    sb_payment_zero_record(record);

    status = sb_json_get_string(message->payload, "tx_id", record->tx_id, SB_LEDGER_TX_ID_LEN);
    if (status != SB_STATUS_OK) {
        status = sb_json_get_string(message->payload, "txn_id", record->tx_id, SB_LEDGER_TX_ID_LEN);
    }
    if (status != SB_STATUS_OK) {
        status = sb_json_get_string(message->payload, "transaction_id", record->tx_id, SB_LEDGER_TX_ID_LEN);
    }
    if (status != SB_STATUS_OK) {
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_json_get_amount_paise(message->payload, &record->amount_paise);
    if ((status != SB_STATUS_OK) || (record->amount_paise == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (sb_json_get_string(message->payload, "provider", provider_text, (u32)sizeof(provider_text)) == SB_STATUS_OK) {
        record->provider = sb_payment_provider_from_text(provider_text);
    } else {
        record->provider = SB_AUDIO_PROVIDER_OTHER;
    }

    if (sb_time_get_rtc(&time_status) == SB_STATUS_OK) {
        record->date_yyyymmdd = sb_transaction_date_from_rtc(&time_status.rtc);
        record->time_hhmmss = sb_transaction_time_from_rtc(&time_status.rtc);
    }

    return SB_STATUS_OK;
}

sb_status_t sb_payment_processor_init(void)
{
    if (s_payment_ready != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }
    sb_config_make_defaults(&s_payment_config);
    (void)sb_config_get(&s_payment_config);
    sb_demo_expand_config_runtime(&s_payment_config);
    s_payment_ready = 1;
    return SB_STATUS_OK;
}

sb_status_t sb_payment_processor_handle_message(const sb_mqtt_inbound_message_t *message)
{
    sb_transaction_record_t record;
    sb_audio_language_t play_language;
    sb_status_t status;
    int duplicate = 0;

    if ((message == 0) || (message->type != SB_MQTT_MESSAGE_PAYMENT)) {
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_payment_record_from_message(message, &record, &play_language);
    if (status == SB_STATUS_UNSUPPORTED) {
        SB_LOGW(SB_PAYMENT_MODULE_NAME, "compat non-payment payload ignored");
        sb_payment_post_event(SB_EVENT_PAYMENT_FAULT, (s32)status, 0u, "compat_command");
        return status;
    }
    if (status != SB_STATUS_OK) {
        SB_LOGW(SB_PAYMENT_MODULE_NAME, "invalid payment payload");
        sb_payment_post_event(SB_EVENT_PAYMENT_FAULT, (s32)status, 0u, "parse");
        (void)sb_audio_service_play_prompt(sb_audio_language_from_code(s_payment_config.language), SB_AUDIO_PROMPT_TRANSACTION_ERROR);
        return status;
    }

    status = sb_transaction_ledger_add(&record, &duplicate);
    if (status != SB_STATUS_OK) {
        sb_payment_post_event(SB_EVENT_PAYMENT_FAULT, (s32)status, 0u, "ledger");
        return status;
    }

    if (duplicate != 0) {
        SB_LOGI(SB_PAYMENT_MODULE_NAME, "duplicate tx ignored");
        sb_payment_publish_ack(record.tx_id, "duplicate");
        sb_payment_post_event(SB_EVENT_PAYMENT_DUPLICATE, SB_STATUS_OK, 0u, "duplicate");
        return SB_STATUS_OK;
    }

    SB_LOGI(SB_PAYMENT_MODULE_NAME, "payment accepted amount_paise=%u", (u32)record.amount_paise);
    sb_payment_publish_ack(record.tx_id, "accepted");
    sb_payment_post_event(SB_EVENT_PAYMENT_ACCEPTED, SB_STATUS_OK, (u32)record.amount_paise, "accepted");
    (void)sb_audio_service_play_amount(play_language, record.provider, record.amount_paise);
    return SB_STATUS_OK;
}
