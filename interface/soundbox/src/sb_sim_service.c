/*================================================================
 * Static QR UPI Soundbox - SIM Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_type.h"
#include "ql_sim.h"
#include "sb_sim_service.h"

sb_status_t sb_sim_service_check(sb_sim_status_t *status)
{
    int card_status = 0;

    if (status == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    status->card_status = 0;
    status->ready = 0;

    if (ql_sim_get_card_status(&card_status) != QL_SIM_SUCCESS) {
        return SB_STATUS_SIM_ERROR;
    }

    status->card_status = card_status;
    status->ready = (card_status == QL_SIM_STATUS_READY) ? 1 : 0;

    return status->ready ? SB_STATUS_OK : SB_STATUS_NOT_READY;
}

const char *sb_sim_status_name(int card_status)
{
    switch (card_status) {
    case QL_SIM_STATUS_NOT_INSERTED:
        return "NOT_INSERTED";
    case QL_SIM_STATUS_READY:
        return "READY";
    case QL_SIM_STATUS_SIM_PIN:
        return "SIM_PIN";
    case QL_SIM_STATUS_SIM_PUK:
        return "SIM_PUK";
    case QL_SIM_STATUS_PH_SIM_LOCK_PIN:
        return "PH_SIM_LOCK_PIN";
    case QL_SIM_STATUS_PH_SIM_LOCK_PUK:
        return "PH_SIM_LOCK_PUK";
    case QL_SIM_STATUS_PH_FSIM_PIN:
        return "PH_FSIM_PIN";
    case QL_SIM_STATUS_PH_FSIM_PUK:
        return "PH_FSIM_PUK";
    case QL_SIM_STATUS_SIM_PIN2:
        return "SIM_PIN2";
    default:
        return "UNKNOWN";
    }
}
