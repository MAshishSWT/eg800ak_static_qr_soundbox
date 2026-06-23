# Static QR UPI Soundbox - EG800AK-CN

Clean MQTT demo build for KAE8_SQ1 hardware.

Version: `1.0.0-phase18-mqtt-buffer-clean`

This build disables external NOR, HTTP health update, and OTA code paths. Audio assets are resolved from the internal QuecOpen user filesystem under `U:/audio/...`. MQTT uses EC200U demo defaults and TLS certificate buffers through `SSL_CERT_FROM_BUF`.
