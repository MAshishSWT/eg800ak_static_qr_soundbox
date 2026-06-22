# Phase 10 Test Procedure

## Boot without external NOR/audio assets

1. Flash the application.
2. Boot KAE8_SQ1.
3. Confirm boot continues even if external NOR reports `NOT_FOUND`.
4. Confirm audio asset store logs U: debug fallback.
5. Confirm missing audio files report `audio fault` without rebooting.

Expected lines:

```text
[SB][I][app] starting 1.0.0-phase10-audio-assets-business-logic
[SB][I][ota] task started
[SB][I][business] task started
[SB][W][asset_store] external nor unavailable, using U: debug fallback
```

## UFS debug asset playback

1. Copy a small set of assets to `U:/audio/...`:
   - `U:/audio/common/start_tune.mp3`
   - `U:/audio/common/ping.mp3`
   - `U:/audio/en/alerts/no_transactions.mp3`
   - `U:/audio/en/audio_files/transaction_prefix.mp3`
   - `U:/audio/en/audio_files/1.mp3`
   - `U:/audio/en/audio_files/rupees.mp3`
   - `U:/audio/en/audio_files/bank.mp3`
   - `U:/audio/en/audio_files/thankyou.mp3`
2. Reboot.
3. Press volume keys and confirm `ping.mp3` is queued.
4. Press MODE with no transaction and confirm `no_transactions.mp3` is requested.

## Payment grammar

1. Provision MQTT.
2. Publish:

```json
{"tx_id":"T1001","amount_paise":12500,"provider":"paytm"}
```

3. Confirm Paytm falls back to `bank.mp3` and payment announcement completes if required files exist.
4. Publish duplicate `T1001` and confirm duplicate is ignored.

## Paise grammar

Publish:

```json
{"tx_id":"T1002","amount_paise":12575,"provider":"googlepay"}
```

Confirm `googlepay.mp3`, `and.mp3`, and `paise.mp3` are used.

## Language alias

1. Provision config language `mr`; confirm asset path uses `audio/ma/...`.
2. Provision config language `te`; confirm asset path uses `audio/tl/...`.

## LED patterns

1. Bring data call online: LED should go internet OK.
2. Keep MQTT unconfigured: LED should move to no-MQTT pattern.
3. Force low battery sample or simulate battery event: LED should move to battery-low pattern.
4. Press volume key: one short blink.
5. Press mode key: two short blinks.


## Raw audio asset pack UART/FTP update

The updated Vi_mp3 package uses a flat folder structure. Runtime paths are now `audio/<file>.mp3` for common prompts and `audio/<lang>/<file>.mp3` for language prompts. The firmware keeps the folder-path strings inside an SBAS index stored in external NOR. UART factory commands and FTP download can write the full SBAS pack directly to external NOR. See `SOUNDBOX_PHASE10_RAWPACK_UART_FTP.md`.
