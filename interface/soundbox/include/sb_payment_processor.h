/*================================================================
 * Static QR UPI Soundbox - Payment Processor
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_PAYMENT_PROCESSOR_H
#define SB_PAYMENT_PROCESSOR_H

#include "ql_type.h"
#include "sb_error.h"
#include "sb_mqtt_service.h"
#include "sb_transaction_ledger.h"

#ifdef __cplusplus
extern "C" {
#endif

sb_status_t sb_payment_processor_init(void);
sb_status_t sb_payment_processor_handle_message(const sb_mqtt_inbound_message_t *message);
sb_audio_provider_t sb_payment_provider_from_text(const char *provider_text);

#ifdef __cplusplus
}
#endif

#endif /* SB_PAYMENT_PROCESSOR_H */
