/*================================================================
 * Static QR UPI Soundbox - Application Supervisor
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "sb_audio_service.h"
#include "sb_audio_types.h"
#include "sb_bsp_kae8_sq1.h"
#include "sb_config.h"
#include "sb_business_service.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_log.h"
#include "sb_led_status.h"
#include "sb_supervisor.h"

#define SB_SUPERVISOR_MODULE_NAME "supervisor"

static ql_task_t s_supervisor_task = 0;
static ql_timer_t s_heartbeat_timer = 0;
static int s_supervisor_started = 0;
static int s_alert_sim_fault_announced = 0;
static int s_alert_internet_announced = 0;
static int s_alert_no_internet_announced = 0;
static int s_alert_no_mqtt_announced = 0;
static u32 s_alert_last_battery_level = 0u;

static sb_audio_language_t sb_supervisor_language(void)
{
    sb_config_payload_t config;

    sb_config_make_defaults(&config);
    (void)sb_config_get(&config);
    return sb_audio_language_from_code(config.language);
}

static void sb_supervisor_play_alert_once(int *flag, sb_audio_prompt_id_t prompt)
{
    if (flag == 0) {
        return;
    }
    if (*flag == 0) {
        (void)sb_audio_service_play_prompt(sb_supervisor_language(), prompt);
        *flag = 1;
    }
}

static void sb_supervisor_reset_network_audio_flags(void)
{
    s_alert_internet_announced = 0;
    s_alert_no_internet_announced = 0;
    s_alert_no_mqtt_announced = 0;
}

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
        (void)sb_led_status_on_heartbeat();
        sb_supervisor_log_battery_sample();
        SB_LOGD(SB_SUPERVISOR_MODULE_NAME, "heartbeat pending=%u", sb_event_pending_count());
        break;

    case SB_EVENT_KEY_EDGE:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "key=%u pressed=%d", event->param_u32, event->param_s32);
        (void)sb_business_service_handle_key_edge(event->param_u32, event->param_s32, event->timestamp_ticks);
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
        if ((event->param_s32 >= 0) && (event->param_s32 <= 15)) {
            (void)sb_led_status_set(SB_LED_STATUS_BATTERY_LOW);
            if (((event->param_s32 <= 15) && (s_alert_last_battery_level > 15u)) ||
                ((event->param_s32 <= 10) && (s_alert_last_battery_level > 10u))) {
                (void)sb_audio_service_play_prompt(sb_supervisor_language(), SB_AUDIO_PROMPT_BATTERY_LOW);
            }
        }
        if (event->param_s32 >= 0) {
            s_alert_last_battery_level = (u32)event->param_s32;
        }
        break;

    case SB_EVENT_STORAGE_READY:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "userfs ready status=%d", event->param_s32);
        break;

    case SB_EVENT_CONFIG_READY:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "config ready seq=%u source=%d", event->param_u32, event->param_s32);
        break;

    case SB_EVENT_SIM_READY:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "sim ready status=%d", event->param_s32);
        s_alert_sim_fault_announced = 0;
        break;

    case SB_EVENT_SIM_FAULT:
        SB_LOGW(SB_SUPERVISOR_MODULE_NAME, "sim fault status=%d card=%u text=%s", event->param_s32, event->param_u32, event->text);
        (void)sb_led_status_set(SB_LED_STATUS_ERROR);
        sb_supervisor_play_alert_once(&s_alert_sim_fault_announced, SB_AUDIO_PROMPT_NO_SIM);
        break;

    case SB_EVENT_NETWORK_REGISTERED:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "network registered");
        break;

    case SB_EVENT_NETWORK_LOST:
        SB_LOGW(SB_SUPERVISOR_MODULE_NAME, "network lost status=%d text=%s", event->param_s32, event->text);
        (void)sb_led_status_set(SB_LED_STATUS_NO_INTERNET);
        s_alert_internet_announced = 0;
        sb_supervisor_play_alert_once(&s_alert_no_internet_announced, SB_AUDIO_PROMPT_NO_INTERNET);
        break;

    case SB_EVENT_DATACALL_READY:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "data call ready cid=%u status=%d", event->param_u32, event->param_s32);
        (void)sb_led_status_set(SB_LED_STATUS_INTERNET_OK);
        s_alert_no_internet_announced = 0;
        sb_supervisor_play_alert_once(&s_alert_internet_announced, SB_AUDIO_PROMPT_READY);
        break;

    case SB_EVENT_DATACALL_FAULT:
        SB_LOGW(SB_SUPERVISOR_MODULE_NAME, "data call fault status=%d text=%s", event->param_s32, event->text);
        (void)sb_led_status_set(SB_LED_STATUS_NO_INTERNET);
        s_alert_internet_announced = 0;
        sb_supervisor_play_alert_once(&s_alert_no_internet_announced, SB_AUDIO_PROMPT_NO_INTERNET);
        break;

    case SB_EVENT_CSQ_SAMPLE:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "csq=%d", event->param_s32);
        break;

    case SB_EVENT_TIME_SYNCED:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "time synced status=%d text=%s", event->param_s32, event->text);
        break;

    case SB_EVENT_TIME_FAULT:
        SB_LOGW(SB_SUPERVISOR_MODULE_NAME, "time fault status=%d text=%s", event->param_s32, event->text);
        break;

    case SB_EVENT_MQTT_READY:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "mqtt ready port=%u status=%d", event->param_u32, event->param_s32);
        (void)sb_led_status_set(SB_LED_STATUS_INTERNET_OK);
        s_alert_no_mqtt_announced = 0;
        break;

    case SB_EVENT_MQTT_FAULT:
        SB_LOGW(SB_SUPERVISOR_MODULE_NAME, "mqtt fault status=%d text=%s", event->param_s32, event->text);
        (void)sb_led_status_set(SB_LED_STATUS_NO_MQTT);
        sb_supervisor_play_alert_once(&s_alert_no_mqtt_announced, SB_AUDIO_PROMPT_NO_MQTT);
        break;

    case SB_EVENT_MQTT_DISCONNECTED:
        SB_LOGW(SB_SUPERVISOR_MODULE_NAME, "mqtt disconnected status=%d text=%s", event->param_s32, event->text);
        (void)sb_led_status_set(SB_LED_STATUS_NO_MQTT);
        sb_supervisor_play_alert_once(&s_alert_no_mqtt_announced, SB_AUDIO_PROMPT_NO_MQTT);
        break;

    case SB_EVENT_MQTT_PAYMENT_MESSAGE:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "mqtt payment message len=%u", event->param_u32);
        break;

    case SB_EVENT_MQTT_COMMAND_MESSAGE:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "mqtt command message len=%u", event->param_u32);
        break;

    case SB_EVENT_MQTT_MESSAGE:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "mqtt message len=%u text=%s", event->param_u32, event->text);
        break;

    case SB_EVENT_MQTT_PUBLISHED:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "mqtt published len=%u topic=%s", event->param_u32, event->text);
        break;

    case SB_EVENT_PAYMENT_ACCEPTED:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "payment accepted amount_paise=%u", event->param_u32);
        break;

    case SB_EVENT_PAYMENT_DUPLICATE:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "payment duplicate text=%s", event->text);
        break;

    case SB_EVENT_PAYMENT_FAULT:
        SB_LOGW(SB_SUPERVISOR_MODULE_NAME, "payment fault status=%d text=%s", event->param_s32, event->text);
        break;

    case SB_EVENT_COMMAND_ACCEPTED:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "command accepted %s", event->text);
        break;

    case SB_EVENT_COMMAND_REJECTED:
        SB_LOGW(SB_SUPERVISOR_MODULE_NAME, "command rejected status=%d text=%s", event->param_s32, event->text);
        break;

    case SB_EVENT_KEY_ACTION:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "key action=%s value=%u status=%d", event->text, event->param_u32, event->param_s32);
        break;

    case SB_EVENT_DAILY_SUMMARY_READY:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "daily summary count=%u status=%d", event->param_u32, event->param_s32);
        break;

    case SB_EVENT_FACTORY_READY:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "factory ready text=%s", event->text);
        break;

    case SB_EVENT_FACTORY_COMMAND:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "factory command channel=%u", event->param_u32);
        break;

    case SB_EVENT_FACTORY_COMMAND_DONE:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "factory command done channel=%s status=%d", event->text, event->param_s32);
        break;

    case SB_EVENT_FACTORY_COMMAND_REJECTED:
        SB_LOGW(SB_SUPERVISOR_MODULE_NAME, "factory command rejected channel=%s status=%d", event->text, event->param_s32);
        break;

    case SB_EVENT_SERIAL_READY:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "serial ready text=%s", event->text);
        break;

    case SB_EVENT_SERIAL_COMMAND:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "serial command len=%u", event->param_u32);
        break;

    case SB_EVENT_SMS_READY:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "sms ready text=%s", event->text);
        break;

    case SB_EVENT_SMS_COMMAND:
        SB_LOGI(SB_SUPERVISOR_MODULE_NAME, "sms command index=%u", event->param_u32);
        break;

    case SB_EVENT_SMS_FAULT:
        SB_LOGW(SB_SUPERVISOR_MODULE_NAME, "sms fault index=%u status=%d text=%s", event->param_u32, event->param_s32, event->text);
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
