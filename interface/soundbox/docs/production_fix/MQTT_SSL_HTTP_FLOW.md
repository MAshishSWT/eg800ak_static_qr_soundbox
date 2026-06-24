# MQTT, SSL, and HTTP Flow

## TLS files

Factory provisioning writes the MQTT certificate set to:

- `U:/certs/mqtt_root_ca.pem`
- `U:/certs/mqtt_client.crt`
- `U:/certs/mqtt_client.key`

The HTTP registration service uses `U:/certs/mqtt_root_ca.pem` as the root CA path by default. Use a separate CA only after changing `SB_HTTP_ROOT_CA_PATH` and provisioning that file.

## MQTT state flow

1. SIM ready.
2. PDP data call ready.
3. RTC/NTP time valid.
4. TLS files present and PEM headers valid.
5. TCP/TLS connect.
6. MQTT connect.
7. Topic subscription success.
8. `MQTT_READY` event.

`MQTT_READY` is not posted before subscription success. Errors are classified as no internet, SSL failure, broker/connect failure, subscription failure, or publish failure.

## HTTP registration flow

1. Wait for network online.
2. Wait for valid time.
3. Send HTTPS registration payload containing device ID, firmware version, hardware version, and health JSON.
4. If server returns Kiotel true / registered true, normal MQTT operation continues.
5. If server returns Kiotel false / registered false, the supervisor raises unregistered state and plays the setup/unregistered prompt.
6. Request failures use backoff and do not block audio or key tasks.

Default registration path: `/api/soundbox/register`.
Production host is taken from the configured MQTT host unless the config schema is extended with a dedicated HTTP host.
