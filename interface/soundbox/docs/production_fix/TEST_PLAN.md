# Phase 23 Test Plan

## Boot log acceptance

Expected boot summary contains:

```text
app: feature summary phase=23 nor=enabled http=enabled mqtt_tls=file serial=main_uart
storage: U drive mounted
storage: fs self-test passed
nor: JEDEC ID accepted
asset_manifest: ready entries=<count>
audio: ready
http: task started
mqtt: connected and subscribed
```

## U-drive self-test

Command: `fs_self_test`.

Pass criteria:

```text
storage: fs self-test passed crc=<value>
```

## NOR ID test

Command: `nor_id`.

Pass criteria:

```text
nor: id mfg=0xEF memory=0x40 capacity=0x17
```

Equivalent W25Q64 IDs from approved supply lots may be accepted after engineering review.

## NOR read/write manufacturing test

Command: `nor_rw_test`.

Pass criteria:

```text
nor: factory rw test passed addr=0x007FF000
```

## Audio playback

Common files:

```text
play_common start_tune.mp3
play_common ping.mp3
play_common good_bye.mp3
play_common transaction_error.mp3
```

Language files:

```text
play_lang en internet.mp3
play_lang hi no_internet.mp3
play_lang kn trasnsactions.mp3
```

Pass criteria: audible prompt, no codec error, no cache CRC mismatch.

## Key validation

- SW1 short press: volume up and ping.
- SW2 short press: volume down and ping.
- SW3 short press: mode/action.
- Hold key: long-press event after configured threshold.
- Boot with a pressed key: stuck-key diagnostic is logged.

## One LED validation

- Solid on: ready.
- Slow blink: no internet.
- Fast blink: no MQTT.
- Double blink: unregistered.
- Triple blink: low battery.
- Rapid blink: fatal/storage fault.
- Short pulse: key acknowledgement.

## Network states

- SIM removed: no SIM prompt.
- PDP fail: no internet prompt and slow blink.
- HTTPS registered true: normal MQTT flow.
- HTTPS registered false: unregistered prompt and double blink.
- MQTT TLS fail: MQTT fault without ready event.
- MQTT subscribe fail: MQTT fault without ready event.

## Shutdown

Command or power-key shutdown sequence must play `U:/good_bye.mp3` before final power transition when enough battery is available.
