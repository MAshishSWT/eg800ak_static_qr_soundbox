/*================================================================
 * Static QR UPI Soundbox - Serial Factory Provisioning Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_SERIAL_SERVICE_H
#define SB_SERIAL_SERVICE_H

#include "ql_type.h"
#include "sb_config.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_SERIAL_TASK_STACK_SIZE_BYTES   (8u * 1024u)
#define SB_SERIAL_TASK_PRIORITY           (21u)
#define SB_SERIAL_QUEUE_DEPTH             (4u)
#define SB_SERIAL_LINE_LEN                (2304u)
#define SB_SERIAL_POOL_DEPTH              (4u)

sb_status_t sb_serial_service_init(const sb_config_payload_t *config);
int sb_serial_service_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* SB_SERIAL_SERVICE_H */
