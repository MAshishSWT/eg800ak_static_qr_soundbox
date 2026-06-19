# Phase 0 Review Fixes

This document records the fixes applied after the strict Phase 0 firmware review.

## Review result addressed

| Review comment | Fix applied |
|---|---|
| Soundbox folder looked like a documentation-only folder | Added a real Phase 0 build anchor module under `interface/soundbox`. |
| No `interface/soundbox/Makefile` | Added `interface/soundbox/Makefile` using the EG800AK SDK module style and `config/common/makefile.mk`. |
| Root build did not register the soundbox module | Added `interface/soundbox` to root `COMMPILE_DIRS`. |
| No stable public headers | Added `sb_phase0_contract.h` and `sb_board_kae8_sq1.h`. |
| KAE8 key/LED/SPK_SHDN GPIO mappings were not resolved | Added schematic-derived GPIO constants for SW1, SW2, SW3, USER_LED_1, and SPK_SHDN. |
| Build documentation mentioned an unverified firmware packaging command | Removed that command and documented the uploaded `build.bat` behavior. |
| Acceptance checklist had unchecked Phase 0 items | Replaced with a status table showing Phase 0 closure and Phase 1 entry gates. |

## Added buildable source

`interface/soundbox/src/sb_phase0_build_anchor.c` provides two small functions:

- `sb_phase0_contract_version()`
- `sb_phase0_contract_magic()`

The file does not call Quectel runtime APIs and does not register `application_init()`.

## Hardware mapping closure

The following KAE8_SQ1 schematic mappings are captured in `sb_board_kae8_sq1.h`:

| Net | EG800AK pin/function | Firmware symbol |
|---|---|---|
| `SW1` | U1B pin 54, GPIO5 | `SB_KAE8_KEY_VOLUME_UP_GPIO` |
| `SW2` | U1B pin 55, GPIO6 | `SB_KAE8_KEY_VOLUME_DOWN_GPIO` |
| `SW3` | U1C pin 81, GPIO8 | `SB_KAE8_KEY_MODE_GPIO` |
| `USER_LED_1` | U1C pin 83, GPIO9 | `SB_KAE8_STATUS_LED_GPIO` |
| `SPK_SHDN` | U1A pin 22, MAIN_CTS net | `SB_KAE8_SPK_SHDN_GPIO` |
| `BATT_VTG_SENS` | U1A pin 9, ADC0 | `SB_KAE8_ADC_BATTERY_CHANNEL` |

The W25Q64 physical FLASH_* nets are also captured. The `ql_spi_nor.h` port macro is selected during Phase 1 read-ID bring-up because the SDK exposes platform port constants rather than schematic net names.
