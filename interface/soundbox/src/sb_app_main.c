/*================================================================
 * Static QR UPI Soundbox - EG800AK Application Entry
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_application.h"
#include "ql_rtos.h"
#include "sb_app.h"
#include "sb_bsp_kae8_sq1.h"
#include "sb_error.h"
#include "sb_event_bus.h"
#include "sb_log.h"
#include "sb_supervisor.h"

#define SB_APP_MODULE_NAME "app"

static void sb_app_entry(void *argv)
{
    sb_status_t status;

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
