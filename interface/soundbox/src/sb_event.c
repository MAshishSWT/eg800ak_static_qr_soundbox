/*================================================================
 * Static QR UPI Soundbox - Event Definitions
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "sb_event.h"

static void sb_zero_event(sb_event_t *event)
{
    u32 i;
    unsigned char *p;

    if (event == 0) {
        return;
    }

    p = (unsigned char *)event;
    for (i = 0; i < (u32)sizeof(sb_event_t); i++) {
        p[i] = 0u;
    }
}

void sb_event_init(sb_event_t *event, sb_event_id_t id, sb_event_source_t source)
{
    if (event == 0) {
        return;
    }

    sb_zero_event(event);
    event->id = id;
    event->source = source;
    event->timestamp_ticks = ql_rtos_get_systicks();
}

sb_status_t sb_event_set_text(sb_event_t *event, const char *text)
{
    u32 i;

    if ((event == 0) || (text == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    for (i = 0; i < (SB_EVENT_TEXT_LEN - 1u); i++) {
        event->text[i] = text[i];
        if (text[i] == '\0') {
            return SB_STATUS_OK;
        }
    }

    event->text[SB_EVENT_TEXT_LEN - 1u] = '\0';
    return SB_STATUS_OK;
}

const char *sb_event_id_to_string(sb_event_id_t id)
{
    switch (id) {
    case SB_EVENT_NONE:
        return "NONE";
    case SB_EVENT_SYSTEM_BOOT:
        return "SYSTEM_BOOT";
    case SB_EVENT_SYSTEM_READY:
        return "SYSTEM_READY";
    case SB_EVENT_BOARD_READY:
        return "BOARD_READY";
    case SB_EVENT_SUPERVISOR_STARTED:
        return "SUPERVISOR_STARTED";
    case SB_EVENT_SUPERVISOR_HEARTBEAT:
        return "SUPERVISOR_HEARTBEAT";
    case SB_EVENT_SUPERVISOR_FAULT:
        return "SUPERVISOR_FAULT";
    case SB_EVENT_KEY_EDGE:
        return "KEY_EDGE";
    case SB_EVENT_KEY_SHORT_PRESS:
        return "KEY_SHORT_PRESS";
    case SB_EVENT_KEY_LONG_PRESS:
        return "KEY_LONG_PRESS";
    case SB_EVENT_CONNECTIVITY_STATE:
        return "CONNECTIVITY_STATE";
    case SB_EVENT_AUDIO_STATE:
        return "AUDIO_STATE";
    case SB_EVENT_AUDIO_READY:
        return "AUDIO_READY";
    case SB_EVENT_AUDIO_PLAY_STARTED:
        return "AUDIO_PLAY_STARTED";
    case SB_EVENT_AUDIO_PLAY_DONE:
        return "AUDIO_PLAY_DONE";
    case SB_EVENT_AUDIO_FAULT:
        return "AUDIO_FAULT";
    case SB_EVENT_BATTERY_SAMPLE:
        return "BATTERY_SAMPLE";
    case SB_EVENT_STORAGE_READY:
        return "STORAGE_READY";
    case SB_EVENT_EXTNOR_READY:
        return "EXTNOR_READY";
    case SB_EVENT_EXTNOR_FAULT:
        return "EXTNOR_FAULT";
    case SB_EVENT_CONFIG_READY:
        return "CONFIG_READY";
    case SB_EVENT_SIM_READY:
        return "SIM_READY";
    case SB_EVENT_SIM_FAULT:
        return "SIM_FAULT";
    case SB_EVENT_NETWORK_REGISTERED:
        return "NETWORK_REGISTERED";
    case SB_EVENT_NETWORK_LOST:
        return "NETWORK_LOST";
    case SB_EVENT_DATACALL_READY:
        return "DATACALL_READY";
    case SB_EVENT_DATACALL_FAULT:
        return "DATACALL_FAULT";
    case SB_EVENT_CSQ_SAMPLE:
        return "CSQ_SAMPLE";
    case SB_EVENT_TIME_SYNCED:
        return "TIME_SYNCED";
    case SB_EVENT_TIME_FAULT:
        return "TIME_FAULT";
    case SB_EVENT_MQTT_READY:
        return "MQTT_READY";
    case SB_EVENT_MQTT_FAULT:
        return "MQTT_FAULT";
    case SB_EVENT_MQTT_DISCONNECTED:
        return "MQTT_DISCONNECTED";
    case SB_EVENT_MQTT_MESSAGE:
        return "MQTT_MESSAGE";
    case SB_EVENT_MQTT_PAYMENT_MESSAGE:
        return "MQTT_PAYMENT_MESSAGE";
    case SB_EVENT_MQTT_COMMAND_MESSAGE:
        return "MQTT_COMMAND_MESSAGE";
    case SB_EVENT_MQTT_PUBLISHED:
        return "MQTT_PUBLISHED";
    case SB_EVENT_HTTP_HEALTH_DONE:
        return "HTTP_HEALTH_DONE";
    case SB_EVENT_HTTP_COMMAND_RESPONSE_DONE:
        return "HTTP_COMMAND_RESPONSE_DONE";
    case SB_EVENT_HTTP_FAULT:
        return "HTTP_FAULT";
    case SB_EVENT_PAYMENT_ACCEPTED:
        return "PAYMENT_ACCEPTED";
    case SB_EVENT_PAYMENT_DUPLICATE:
        return "PAYMENT_DUPLICATE";
    case SB_EVENT_PAYMENT_FAULT:
        return "PAYMENT_FAULT";
    case SB_EVENT_COMMAND_ACCEPTED:
        return "COMMAND_ACCEPTED";
    case SB_EVENT_COMMAND_REJECTED:
        return "COMMAND_REJECTED";
    case SB_EVENT_KEY_ACTION:
        return "KEY_ACTION";
    case SB_EVENT_DAILY_SUMMARY_READY:
        return "DAILY_SUMMARY_READY";
    case SB_EVENT_OTA_STARTED:
        return "OTA_STARTED";
    case SB_EVENT_OTA_PROGRESS:
        return "OTA_PROGRESS";
    case SB_EVENT_OTA_STAGED:
        return "OTA_STAGED";
    case SB_EVENT_OTA_FAILED:
        return "OTA_FAILED";
    case SB_EVENT_FACTORY_READY:
        return "FACTORY_READY";
    case SB_EVENT_FACTORY_COMMAND:
        return "FACTORY_COMMAND";
    case SB_EVENT_FACTORY_COMMAND_DONE:
        return "FACTORY_COMMAND_DONE";
    case SB_EVENT_FACTORY_COMMAND_REJECTED:
        return "FACTORY_COMMAND_REJECTED";
    case SB_EVENT_SERIAL_READY:
        return "SERIAL_READY";
    case SB_EVENT_SERIAL_COMMAND:
        return "SERIAL_COMMAND";
    case SB_EVENT_SMS_READY:
        return "SMS_READY";
    case SB_EVENT_SMS_COMMAND:
        return "SMS_COMMAND";
    case SB_EVENT_SMS_FAULT:
        return "SMS_FAULT";
    default:
        return "UNKNOWN";
    }
}
