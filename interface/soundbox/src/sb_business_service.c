/*================================================================
 * Static QR UPI Soundbox - Business Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "sb_audio_service.h"
#include "sb_audio_types.h"
#include "sb_bsp_kae8_sq1.h"
#include "sb_business_service.h"
#include "sb_cloud_utils.h"
#include "sb_command_dispatcher.h"
#include "sb_config.h"
#include "sb_demo_profile.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_hal_key.h"
#include "sb_log.h"
#include "sb_led_status.h"
#include "sb_mqtt_service.h"
#include "sb_network_service.h"
#include "sb_payment_processor.h"
#include "sb_time_service.h"
#include "sb_transaction_ledger.h"

#define SB_BUSINESS_MODULE_NAME "business"
#define SB_BUSINESS_SHORT_DELAY_MS (20u)

typedef struct {
    int pressed;
    u32 tick;
} sb_business_key_state_t;

static ql_task_t s_business_task = 0;
static ql_mutex_t s_business_mutex = 0;
static int s_business_started = 0;
static sb_config_payload_t s_business_config;
static sb_business_key_state_t s_key_state[4];

static void sb_business_post_event(sb_event_id_t id, s32 status, u32 value, const char *text)
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

static u32 sb_business_elapsed_ms(u32 start_tick, u32 end_tick)
{
    u32 elapsed_ticks = end_tick - start_tick;

    return (elapsed_ticks * 5u);
}

const char *sb_business_key_action_name(sb_business_key_action_t action)
{
    switch (action) {
    case SB_BUSINESS_KEY_ACTION_VOLUME_UP:
        return "volume_up";
    case SB_BUSINESS_KEY_ACTION_VOLUME_DOWN:
        return "volume_down";
    case SB_BUSINESS_KEY_ACTION_LAST_TRANSACTION:
        return "last_transaction";
    case SB_BUSINESS_KEY_ACTION_BATTERY:
        return "battery";
    case SB_BUSINESS_KEY_ACTION_SIGNAL:
        return "signal";
    case SB_BUSINESS_KEY_ACTION_DAILY_SUMMARY:
        return "daily_summary";
    default:
        return "none";
    }
}

static sb_audio_language_t sb_business_language(void)
{
    return sb_audio_language_from_code(s_business_config.language);
}

static sb_status_t sb_business_save_volume(u32 volume)
{
    sb_status_t status;

    if (volume > 100u) {
        volume = 100u;
    }

    (void)ql_rtos_mutex_lock(s_business_mutex, QL_WAIT_FOREVER);
    s_business_config.volume_percent = volume;
    status = sb_config_commit(&s_business_config);
    (void)ql_rtos_mutex_unlock(s_business_mutex);

    if (status == SB_STATUS_OK) {
        status = sb_audio_service_set_volume(volume);
    }

    return status;
}

static void sb_business_handle_volume(int increase)
{
    u32 volume;
    u32 old_volume;
    const char *action_text;

    (void)ql_rtos_mutex_lock(s_business_mutex, QL_WAIT_FOREVER);
    old_volume = s_business_config.volume_percent;
    volume = old_volume;
    (void)ql_rtos_mutex_unlock(s_business_mutex);

    (void)sb_audio_service_play_common("ping.mp3");
    (void)sb_led_status_set(SB_LED_STATUS_VOLUME_MODE);

    if (increase != 0) {
        volume = (volume + SB_BUSINESS_VOLUME_STEP_PERCENT > 100u) ? 100u : (volume + SB_BUSINESS_VOLUME_STEP_PERCENT);
        action_text = "volume_up";
    } else {
        volume = (volume < SB_BUSINESS_VOLUME_STEP_PERCENT) ? 0u : (volume - SB_BUSINESS_VOLUME_STEP_PERCENT);
        action_text = "volume_down";
    }

    if (volume != old_volume) {
        (void)sb_business_save_volume(volume);
    }

    if ((increase != 0) && (volume >= 100u)) {
        (void)sb_audio_service_play_alert(sb_business_language(), "volume_max.mp3");
    } else if ((increase == 0) && (volume == 0u)) {
        (void)sb_audio_service_play_alert(sb_business_language(), "volume_min.mp3");
    }

    sb_business_post_event(SB_EVENT_KEY_ACTION, SB_STATUS_OK, volume, action_text);
}

static void sb_business_play_last_transaction(void)
{
    sb_transaction_record_t record;
    sb_status_t status;

    (void)sb_led_status_set(SB_LED_STATUS_TRANSACTION_MODE);
    status = sb_transaction_ledger_get_last(&record);
    if (status == SB_STATUS_OK) {
        (void)sb_audio_service_play_last_transaction(sb_business_language(), &record);
        sb_business_post_event(SB_EVENT_KEY_ACTION, SB_STATUS_OK, (u32)record.amount_paise, "last_transaction");
    } else {
        (void)sb_audio_service_play_alert(sb_business_language(), "no_transactions.mp3");
        sb_business_post_event(SB_EVENT_KEY_ACTION, (s32)status, 0u, "last_transaction_empty");
    }
}

static void sb_business_play_battery_status(void)
{
    sb_battery_sample_t sample;
    sb_status_t status;

    status = sb_bsp_board_read_battery(&sample);
    if (status == SB_STATUS_OK) {
        (void)sb_audio_service_play_health(sb_business_language(), 1, sample.battery_percent);
        if (sample.battery_percent <= 15u) {
            (void)sb_led_status_set(SB_LED_STATUS_BATTERY_LOW);
        }
        sb_business_post_event(SB_EVENT_KEY_ACTION, SB_STATUS_OK, sample.battery_percent, "battery");
    } else {
        sb_business_post_event(SB_EVENT_KEY_ACTION, (s32)status, 0u, "battery");
    }
}

static void sb_business_play_signal_status(void)
{
    sb_network_status_t net;
    sb_status_t status;
    u32 signal_percent;

    status = sb_network_get_status(&net);
    if (status == SB_STATUS_OK) {
        signal_percent = (net.csq <= 0) ? 0u : (u32)((net.csq >= 31) ? 100 : ((net.csq * 100) / 31));
        (void)sb_audio_service_play_health(sb_business_language(), 0, signal_percent);
        sb_business_post_event(SB_EVENT_KEY_ACTION, SB_STATUS_OK, (u32)net.csq, "signal");
    } else {
        sb_business_post_event(SB_EVENT_KEY_ACTION, (s32)status, 0u, "signal");
    }
}

static void sb_business_play_daily_summary(void)
{
    sb_daily_summary_t summary;
    sb_status_t status;

    status = sb_transaction_ledger_get_daily(&summary);
    if (status == SB_STATUS_OK) {
        if (summary.count > 0u) {
            (void)sb_audio_service_play_daily_summary(sb_business_language(), &summary);
        } else {
            (void)sb_audio_service_play_alert(sb_business_language(), "no_transactions.mp3");
        }
        sb_business_post_event(SB_EVENT_DAILY_SUMMARY_READY, SB_STATUS_OK, summary.count, "daily_summary");
    } else {
        sb_business_post_event(SB_EVENT_DAILY_SUMMARY_READY, (s32)status, 0u, "daily_summary");
    }
}

static void sb_business_handle_short_key(u32 key_id)
{
    if (key_id == (u32)SB_KEY_VOLUME_UP) {
        sb_business_handle_volume(1);
    } else if (key_id == (u32)SB_KEY_VOLUME_DOWN) {
        sb_business_handle_volume(0);
    } else if (key_id == (u32)SB_KEY_MODE) {
        sb_business_play_last_transaction();
    } else {
        sb_business_post_event(SB_EVENT_KEY_ACTION, SB_STATUS_INVALID_PARAM, key_id, "unknown_key");
    }
}

static void sb_business_handle_long_key(u32 key_id)
{
    if (key_id == (u32)SB_KEY_VOLUME_UP) {
        sb_business_play_signal_status();
    } else if (key_id == (u32)SB_KEY_VOLUME_DOWN) {
        sb_business_play_battery_status();
    } else if (key_id == (u32)SB_KEY_MODE) {
        sb_business_play_daily_summary();
    } else {
        sb_business_post_event(SB_EVENT_KEY_ACTION, SB_STATUS_INVALID_PARAM, key_id, "unknown_key");
    }
}

sb_status_t sb_business_service_handle_key_edge(u32 key_id, int pressed, u32 timestamp_ticks)
{
    u32 elapsed_ms;

    if ((s_business_started == 0) || (key_id >= (u32)(sizeof(s_key_state) / sizeof(s_key_state[0])))) {
        return SB_STATUS_NOT_READY;
    }

    if (pressed != 0) {
        s_key_state[key_id].pressed = 1;
        s_key_state[key_id].tick = timestamp_ticks;
        return SB_STATUS_OK;
    }

    if (s_key_state[key_id].pressed == 0) {
        return SB_STATUS_OK;
    }

    elapsed_ms = sb_business_elapsed_ms(s_key_state[key_id].tick, timestamp_ticks);
    s_key_state[key_id].pressed = 0;

    if (elapsed_ms >= SB_BUSINESS_LONG_PRESS_MS) {
        sb_business_handle_long_key(key_id);
    } else {
        sb_business_handle_short_key(key_id);
    }

    return SB_STATUS_OK;
}

static sb_status_t sb_business_reset_daily_by_rtc(void)
{
    sb_time_status_t time_status;
    u32 date;

    if (sb_time_get_rtc(&time_status) != SB_STATUS_OK) {
        return SB_STATUS_TIME_ERROR;
    }
    if (time_status.rtc_valid == 0) {
        return SB_STATUS_TIME_ERROR;
    }
    date = sb_transaction_date_from_rtc(&time_status.rtc);
    return sb_transaction_ledger_reset_daily_if_needed(date);
}

static sb_status_t sb_business_append_checked(sb_status_t status)
{
    if (status == SB_STATUS_OK) {
        return SB_STATUS_OK;
    }
    if (status == SB_STATUS_NO_MEMORY) {
        return SB_STATUS_NO_MEMORY;
    }
    return SB_STATUS_ERROR;
}

static sb_status_t sb_business_append_amount_text(char *payload, u32 payload_len, u64 amount_paise)
{
    u32 rupees;
    u32 paise;

    rupees = (u32)(amount_paise / 100u);
    paise = (u32)(amount_paise % 100u);

    if (sb_cloud_append_u32(payload, payload_len, rupees) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(payload, payload_len, ".") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (paise < 10u) {
        if (sb_cloud_append_string(payload, payload_len, "0") != SB_STATUS_OK) {
            return SB_STATUS_NO_MEMORY;
        }
    }
    return sb_cloud_append_u32(payload, payload_len, paise);
}

sb_status_t sb_business_service_build_health_payload(char *payload, u32 payload_len)
{
    sb_network_status_t net;
    sb_battery_sample_t battery;
    sb_transaction_record_t last_record;
    sb_status_t status;
    u32 csq = 0u;
    u32 battery_percent = 0u;
    u64 last_amount_paise = 0u;

    if ((payload == 0) || (payload_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }

    payload[0] = '\0';

    if (sb_network_get_status(&net) == SB_STATUS_OK) {
        csq = (net.csq < 0) ? 0u : (u32)net.csq;
    }
    if (sb_bsp_board_read_battery(&battery) == SB_STATUS_OK) {
        battery_percent = battery.battery_percent;
    }
    if (sb_transaction_ledger_get_last(&last_record) == SB_STATUS_OK) {
        last_amount_paise = last_record.amount_paise;
    }

    status = sb_business_append_checked(sb_cloud_append_string(payload, payload_len, "{\"soundboxSerial\":"));
    if (status != SB_STATUS_OK) {
        return status;
    }
    status = sb_business_append_checked(sb_cloud_append_json_string(payload, payload_len, s_business_config.device_id));
    if (status != SB_STATUS_OK) {
        return status;
    }
    status = sb_business_append_checked(sb_cloud_append_string(payload, payload_len, ",\"healthParams\":{\"networkStrength\":\""));
    if (status != SB_STATUS_OK) {
        return status;
    }
    status = sb_business_append_checked(sb_cloud_append_u32(payload, payload_len, csq));
    if (status != SB_STATUS_OK) {
        return status;
    }
    status = sb_business_append_checked(sb_cloud_append_string(payload, payload_len, "\",\"batteryLevel\":\""));
    if (status != SB_STATUS_OK) {
        return status;
    }
    status = sb_business_append_checked(sb_cloud_append_u32(payload, payload_len, battery_percent));
    if (status != SB_STATUS_OK) {
        return status;
    }
    status = sb_business_append_checked(sb_cloud_append_string(payload, payload_len, "\",\"powerConnected\":true,\"lastTransactionAmount\":\""));
    if (status != SB_STATUS_OK) {
        return status;
    }
    status = sb_business_append_checked(sb_business_append_amount_text(payload, payload_len, last_amount_paise));
    if (status != SB_STATUS_OK) {
        return status;
    }
    return sb_business_append_checked(sb_cloud_append_string(payload, payload_len, "\"}}"));
}

static void sb_business_process_mqtt_message(const sb_mqtt_inbound_message_t *message)
{
    if (message == 0) {
        return;
    }

    if (message->type == SB_MQTT_MESSAGE_PAYMENT) {
        (void)sb_payment_processor_handle_message(message);
    } else if (message->type == SB_MQTT_MESSAGE_COMMAND) {
        (void)sb_command_dispatcher_handle_message(message);
    }
}

static void sb_business_task(void *argv)
{
    sb_mqtt_inbound_message_t message;
    u32 daily_poll_count = 0u;

    (void)argv;
    SB_LOGI(SB_BUSINESS_MODULE_NAME, "task started");

    while (1) {
        if (sb_mqtt_service_receive(&message, SB_BUSINESS_POLL_MS) == SB_STATUS_OK) {
            sb_business_process_mqtt_message(&message);
        }

        daily_poll_count++;
        if (daily_poll_count >= 30u) {
            daily_poll_count = 0u;
            (void)sb_business_reset_daily_by_rtc();
        }
    }
}

sb_status_t sb_business_service_init(void)
{
    QlOSStatus ret;
    sb_status_t status;

    if (s_business_started != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }

    if (ql_rtos_mutex_create(&s_business_mutex) != 0) {
        return SB_STATUS_NO_MEMORY;
    }

    sb_config_make_defaults(&s_business_config);
    (void)sb_config_get(&s_business_config);
    sb_demo_expand_config_runtime(&s_business_config);

    status = sb_transaction_ledger_init();
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) {
        return status;
    }
    status = sb_payment_processor_init();
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) {
        return status;
    }
    status = sb_command_dispatcher_init();
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) {
        return status;
    }

    ret = ql_rtos_task_create(&s_business_task,
                              SB_BUSINESS_TASK_STACK_SIZE_BYTES,
                              SB_BUSINESS_TASK_PRIORITY,
                              "sb_business",
                              sb_business_task,
                              0);
    if (ret != 0) {
        return SB_STATUS_TASK_ERROR;
    }

    s_business_started = 1;
    return SB_STATUS_OK;
}
