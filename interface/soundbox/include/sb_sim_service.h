/*================================================================
 * Static QR UPI Soundbox - SIM Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_SIM_SERVICE_H
#define SB_SIM_SERVICE_H

#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int card_status;
    int ready;
} sb_sim_status_t;

sb_status_t sb_sim_service_check(sb_sim_status_t *status);
const char *sb_sim_status_name(int card_status);

#ifdef __cplusplus
}
#endif

#endif /* SB_SIM_SERVICE_H */
