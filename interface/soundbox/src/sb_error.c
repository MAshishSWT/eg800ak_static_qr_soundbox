/*================================================================
 * Static QR UPI Soundbox - Error Codes
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "sb_error.h"

const char *sb_status_to_string(sb_status_t status)
{
    switch (status) {
    case SB_STATUS_OK:
        return "OK";
    case SB_STATUS_ERROR:
        return "ERROR";
    case SB_STATUS_INVALID_PARAM:
        return "INVALID_PARAM";
    case SB_STATUS_NOT_READY:
        return "NOT_READY";
    case SB_STATUS_ALREADY_INITIALIZED:
        return "ALREADY_INITIALIZED";
    case SB_STATUS_NO_MEMORY:
        return "NO_MEMORY";
    case SB_STATUS_QUEUE_ERROR:
        return "QUEUE_ERROR";
    case SB_STATUS_TIMER_ERROR:
        return "TIMER_ERROR";
    case SB_STATUS_TASK_ERROR:
        return "TASK_ERROR";
    case SB_STATUS_TIMEOUT:
        return "TIMEOUT";
    case SB_STATUS_UNSUPPORTED:
        return "UNSUPPORTED";
    default:
        return "UNKNOWN";
    }
}
