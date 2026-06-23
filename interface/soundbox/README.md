# Static QR UPI Soundbox - EG800AK-CN

Clean MQTT demo build for KAE8_SQ1 hardware.

Version: `1.0.0-phase19-mqtt-lf-buffer`

This build disables external NOR, HTTP health update, and OTA code paths. Audio assets are resolved from the internal QuecOpen user filesystem under `U:/audio/...`. MQTT uses EC200U demo defaults and TLS certificate buffers through `SSL_CERT_FROM_BUF`.


Phase 19 normalizes embedded MQTT PEM buffers to LF-only line endings to match Quectel SDK MQTT examples and avoid mbedtls_pk_parse_key failures on the module.

Phase 20 adds MAIN UART file provisioning for audio files into `U:/audio/...` using `tools/soundbox_uart_ufs_push.py`.

Phase 21 changes the UART tool to transfer only essential demo prompts by default and switches MQTT TLS to root-level UFS certificate-file mode because buffer mode failed with `mbedtls_pk_parse_key` on the module.
