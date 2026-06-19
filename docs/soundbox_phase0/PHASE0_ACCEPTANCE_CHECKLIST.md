# Phase 0 Acceptance Checklist

## Phase 0 closure status

| Item | Status |
|---|---:|
| Architecture and migration documents added | PASS |
| Build-integrated soundbox anchor module added | PASS |
| No production runtime application task registered | PASS |
| No Quectel `common/include` header modified | PASS |
| No Quectel `common/lib` file modified | PASS |
| Root Makefile registers `interface/soundbox` anchor module | PASS |
| `interface/soundbox/Makefile` follows SDK module pattern | PASS |
| Public Phase 0 headers are present | PASS |
| KAE8 key/LED/audio/battery schematic mappings are captured | PASS |
| EC200U-only APIs are listed as forbidden for EG800AK port | PASS |
| No hardcoded credentials or certificates added | PASS |

## EG800AK SDK deep-dive acceptance

| SDK area | Status |
|---|---:|
| `ql_application.h` entry style documented | PASS |
| `ql_rtos.h` task/queue/semaphore/mutex/timer/flag APIs documented | PASS |
| GPIO/EINT APIs and examples mapped | PASS |
| ADC API and battery use case mapped | PASS |
| Audio APIs and MP3 playback options mapped | PASS |
| File system and SPI NOR APIs mapped | PASS |
| SIM/network/data-call APIs mapped | PASS |
| Paho-style MQTT APIs mapped | PASS |
| HTTP client APIs mapped | PASS |
| SSL/TLS configuration boundary mapped | PASS |
| RTC/NTP APIs mapped | PASS |
| FOTA APIs mapped | PASS |
| SMS APIs mapped | PASS |
| Watchdog/power recovery APIs mapped | PASS |
| Secure data API mapped | PASS |

## EC200U business-logic acceptance

| EC200U logic area | Status |
|---|---:|
| MQTT payment parsing behavior identified | PASS |
| Command set and command response behavior identified | PASS |
| Health packet behavior identified | PASS |
| Audio prompt and multilingual amount grammar modules identified | PASS |
| Key actions and long press behaviors identified | PASS |
| Daily reset behavior identified | PASS |
| Firmware/audio OTA concepts identified | PASS |
| SMS recovery concept identified | PASS |
| EC200U-only APIs explicitly listed as forbidden | PASS |

## Architecture acceptance

| Architecture requirement | Status |
|---|---:|
| Clean layers defined: BSP/HAL, OS/event bus, platform services, connectivity services, domain services, supervisor | PASS |
| Event-driven task model defined | PASS |
| A/B config slot design with magic/version/CRC/migration defined | PASS |
| Transaction ledger with transaction ID idempotency defined | PASS |
| Signed command validation pipeline defined | PASS |
| Secure OTA manifest with hash/signature verification boundary defined | PASS |
| Secure credential handling with `ql_securedata` defined | PASS |
| Production logging policy defined | PASS |
| Watchdog and fault recovery policy defined | PASS |

## Hardware acceptance

| Hardware mapping | Status |
|---|---:|
| Battery ADC divider and formula documented | PASS |
| ES8311 + 8002A audio path documented | PASS |
| W25Q64JWSIQ external NOR physical nets documented | PASS |
| SIM/antenna interfaces documented | PASS |
| SW1/SW2/SW3 active-low key circuits documented | PASS |
| SW1/SW2/SW3 EG800AK pin/GPIO symbols captured in `sb_board_kae8_sq1.h` | PASS |
| USER_LED_1 EG800AK pin/GPIO symbol captured in `sb_board_kae8_sq1.h` | PASS |
| SPK_SHDN EG800AK pin/GPIO symbol captured in `sb_board_kae8_sq1.h` | PASS |

## Phase 1 entry gates

| Gate | Required result before production app code acceptance |
|---|---|
| Production root build selection | Example directories with active `application_init()` excluded when soundbox supervisor is introduced |
| SPI NOR port selection | W25Q64 read-ID succeeds through selected `ql_spi_nor.h` port constant |
| Audio PA polarity | `SPK_SHDN` enable/disable polarity confirmed on assembled KAE8_SQ1 hardware |
| LED polarity | `USER_LED_1` on/off polarity confirmed on assembled hardware |
| Backend payload schema | MQTT payment and command schema finalized before parser implementation |
| Audio asset convention | Provider/language/path naming finalized before asset manager implementation |
| Credential provisioning | Secure provisioning flow finalized before MQTT/HTTPS implementation |
