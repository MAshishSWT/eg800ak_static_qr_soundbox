# Phase 4 README - EG800AK Static QR UPI Soundbox Audio Service

## Implemented

Phase 4 adds the production audio service layer on top of the approved Phase 3 package:

- EG800AK audio HAL wrapper around `ql_get_audio_state`, `ql_codec_choose`, `ql_set_audio_path_speaker`, `ql_set_volume`, `ql_play_mp3`, and `ql_stop_mp3_play`.
- Optional ES8311 external codec initialization using Quectel's codec example register flow.
- Queue-backed MP3 playback service using QuecOpen RTOS task, queue, and semaphore APIs.
- Multilingual asset path model for English, Hindi, Marathi, Gujarati, Bengali, Kannada, Malayalam, Tamil, Telugu, and Punjabi.
- Provider prompt support for Paytm, PhonePe, Google Pay, BHIM, and other providers.
- Status prompt script support for power-on, ready, setup, no SIM, no network, no internet, no MQTT, battery low, and transaction error prompts.
- Integer paise-based amount tokenization, avoiding floating point amount handling.
- Indian numbering script support up to 99,99,999.99 rupees.
- Basic asset validation before playback.
- Audio events integrated into the supervisor.

## EG800AK SDK examples referenced

- `interface/audio/example_mp3.c`
- `interface/audio/example_play_audio.c`
- `interface/audio/example_codec_choose.c`
- `interface/os/example_rtos.c`
- `interface/fs/example_fs.c`

## Quectel documents referenced

- QuecOpen SDK Quick Start Guide
- RTOS Development Guide
- RTOS API Mapping User Guide
- Audio Development Guide
- Voice Development Guide
- File System Development Guide
- File Application Note
- I2C Development Guide

## EC200U source modules migrated conceptually

- `audio_prcs.c` - playback sequencing concept
- `transaction_play.c` - amount announcement concept
- `summary_play.c` - future summary prompt sequencing boundary
- `dailylog_play.c` - future last-transaction prompt boundary
- `keypad_prcs.c` - future volume and mode-triggered audio boundary

No EC200U platform API is copied into the Phase 4 implementation.

## Audio asset layout

The service looks for MP3 assets under `U:/audio/<language>/`.

Examples:

```text
U:/audio/en/prompt_power_on.mp3
U:/audio/en/prompt_ready.mp3
U:/audio/en/prompt_payment_received.mp3
U:/audio/en/provider_paytm.mp3
U:/audio/en/num_1.mp3
U:/audio/en/num_20.mp3
U:/audio/en/scale_hundred.mp3
U:/audio/en/scale_thousand.mp3
U:/audio/en/scale_lakh.mp3
U:/audio/en/currency_rupees.mp3
U:/audio/en/currency_paise.mp3
U:/audio/en/currency_only.mp3
```

Language directory codes:

```text
en hi mr gu bn kn ml ta te pa
```

## Integration

Copy this package over the EG800AK QuecOpen SDK application tree. The root Makefile builds only:

```text
interface/soundbox
```

The soundbox module Makefile includes all Phase 4 audio sources and sets:

```text
-DSB_PHASE_NUMBER=4
```

## Build

Use the Quectel Windows SDK build environment:

```bat
OpenEntry.bat
```

For Phase 4 runtime audio playback, enable at least MP3 in OpenEntry. If TLS/MQTT phases are also enabled later, MBEDTLS and other network features must also be selected.

## Hardware test on KAE8_SQ1

1. Flash the Phase 4 build.
2. Confirm boot logs show the app entry and BSP logs.
3. Confirm audio service logs either `audio ready` or a clear codec/audio fault.
4. Copy a small MP3 asset to `U:/audio/en/prompt_ready.mp3` using the SDK-supported file transfer method.
5. Call `sb_audio_service_play_prompt(SB_AUDIO_LANG_EN, SB_AUDIO_PROMPT_READY)` from a test hook or later command handler.
6. Confirm speaker PA enables and MP3 playback completes.
7. Add amount token assets and call `sb_audio_service_play_amount(...)` from a test hook or later MQTT transaction handler.

## Known assumptions

- `U:` is currently a small Quectel user filesystem partition and is adequate for config and a few test assets only.
- Production multilingual MP3 packs should use the external NOR/audio-pack layer once the hardware route is finalized.
- KAE8_SQ1 has ES8311 codec hardware. Phase 4 requires ES8311 on KAE8 production hardware and reports an audio fault if the external codec is not detected.
- Phase 4 does not implement MQTT transaction ingestion. That belongs to the connectivity/payment phases.
