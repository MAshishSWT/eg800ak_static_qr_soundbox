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
    case SB_STATUS_CRC_ERROR:
        return "CRC_ERROR";
    case SB_STATUS_FILE_ERROR:
        return "FILE_ERROR";
    case SB_STATUS_FLASH_ERROR:
        return "FLASH_ERROR";
    case SB_STATUS_NOT_FOUND:
        return "NOT_FOUND";
    case SB_STATUS_INVALID_STATE:
        return "INVALID_STATE";
    case SB_STATUS_NETWORK_ERROR:
        return "NETWORK_ERROR";
    case SB_STATUS_SIM_ERROR:
        return "SIM_ERROR";
    case SB_STATUS_DATACALL_ERROR:
        return "DATACALL_ERROR";
    case SB_STATUS_TIME_ERROR:
        return "TIME_ERROR";
    case SB_STATUS_MQTT_ERROR:
        return "MQTT_ERROR";
    case SB_STATUS_HTTP_ERROR:
        return "HTTP_ERROR";
    case SB_STATUS_SSL_ERROR:
        return "SSL_ERROR";
    case SB_STATUS_CONFIG_ERROR:
        return "CONFIG_ERROR";
    case SB_STATUS_SECURITY_ERROR:
        return "SECURITY_ERROR";
    default:
        return "UNKNOWN";
    }
}
