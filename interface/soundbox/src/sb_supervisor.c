/*================================================================
 * Static QR UPI Soundbox - Application Supervisor
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "sb_bsp_kae8_sq1.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_log.h"
#include "sb_supervisor.h"

#define SB_SUPERVISOR_MODULE_NAME "supervisor"

static ql_task_t s_supervisor_task = 0;
static ql_timer_t s_heartbeat_timer = 0;
static int s_supervisor_started = 0;

static void sb_supervisor_log_battery_sample(void)
{
    sb_battery_sample_t sample;
    sb_status_t status;

    status = sb_bsp_board_read_battery(&sample);
    if (status == SB_STATUS_OK) {
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME,
                "battery adc=%umV battery=%umV percent=%u",
                sample.adc_mv, sample.battery_mv, sample.battery_percent);
    } else {
        SB_LOGW(SB_SUPERVISOR_MODULE_NAME,
                "battery sample status=%s",
                sb_status_to_string(status));
    }
}

static void sb_supervisor_timer_cb(u32 timer_arg)
{
    sb_event_t event;

    (void)timer_arg;
    sb_event_init(&event, SB_EVENT_SUPERVISOR_HEARTBEAT, SB_EVENT_SOURCE_TIMER);
    (void)sb_event_post(&event, QL_NO_WAIT);
}

static void sb_supervisor_handle_event(const sb_event_t *event)
{
    if (event == 0) {
        return;
    }

    switch (event->id) {
    case SB_EVENT_SYSTEM_BOOT:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "boot event received");
        break;

    case SB_EVENT_BOARD_READY:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "board ready");
        break;

    case SB_EVENT_SUPERVISOR_STARTED:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "started event received");
        break;

    case SB_EVENT_SUPERVISOR_HEARTBEAT:
        (void)sb_bsp_board_toggle_status_led();
        sb_supervisor_log_battery_sample();
        SB_LOGD(SB_SUPERVISOR_MODULE_NAME, "heartbeat pending=%u", sb_event_pending_count());
        break;

    case SB_EVENT_KEY_EDGE:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "key=%u pressed=%d", event->param_u32, event->param_s32);
        break;

    case SB_EVENT_AUDIO_READY:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "audio ready status=%d", event->param_s32);
        break;

    case SB_EVENT_AUDIO_PLAY_STARTED:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "audio play started %s", event->text);
        break;

    case SB_EVENT_AUDIO_PLAY_DONE:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "audio play done");
        break;

    case SB_EVENT_AUDIO_FAULT:
        SB_LOGW(SB_SUPERVISOR_MODULE_NAME, "audio fault status=%d text=%s", event->param_s32, event->text);
        break;

    case SB_EVENT_BATTERY_SAMPLE:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "battery=%umV percent=%d", event->param_u32, event->param_s32);
        break;

    case SB_EVENT_STORAGE_READY:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "userfs ready status=%d", event->param_s32);
        break;

    case SB_EVENT_EXTNOR_READY:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "external nor ready capacity=%u status=%d",
                event->param_u32, event->param_s32);
        break;

    case SB_EVENT_EXTNOR_FAULT:
        SB_LOGW(SB_SUPERVISOR_MODULE_NAME, "external nor fault capacity=%u status=%d",
                event->param_u32, event->param_s32);
        break;

    case SB_EVENT_CONFIG_READY:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "config ready seq=%u source=%d", event->param_u32, event->param_s32);
        break;

    case SB_EVENT_SUPERVISOR_FAULT:
        SB_LOGE(SB_SUPERVISOR_MODULE_NAME, "fault event code=%d", event->param_s32);
        break;

    default:
        SB_LOGD(SB_SUPERVISOR_MODULE_NAME, "event=%s source=%d", sb_event_id_to_string(event->id), event->source);
        break;
    }
}

static void sb_supervisor_task(void *argv)
{
    sb_event_t event;
    sb_status_t status;

    (void)argv;

    SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "task started");

    sb_event_init(&event, SB_EVENT_SUPERVISOR_STARTED, SB_EVENT_SOURCE_SUPERVISOR);
    (void)sb_event_post(&event, QL_NO_WAIT);

    if (s_heartbeat_timer != 0) {
        if (ql_rtos_timer_start(s_heartbeat_timer, SB_SUPERVISOR_HEARTBEAT_PERIOD_MS, TRUE,
                                sb_supervisor_timer_cb, 0u) != 0) {
            SB_LOGE(SB_SUPERVISOR_MODULE_NAME, "heartbeat timer start failed");
        }
    }

    while (1) {
        status = sb_event_wait(&event, QL_WAIT_FOREVER);
        if (status == SB_STATUS_OK) {
            sb_supervisor_handle_event(&event);
        } else {
            SB_LOGW(SB_SUPERVISOR_MODULE_NAME, "event wait status=%s", sb_status_to_string(status));
            ql_rtos_task_sleep_ms(100u);
        }
    }
}

sb_status_t sb_supervisor_post_boot_event(void)
{
    sb_event_t event;

    sb_event_init(&event, SB_EVENT_SYSTEM_BOOT, SB_EVENT_SOURCE_SYSTEM);
    return sb_event_post(&event, QL_NO_WAIT);
}

sb_status_t sb_supervisor_start(void)
{
    QlOSStatus ret;

    if (s_supervisor_started != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }

    ret = ql_rtos_timer_create(&s_heartbeat_timer);
    if (ret != 0) {
        s_heartbeat_timer = 0;
        return SB_STATUS_TIMER_ERROR;
    }

    ret = ql_rtos_task_create(&s_supervisor_task,
                              SB_SUPERVISOR_TASK_STACK_SIZE_BYTES,
                              SB_SUPERVISOR_TASK_PRIORITY,
                              "sb_supervisor",
                              sb_supervisor_task,
                              0);
    if (ret != 0) {
        (void)ql_rtos_timer_delete(s_heartbeat_timer);
        s_heartbeat_timer = 0;
        s_supervisor_task = 0;
        return SB_STATUS_TASK_ERROR;
    }

    s_supervisor_started = 1;
    return SB_STATUS_OK;
}
