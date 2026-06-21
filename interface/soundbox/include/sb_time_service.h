/*================================================================
 * Static QR UPI Soundbox - Time Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_TIME_SERVICE_H
#define SB_TIME_SERVICE_H

#include "ql_rtc.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_TIME_NTP_SERVER_LEN      (64u)

typedef struct {
    ql_rtc_time_t rtc;
    int rtc_valid;
    int ntp_last_status;
} sb_time_status_t;

sb_status_t sb_time_service_init(int pdp_cid, const char *server);
sb_status_t sb_time_get_rtc(sb_time_status_t *status);
sb_status_t sb_time_start_ntp_sync(void);

#ifdef __cplusplus
}
#endif

#endif /* SB_TIME_SERVICE_H */
