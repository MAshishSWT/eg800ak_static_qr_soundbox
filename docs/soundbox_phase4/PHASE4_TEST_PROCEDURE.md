# Phase 4 Test Procedure

## Build test

1. Run the Quectel SDK build.
2. Confirm `interface/soundbox` builds without compiling Quectel examples.
3. Confirm the final app image is generated.

## Boot test

Expected logs:

```text
[SB][I][app] starting 1.0.0-phase4-audio-service
[SB][I][bsp] led ok
[SB][I][bsp] speaker_pa ok
[SB][I][bsp] adc ok
[SB][I][bsp] keys ok
[SB][I][audio] ready lang=en volume=70
[SB][I][supervisor] audio ready status=0
```

If the external codec is not detected, the HAL must post an audio fault and must not report audio ready on KAE8 production hardware.

## Asset validation test

1. Do not copy audio files.
2. Trigger a prompt playback request.
3. Confirm the service logs the missing asset path and posts an audio fault event.

Expected example:

```text
[SB][W][audio] missing asset=U:/audio/en/prompt_ready.mp3
[SB][W][supervisor] audio fault status=-14 text=U:/audio/en/prompt_ready.mp3
```

## MP3 playback test

1. Copy a known-good MP3 file to `U:/audio/en/prompt_ready.mp3`.
2. Trigger `sb_audio_service_play_prompt(SB_AUDIO_LANG_EN, SB_AUDIO_PROMPT_READY)`.
3. Confirm playback starts and completes.

## Amount script test

Required minimum English test assets:

```text
U:/audio/en/prompt_payment_received.mp3
U:/audio/en/provider_paytm.mp3
U:/audio/en/num_1.mp3 ... relevant number files
U:/audio/en/scale_hundred.mp3
U:/audio/en/scale_thousand.mp3
U:/audio/en/scale_lakh.mp3
U:/audio/en/currency_rupees.mp3
U:/audio/en/currency_paise.mp3
U:/audio/en/currency_only.mp3
```

Trigger examples:

```text
1.00 rupees  -> num_1 + currency_rupees + currency_only
123.45       -> num_1 + scale_hundred + num_20 + num_3 + currency_rupees + num_40 + num_5 + currency_paise + currency_only
125000.00    -> num_1 + scale_lakh + num_20 + num_5 + scale_thousand + currency_rupees + currency_only
```
