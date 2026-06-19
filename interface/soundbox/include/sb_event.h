/*================================================================
 * Static QR UPI Soundbox - Event Definitions
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_EVENT_H
#define SB_EVENT_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_EVENT_TEXT_LEN      (64u)
#define SB_EVENT_QUEUE_DEPTH   (16u)

typedef enum {
    SB_EVENT_SOURCE_SYSTEM = 0,
    SB_EVENT_SOURCE_SUPERVISOR,
    SB_EVENT_SOURCE_TIMER,
    SB_EVENT_SOURCE_KEY,
    SB_EVENT_SOURCE_CONNECTIVITY,
    SB_EVENT_SOURCE_AUDIO,
    SB_EVENT_SOURCE_FACTORY
} sb_event_source_t;

typedef enum {
    SB_EVENT_NONE = 0,
    SB_EVENT_SYSTEM_BOOT,
    SB_EVENT_SYSTEM_READY,
    SB_EVENT_BOARD_READY,
    SB_EVENT_SUPERVISOR_STARTED,
    SB_EVENT_SUPERVISOR_HEARTBEAT,
    SB_EVENT_SUPERVISOR_FAULT,
    SB_EVENT_KEY_EDGE,
    SB_EVENT_KEY_SHORT_PRESS,
    SB_EVENT_KEY_LONG_PRESS,
    SB_EVENT_CONNECTIVITY_STATE,
    SB_EVENT_AUDIO_STATE,
    SB_EVENT_BATTERY_SAMPLE,
    SB_EVENT_FACTORY_COMMAND
} sb_event_id_t;

typedef struct {
    sb_event_id_t id;
    sb_event_source_t source;
    u32 timestamp_ticks;
    u32 param_u32;
    s32 param_s32;
    char text[SB_EVENT_TEXT_LEN];
} sb_event_t;

void sb_event_init(sb_event_t *event, sb_event_id_t id, sb_event_source_t source);
sb_status_t sb_event_set_text(sb_event_t *event, const char *text);
const char *sb_event_id_to_string(sb_event_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* SB_EVENT_H */
