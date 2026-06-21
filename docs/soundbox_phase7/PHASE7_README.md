# Soundbox Phase 7 - Business Logic

## Implemented

Phase 7 adds the production business layer on top of Phase 6 MQTT/HTTP connectivity:

- Payment MQTT message parsing.
- Transaction ID idempotency.
- Persistent transaction ledger on `U:/sb_ledger.bin` with CRC protection.
- Daily count and daily total tracking with automatic RTC date reset.
- Payment acknowledgement publish.
- Payment amount audio announcement through the Phase 4 audio service.
- Command dispatcher for `ping`, `set_volume`, `play_ready`, and `get_summary`.
- Command response through MQTT publish and HTTP command-response queue.
- Key actions for volume, last transaction, battery, signal, and daily summary.
- Health payload builder for later HTTP/MQTT health reporting expansion.

## EG800AK SDK examples referenced

- `interface/os/example_rtos.c` for RTOS task and mutex usage.
- `interface/fs/example_fs.c` for file persistence patterns.
- `interface/time/example_rtc.c` for RTC time source used by daily reset.
- `interface/mqtt/example_mqtt.c` for inbound MQTT message flow.
- `interface/http/example_httpclient.c` for command response transport dependency.

## Quectel documents referenced

- RTOS Development Guide.
- RTOS API Mapping User Guide.
- File System Development Guide.
- File Application Note.
- RTC Development Guide.
- MQTT Development Guide.
- HTTP Development Guide.

## EC200U logic migrated

The EC200U monolithic business concepts were migrated into small EG800AK services:

- Transaction announcement flow -> `sb_payment_processor`.
- Transaction history / last transaction -> `sb_transaction_ledger`.
- Daily summary -> `sb_transaction_ledger` + key action handling.
- Key actions -> `sb_business_service`.
- Cloud command response -> `sb_command_dispatcher`.

No EC200U platform APIs are used.

## Integration

Copy this package over the EG800AK `ql-application/threadx` tree. The root `Makefile` builds only `interface/soundbox`. No Quectel common headers or libraries are modified.

## Build

Use the normal Quectel OpenEntry / Windows SDK build flow. Ensure network, MQTT, HTTP, SSL, audio, FS, and MP3 options remain enabled as in previous phases.

## KAE8_SQ1 hardware test

1. Boot with SIM inserted.
2. Confirm BSP, audio, SIM, network, data call, and time are ready.
3. Provision MQTT and HTTP fields in config.
4. Publish a payment payload to the payment topic.
5. Confirm ledger update, MQTT ack, and payment amount audio.
6. Press volume keys for volume change.
7. Short-press mode for last transaction announcement.
8. Long-press mode for daily summary announcement.

## Known assumptions

- Payment payload is JSON and includes `tx_id`/`txn_id`/`transaction_id` plus `amount_paise` or decimal `amount`.
- Command signature is a bounded validation hook using CRC32 over command/request/device id. A later security phase should replace this with HMAC/signature using secure credential storage.
- External NOR remains a separate hardware/SPI issue; Phase 7 ledger uses internal user filesystem `U:`.
