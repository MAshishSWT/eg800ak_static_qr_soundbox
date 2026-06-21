/*================================================================
 * Static QR UPI Soundbox - Network/Data Call Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_data_call.h"
#include "ql_nw.h"
#include "ql_rtos.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_log.h"
#include "sb_network_service.h"
#include "sb_sim_service.h"
#include "sb_time_service.h"

#define SB_NETWORK_MODULE_NAME        "network"
#define SB_NETWORK_DEFAULT_NTP        "pool.ntp.org"

static ql_task_t s_network_task = 0;
static ql_mutex_t s_network_mutex = 0;
static int s_network_started = 0;
static volatile int s_datacall_callback_state = -1;
static char s_apn[SB_NETWORK_APN_LEN];
static char s_ntp_server[SB_NETWORK_NTP_SERVER_LEN] = SB_NETWORK_DEFAULT_NTP;

static sb_network_status_t s_network_status = {
    SB_NETWORK_STATE_STOPPED,
    0,
    99,
    0,
    -1,
    0,
    0
};

static void sb_network_copy_text(char *dst, u32 dst_len, const char *src)
{
    u32 i;

    if ((dst == 0) || (dst_len == 0u)) {
        return;
    }

    if (src == 0) {
        dst[0] = '\0';
        return;
    }

    for (i = 0u; (i + 1u < dst_len) && (src[i] != '\0'); i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

static void sb_network_post_event(sb_event_id_t id, s32 status, u32 value, const char *text)
{
    sb_event_t event;

    sb_event_init(&event, id, SB_EVENT_SOURCE_NETWORK);
    event.param_s32 = status;
    event.param_u32 = value;
    if (text != 0) {
        (void)sb_event_set_text(&event, text);
    }
    (void)sb_event_post(&event, QL_NO_WAIT);
}

static void sb_network_status_set(sb_network_state_t state, int error)
{
    if (s_network_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_network_mutex, QL_WAIT_FOREVER);
    }

    s_network_status.state = state;
    s_network_status.last_error = error;

    if (s_network_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_network_mutex);
    }
}

static void sb_network_status_update_sim(const sb_sim_status_t *sim_status)
{
    if (sim_status == 0) {
        return;
    }

    if (s_network_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_network_mutex, QL_WAIT_FOREVER);
    }

    s_network_status.sim_status = sim_status->card_status;

    if (s_network_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_network_mutex);
    }
}

static void sb_network_status_update_csq(int csq)
{
    if (s_network_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_network_mutex, QL_WAIT_FOREVER);
    }

    s_network_status.csq = csq;

    if (s_network_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_network_mutex);
    }
}

static void sb_network_status_update_online(int registered, int datacall_state, int online)
{
    if (s_network_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_network_mutex, QL_WAIT_FOREVER);
    }

    s_network_status.registered = registered;
    s_network_status.datacall_state = datacall_state;
    s_network_status.online = online;

    if (s_network_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_network_mutex);
    }
}

static void sb_network_data_call_cb(int profile_idx, int state)
{
    s_datacall_callback_state = state;
    sb_network_post_event((state == 1) ? SB_EVENT_DATACALL_READY : SB_EVENT_DATACALL_FAULT,
                          (s32)state,
                          (u32)profile_idx,
                          "data_call_cb");
}

static void sb_network_sample_csq(void)
{
    int csq = 99;

    if (ql_nw_get_csq(&csq) == QL_NW_SUCCESS) {
        sb_network_status_update_csq(csq);
        sb_network_post_event(SB_EVENT_CSQ_SAMPLE, (s32)csq, (u32)csq, "csq");
        SB_LOGI(SB_NETWORK_MODULE_NAME, "csq=%d", csq);
    } else {
        SB_LOGW(SB_NETWORK_MODULE_NAME, "csq read failed");
    }
}

static sb_status_t sb_network_wait_datacall_ready(void)
{
    struct ql_data_call_info info;
    u32 waited_ms = 0u;

    while (waited_ms < SB_NETWORK_DATACALL_TIMEOUT_MS) {
        if (s_datacall_callback_state == 1) {
            sb_network_status_update_online(1, 1, 1);
            return SB_STATUS_OK;
        }

        if (ql_get_data_call_info(SB_NETWORK_PDP_PROFILE_ID, SB_NETWORK_IPV4, &info) == 0) {
            if (info.v4.state != 0) {
                sb_network_status_update_online(1, info.v4.state, 1);
                return SB_STATUS_OK;
            }
        }

        ql_rtos_task_sleep_ms(500u);
        waited_ms += 500u;
    }

    return SB_STATUS_TIMEOUT;
}

static sb_status_t sb_network_start_data_call(void)
{
    char *apn_ptr = 0;
    int ret;

    if (s_apn[0] != '\0') {
        apn_ptr = s_apn;
    }

    s_datacall_callback_state = -1;
    (void)ql_set_data_call_asyn_mode(1, sb_network_data_call_cb);
    (void)ql_set_auto_connect(SB_NETWORK_PDP_PROFILE_ID, TRUE);

    ret = ql_start_data_call(SB_NETWORK_PDP_PROFILE_ID, SB_NETWORK_IPV4, apn_ptr, NULL, NULL, 0);
    if (ret != 0) {
        SB_LOGW(SB_NETWORK_MODULE_NAME, "start data call ret=%d", ret);
        return SB_STATUS_DATACALL_ERROR;
    }

    return sb_network_wait_datacall_ready();
}

static void sb_network_online_loop(void)
{
    sb_time_status_t time_status;
    struct ql_data_call_info info;
    sb_status_t status;

    status = sb_time_start_ntp_sync();
    if (status != SB_STATUS_OK) {
        sb_network_post_event(SB_EVENT_TIME_FAULT, (s32)status, 0u, "ntp_start");
    }

    while (1) {
        sb_network_sample_csq();

        if (sb_time_get_rtc(&time_status) == SB_STATUS_OK) {
            SB_LOGI(SB_NETWORK_MODULE_NAME,
                    "rtc=%04d-%02d-%02d %02d:%02d:%02d",
                    time_status.rtc.tm_year,
                    time_status.rtc.tm_mon,
                    time_status.rtc.tm_mday,
                    time_status.rtc.tm_hour,
                    time_status.rtc.tm_min,
                    time_status.rtc.tm_sec);
        }

        if (ql_get_data_call_info(SB_NETWORK_PDP_PROFILE_ID, SB_NETWORK_IPV4, &info) != 0) {
            SB_LOGW(SB_NETWORK_MODULE_NAME, "data call info failed");
            break;
        }

        if (info.v4.state == 0) {
            SB_LOGW(SB_NETWORK_MODULE_NAME, "data call down");
            break;
        }

        ql_rtos_task_sleep_ms(SB_NETWORK_ONLINE_POLL_PERIOD_MS);
    }

    sb_network_status_update_online(0, 0, 0);
    sb_network_post_event(SB_EVENT_NETWORK_LOST, (s32)SB_STATUS_NETWORK_ERROR, 0u, "offline");
}

static void sb_network_task(void *argv)
{
    sb_sim_status_t sim_status;
    sb_status_t status;

    (void)argv;

    SB_LOGI(SB_NETWORK_MODULE_NAME, "task started apn=%s", (s_apn[0] == '\0') ? "<auto>" : s_apn);

    while (1) {
        sb_network_status_set(SB_NETWORK_STATE_SIM_CHECK, 0);
        status = sb_sim_service_check(&sim_status);
        sb_network_status_update_sim(&sim_status);
        if (status != SB_STATUS_OK) {
            SB_LOGW(SB_NETWORK_MODULE_NAME,
                    "sim not ready status=%s card=%s(%d)",
                    sb_status_to_string(status),
                    sb_sim_status_name(sim_status.card_status),
                    sim_status.card_status);
            sb_network_post_event(SB_EVENT_SIM_FAULT, (s32)status, (u32)sim_status.card_status, sb_sim_status_name(sim_status.card_status));
            sb_network_status_set(SB_NETWORK_STATE_BACKOFF, (int)status);
            ql_rtos_task_sleep_ms(SB_NETWORK_BACKOFF_MS);
            continue;
        }

        sb_network_post_event(SB_EVENT_SIM_READY, SB_STATUS_OK, (u32)sim_status.card_status, "sim_ready");
        SB_LOGI(SB_NETWORK_MODULE_NAME, "sim ready");

        sb_network_status_set(SB_NETWORK_STATE_WAIT_REGISTER, 0);
        if (ql_network_register_wait(SB_NETWORK_REGISTER_TIMEOUT_SEC) != 0) {
            SB_LOGW(SB_NETWORK_MODULE_NAME, "network register timeout");
            sb_network_post_event(SB_EVENT_NETWORK_LOST, (s32)SB_STATUS_NETWORK_ERROR, 0u, "register_timeout");
            sb_network_status_set(SB_NETWORK_STATE_BACKOFF, SB_STATUS_NETWORK_ERROR);
            ql_rtos_task_sleep_ms(SB_NETWORK_BACKOFF_MS);
            continue;
        }

        sb_network_status_update_online(1, 0, 0);
        sb_network_post_event(SB_EVENT_NETWORK_REGISTERED, SB_STATUS_OK, 0u, "registered");
        sb_network_sample_csq();

        sb_network_status_set(SB_NETWORK_STATE_START_DATACALL, 0);
        status = sb_network_start_data_call();
        if (status != SB_STATUS_OK) {
            SB_LOGW(SB_NETWORK_MODULE_NAME, "data call status=%s", sb_status_to_string(status));
            sb_network_post_event(SB_EVENT_DATACALL_FAULT, (s32)status, 0u, "datacall_start");
            sb_network_status_set(SB_NETWORK_STATE_BACKOFF, (int)status);
            ql_rtos_task_sleep_ms(SB_NETWORK_BACKOFF_MS);
            continue;
        }

        sb_network_status_set(SB_NETWORK_STATE_ONLINE, 0);
        sb_network_status_update_online(1, 1, 1);
        sb_network_post_event(SB_EVENT_DATACALL_READY, SB_STATUS_OK, SB_NETWORK_PDP_PROFILE_ID, "online");
        SB_LOGI(SB_NETWORK_MODULE_NAME, "online");
        sb_network_online_loop();

        sb_network_status_set(SB_NETWORK_STATE_BACKOFF, SB_STATUS_NETWORK_ERROR);
        ql_rtos_task_sleep_ms(SB_NETWORK_BACKOFF_MS);
    }
}

sb_status_t sb_network_service_init(const sb_config_payload_t *config)
{
    QlOSStatus ret;

    if (s_network_started != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }

    s_apn[0] = '\0';
    if (config != 0) {
        sb_network_copy_text(s_apn, SB_NETWORK_APN_LEN, config->apn);
    }
    sb_network_copy_text(s_ntp_server, SB_NETWORK_NTP_SERVER_LEN, SB_NETWORK_DEFAULT_NTP);

    ret = ql_rtos_mutex_create(&s_network_mutex);
    if (ret != 0) {
        return SB_STATUS_NO_MEMORY;
    }

    (void)sb_time_service_init(SB_NETWORK_PDP_PROFILE_ID, s_ntp_server);

    ret = ql_rtos_task_create(&s_network_task,
                              SB_NETWORK_TASK_STACK_SIZE_BYTES,
                              SB_NETWORK_TASK_PRIORITY,
                              "sb_network",
                              sb_network_task,
                              0);
    if (ret != 0) {
        (void)ql_rtos_mutex_delete(s_network_mutex);
        s_network_mutex = 0;
        return SB_STATUS_TASK_ERROR;
    }

    s_network_started = 1;
    return SB_STATUS_OK;
}

sb_status_t sb_network_get_status(sb_network_status_t *status)
{
    if (status == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (s_network_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_network_mutex, QL_WAIT_FOREVER);
    }

    *status = s_network_status;

    if (s_network_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_network_mutex);
    }

    return SB_STATUS_OK;
}

sb_status_t sb_network_request_time_sync(void)
{
    return sb_time_start_ntp_sync();
}

const char *sb_network_state_name(sb_network_state_t state)
{
    switch (state) {
    case SB_NETWORK_STATE_STOPPED:
        return "STOPPED";
    case SB_NETWORK_STATE_SIM_CHECK:
        return "SIM_CHECK";
    case SB_NETWORK_STATE_WAIT_REGISTER:
        return "WAIT_REGISTER";
    case SB_NETWORK_STATE_START_DATACALL:
        return "START_DATACALL";
    case SB_NETWORK_STATE_ONLINE:
        return "ONLINE";
    case SB_NETWORK_STATE_BACKOFF:
        return "BACKOFF";
    default:
        return "UNKNOWN";
    }
}
