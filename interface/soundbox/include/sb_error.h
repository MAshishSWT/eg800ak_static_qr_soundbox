/*================================================================
 * Static QR UPI Soundbox - Error Codes
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_ERROR_H
#define SB_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SB_STATUS_OK = 0,
    SB_STATUS_ERROR = -1,
    SB_STATUS_INVALID_PARAM = -2,
    SB_STATUS_NOT_READY = -3,
    SB_STATUS_ALREADY_INITIALIZED = -4,
    SB_STATUS_NO_MEMORY = -5,
    SB_STATUS_QUEUE_ERROR = -6,
    SB_STATUS_TIMER_ERROR = -7,
    SB_STATUS_TASK_ERROR = -8,
    SB_STATUS_TIMEOUT = -9,
    SB_STATUS_UNSUPPORTED = -10,
    SB_STATUS_CRC_ERROR = -11,
    SB_STATUS_FILE_ERROR = -12,
    SB_STATUS_FLASH_ERROR = -13,
    SB_STATUS_NOT_FOUND = -14,
    SB_STATUS_INVALID_STATE = -15,
    SB_STATUS_NETWORK_ERROR = -16,
    SB_STATUS_SIM_ERROR = -17,
    SB_STATUS_DATACALL_ERROR = -18,
    SB_STATUS_TIME_ERROR = -19,
    SB_STATUS_MQTT_ERROR = -20,
    SB_STATUS_SSL_ERROR = -22,
    SB_STATUS_CONFIG_ERROR = -23,
    SB_STATUS_SECURITY_ERROR = -24,
    SB_STATUS_HASH_ERROR = -26,
    SB_STATUS_FACTORY_LOCKED = -27,
    SB_STATUS_SMS_ERROR = -28,
    SB_STATUS_SERIAL_ERROR = -29
} sb_status_t;

const char *sb_status_to_string(sb_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* SB_ERROR_H */
