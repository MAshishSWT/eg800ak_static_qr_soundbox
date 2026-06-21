# Phase 5 README - SIM, Network, Data Call and Time Service

## Implemented

Phase 5 adds the connectivity foundation for the EG800AK-CN Static QR UPI Soundbox:

- SIM card readiness check using `ql_sim_get_card_status()`.
- Network registration wait using `ql_network_register_wait()`.
- CSQ signal sampling using `ql_nw_get_csq()`.
- PDP data call setup using `ql_set_data_call_asyn_mode()`, `ql_set_auto_connect()` and `ql_start_data_call()`.
- Data call status callback and reconnect finite-state machine.
- Data call status polling through `ql_get_data_call_info()`.
- RTC read using `ql_rtc_get_time()`.
- NITZ enable using `ql_rtc_set_nitz_mode()`.
- NTP sync using `ql_ntp_set_server()`, `ql_ntp_set_cid()` and `ql_ntp_sync_ex()`. The SDK header declares `ql_ntp_init()`, but the linked `ql_common_api.lib` in this EG800AK SDK does not export it, and Quectel `example_ntp.c` does not call it.
- Network/time events integrated into the application event bus and supervisor.

## EG800AK SDK examples referenced

```text
interface/network/sim/example_sim.c
interface/network/data_call/example_datacall.c
interface/network/nw/example_nw.c
interface/ntp/example_ntp.c
interface/time/example_rtc.c
interface/os/example_rtos.c
```

## Quectel documents referenced

```text
RTOS Development Guide
RTOS API Mapping User Guide
(U)SIM Development Guide
Data Call Development Guide
Device Management Guide
RTC Development Guide
NTP Development/API notes
```

## EC200U source modules migrated conceptually

```text
mqtt_demo.c                 Data readiness and reconnect concept only
device_config_prcs.c        APN/config integration concept
battery_monitoring_prcs.c   Periodic health sampling style only
```

No EC200U platform-specific SIM/network/data-call APIs are copied.

## Integration

The root Makefile builds only:

```text
interface/soundbox
```

The soundbox Makefile adds:

```text
src/sb_sim_service.c
src/sb_time_service.c
src/sb_network_service.c
```

and adds the SDK lwIP include directory required by `ql_data_call.h` / `ql_ntp.h`:

```text
-I${TOP_DIR}/common/include/lwipv4v6
```

## Build

Use Quectel OpenEntry/Windows build environment. Enable the networking features needed by this phase. Do not cut NTP or data-call/network support.

## Runtime test on KAE8_SQ1

Expected startup flow with SIM inserted and network available:

```text
[SB][I][network] task started apn=<auto>
[SB][I][network] sim ready
[SB][I][supervisor] network registered
[SB][I][supervisor] csq=<value>
[SB][I][supervisor] data call ready cid=1 status=0
[SB][I][time] ntp sync requested server=pool.ntp.org
[SB][I][supervisor] time synced status=0 server=pool.ntp.org
```

## Known assumptions

- PDP profile ID is `1`.
- IPv4 is used in Phase 5.
- APN is read from config. Empty APN uses the module/operator default APN path.
- NTP server defaults to `pool.ntp.org`.
- MQTT/HTTP are not implemented in Phase 5; they will consume the data-call-ready state in later phases.
