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

#define SB_TIME_MODULE_NAME       "time"
#define SB_TIME_DEFAULT_NTP       "pool.ntp.org"

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

static void sb_time_ntp_callback(int ntp_status)
{
    sb_event_t event;

    s_ntp_last_status = ntp_status;
    sb_event_init(&event,
                  (ntp_status == 0) ? SB_EVENT_TIME_SYNCED : SB_EVENT_TIME_FAULT,
                  SB_EVENT_SOURCE_TIME);
    event.param_s32 = (s32)ntp_status;
    (void)sb_event_set_text(&event, s_time_server);
    (void)sb_event_post(&event, QL_NO_WAIT);
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

    status->rtc_valid = 1;
    return SB_STATUS_OK;
}

sb_status_t sb_time_start_ntp_sync(void)
{
    QL_NTP_ERROR_CODE ret;

    ret = ql_ntp_set_server(s_time_server);
    if (ret != QL_NTP_SUCCESS) {
        SB_LOGW(SB_TIME_MODULE_NAME, "ntp server set failed ret=%d", ret);
        return SB_STATUS_TIME_ERROR;
    }

    ret = ql_ntp_set_cid(s_time_pdp_cid);
    if (ret != QL_NTP_SUCCESS) {
        SB_LOGW(SB_TIME_MODULE_NAME, "ntp cid set failed ret=%d", ret);
        return SB_STATUS_TIME_ERROR;
    }

    ql_ntp_sync_ex(sb_time_ntp_callback);
    SB_LOGI(SB_TIME_MODULE_NAME, "ntp sync requested server=%s", s_time_server);
    return SB_STATUS_OK;
}
