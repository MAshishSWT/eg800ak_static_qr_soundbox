/*================================================================
 * Static QR UPI Soundbox - Command Dispatcher
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_power.h"
#include "ql_rtos.h"
#include "sb_audio_service.h"
#include "sb_audio_types.h"
#include "sb_cloud_utils.h"
#include "sb_command_dispatcher.h"
#include "sb_config.h"
#include "sb_crc32.h"
#include "sb_demo_profile.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_json.h"
#include "sb_log.h"
#include "sb_transaction_ledger.h"

#define SB_COMMAND_MODULE_NAME       "command"
#define SB_COMMAND_NAME_LEN          (32u)
#define SB_COMMAND_REQ_ID_LEN        (40u)
#define SB_COMMAND_SIG_LEN           (16u)
#define SB_COMMAND_RESPONSE_LEN      (256u)

typedef struct {
    char request_id[SB_COMMAND_REQ_ID_LEN];
    char command[SB_COMMAND_NAME_LEN];
    char signature[SB_COMMAND_SIG_LEN];
    u64 volume;
} sb_command_t;

static sb_config_payload_t s_command_config;
static int s_command_ready = 0;

static void sb_command_post_event(sb_event_id_t id, s32 status, const char *text)
{
    sb_event_t event;
    sb_event_init(&event, id, SB_EVENT_SOURCE_BUSINESS);
    event.param_s32 = status;
    if (text != 0) {
        (void)sb_event_set_text(&event, text);
    }
    (void)sb_event_post(&event, QL_NO_WAIT);
}

static char sb_command_hex_digit(u32 value)
{
    value &= 0x0Fu;
    return (value < 10u) ? (char)('0' + value) : (char)('a' + (value - 10u));
}

static void sb_command_crc_to_hex(u32 crc, char *out, u32 out_len)
{
    u32 i;
    if ((out == 0) || (out_len < 9u)) {
        return;
    }
    for (i = 0u; i < 8u; i++) {
        out[i] = sb_command_hex_digit(crc >> ((7u - i) * 4u));
    }
    out[8] = '\0';
}

static int sb_command_signature_valid(const sb_command_t *cmd)
{
    char material[128];
    char expected[9];
    u32 crc;

    if ((cmd == 0) || (cmd->signature[0] == '\0')) {
        return 0;
    }

    material[0] = '\0';
    (void)sb_cloud_append_string(material, (u32)sizeof(material), cmd->command);
    (void)sb_cloud_append_string(material, (u32)sizeof(material), ":");
    (void)sb_cloud_append_string(material, (u32)sizeof(material), cmd->request_id);
    (void)sb_cloud_append_string(material, (u32)sizeof(material), ":");
    (void)sb_cloud_append_string(material, (u32)sizeof(material), s_command_config.device_id);
    crc = sb_crc32_compute(material, sb_cloud_str_len(material));
    sb_command_crc_to_hex(crc, expected, (u32)sizeof(expected));

    return sb_cloud_text_equal(expected, cmd->signature);
}

static sb_status_t sb_command_parse(const sb_mqtt_inbound_message_t *message, sb_command_t *cmd)
{
    if ((message == 0) || (cmd == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    cmd->request_id[0] = '\0';
    cmd->command[0] = '\0';
    cmd->signature[0] = '\0';
    cmd->volume = 0u;

    if (sb_json_get_string(message->payload, "cmd", cmd->command, (u32)sizeof(cmd->command)) != SB_STATUS_OK) {
        return SB_STATUS_INVALID_PARAM;
    }
    (void)sb_json_get_string(message->payload, "request_id", cmd->request_id, (u32)sizeof(cmd->request_id));
    (void)sb_json_get_string(message->payload, "signature", cmd->signature, (u32)sizeof(cmd->signature));
    (void)sb_json_get_u64(message->payload, "volume", &cmd->volume);
    return SB_STATUS_OK;
}

static void sb_command_send_response(const sb_command_t *cmd, const char *status)
{
    char payload[SB_COMMAND_RESPONSE_LEN];
    if ((cmd == 0) || (status == 0)) {
        return;
    }

    payload[0] = '\0';
    (void)sb_cloud_append_string(payload, (u32)sizeof(payload), "{\"type\":\"cmd_ack\",\"request_id\":");
    (void)sb_cloud_append_json_string(payload, (u32)sizeof(payload), cmd->request_id);
    (void)sb_cloud_append_string(payload, (u32)sizeof(payload), ",\"cmd\":");
    (void)sb_cloud_append_json_string(payload, (u32)sizeof(payload), cmd->command);
    (void)sb_cloud_append_string(payload, (u32)sizeof(payload), ",\"status\":");
    (void)sb_cloud_append_json_string(payload, (u32)sizeof(payload), status);
    (void)sb_cloud_append_string(payload, (u32)sizeof(payload), "}");

    if (s_command_config.mqtt_pub_topic[0] != '\0') {
        (void)sb_mqtt_service_publish(s_command_config.mqtt_pub_topic, payload, sb_cloud_str_len(payload), 1u, 0u);
    }
}


static void sb_command_send_summary_response(const sb_command_t *cmd)
{
    char payload[SB_COMMAND_RESPONSE_LEN];
    sb_daily_summary_t summary;

    if (cmd == 0) {
        return;
    }
    if (sb_transaction_ledger_get_daily(&summary) != SB_STATUS_OK) {
        sb_command_send_response(cmd, "summary_unavailable");
        return;
    }

    payload[0] = '\0';
    (void)sb_cloud_append_string(payload, (u32)sizeof(payload), "{\"type\":\"cmd_ack\",\"request_id\":");
    (void)sb_cloud_append_json_string(payload, (u32)sizeof(payload), cmd->request_id);
    (void)sb_cloud_append_string(payload, (u32)sizeof(payload), ",\"cmd\":");
    (void)sb_cloud_append_json_string(payload, (u32)sizeof(payload), cmd->command);
    (void)sb_cloud_append_string(payload, (u32)sizeof(payload), ",\"status\":\"ok\",\"daily_count\":");
    (void)sb_cloud_append_u32(payload, (u32)sizeof(payload), summary.count);
    (void)sb_cloud_append_string(payload, (u32)sizeof(payload), ",\"daily_total_paise\":");
    (void)sb_cloud_append_u32(payload, (u32)sizeof(payload), (u32)summary.total_paise);
    (void)sb_cloud_append_string(payload, (u32)sizeof(payload), "}");

    if (s_command_config.mqtt_pub_topic[0] != '\0') {
        (void)sb_mqtt_service_publish(s_command_config.mqtt_pub_topic, payload, sb_cloud_str_len(payload), 1u, 0u);
    }
}

static sb_status_t sb_command_execute(const sb_command_t *cmd)
{
    sb_daily_summary_t summary;
    u32 volume;

    if (sb_cloud_text_equal(cmd->command, "ping") != 0) {
        return SB_STATUS_OK;
    }

#ifndef SB_ENABLE_INSECURE_CRC32_COMMAND_AUTH
    return SB_STATUS_SECURITY_ERROR;
#endif

    if (sb_command_signature_valid(cmd) == 0) {
        return SB_STATUS_SECURITY_ERROR;
    }

    if (sb_cloud_text_equal(cmd->command, "set_volume") != 0) {
        volume = (cmd->volume > 100u) ? 100u : (u32)cmd->volume;
        s_command_config.volume_percent = volume;
        (void)sb_config_commit(&s_command_config);
        return sb_audio_service_set_volume(volume);
    }

    if (sb_cloud_text_equal(cmd->command, "play_ready") != 0) {
        return sb_audio_service_play_prompt(sb_audio_language_from_code(s_command_config.language), SB_AUDIO_PROMPT_READY);
    }

    if (sb_cloud_text_equal(cmd->command, "get_summary") != 0) {
        if (sb_transaction_ledger_get_daily(&summary) == SB_STATUS_OK) {
            return SB_STATUS_OK;
        }
        return SB_STATUS_NOT_FOUND;
    }

    if (sb_cloud_text_equal(cmd->command, "power_down") != 0) {
        (void)sb_audio_service_play_common("good_bye.mp3");
        ql_rtos_task_sleep_ms(3000u);
        ql_power_down(1u);
        return SB_STATUS_OK;
    }

    return SB_STATUS_UNSUPPORTED;
}

sb_status_t sb_command_dispatcher_init(void)
{
    if (s_command_ready != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }
    sb_config_make_defaults(&s_command_config);
    (void)sb_config_get(&s_command_config);
    sb_demo_expand_config_runtime(&s_command_config);
    s_command_ready = 1;
    return SB_STATUS_OK;
}

sb_status_t sb_command_dispatcher_handle_message(const sb_mqtt_inbound_message_t *message)
{
    sb_command_t cmd;
    sb_status_t status;

    if ((message == 0) || (message->type != SB_MQTT_MESSAGE_COMMAND)) {
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_command_parse(message, &cmd);
    if (status != SB_STATUS_OK) {
        sb_command_post_event(SB_EVENT_COMMAND_REJECTED, (s32)status, "parse");
        return status;
    }

    status = sb_command_execute(&cmd);
    if (status == SB_STATUS_OK) {
        if (sb_cloud_text_equal(cmd.command, "get_summary") != 0) {
            sb_command_send_summary_response(&cmd);
        } else {
            sb_command_send_response(&cmd, "ok");
        }
        sb_command_post_event(SB_EVENT_COMMAND_ACCEPTED, SB_STATUS_OK, cmd.command);
    } else {
        sb_command_send_response(&cmd, "rejected");
        sb_command_post_event(SB_EVENT_COMMAND_REJECTED, (s32)status, cmd.command);
    }
    SB_LOGI(SB_COMMAND_MODULE_NAME, "command=%s status=%s", cmd.command, sb_status_to_string(status));
    return status;
}
