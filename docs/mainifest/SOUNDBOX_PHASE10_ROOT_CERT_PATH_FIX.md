# Phase 10 Root Certificate Path Fix

The EG800AK user filesystem on this build is used with certificate files in the `U:/` root. The MQTT and HTTP TLS certificate paths were updated from `U:/certs/...` to root-level files.

## Required root-level certificate files

```text
U:/mqtt_root_ca.pem
U:/mqtt_client.crt
U:/mqtt_client.key
U:/http_root_ca.pem
```

## Runtime behavior

- MQTT checks the three root-level MQTT files before entering TLS handshake.
- HTTP checks the root-level HTTP CA file before creating the Quectel HTTP client.
- Missing certificate logs are rate-limited to avoid repeated log flooding.
- UART fast asset-pack transfer is preserved.
