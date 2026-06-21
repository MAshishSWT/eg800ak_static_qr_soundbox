/*================================================================
 * Static QR UPI Soundbox - SMS Recovery Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "ql_sms.h"
#include "sb_cloud_utils.h"
#include "sb_error.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_factory_diag.h"
#include "sb_log.h"
#include "sb_sms_service.h"

#define SB_SMS_MODULE_NAME "sms"

static char s_sms_code_gsm[] = "GSM";
static char s_sms_storage_sm[] = "SM";

typedef struct {
    u32 index;
} sb_sms_queue_item_t;

static ql_task_t s_sms_task = 0;
static ql_queue_t s_sms_queue = 0;
static int s_sms_started = 0;
static int s_sms_enabled = 0;

static void sb_sms_post_event(sb_event_id_t id, sb_status_t status, u32 value, const char *text)
{
    sb_event_t event;

    sb_event_init(&event, id, SB_EVENT_SOURCE_FACTORY);
    event.param_s32 = (s32)status;
    event.param_u32 = value;
    if (text != 0) {
        (void)sb_event_set_text(&event, text);
    }
    (void)sb_event_post(&event, QL_NO_WAIT);
}

static void sb_sms_zero_recv(recvmessage *message)
{
    u32 i;
    unsigned char *ptr;

    if (message == 0) {
        return;
    }
    ptr = (unsigned char *)message;
    for (i = 0u; i < (u32)sizeof(*message); i++) {
        ptr[i] = 0u;
    }
}

static void sb_sms_event_handler(unsigned int flag,
                                 void *msg_buf,
                                 unsigned int msg_len,
                                 void *context)
{
    sb_sms_queue_item_t item;

    (void)msg_len;
    (void)context;

    if ((s_sms_queue == 0) || (msg_buf == 0)) {
        return;
    }

    item.index = 0u;
    if (flag == QUEC_CI_MSG_PRIM_NEWMSG_INDEX_IND) {
        item.index = (u32)((QL_NEWMSG_INDEX *)msg_buf)->index;
    } else if (flag == QUEC_VOLTE_NEWMSG_INDEX_IND) {
        item.index = (u32)((quec_volte_sms_info_cmti *)msg_buf)->index;
    } else {
        return;
    }

    (void)ql_rtos_queue_release(s_sms_queue, sizeof(item), (u8 *)&item, QL_NO_WAIT);
}

static void sb_sms_send_reply(const char *number, const char *reply)
{
    if ((number == 0) || (reply == 0) || (number[0] == '\0') || (reply[0] == '\0')) {
        return;
    }
    (void)ql_sms_send_text_msg((uint_8 *)number, (uint_8 *)reply, s_sms_code_gsm);
}

static void sb_sms_task(void *argv)
{
    sb_sms_queue_item_t item;
    recvmessage message;
    char reply[SB_FACTORY_REPLY_LEN];
    sb_status_t status;

    (void)argv;
    SB_LOGI(SB_SMS_MODULE_NAME, "task started");
    sb_sms_post_event(SB_EVENT_SMS_READY, SB_STATUS_OK, 0u, "enabled");

    while (1) {
        if (ql_rtos_queue_wait(s_sms_queue, (u8 *)&item, sizeof(item), QL_WAIT_FOREVER) != 0) {
            ql_rtos_task_sleep_ms(100u);
            continue;
        }

        sb_sms_zero_recv(&message);
        if (ql_search_sms_text_message((int)item.index, &message) != QL_SMS_SUCCESS) {
            sb_sms_post_event(SB_EVENT_SMS_FAULT, SB_STATUS_SMS_ERROR, item.index, "read");
            continue;
        }

        if (message.buf[0] == '\0') {
            sb_sms_post_event(SB_EVENT_SMS_FAULT, SB_STATUS_INVALID_PARAM, item.index, "empty");
            (void)ql_sms_delete_msg_ex(item.index, QL_SMS_DEL_INDEX);
            continue;
        }

        if (sb_factory_diag_sms_sender_allowed(message.num) == 0) {
            sb_sms_post_event(SB_EVENT_FACTORY_COMMAND_REJECTED, SB_STATUS_SECURITY_ERROR, item.index, "sms_sender");
            (void)ql_sms_delete_msg_ex(item.index, QL_SMS_DEL_INDEX);
            continue;
        }

        sb_sms_post_event(SB_EVENT_SMS_COMMAND, SB_STATUS_OK, item.index, "rx");
        status = sb_factory_diag_dispatch_json(message.buf,
                                               SB_FACTORY_CHANNEL_SMS,
                                               reply,
                                               (u32)sizeof(reply));
        if (status == SB_STATUS_OK) {
            sb_sms_post_event(SB_EVENT_FACTORY_COMMAND_DONE, status, item.index, "sms");
        } else {
            sb_sms_post_event(SB_EVENT_FACTORY_COMMAND_REJECTED, status, item.index, "sms");
        }
        sb_sms_send_reply(message.num, reply);
        (void)ql_sms_delete_msg_ex(item.index, QL_SMS_DEL_INDEX);
    }
}

sb_status_t sb_sms_service_init(const sb_config_payload_t *config)
{
    QlOSStatus os_ret;

    if (s_sms_started != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }

    if ((config == 0) || (config->sms_recovery_enabled == 0u)) {
        s_sms_enabled = 0;
        SB_LOGI(SB_SMS_MODULE_NAME, "disabled by config");
        return SB_STATUS_OK;
    }

    os_ret = ql_rtos_queue_create(&s_sms_queue, sizeof(sb_sms_queue_item_t), SB_SMS_QUEUE_DEPTH);
    if (os_ret != 0) {
        s_sms_queue = 0;
        return SB_STATUS_QUEUE_ERROR;
    }

    if (ql_sms_add_event_handler(sb_sms_event_handler, 0) != QL_SMS_SUCCESS) {
        s_sms_queue = 0;
        return SB_STATUS_SMS_ERROR;
    }
    (void)ql_set_sms_msg_mode(1);
    (void)ql_set_sms_code_mode(s_sms_code_gsm);
    (void)ql_set_sms_save_location(s_sms_storage_sm, s_sms_storage_sm, s_sms_storage_sm);
    (void)ql_set_sms_recive_dealmode(0);

    os_ret = ql_rtos_task_create(&s_sms_task,
                                 SB_SMS_TASK_STACK_SIZE_BYTES,
                                 SB_SMS_TASK_PRIORITY,
                                 "sb_sms",
                                 sb_sms_task,
                                 0);
    if (os_ret != 0) {
        s_sms_task = 0;
        s_sms_queue = 0;
        return SB_STATUS_TASK_ERROR;
    }

    s_sms_started = 1;
    s_sms_enabled = 1;
    return SB_STATUS_OK;
}

int sb_sms_service_is_enabled(void)
{
    return s_sms_enabled;
}
