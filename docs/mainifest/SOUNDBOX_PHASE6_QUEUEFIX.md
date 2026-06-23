# Phase 6 Queue Fix

## Runtime observation

The first Phase 6 boot reported:

```text
[SB][W][app] mqtt service init status=QUEUE_ERROR
[SB][W][app] http service init status=QUEUE_ERROR
```

## Root cause

The initial MQTT/HTTP services created RTOS queues with large message sizes containing 512-byte payload buffers. On this EG800AK ThreadX QuecOpen SDK, `ql_rtos_queue_create()` is not suitable for large message objects.

## Fix

The queues now carry only a small `u32` slot index. Payloads are stored in static bounded pools:

```text
MQTT RX pool: 8 messages
MQTT TX pool: 8 messages
HTTP request pool: 4 messages
Queue item size: sizeof(u32)
```

This keeps payload capacity while reducing RTOS queue message size.

## Modified files

```text
interface/soundbox/src/sb_mqtt_service.c
interface/soundbox/src/sb_http_service.c
```
