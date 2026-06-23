/*================================================================
 * Static QR UPI Soundbox - EC200U Demo Cloud Profile
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_dev.h"
#include "sb_cloud_utils.h"
#include "sb_demo_profile.h"

#define SB_DEMO_FALLBACK_IMEI "000000000000000"

static int sb_demo_has_token(const char *text, u32 pos)
{
    return ((text[pos] == '{') &&
            (text[pos + 1u] == 'i') &&
            (text[pos + 2u] == 'm') &&
            (text[pos + 3u] == 'e') &&
            (text[pos + 4u] == 'i') &&
            (text[pos + 5u] == '}')) ? 1 : 0;
}

sb_status_t sb_demo_get_imei(char *imei, u32 imei_len)
{
    if ((imei == 0) || (imei_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }

    imei[0] = '\0';
    if (ql_dev_get_imei(imei, imei_len) != QL_DEV_SUCCESS) {
        sb_cloud_copy_string(imei, imei_len, SB_DEMO_FALLBACK_IMEI);
        return SB_STATUS_ERROR;
    }

    imei[imei_len - 1u] = '\0';
    if (imei[0] == '\0') {
        sb_cloud_copy_string(imei, imei_len, SB_DEMO_FALLBACK_IMEI);
        return SB_STATUS_ERROR;
    }

    return SB_STATUS_OK;
}

sb_status_t sb_demo_expand_text(const char *input, char *output, u32 output_len)
{
    char imei[24];
    u32 i;
    u32 pos;

    if ((input == 0) || (output == 0) || (output_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }

    (void)sb_demo_get_imei(imei, (u32)sizeof(imei));
    output[0] = '\0';
    pos = 0u;

    for (i = 0u; input[i] != '\0'; i++) {
        if (sb_demo_has_token(input, i) != 0) {
            if (sb_cloud_append_string(output, output_len, imei) != SB_STATUS_OK) {
                return SB_STATUS_NO_MEMORY;
            }
            pos = sb_cloud_str_len(output);
            i += 5u;
        } else {
            char ch[2];
            if ((pos + 1u) >= output_len) {
                output[output_len - 1u] = '\0';
                return SB_STATUS_NO_MEMORY;
            }
            ch[0] = input[i];
            ch[1] = '\0';
            if (sb_cloud_append_string(output, output_len, ch) != SB_STATUS_OK) {
                return SB_STATUS_NO_MEMORY;
            }
            pos++;
        }
    }

    return SB_STATUS_OK;
}

void sb_demo_apply_config_defaults(sb_config_payload_t *config)
{
    if (config == 0) {
        return;
    }

    if (config->device_id[0] == '\0') {
        sb_cloud_copy_string(config->device_id, SB_CONFIG_DEVICE_ID_LEN, "{imei}");
    }
    if (config->apn[0] == '\0') {
        sb_cloud_copy_string(config->apn, SB_CONFIG_APN_LEN, SB_DEMO_APN);
    }
    if (config->mqtt_host[0] == '\0') {
        sb_cloud_copy_string(config->mqtt_host, SB_CONFIG_MQTT_HOST_LEN, SB_DEMO_MQTT_HOST);
    }
    if (config->mqtt_port == 0u) {
        config->mqtt_port = SB_DEMO_MQTT_PORT;
    }
    if (config->mqtt_client_id[0] == '\0') {
        sb_cloud_copy_string(config->mqtt_client_id, SB_CONFIG_MQTT_CLIENT_ID_LEN, SB_DEMO_MQTT_CLIENT_ID_TEMPLATE);
    }
    if (config->mqtt_sub_topic[0] == '\0') {
        sb_cloud_copy_string(config->mqtt_sub_topic, SB_CONFIG_TOPIC_LEN, SB_DEMO_MQTT_SUB_TOPIC_TEMPLATE);
    }
    if (config->mqtt_pub_topic[0] == '\0') {
        sb_cloud_copy_string(config->mqtt_pub_topic, SB_CONFIG_TOPIC_LEN, SB_DEMO_MQTT_PUB_TOPIC_TEMPLATE);
    }
}

static void sb_demo_expand_field(char *field, u32 field_len)
{
    char expanded[SB_CONFIG_TOPIC_LEN];

    if ((field == 0) || (field_len == 0u) || (field[0] == '\0')) {
        return;
    }

    if (sb_demo_expand_text(field, expanded, (u32)sizeof(expanded)) == SB_STATUS_OK) {
        sb_cloud_copy_string(field, field_len, expanded);
    }
}

void sb_demo_expand_config_runtime(sb_config_payload_t *config)
{
    if (config == 0) {
        return;
    }

    sb_demo_apply_config_defaults(config);
    sb_demo_expand_field(config->device_id, SB_CONFIG_DEVICE_ID_LEN);
    sb_demo_expand_field(config->mqtt_client_id, SB_CONFIG_MQTT_CLIENT_ID_LEN);
    sb_demo_expand_field(config->mqtt_sub_topic, SB_CONFIG_TOPIC_LEN);
    sb_demo_expand_field(config->mqtt_pub_topic, SB_CONFIG_TOPIC_LEN);
}
