# Static QR UPI Soundbox - Phase 2 BSP/HAL

## Implemented scope

Phase 2 adds the KAE8_SQ1 board support package and hardware abstraction layer to the approved Phase 1 EG800AK-CN QuecOpen application skeleton.

Implemented runtime items:

- GPIO wrapper over EG800AK `ql_gpio_*` and `ql_eint_*` APIs.
- Status LED HAL for `USER_LED_1`.
- Key HAL for SW1, SW2, and SW3 using both-edge EINT callbacks.
- Battery ADC HAL for `BATT_VTG_SENS` on ADC0 with KAE8 resistor-divider conversion.
- Speaker amplifier shutdown HAL for `SPK_SHDN`.
- KAE8_SQ1 board initialization sequence.
- Startup board-ready and battery-sample events posted through the Phase 1 event bus.
- Supervisor handling for board, key-edge, and battery events.
- Heartbeat LED toggle for board bring-up visibility.

## EG800AK SDK headers and examples referenced

The BSP/HAL code was aligned to APIs and call style from the uploaded `ql_application_eg800ak.zip` package:

- `common/include/ql_gpio.h`
- `common/include/ql_adc.h`
- `common/include/ql_rtos.h`
- `common/include/ql_application.h`
- `interface/driver/example_gpio.c`
- `interface/driver/example_eint.c`
- `interface/driver/example_adc.c`
- `interface/os/example_rtos.c`
- `config/common/makefile.mk`
- Existing interface module Makefiles

## Quectel documents used

The Phase 2 implementation is based on the EG800AK SDK package and these document categories:

- Quick Start Guide
- RTOS Development Guide
- RTOS API Mapping User Guide
- GPIO Development Guide
- ADC Development Guide
- Audio Development Guide, for speaker-amplifier control boundary only
- RTOS Flash API and SPI NOR documents, for pin reservation boundary only

## EC200U source logic migrated

Phase 2 migrates only reusable hardware behavior concepts from the EC200U source package:

- `keypad_prcs.c`: physical key identity and edge-reporting concept.
- `battery_monitoring_prcs.c`: battery sampling and percent-reporting concept.
- `audio_prcs.c`: speaker power-control boundary concept.
- `common_prj_def.c/.h`: product hardware state concepts, replaced by typed BSP/HAL contracts.

No EC200U platform API, MQTT API, NVM API, or monolithic global state was copied.

## KAE8_SQ1 hardware mapping

| Function | Firmware symbol | EG800AK mapping |
|---|---|---|
| Volume up key | `SB_KAE8_KEY_VOLUME_UP_GPIO` | SW1 -> U1B pin 54 GPIO5 -> GPIO[57] -> `GPIO_PIN_NO_57` |
| Volume down key | `SB_KAE8_KEY_VOLUME_DOWN_GPIO` | SW2 -> U1B pin 55 GPIO6 -> GPIO[87] -> `GPIO_PIN_NO_87` |
| Mode key | `SB_KAE8_KEY_MODE_GPIO` | SW3 -> U1C pin 81 GPIO8 -> GPIO[8] -> `GPIO_PIN_NO_8` |
| Status LED | `SB_KAE8_STATUS_LED_GPIO` | USER_LED_1 -> U1C pin 83 GPIO9 -> GPIO[69] -> `GPIO_PIN_NO_69` |
| Speaker PA shutdown | `SB_KAE8_SPK_SHDN_GPIO` | SPK_SHDN -> U1A pin 22 MAIN_CTS -> GPIO[54] -> `GPIO_PIN_NO_54` |
| Battery ADC | `SB_KAE8_ADC_BATTERY_CHANNEL` | BATT_VTG_SENS -> ADC0 |

Battery conversion uses the schematic divider `R_HIGH = 120 kOhm`, `R_LOW = 47 kOhm`:

```text
battery_mv = adc_mv * (120000 + 47000) / 47000
```

The status LED is driven through a low-side transistor and is configured active-high. The speaker-amplifier shutdown net defaults to the safe disabled state during board initialization.

## Integration

Copy this package over the EG800AK SDK application package. The root `Makefile` already compiles only:

```text
COMMPILE_DIRS := interface/soundbox
```

The soundbox module Makefile includes all Phase 1 and Phase 2 source files and keeps all Quectel common headers/libraries unmodified.

## Build

Use the normal EG800AK QuecOpen application build environment.

```bat
build.bat clean
build.bat app
```

The uploaded build script treats `clean` specially and otherwise invokes the default SDK Makefile flow. Final binary packaging must follow the vendor release procedure supplied with the EG800AK SDK toolchain.

## KAE8_SQ1 hardware test

1. Flash the generated application using the Quectel-supported EG800AK application download flow.
2. Open the configured debug UART/USB log port.
3. Power the KAE8_SQ1 board.
4. Confirm logs show `starting 1.0.0-phase2-bsp-hal`, BSP init success, supervisor start, board-ready event, and one battery sample.
5. Confirm the status LED turns on after BSP init and toggles on heartbeat.
6. Press and release SW1, SW2, and SW3.
7. Confirm logs report `key=1`, `key=2`, and `key=3` with pressed/released state.
8. Measure battery voltage and compare the logged ADC-derived battery millivolts to the expected divider result.
9. Confirm the speaker PA remains disabled at boot and can be controlled by calling `sb_bsp_board_set_speaker_amp()` from the audio service phase.

## Known assumptions

- SW1/SW2/SW3 are active-low with pull-up, matching the KAE8_SQ1 schematic key circuit.
- `USER_LED_1` is active-high because the GPIO drives a low-side transistor for LD2.
- `SPK_SHDN` is treated as active-high shutdown, so boot defaults to speaker disabled.
- Phase 2 reports raw key edges. Long-press, short-press, volume, mode, and daily-summary interpretation belongs to the key action/domain phase.
- External SPI NOR nets are recorded in the board header; SPI NOR initialization and read-ID validation are assigned to the filesystem/audio asset phase.
