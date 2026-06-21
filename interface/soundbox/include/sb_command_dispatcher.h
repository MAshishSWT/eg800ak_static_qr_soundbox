/*================================================================
 * Static QR UPI Soundbox - Command Dispatcher
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_COMMAND_DISPATCHER_H
#define SB_COMMAND_DISPATCHER_H

#include "sb_error.h"
#include "sb_mqtt_service.h"

#ifdef __cplusplus
extern "C" {
#endif

sb_status_t sb_command_dispatcher_init(void);
sb_status_t sb_command_dispatcher_handle_message(const sb_mqtt_inbound_message_t *message);

#ifdef __cplusplus
}
#endif

#endif /* SB_COMMAND_DISPATCHER_H */
