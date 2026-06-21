/*================================================================
 * Static QR UPI Soundbox - Payment Processor
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "sb_audio_service.h"
#include "sb_audio_types.h"
#include "sb_cloud_utils.h"
#include "sb_config.h"
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

static sb_status_t sb_payment_record_from_message(const sb_mqtt_inbound_message_t *message,
                                                  sb_transaction_record_t *record)
{
    char provider_text[SB_LEDGER_PROVIDER_LEN];
    sb_time_status_t time_status;
    sb_status_t status;

    if ((message == 0) || (record == 0)) {
        return SB_STATUS_INVALID_PARAM;
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
    s_payment_ready = 1;
    return SB_STATUS_OK;
}

sb_status_t sb_payment_processor_handle_message(const sb_mqtt_inbound_message_t *message)
{
    sb_transaction_record_t record;
    sb_status_t status;
    int duplicate = 0;

    if ((message == 0) || (message->type != SB_MQTT_MESSAGE_PAYMENT)) {
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_payment_record_from_message(message, &record);
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
    (void)sb_audio_service_play_amount(sb_audio_language_from_code(s_payment_config.language), record.provider, record.amount_paise);
    return SB_STATUS_OK;
}
