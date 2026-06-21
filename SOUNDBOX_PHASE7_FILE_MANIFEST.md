# Soundbox Phase 7 File Manifest

| Path | Purpose |
|---|---|
| `interface/soundbox/include/sb_business_service.h` | Business API |
| `interface/soundbox/include/sb_command_dispatcher.h` | Command dispatcher API |
| `interface/soundbox/include/sb_json.h` | Small JSON helper API |
| `interface/soundbox/include/sb_payment_processor.h` | Payment processor API |
| `interface/soundbox/include/sb_transaction_ledger.h` | Ledger API |
| `interface/soundbox/src/sb_business_service.c` | Business task, key actions, health payload |
| `interface/soundbox/src/sb_command_dispatcher.c` | Command parsing, validation, response |
| `interface/soundbox/src/sb_json.c` | Bounded JSON field parsing |
| `interface/soundbox/src/sb_payment_processor.c` | Payment parsing, idempotency, audio trigger |
| `interface/soundbox/src/sb_transaction_ledger.c` | Persistent Aware transaction ledger |
| `interface/soundbox/include/sb_app.h` | Phase 7 version |
| `interface/soundbox/include/sb_error.h` | Phase 7 error codes |
| `interface/soundbox/include/sb_event.h` | Phase 7 event IDs |
| `interface/soundbox/src/sb_app_main.c` | Business init integration |
| `interface/soundbox/src/sb_error.c` | Error strings |
| `interface/soundbox/src/sb_event.c` | Event strings |
| `interface/soundbox/src/sb_supervisor.c` | Business event/key handling |
| `interface/soundbox/src/sb_mqtt_service.c` | Phase 6 log cleanup retained |
| `interface/soundbox/src/sb_http_service.c` | Phase 6 HTTP disconnect cleanup retained |
| `interface/soundbox/Makefile` | Build registration |
| `docs/soundbox_phase7/PHASE7_README.md` | Phase README |
| `docs/soundbox_phase7/PHASE7_TEST_PROCEDURE.md` | Test procedure |
| `docs/soundbox_phase7/PHASE7_ACCEPTANCE_CHECKLIST.md` | Acceptance checklist |
