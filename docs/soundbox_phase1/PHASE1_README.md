# Phase 1 README - EG800AK-CN Static QR UPI Soundbox Skeleton

## Phase implemented

Phase 1 creates the EG800AK QuecOpen application skeleton inside the uploaded `ql_application_eg800ak.zip` layout.

Implemented in this phase:

- `interface/soundbox` production module folder.
- EG800AK SDK-compatible `interface/soundbox/Makefile`.
- Root `Makefile` production build registration for `interface/soundbox` only.
- One soundbox application entry registered through `application_init()`.
- Application supervisor task using `ql_rtos_task_create()`.
- Event definitions and event bus using `ql_rtos_queue_create()`, `ql_rtos_queue_release()`, and `ql_rtos_queue_wait()`.
- Supervisor heartbeat timer using `ql_rtos_timer_create()` and `ql_rtos_timer_start()`.
- Production logging skeleton using bounded module names and no credential logging.
- Error/status code contract.
- KAE8_SQ1 board mapping public header retained from Phase 0 review closure.

## Implementation plan

The Phase 1 code establishes the boot and message-passing foundation for all later services. The `application_init()` entry creates the event bus, starts the supervisor task, posts the boot event, then deletes the entry task using the same pattern demonstrated by Quectel examples. The supervisor receives typed events through a queue and owns the heartbeat timer. Service modules added in following phases will post events to this bus instead of sharing mutable global flags.

## EG800AK SDK examples and headers referenced

Primary implementation reference was the uploaded EG800AK SDK source tree.

| SDK area | Files referenced |
|---|---|
| App registration | `common/include/ql_application.h`, `interface/app/example_app.c` |
| RTOS task and delete flow | `common/include/ql_rtos.h`, `interface/os/example_rtos.c`, `interface/app/example_app.c` |
| Queue style | `interface/os/example_rtos.c` |
| Timer style | `common/include/ql_rtos.h`, `interface/time/example_timer.c`, `interface/platform/example_wtd.c` |
| GPIO mapping basis | `common/include/ql_gpio.h`, `interface/driver/example_gpio.c`, `interface/driver/example_eint.c` |
| Build structure | root `Makefile`, `interface/*/Makefile`, `config/common/makefile.mk` |

## Quectel documents used

- Quick Start Guide
- RTOS Development Guide
- RTOS API Mapping User Guide
- GPIO Development Guide
- Serial Port Development Guide

Other Quectel documents from the uploaded reference set remain mapped in Phase 0 and will be used when the corresponding feature modules are generated.

## EC200U modules migrated

Phase 1 does not copy EC200U code. It migrates the structural lessons needed for a clean EG800AK skeleton:

| EC200U module | Migration result in Phase 1 |
|---|---|
| `common_prj_def.c/.h` | Replaced with typed status/error/event contracts; no global application database is introduced. |
| `systick.c/.h` | Replaced by Quectel `ql_rtos_get_systicks()` timestamps and RTOS timer heartbeat. |
| `keypad_prcs.c` | Key event IDs are defined; GPIO key service is assigned to the GPIO BSP phase. |
| `mqtt_demo.c` | Connectivity/payment event IDs are defined; MQTT service is assigned to the connectivity phase. |
| `audio_prcs.c` | Audio state event ID is defined; audio service is assigned to the audio phase. |

## Integration inside `ql_application_eg800ak.zip`

Copy the package contents over an extracted `ql_application_eg800ak.zip` tree or build directly from `phase1_pkg`.

The root `Makefile` is configured for the Phase 1 production skeleton:

```make
COMMPILE_DIRS:= \
  interface/soundbox \
```

This avoids compiling example modules that contain active `application_init()` entries. The soundbox module itself has exactly one application entry in `src/sb_app_main.c`:

```c
application_init(sb_app_entry, SB_APP_NAME, SB_APP_ENTRY_STACK_KIB, SB_APP_ENTRY_STARTUP_PRIORITY);
```

## How to build

Run in the Quectel-supported Windows build environment:

```bat
cd <extracted_phase1_pkg_root>
build.bat app
```

The uploaded `build.bat` treats `clean` specially and otherwise invokes the default Makefile build.

## How to test on KAE8_SQ1 EG800AK-CN hardware

1. Build and flash the generated application image using the standard EG800AK QuecOpen process.
2. Open the configured debug output channel.
3. Confirm the application logs:
   - `starting 1.0.0-phase1-skeleton`
   - `task started`
   - `boot event received`
4. Confirm heartbeat logs appear at the configured heartbeat period when log level is set to debug.
5. Confirm the device does not start any Quectel example app such as `quec_gpio_test`.
6. Confirm there is no SIM, network, MQTT, audio, OTA, SMS, or filesystem action in Phase 1 runtime.

## Known assumptions

- `printf()` and `vprintf()` are available in the EG800AK application runtime as demonstrated by SDK examples.
- `application_init()` stack size is in KiB as defined in `ql_application.h`.
- `ql_rtos_task_create()` stack size is in bytes as defined in `ql_rtos.h`.
- `GPIO_PIN_NO_54`, `GPIO_PIN_NO_55`, `GPIO_PIN_NO_81`, `GPIO_PIN_NO_83`, and `GPIO_PIN_NO_22` are schematic-derived firmware symbols for KAE8_SQ1 and will be exercised by the BSP phase.
