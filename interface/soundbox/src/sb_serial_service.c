/*================================================================
 * Static QR UPI Soundbox - Serial Factory Provisioning Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "ql_uart.h"
#include "sb_cloud_utils.h"
#include "sb_error.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_factory_diag.h"
#include "sb_log.h"
#include "sb_mode_service.h"
#include "sb_serial_service.h"

#define SB_SERIAL_MODULE_NAME "serial"
#define SB_SERIAL_PORT        QL_USB_CDC_PORT

typedef struct {
    char line[SB_SERIAL_LINE_LEN];
    u32 len;
    int used;
} sb_serial_line_slot_t;

static ql_task_t s_serial_task = 0;
static ql_queue_t s_serial_queue = 0;
static ql_mutex_t s_serial_mutex = 0;
static sb_serial_line_slot_t s_serial_pool[SB_SERIAL_POOL_DEPTH];
static int s_serial_started = 0;
static int s_serial_enabled = 0;

static void sb_serial_post_event(sb_event_id_t id, sb_status_t status, const char *text)
{
    sb_event_t event;

    sb_event_init(&event, id, SB_EVENT_SOURCE_FACTORY);
    event.param_s32 = (s32)status;
    if (text != 0) {
        (void)sb_event_set_text(&event, text);
    }
    (void)sb_event_post(&event, QL_NO_WAIT);
}

static void sb_serial_zero_pool(void)
{
    u32 i;
    u32 j;

    for (i = 0u; i < SB_SERIAL_POOL_DEPTH; i++) {
        s_serial_pool[i].len = 0u;
        s_serial_pool[i].used = 0;
        for (j = 0u; j < SB_SERIAL_LINE_LEN; j++) {
            s_serial_pool[i].line[j] = '\0';
        }
    }
}

static int sb_serial_pool_alloc(const unsigned char *data, u32 len, u32 *slot_out)
{
    u32 i;
    u32 copy_len;

    if ((data == 0) || (slot_out == 0) || (len == 0u)) {
        return 0;
    }

    copy_len = (len >= SB_SERIAL_LINE_LEN) ? (SB_SERIAL_LINE_LEN - 1u) : len;

    if (s_serial_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_serial_mutex, QL_WAIT_FOREVER);
    }

    for (i = 0u; i < SB_SERIAL_POOL_DEPTH; i++) {
        if (s_serial_pool[i].used == 0) {
            u32 j;
            s_serial_pool[i].used = 1;
            s_serial_pool[i].len = copy_len;
            for (j = 0u; j < copy_len; j++) {
                char c = (char)data[j];
                if ((c == '\r') || (c == '\n')) {
                    break;
                }
                s_serial_pool[i].line[j] = c;
            }
            s_serial_pool[i].line[j] = '\0';
            s_serial_pool[i].len = j;
            *slot_out = i;
            if (s_serial_mutex != 0) {
                (void)ql_rtos_mutex_unlock(s_serial_mutex);
            }
            return 1;
        }
    }

    if (s_serial_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_serial_mutex);
    }
    return 0;
}

static int sb_serial_pool_get(u32 slot, char *out, u32 out_len)
{
    int ok = 0;

    if ((out == 0) || (out_len == 0u) || (slot >= SB_SERIAL_POOL_DEPTH)) {
        return 0;
    }

    if (s_serial_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_serial_mutex, QL_WAIT_FOREVER);
    }

    if (s_serial_pool[slot].used != 0) {
        (void)sb_cloud_copy_string(out, out_len, s_serial_pool[slot].line);
        ok = 1;
    }
    s_serial_pool[slot].used = 0;
    s_serial_pool[slot].len = 0u;
    s_serial_pool[slot].line[0] = '\0';

    if (s_serial_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_serial_mutex);
    }
    return ok;
}

static void sb_serial_uart_cb(QL_UART_PORT_NUMBER_E port, void *para)
{
    unsigned char rx[SB_SERIAL_LINE_LEN];
    int read_len;
    u32 slot;

    (void)para;
    if ((s_serial_queue == 0) || (port != SB_SERIAL_PORT)) {
        return;
    }

    read_len = ql_uart_read(port, rx, (int)sizeof(rx));
    if (read_len <= 0) {
        return;
    }
    if (sb_serial_pool_alloc(rx, (u32)read_len, &slot) == 0) {
        return;
    }
    (void)ql_rtos_queue_release(s_serial_queue, sizeof(slot), (u8 *)&slot, QL_NO_WAIT);
}

static void sb_serial_task(void *argv)
{
    u32 slot;
    char line[SB_SERIAL_LINE_LEN];
    char reply[SB_FACTORY_REPLY_LEN];
    sb_status_t status;

    (void)argv;
    SB_LOGI(SB_SERIAL_MODULE_NAME, "task started");
    sb_serial_post_event(SB_EVENT_SERIAL_READY, SB_STATUS_OK, "usb_cdc");

    while (1) {
        if (ql_rtos_queue_wait(s_serial_queue, (u8 *)&slot, sizeof(slot), QL_WAIT_FOREVER) != 0) {
            ql_rtos_task_sleep_ms(100u);
            continue;
        }
        if (sb_serial_pool_get(slot, line, (u32)sizeof(line)) == 0) {
            continue;
        }
        if (line[0] == '\0') {
            continue;
        }

        status = sb_factory_diag_dispatch_json(line,
                                               SB_FACTORY_CHANNEL_SERIAL,
                                               reply,
                                               (u32)sizeof(reply));
        if (status == SB_STATUS_OK) {
            sb_serial_post_event(SB_EVENT_FACTORY_COMMAND_DONE, status, "serial");
        } else {
            sb_serial_post_event(SB_EVENT_FACTORY_COMMAND_REJECTED, status, "serial");
        }
        (void)ql_uart_write(SB_SERIAL_PORT, (unsigned char *)reply, (int)sb_cloud_str_len(reply));
        (void)ql_uart_write(SB_SERIAL_PORT, (unsigned char *)"\r\n", 2);
    }
}

sb_status_t sb_serial_service_init(const sb_config_payload_t *config)
{
    QlOSStatus os_ret;
    int ret;

    (void)config;

    if (s_serial_started != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }

    if (sb_mode_factory_access_allowed() == 0) {
#ifndef SB_SERIAL_ASSET_PROVISIONING_ENABLED
        s_serial_enabled = 0;
        SB_LOGI(SB_SERIAL_MODULE_NAME, "disabled in production mode");
        return SB_STATUS_OK;
#else
        SB_LOGI(SB_SERIAL_MODULE_NAME, "enabled for asset provisioning");
#endif
    }

    if (ql_rtos_mutex_create(&s_serial_mutex) != 0) {
        return SB_STATUS_NO_MEMORY;
    }
    sb_serial_zero_pool();
    os_ret = ql_rtos_queue_create(&s_serial_queue, sizeof(u32), SB_SERIAL_QUEUE_DEPTH);
    if (os_ret != 0) {
        s_serial_queue = 0;
        return SB_STATUS_QUEUE_ERROR;
    }

    ret = ql_uart_open(SB_SERIAL_PORT, QL_UART_BAUD_115200, QL_FC_NONE);
    if (ret != 0) {
        s_serial_queue = 0;
        return SB_STATUS_SERIAL_ERROR;
    }
    ql_uart_register_cb(SB_SERIAL_PORT, sb_serial_uart_cb);

    os_ret = ql_rtos_task_create(&s_serial_task,
                                 SB_SERIAL_TASK_STACK_SIZE_BYTES,
                                 SB_SERIAL_TASK_PRIORITY,
                                 "sb_serial",
                                 sb_serial_task,
                                 0);
    if (os_ret != 0) {
        (void)ql_uart_close(SB_SERIAL_PORT);
        s_serial_queue = 0;
        s_serial_task = 0;
        return SB_STATUS_TASK_ERROR;
    }

    s_serial_enabled = 1;
    s_serial_started = 1;
    return SB_STATUS_OK;
}

int sb_serial_service_is_enabled(void)
{
    return s_serial_enabled;
}
