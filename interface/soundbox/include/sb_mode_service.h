/*================================================================
 * Static QR UPI Soundbox - Production/Factory Mode Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_MODE_SERVICE_H
#define SB_MODE_SERVICE_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_MODE_PATH       "U:/sb_mode.bin"
#define SB_MODE_TEMP_PATH  "U:/sb_mode.tmp"

typedef enum {
    SB_DEVICE_MODE_PRODUCTION = 0,
    SB_DEVICE_MODE_FACTORY = 1,
    SB_DEVICE_MODE_DEBUG = 2
} sb_device_mode_t;

sb_status_t sb_mode_service_init(void);
sb_status_t sb_mode_get(sb_device_mode_t *mode);
sb_status_t sb_mode_set(sb_device_mode_t mode);
int sb_mode_factory_access_allowed(void);
const char *sb_mode_name(sb_device_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* SB_MODE_SERVICE_H */
