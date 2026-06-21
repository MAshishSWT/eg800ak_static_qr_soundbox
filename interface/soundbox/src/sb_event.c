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
    case SB_EVENT_FACTORY_COMMAND:
        return "FACTORY_COMMAND";
    default:
        return "UNKNOWN";
    }
}
