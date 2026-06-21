# Phase 8 Test Procedure

## Boot validation

Expected:

```text
[SB][I][app] starting 1.0.0-phase8-ota-service
[SB][I][ota] task started
```

## Secure key provisioning

Provision a 32-byte HMAC key into secure data index 1 using a factory/debug provisioning command or securedata test utility.

## Firmware OTA test

Publish an MQTT command payload with `cmd=ota_firmware` and a signed manifest. Expected flow:

```text
[SB][I][supervisor] ota started type=firmware status=0
[SB][I][supervisor] ota progress=<n> type=firmware
[SB][I][supervisor] ota staged kind=0 version=<version>
```

The FOTA update flag is set only after FOTA package verification succeeds.

## Audio-pack OTA test

Publish an MQTT command payload with `cmd=ota_audio_pack` and a signed manifest. Expected flow:

```text
[SB][I][supervisor] ota started type=audio_pack status=0
[SB][I][supervisor] ota progress=<n> type=audio_pack
[SB][I][supervisor] ota staged kind=1 version=<version>
```

Verify the target file exists under `U:` and the state file `U:/sb_audio_state.json` contains the activated version.

## Negative tests

1. Bad signature: must reject with `OTA_FAILED` and `SECURITY_ERROR`.
2. Wrong SHA-256: must reject with `HASH_ERROR`.
3. Wrong size: must reject with `HASH_ERROR`.
4. Missing secure key: must reject with `SECURITY_ERROR`.
5. Network down: must reject with `NETWORK_ERROR`.
