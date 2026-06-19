/*================================================================
 * Static QR UPI Soundbox - Application Supervisor
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_SUPERVISOR_H
#define SB_SUPERVISOR_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_SUPERVISOR_TASK_STACK_SIZE_BYTES   (12u * 1024u)
#define SB_SUPERVISOR_TASK_PRIORITY           (10u)
#define SB_SUPERVISOR_HEARTBEAT_PERIOD_MS     (30000u)

sb_status_t sb_supervisor_start(void);
sb_status_t sb_supervisor_post_boot_event(void);

#ifdef __cplusplus
}
#endif

#endif /* SB_SUPERVISOR_H */
