/*================================================================
 * Static QR UPI Soundbox - EG800AK Application Entry
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_application.h"
#include "ql_rtos.h"
#include "sb_app.h"
#include "sb_audio_service.h"
#include "sb_audio_types.h"
#include "sb_bsp_kae8_sq1.h"
#include "sb_business_service.h"
#include "sb_config.h"
#include "sb_error.h"
#include "sb_event_bus.h"
#include "sb_extnor.h"
#include "sb_http_service.h"
#include "sb_factory_diag.h"
#include "sb_log.h"
#include "sb_mqtt_service.h"
#include "sb_mode_service.h"
#include "sb_network_service.h"
#include "sb_ota_service.h"
#include "sb_serial_service.h"
#include "sb_sms_service.h"
#include "sb_supervisor.h"

#define SB_APP_MODULE_NAME "app"

static void sb_app_entry(void *argv)
{
    sb_status_t status;
    sb_config_payload_t config;
    sb_audio_language_t audio_language;
    u32 audio_volume;

    (void)argv;

    sb_log_init(SB_LOG_LEVEL_INFO);
    SB_LOGI(SB_APP_MODULE_NAME, "starting %s", SB_APP_VERSION_STRING);

    status = sb_event_bus_init();
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) {
        SB_LOGE(SB_APP_MODULE_NAME, "event bus init failed status=%s", sb_status_to_string(status));
        ql_rtos_task_delete(NULL);
        return;
    }

    status = sb_bsp_board_init();
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) {
        SB_LOGE(SB_APP_MODULE_NAME, "board init failed status=%s", sb_status_to_string(status));
        ql_rtos_task_delete(NULL);
        return;
    }

    status = sb_config_service_init();
    if (status != SB_STATUS_OK) {
        SB_LOGE(SB_APP_MODULE_NAME, "config service init failed status=%s", sb_status_to_string(status));
        ql_rtos_task_delete(NULL);
        return;
    }

    sb_config_make_defaults(&config);
    if (sb_config_get(&config) != SB_STATUS_OK) {
        SB_LOGW(SB_APP_MODULE_NAME, "config get failed, using audio defaults");
    }

    status = sb_mode_service_init();
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) {
        SB_LOGW(SB_APP_MODULE_NAME, "mode service init status=%s", sb_status_to_string(status));
    }

    status = sb_factory_diag_init();
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) {
        SB_LOGW(SB_APP_MODULE_NAME, "factory diag init status=%s", sb_status_to_string(status));
    }
    audio_language = sb_audio_language_from_code(config.language);
    audio_volume = config.volume_percent;

    status = sb_audio_service_init(audio_language, audio_volume);
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) {
        SB_LOGW(SB_APP_MODULE_NAME, "audio service init status=%s", sb_status_to_string(status));
    }

    status = sb_extnor_init();
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) {
        SB_LOGW(SB_APP_MODULE_NAME, "external nor init status=%s", sb_status_to_string(status));
    }

    status = sb_network_service_init(&config);
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) {
        SB_LOGW(SB_APP_MODULE_NAME, "network service init status=%s", sb_status_to_string(status));
    }

    status = sb_mqtt_service_init(&config);
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) {
        SB_LOGW(SB_APP_MODULE_NAME, "mqtt service init status=%s", sb_status_to_string(status));
    }

    status = sb_http_service_init(&config);
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) {
        SB_LOGW(SB_APP_MODULE_NAME, "http service init status=%s", sb_status_to_string(status));
    }

    status = sb_ota_service_init(&config);
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) {
        SB_LOGW(SB_APP_MODULE_NAME, "ota service init status=%s", sb_status_to_string(status));
    }

    status = sb_serial_service_init(&config);
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) {
        SB_LOGW(SB_APP_MODULE_NAME, "serial service init status=%s", sb_status_to_string(status));
    }

    status = sb_sms_service_init(&config);
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) {
        SB_LOGW(SB_APP_MODULE_NAME, "sms service init status=%s", sb_status_to_string(status));
    }

    status = sb_business_service_init();
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) {
        SB_LOGW(SB_APP_MODULE_NAME, "business service init status=%s", sb_status_to_string(status));
    }

    status = sb_supervisor_start();
    if (status != SB_STATUS_OK) {
        SB_LOGE(SB_APP_MODULE_NAME, "supervisor start failed status=%s", sb_status_to_string(status));
        ql_rtos_task_delete(NULL);
        return;
    }

    status = sb_supervisor_post_boot_event();
    if (status != SB_STATUS_OK) {
        SB_LOGW(SB_APP_MODULE_NAME, "boot event post status=%s", sb_status_to_string(status));
    }

    status = sb_bsp_board_post_status();
    if (status != SB_STATUS_OK) {
        SB_LOGW(SB_APP_MODULE_NAME, "board status post status=%s", sb_status_to_string(status));
    }

    SB_LOGI(SB_APP_MODULE_NAME, "entry task complete");
    ql_rtos_task_delete(NULL);
}

application_init(sb_app_entry, SB_APP_NAME, SB_APP_ENTRY_STACK_KIB, SB_APP_ENTRY_STARTUP_PRIORITY);
