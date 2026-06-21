/*================================================================
 * Static QR UPI Soundbox - Factory Diagnostic Dispatcher
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_FACTORY_DIAG_H
#define SB_FACTORY_DIAG_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_FACTORY_REPLY_LEN       (384u)
#define SB_FACTORY_COMMAND_LEN     (512u)
#define SB_FACTORY_KEY_HEX_LEN     (65u)
#define SB_FACTORY_SECURE_KEY_LEN  (32u)
#define SB_FACTORY_SMS_NUMBER_INDEX (3u)
#define SB_FACTORY_SMS_NUMBER_LEN   (32u)

typedef enum {
    SB_FACTORY_CHANNEL_SERIAL = 0,
    SB_FACTORY_CHANNEL_SMS = 1,
    SB_FACTORY_CHANNEL_INTERNAL = 2
} sb_factory_channel_t;

sb_status_t sb_factory_diag_init(void);
int sb_factory_diag_sms_sender_allowed(const char *sender);
sb_status_t sb_factory_diag_dispatch_json(const char *json,
                                           sb_factory_channel_t channel,
                                           char *reply,
                                           u32 reply_len);

#ifdef __cplusplus
}
#endif

#endif /* SB_FACTORY_DIAG_H */
