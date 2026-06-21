# Phase 6 README - MQTT, HTTP and SSL Cloud Connectivity

## Implemented

Phase 6 adds cloud connectivity services on top of the Phase 5 network-ready state:

- MQTT service task using Quectel/Paho `MQTTClient` API.
- MQTT network connection using `NetworkInit()` and `NetworkConnect()`.
- MQTT connect, subscribe, publish, yield, disconnect and reconnect FSM.
- Payment topic subscription from `config.mqtt_sub_topic`.
- Command topic subscription derived as `<payment-topic>/cmd`.
- MQTT inbound message queue for later transaction/command domain phases.
- MQTT outbound publish queue.
- MQTT periodic health publish to `config.mqtt_pub_topic`.
- HTTP service task using `ql_http_client_request()`.
- HTTP JSON health POST to `<http_base_url>/health`.
- HTTP command-response POST queue to `<http_base_url>/command/response`.
- SSL/TLS configuration using `SSLConfig` from `ql_ssl_hal.h`.
- TLS CA files are referenced from filesystem paths, not hardcoded in firmware.
- Supervisor integration for MQTT/HTTP events.

## EG800AK SDK examples referenced

```text
interface/mqtt/example_mqtt.c
interface/http/example_httpclient.c
interface/http/example_httpclient_perform.c
interface/ssl/example_ssl.c
interface/network/data_call/example_datacall.c
interface/os/example_rtos.c
```

## Quectel documents referenced

```text
MQTT Development Guide
HTTP Development Guide
SSL Application Note
Data Call Development Guide
RTOS Development Guide
RTOS API Mapping User Guide
File System Development Guide
```

## EC200U source concepts migrated

```text
MQTT connection/reconnect concept
payment notification topic handling concept
cloud command topic handling concept
HTTP health packet concept
HTTP command response concept
```

No EC200U platform API is copied. All platform calls are through EG800AK QuecOpen APIs and SDK MQTT/HTTP/SSL wrappers.

## Integration

The root Makefile still builds only:

```text
interface/soundbox
```

The soundbox Makefile adds:

```text
src/sb_cloud_utils.c
src/sb_mqtt_service.c
src/sb_http_service.c
```

## Configuration assumptions

The service uses existing config fields:

```text
mqtt_host
mqtt_port
mqtt_client_id
mqtt_sub_topic
mqtt_pub_topic
http_base_url
mqtt_keepalive_sec
health_interval_sec
```

Empty MQTT/HTTP configuration does not crash the product. The service reports `not_configured` and waits for provisioning.

## TLS certificate assumptions

No certificate or private key is hardcoded. TLS uses filesystem certificate paths:

```text
U:/certs/mqtt_root_ca.pem
U:/certs/http_root_ca.pem
```

For MQTT TLS, use port `8883`. For MQTT plaintext test, use port `1883`. For HTTPS, `http_base_url` must start with `https://`.

## Build

Build using the Quectel OpenEntry/Windows SDK environment. Enable MQTT, HTTP, SSL/MBEDTLS and networking libraries.

## Hardware test on KAE8_SQ1

Phase 6 depends on Phase 5 network/data-call readiness. Expected logs after provisioning MQTT/HTTP config:

```text
[SB][I][mqtt] task started
[SB][I][mqtt] connected host=<broker> port=<port>
[SB][I][supervisor] mqtt ready port=<port> status=0
[SB][I][supervisor] mqtt payment message len=<n>
[SB][I][supervisor] http health done code=200
```

## Known assumptions

- Phase 6 does not parse payment JSON into transactions; that belongs to the transaction/domain phase.
- Phase 6 does not execute cloud commands; it queues command payloads for the command/domain phase.
- MQTT credentials and certificate provisioning are handled by later secure provisioning/securedata phases.
- External NOR remains a separate Phase 3 hardware/SPI issue and is not required for MQTT/HTTP bring-up.
