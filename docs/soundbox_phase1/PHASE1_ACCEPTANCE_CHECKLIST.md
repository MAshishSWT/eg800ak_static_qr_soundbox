# Phase 1 Acceptance Checklist

| Requirement | Status |
|---|---:|
| Soundbox folder tree exists under `interface/soundbox` | PASS |
| Root Makefile builds `interface/soundbox` only | PASS |
| Module Makefile follows EG800AK SDK pattern | PASS |
| Exactly one active `application_init()` exists in soundbox module | PASS |
| Quectel common headers are not modified | PASS |
| Quectel common libraries are not modified | PASS |
| EC200U code is not copied into the module | PASS |
| EC200U-only APIs are not included | PASS |
| App entry is implemented | PASS |
| Supervisor task is implemented | PASS |
| Event definitions are implemented | PASS |
| Event bus is implemented with Quectel RTOS queue APIs | PASS |
| Heartbeat timer is implemented with Quectel RTOS timer APIs | PASS |
| Logging skeleton is implemented | PASS |
| Error/status code contract is implemented | PASS |
| KAE8_SQ1 board mapping header is present | PASS |
| No hardcoded credentials or certificates are present | PASS |
| No unsafe fixed-buffer copy or concatenation API usage is present | PASS |
| No MQTT/audio/data-call/OTA runtime action is triggered in Phase 1 | PASS |

## Phase 2 entry gates

| Gate | Required before Phase 2 acceptance |
|---|---|
| GPIO BSP | Validate key/LED/SPK_SHDN pin function using `ql_pin_set_func()` and GPIO APIs. |
| Battery BSP | Validate ADC0 battery conversion with measured VBAT. |
| Audio BSP | Validate speaker path and PA shutdown polarity. |
| NOR BSP | Validate W25Q64 read-ID and storage strategy. |
| Factory/debug | Select UART and command framing. |
