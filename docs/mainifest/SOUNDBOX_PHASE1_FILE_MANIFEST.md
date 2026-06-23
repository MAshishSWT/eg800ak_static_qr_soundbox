# EG800AK Static QR UPI Soundbox - Phase 1 File Manifest

Phase 1 creates the ready-to-build EG800AK-CN QuecOpen soundbox application skeleton.

The package preserves Quectel SDK `common/include` headers and `common/lib` libraries. The only SDK build file modified is the root `Makefile`, to compile the production soundbox module instead of example modules.

## Modified existing file

| Path | Change |
|---|---|
| `Makefile` | `COMMPILE_DIRS` is set to `interface/soundbox` for a single soundbox startup app. |

## Soundbox module files

| Path | Purpose |
|---|---|
| `interface/soundbox/Makefile` | Module build integration. |
| `interface/soundbox/README.md` | Phase 1 module summary. |
| `interface/soundbox/include/sb_app.h` | App metadata and entry stack/priority settings. |
| `interface/soundbox/include/sb_board_kae8_sq1.h` | KAE8_SQ1 schematic-derived board mapping. |
| `interface/soundbox/include/sb_error.h` | Error/status contract. |
| `interface/soundbox/include/sb_event.h` | Event definitions. |
| `interface/soundbox/include/sb_event_bus.h` | Event bus API. |
| `interface/soundbox/include/sb_log.h` | Logging API. |
| `interface/soundbox/include/sb_supervisor.h` | Supervisor API. |
| `interface/soundbox/src/sb_app_main.c` | `application_init()` entry and boot sequence. |
| `interface/soundbox/src/sb_error.c` | Status-to-string implementation. |
| `interface/soundbox/src/sb_event.c` | Event helpers. |
| `interface/soundbox/src/sb_event_bus.c` | RTOS queue-backed event bus. |
| `interface/soundbox/src/sb_log.c` | Logging implementation. |
| `interface/soundbox/src/sb_supervisor.c` | Supervisor task and heartbeat timer. |

## Phase documents

| Path | Purpose |
|---|---|
| `docs/soundbox_phase1/PHASE1_README.md` | Implementation summary and build/test instructions. |
| `docs/soundbox_phase1/PHASE1_TEST_PROCEDURE.md` | Phase 1 validation procedure. |
| `docs/soundbox_phase1/PHASE1_ARCHITECTURE.md` | Layer placement, runtime sequence, and event bus policy. |
| `docs/soundbox_phase1/PHASE1_ACCEPTANCE_CHECKLIST.md` | Phase 1 acceptance checklist. |
| `docs/soundbox_phase1/PHASE1_FILE_MANIFEST.md` | Detailed Phase 1 manifest. |
