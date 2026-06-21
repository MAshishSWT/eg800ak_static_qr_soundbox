# Phase 6 Test Procedure

## 1. Build test

Build in the Quectel SDK environment and confirm `build/app.elf` and `build/app.bin` are produced.

## 2. Empty configuration test

Boot with default config. Expected:

```text
[SB][W][supervisor] mqtt fault status=-23 text=not_configured
```

The device must keep running.

## 3. Plain MQTT test

Provision:

```text
mqtt_host=<broker host or IP>
mqtt_port=1883
mqtt_client_id=<device id>
mqtt_sub_topic=<payment topic>
mqtt_pub_topic=<health/status topic>
```

Expected:

```text
[SB][I][mqtt] connected host=<broker> port=1883
[SB][I][supervisor] mqtt ready port=1883 status=0
```

Publish a test payload to `<payment topic>`. Expected:

```text
[SB][I][supervisor] mqtt payment message len=<n>
```

Publish a test payload to `<payment topic>/cmd`. Expected:

```text
[SB][I][supervisor] mqtt command message len=<n>
```

## 4. MQTT TLS test

Provision `mqtt_port=8883` and place CA certificate at:

```text
U:/certs/mqtt_root_ca.pem
```

Expected successful TLS connection and MQTT ready event.

## 5. HTTP health test

Provision:

```text
http_base_url=http://<server>
```

or HTTPS with CA file:

```text
http_base_url=https://<server>
U:/certs/http_root_ca.pem
```

Expected:

```text
[SB][I][supervisor] http health done code=200
```

## 6. Reconnect test

Remove antenna or stop the broker. Expected:

```text
[SB][W][supervisor] mqtt disconnected status=<n> text=yield
```

Restore network/broker. MQTT service should reconnect automatically.
