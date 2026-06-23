# Soundbox Phase 8 File Manifest

| Path | Purpose |
|---|---|
| `interface/soundbox/include/sb_ota_crypto.h` | SHA-256/HMAC helper API |
| `interface/soundbox/include/sb_ota_manifest.h` | Signed OTA manifest API |
| `interface/soundbox/include/sb_ota_service.h` | OTA service API |
| `interface/soundbox/src/sb_ota_crypto.c` | SHA-256 and HMAC-SHA256 implementation |
| `interface/soundbox/src/sb_ota_manifest.c` | Manifest parse and HMAC validation |
| `interface/soundbox/src/sb_ota_service.c` | HTTP download, FOTA staging, audio-pack staging |
| `interface/soundbox/include/sb_app.h` | Phase 8 version |
| `interface/soundbox/include/sb_error.h` | OTA/hash error codes |
| `interface/soundbox/include/sb_event.h` | OTA events |
| `interface/soundbox/src/sb_app_main.c` | OTA service startup |
| `interface/soundbox/src/sb_command_dispatcher.c` | OTA command hook |
| `interface/soundbox/src/sb_error.c` | OTA error strings |
| `interface/soundbox/src/sb_event.c` | OTA event strings |
| `interface/soundbox/src/sb_supervisor.c` | OTA event logging |
| `interface/soundbox/Makefile` | Phase 8 build integration |
| `docs/soundbox_phase8/PHASE8_README.md` | Phase README |
| `docs/soundbox_phase8/PHASE8_TEST_PROCEDURE.md` | Test procedure |
| `docs/soundbox_phase8/PHASE8_ACCEPTANCE_CHECKLIST.md` | Acceptance checklist |
| `SOUNDBOX_PHASE8_LINKFIX.md` | FOTA library link fix note |
| `SOUNDBOX_PHASE8_REVIEW_FIXES.md` | Strict review fix summary |
