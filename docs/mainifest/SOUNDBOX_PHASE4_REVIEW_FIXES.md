# Phase 4 Review Fixes

Review fixes applied:

- Changed ES8311 I2C bus from `1u` to `0u` for KAE8 CI2C pins 66/67.
- Removed silent internal codec fallback for KAE8 production audio path.
- Audio HAL now reports an audio fault when ES8311 is not detected.
- Cleaned audio ready log to avoid printing `ql_get_volume()` return value as readiness level.

Updated files:

```text
interface/soundbox/src/sb_audio_codec_es8311.c
interface/soundbox/src/sb_audio_hal.c
docs/soundbox_phase4/PHASE4_README.md
docs/soundbox_phase4/PHASE4_TEST_PROCEDURE.md
docs/soundbox_phase4/PHASE4_ACCEPTANCE_CHECKLIST.md
```
