# Phase 1 Architecture - EG800AK-CN Static QR UPI Soundbox

## Layer placement in Phase 1

| Required layer | Phase 1 artifact |
|---|---|
| Application supervisor | `src/sb_app_main.c`, `src/sb_supervisor.c`, `include/sb_supervisor.h` |
| Domain/business services | Event IDs are defined in `include/sb_event.h`; service source files are assigned to later feature phases. |
| Connectivity services | Connectivity event IDs are defined in `include/sb_event.h`; service source files are assigned to the connectivity phase. |
| Platform services | `src/sb_log.c`, `src/sb_error.c` |
| OS abstraction/event bus | `src/sb_event.c`, `src/sb_event_bus.c`, `include/sb_event_bus.h` |
| BSP/HAL | `include/sb_board_kae8_sq1.h` contains board constants; runtime BSP source files are assigned to the BSP phase. |

## Runtime sequence

1. EG800AK application framework invokes `sb_app_entry()` through `application_init()`.
2. `sb_app_entry()` initializes logging.
3. `sb_app_entry()` creates the event bus queue.
4. `sb_app_entry()` starts the supervisor task.
5. `sb_app_entry()` posts `SB_EVENT_SYSTEM_BOOT`.
6. `sb_app_entry()` deletes itself through `ql_rtos_task_delete(NULL)`.
7. `sb_supervisor_task()` owns the main event loop.
8. Supervisor heartbeat timer posts `SB_EVENT_SUPERVISOR_HEARTBEAT`.

## Event bus policy

- Events are fixed-size `sb_event_t` values.
- Event text is bounded to `SB_EVENT_TEXT_LEN`.
- Event post/wait operations use Quectel RTOS queue APIs.
- ISRs and timer callbacks must use `QL_NO_WAIT` event posting.
- Cross-module communication must use `sb_event_post()` rather than writable global flags.

## Build policy

The root `Makefile` compiles only `interface/soundbox` in Phase 1. This prevents active Quectel example applications from registering startup tasks in the soundbox image.
