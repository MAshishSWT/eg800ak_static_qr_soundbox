# Phase 10 MQTT/HTTP Certificate Preflight Fix

## Reason

Runtime logs showed repeated MQTT TLS failures:

```text
SSLHandshake failed: net_set_nonblock returned -0x1
mqtt network connect failed rc=-1
```

The logs also showed a modem exception in the internal HTTP client task:

```text
EE LOG: Task: http_cli
EE LOG: Silent Reset
SW restart reason: Error reset (WDT)
```

This indicates the device reset was triggered by a Quectel internal HTTP client task exception/watchdog after HTTPS/HTTP fault handling, not by BSP, external NOR, audio, or the business task.

## Fix

- MQTT now checks required TLS certificate/key files before calling `NetworkConnect()`.
- HTTP now checks its root CA file before creating the HTTP client.
- If certificate assets are missing, the service posts a clear fault and backs off instead of repeatedly entering the TLS/HTTP stack.

## Required files

```text
U:/mqtt_root_ca.pem
U:/mqtt_client.crt
U:/mqtt_client.key
U:/http_root_ca.pem
```

No private certificate, key, password, or token is embedded in the firmware.
