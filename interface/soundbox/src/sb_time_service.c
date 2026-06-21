/*================================================================
 * Static QR UPI Soundbox - Time Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "ql_ntp.h"
#include "ql_rtc.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_log.h"
#include "sb_time_service.h"

#define SB_TIME_MODULE_NAME              "time"
#define SB_TIME_DEFAULT_NTP              "pool.ntp.org"
#define SB_TIME_FALLBACK_NTP_1           "time.google.com"
#define SB_TIME_FALLBACK_NTP_2           "ntp.aliyun.com"
#define SB_TIME_FALLBACK_NTP_3           "time.windows.com"
#define SB_TIME_RTC_VALID_YEAR_MIN       (2023)
#define SB_TIME_NTP_SET_RETRY_DELAY_MS   (1000u)

static int s_time_pdp_cid = 1;
static char s_time_server[SB_TIME_NTP_SERVER_LEN] = SB_TIME_DEFAULT_NTP;
static int s_ntp_last_status = -1;

static void sb_time_copy_server(const char *server)
{
    u32 i;

    if ((server == 0) || (server[0] == '\0')) {
        server = SB_TIME_DEFAULT_NTP;
    }

    for (i = 0u; (i + 1u < SB_TIME_NTP_SERVER_LEN) && (server[i] != '\0'); i++) {
        s_time_server[i] = server[i];
    }
    s_time_server[i] = '\0';
}

static int sb_time_rtc_is_valid(const ql_rtc_time_t *rtc)
{
    if (rtc == 0) {
        return 0;
    }

    if ((rtc->tm_year >= SB_TIME_RTC_VALID_YEAR_MIN) &&
        (rtc->tm_mon >= 1) && (rtc->tm_mon <= 12) &&
        (rtc->tm_mday >= 1) && (rtc->tm_mday <= 31) &&
        (rtc->tm_hour >= 0) && (rtc->tm_hour <= 23) &&
        (rtc->tm_min >= 0) && (rtc->tm_min <= 59) &&
        (rtc->tm_sec >= 0) && (rtc->tm_sec <= 59)) {
        return 1;
    }

    return 0;
}

static void sb_time_post_event(sb_event_id_t id, s32 status, const char *text)
{
    sb_event_t event;

    sb_event_init(&event, id, SB_EVENT_SOURCE_TIME);
    event.param_s32 = status;
    if (text != 0) {
        (void)sb_event_set_text(&event, text);
    }
    (void)sb_event_post(&event, QL_NO_WAIT);
}

static void sb_time_ntp_callback(int ntp_status)
{
    s_ntp_last_status = ntp_status;
    sb_time_post_event((ntp_status == 0) ? SB_EVENT_TIME_SYNCED : SB_EVENT_TIME_FAULT,
                       (s32)ntp_status,
                       s_time_server);
}

static sb_status_t sb_time_accept_rtc_fallback(const char *reason)
{
    ql_rtc_time_t rtc;

    if (ql_rtc_get_time(&rtc) != 0) {
        return SB_STATUS_TIME_ERROR;
    }

    if (sb_time_rtc_is_valid(&rtc) == 0) {
        return SB_STATUS_TIME_ERROR;
    }

    SB_LOGI(SB_TIME_MODULE_NAME,
            "rtc fallback accepted reason=%s time=%04d-%02d-%02d %02d:%02d:%02d",
            reason,
            rtc.tm_year,
            rtc.tm_mon,
            rtc.tm_mday,
            rtc.tm_hour,
            rtc.tm_min,
            rtc.tm_sec);
    sb_time_post_event(SB_EVENT_TIME_SYNCED, (s32)SB_STATUS_OK, reason);
    return SB_STATUS_OK;
}

static const char *sb_time_candidate_server(u32 index)
{
    switch (index) {
    case 0u:
        return s_time_server;
    case 1u:
        return SB_TIME_FALLBACK_NTP_1;
    case 2u:
        return SB_TIME_FALLBACK_NTP_2;
    case 3u:
        return SB_TIME_FALLBACK_NTP_3;
    default:
        return 0;
    }
}

sb_status_t sb_time_service_init(int pdp_cid, const char *server)
{
    if (pdp_cid <= 0) {
        pdp_cid = 1;
    }

    s_time_pdp_cid = pdp_cid;
    sb_time_copy_server(server);
    ql_rtc_set_nitz_mode(1u);

    SB_LOGI(SB_TIME_MODULE_NAME, "init cid=%d server=%s", s_time_pdp_cid, s_time_server);
    return SB_STATUS_OK;
}

sb_status_t sb_time_get_rtc(sb_time_status_t *status)
{
    if (status == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    status->rtc_valid = 0;
    status->ntp_last_status = s_ntp_last_status;

    if (ql_rtc_get_time(&status->rtc) != 0) {
        return SB_STATUS_TIME_ERROR;
    }

    status->rtc_valid = sb_time_rtc_is_valid(&status->rtc);
    return SB_STATUS_OK;
}

sb_status_t sb_time_start_ntp_sync(void)
{
    QL_NTP_ERROR_CODE ret;
    const char *server;
    u32 index;

    ret = ql_ntp_set_cid(s_time_pdp_cid);
    if (ret != QL_NTP_SUCCESS) {
        SB_LOGW(SB_TIME_MODULE_NAME, "ntp cid set failed ret=%d", ret);
        return sb_time_accept_rtc_fallback("rtc_nitz_cid");
    }

    for (index = 0u; index < 4u; index++) {
        server = sb_time_candidate_server(index);
        if ((server == 0) || (server[0] == '\0')) {
            continue;
        }

        ret = ql_ntp_set_server((char *)server);
        if (ret == QL_NTP_SUCCESS) {
            if (server != s_time_server) {
                sb_time_copy_server(server);
            }
            ql_ntp_sync_ex(sb_time_ntp_callback);
            SB_LOGI(SB_TIME_MODULE_NAME, "ntp sync requested server=%s", s_time_server);
            return SB_STATUS_OK;
        }

        SB_LOGW(SB_TIME_MODULE_NAME, "ntp server set failed server=%s ret=%d", server, ret);
        ql_rtos_task_sleep_ms(SB_TIME_NTP_SET_RETRY_DELAY_MS);
    }

    return sb_time_accept_rtc_fallback("rtc_nitz_ntp_server");
}
