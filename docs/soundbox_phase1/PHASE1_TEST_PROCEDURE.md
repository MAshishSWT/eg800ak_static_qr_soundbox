# Phase 1 Test Procedure

## 1. Static package checks

1. Confirm `interface/soundbox/Makefile` exists.
2. Confirm root `Makefile` compiles only `interface/soundbox`.
3. Confirm `src/sb_app_main.c` contains exactly one active `application_init()`.
4. Confirm Quectel `common/include` and `common/lib` are unmodified.
5. Confirm no EC200U-only headers are included.
6. Confirm there are no hardcoded credentials, certificates, tokens, passwords, or private keys.

## 2. Source compile checks

Compile using the Quectel-supported environment:

```bat
build.bat app
```

Expected result:

- `sb_app_main.c`, `sb_supervisor.c`, `sb_event.c`, `sb_event_bus.c`, `sb_log.c`, and `sb_error.c` compile.
- Only the soundbox app entry is registered.
- No Quectel example application is linked as a startup app.

## 3. Runtime boot checks

1. Flash the image to KAE8_SQ1 EG800AK-CN hardware.
2. Power cycle the board.
3. Observe boot logs.
4. Confirm the app entry initializes logging and event bus.
5. Confirm supervisor task starts.
6. Confirm boot event is processed.
7. Set log level to debug in `sb_app_main.c` only for engineering validation and confirm heartbeat event processing.

## 4. Negative checks

- Remove `interface/soundbox` from root `COMMPILE_DIRS` and confirm the soundbox app entry is absent from build output.
- Restore `interface/soundbox` and confirm build returns to normal.
- Confirm no module attempts SIM, network, MQTT, HTTP, audio, FOTA, SMS, flash, or ADC operations in Phase 1.

## 5. Phase 2 readiness checks

- Event bus queue depth and event structure size are stable.
- Supervisor task stack size and priority are defined in `sb_supervisor.h`.
- Board mapping constants are available in `sb_board_kae8_sq1.h`.
- Logging levels are defined in `sb_log.h`.
- Error codes are defined in `sb_error.h`.
