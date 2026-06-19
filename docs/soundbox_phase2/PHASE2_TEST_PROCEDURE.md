# Phase 2 Test Procedure

## 1. Static source inspection

Run from the package root:

```sh
grep -R "application_init" interface/soundbox interface/*/*.c | cat
grep -R "SOURCE_DEFER_MARKER" interface/soundbox docs/soundbox_phase2 | cat
```

Expected result:

- Exactly one active `application_init()` in `interface/soundbox/src/sb_app_main.c`.
- No deferred-code markers in Phase 2 source and Phase 2 documents.

## 2. Build registration inspection

Run:

```sh
grep -n "COMMPILE_DIRS" Makefile
sed -n '1,120p' interface/soundbox/Makefile
```

Expected result:

- Root build uses `COMMPILE_DIRS := interface/soundbox`.
- Soundbox Makefile lists all Phase 1 and Phase 2 source files.

## 3. Vendor build

Use the EG800AK QuecOpen build environment:

```bat
build.bat clean
build.bat app
```

Expected result:

- No duplicate `application_init()` symbols.
- No missing header or unresolved source-file errors from the soundbox module.
- No build output generated from Quectel example applications.

## 4. Boot log test

Flash the application and open the debug log port.

Expected logs:

```text
[SB][I][app] starting 1.0.0-phase2-bsp-hal
[SB][I][bsp] led ok
[SB][I][bsp] speaker_pa ok
[SB][I][bsp] adc ok
[SB][I][bsp] keys ok
[SB][I][supervisor] task started
[SB][I][supervisor] boot event received
[SB][I][supervisor] board ready
[SB][I][supervisor] battery=<measured>mV percent=<0..100>
```

## 5. LED test

Expected result:

- LD2 turns on after BSP initialization.
- LD2 toggles on supervisor heartbeat.

## 6. Key test

Press and release each physical key.

Expected log behavior:

| Key | Expected event parameter |
|---|---|
| SW1 / Volume up | `key=1` |
| SW2 / Volume down | `key=2` |
| SW3 / Mode | `key=3` |

`pressed=1` is expected while the active-low key is held. `pressed=0` is expected on release.

## 7. Battery ADC test

Measure the battery voltage with a calibrated meter. Compare the logged battery millivolts with:

```text
battery_mv = adc_mv * 167000 / 47000
```

Expected result:

- Logged value follows the divider calculation.
- Percent saturates at 0% below 3300 mV and 100% above 4200 mV.

## 8. Speaker PA safe-state test

Expected result:

- On boot, `SPK_SHDN` is asserted so the speaker amplifier starts disabled.
- Enabling through `sb_bsp_board_set_speaker_amp(1)` deasserts `SPK_SHDN`.
