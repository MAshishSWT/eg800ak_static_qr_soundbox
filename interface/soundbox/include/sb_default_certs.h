/*================================================================
 * Static QR UPI Soundbox - EC200U Demo Certificate Buffers
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_DEFAULT_CERTS_H
#define SB_DEFAULT_CERTS_H

#ifdef __cplusplus
extern "C" {
#endif

const char *sb_default_certs_mqtt_root_ca(void);
const char *sb_default_certs_mqtt_client_crt(void);
const char *sb_default_certs_mqtt_client_key(void);

#ifdef __cplusplus
}
#endif

#endif /* SB_DEFAULT_CERTS_H */
