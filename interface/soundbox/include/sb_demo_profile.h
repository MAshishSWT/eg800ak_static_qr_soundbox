/*================================================================
 * Static QR UPI Soundbox - EC200U Demo Cloud Profile
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_DEMO_PROFILE_H
#define SB_DEMO_PROFILE_H

#include "ql_type.h"
#include "sb_config.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_DEMO_APN                         "m2misafe"
#define SB_DEMO_CLIENT_CODE                 "client1"
#define SB_DEMO_PROVIDER                    "vi"
#define SB_DEMO_MODEL_NO                    "ENPAY4G"
#define SB_DEMO_MQTT_HOST                   "a3pzee9xiwvo6-ats.iot.us-east-1.amazonaws.com"
#define SB_DEMO_MQTT_PORT                   (8883u)
#define SB_DEMO_MQTT_CLIENT_ID_TEMPLATE     "device-{imei}"
#define SB_DEMO_MQTT_SUB_TOPIC_TEMPLATE     "kiotel/client1/sb/{imei}"
#define SB_DEMO_MQTT_PUB_TOPIC_TEMPLATE     "kiotel/client1/sb/{imei}/health"
#define SB_DEMO_HTTP_BASE_URL               "https://103.75.249.183/dms-OMADM"
#define SB_DEMO_MQTT_CLIENT_CERT_PATH       "U:/mqtt_client.crt"
#define SB_DEMO_MQTT_CLIENT_KEY_PATH        "U:/mqtt_client.key"

sb_status_t sb_demo_get_imei(char *imei, u32 imei_len);
sb_status_t sb_demo_expand_text(const char *input, char *output, u32 output_len);
void sb_demo_apply_config_defaults(sb_config_payload_t *config);
void sb_demo_expand_config_runtime(sb_config_payload_t *config);

#ifdef __cplusplus
}
#endif

#endif /* SB_DEMO_PROFILE_H */
