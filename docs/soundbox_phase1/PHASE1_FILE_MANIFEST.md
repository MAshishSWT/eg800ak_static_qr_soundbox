# Phase 1 File Manifest

## New or updated build files

| Path | Purpose |
|---|---|
| `Makefile` | Production skeleton build registration; compiles `interface/soundbox` only. |
| `interface/soundbox/Makefile` | SDK-compatible module Makefile. |

## New or updated source files

| Path | Purpose |
|---|---|
| `interface/soundbox/src/sb_app_main.c` | EG800AK app entry and `application_init()` registration. |
| `interface/soundbox/src/sb_supervisor.c` | Supervisor task and heartbeat timer. |
| `interface/soundbox/src/sb_event.c` | Event helper functions and event names. |
| `interface/soundbox/src/sb_event_bus.c` | Queue-backed event bus. |
| `interface/soundbox/src/sb_log.c` | Production logging skeleton. |
| `interface/soundbox/src/sb_error.c` | Error/status string mapping. |

## New or updated public headers

| Path | Purpose |
|---|---|
| `interface/soundbox/include/sb_app.h` | App metadata and entry settings. |
| `interface/soundbox/include/sb_supervisor.h` | Supervisor task contract. |
| `interface/soundbox/include/sb_event.h` | Event IDs, source IDs, event structure. |
| `interface/soundbox/include/sb_event_bus.h` | Event bus API. |
| `interface/soundbox/include/sb_log.h` | Logging API and macros. |
| `interface/soundbox/include/sb_error.h` | Status/error API. |
| `interface/soundbox/include/sb_board_kae8_sq1.h` | KAE8_SQ1 board mapping constants. |
