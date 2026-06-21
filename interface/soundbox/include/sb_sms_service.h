/*================================================================
 * Static QR UPI Soundbox - SMS Recovery Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_SMS_SERVICE_H
#define SB_SMS_SERVICE_H

#include "ql_type.h"
#include "sb_config.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_SMS_TASK_STACK_SIZE_BYTES   (8u * 1024u)
#define SB_SMS_TASK_PRIORITY           (22u)
#define SB_SMS_QUEUE_DEPTH             (4u)
#define SB_SMS_BODY_LEN                (280u)
#define SB_SMS_NUMBER_LEN              (32u)

sb_status_t sb_sms_service_init(const sb_config_payload_t *config);
int sb_sms_service_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* SB_SMS_SERVICE_H */
