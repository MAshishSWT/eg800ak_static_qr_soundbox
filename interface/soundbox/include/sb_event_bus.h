/*================================================================
 * Static QR UPI Soundbox - Event Bus
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_EVENT_BUS_H
#define SB_EVENT_BUS_H

#include "ql_type.h"
#include "sb_event.h"

#ifdef __cplusplus
extern "C" {
#endif

sb_status_t sb_event_bus_init(void);
sb_status_t sb_event_bus_deinit(void);
sb_status_t sb_event_post(const sb_event_t *event, u32 timeout_ms);
sb_status_t sb_event_wait(sb_event_t *event, u32 timeout_ms);
u32 sb_event_pending_count(void);

#ifdef __cplusplus
}
#endif

#endif /* SB_EVENT_BUS_H */
