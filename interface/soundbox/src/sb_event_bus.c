/*================================================================
 * Static QR UPI Soundbox - Event Bus
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "sb_event_bus.h"

static ql_queue_t s_event_queue = 0;
static int s_event_bus_ready = 0;

sb_status_t sb_event_bus_init(void)
{
    QlOSStatus ret;

    if (s_event_bus_ready != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }

    ret = ql_rtos_queue_create(&s_event_queue, (u32)sizeof(sb_event_t), SB_EVENT_QUEUE_DEPTH);
    if (ret != 0) {
        s_event_queue = 0;
        return SB_STATUS_QUEUE_ERROR;
    }

    s_event_bus_ready = 1;
    return SB_STATUS_OK;
}

sb_status_t sb_event_bus_deinit(void)
{
    if (s_event_bus_ready == 0) {
        return SB_STATUS_NOT_READY;
    }

    if (ql_rtos_queue_delete(s_event_queue) != 0) {
        return SB_STATUS_QUEUE_ERROR;
    }

    s_event_queue = 0;
    s_event_bus_ready = 0;
    return SB_STATUS_OK;
}

sb_status_t sb_event_post(const sb_event_t *event, u32 timeout_ms)
{
    if ((s_event_bus_ready == 0) || (s_event_queue == 0)) {
        return SB_STATUS_NOT_READY;
    }

    if (event == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (ql_rtos_queue_release(s_event_queue, (u32)sizeof(sb_event_t), (u8 *)event, timeout_ms) != 0) {
        return SB_STATUS_QUEUE_ERROR;
    }

    return SB_STATUS_OK;
}

sb_status_t sb_event_wait(sb_event_t *event, u32 timeout_ms)
{
    if ((s_event_bus_ready == 0) || (s_event_queue == 0)) {
        return SB_STATUS_NOT_READY;
    }

    if (event == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (ql_rtos_queue_wait(s_event_queue, (u8 *)event, (u32)sizeof(sb_event_t), timeout_ms) != 0) {
        return SB_STATUS_TIMEOUT;
    }

    return SB_STATUS_OK;
}

u32 sb_event_pending_count(void)
{
    u32 count = 0u;

    if ((s_event_bus_ready == 0) || (s_event_queue == 0)) {
        return 0u;
    }

    if (ql_rtos_queue_get_cnt(s_event_queue, &count) != 0) {
        return 0u;
    }

    return count;
}
