# KAE8_SQ1 EG800AK-CN Hardware Mapping

This mapping is derived from `KAE8_SQ1_260611_Sch_01` schematic sheets 1-3 and is mirrored in `interface/soundbox/include/sb_board_kae8_sq1.h` for Phase 0 build integration.

## 1. EG800AK-CN core interfaces

| Function | Schematic net / component | EG800AK pin/function | Firmware mapping |
|---|---|---|---|
| Main antenna | `ANT_MAIN`, J2 antenna | U1A `ANT_MAIN` pin 35 | Connectivity bring-up |
| Main UART RX/TX | `MAIN_RXD`, `MAIN_TXD`; board nets `M_RX`, `M_TX` | U1A pins 17/18 | Factory/debug provisioning |
| Auxiliary UART RX/TX | `AUX_RXD`, `AUX_TXD`; board nets `A_RX`, `A_TX` | U1A pins 28/29 | Optional debug/factory channel |
| Debug UART RX/TX | `DBG_RXD`, `DBG_TXD`; board nets `D_RX`, `D_TX` | U1A pins 38/39 | Low-level debug |
| USB | `USB_DP`, `USB_DM`, `USB_VBUS`, J3 USB-C, J4 header | U1B pins 59/60/61 | Flashing, debug, VBUS detect if firmware support is enabled |
| Power key | `PWRKEY`, SW4 | U1A pin 7 | Power control through Quectel PWRKEY APIs |
| Reset | `RESET_N`, SW5 | U1A pin 15 | Hardware reset only |
| SIM | `USIM1_DATA`, `USIM1_CLK`, `USIM1_RST`, `USIM1_VDD`, J1 Nano SIM | U1A pins 11/13/12/14 | `sb_hal_sim`, `sb_connectivity_service` |
| Battery ADC | `BATT_VTG_SENS` | U1A pin 9, ADC0 | `SB_KAE8_ADC_BATTERY_CHANNEL = 0` |

## 2. User keys and LED

| Product control | Schematic net | EG800AK pin/function | Firmware symbol | Electrical behavior |
|---|---|---|---|---|
| Volume up / log next | `SW1` | U1B pin 54, `GPIO5` | `SB_KAE8_KEY_VOLUME_UP_GPIO = GPIO_PIN_NO_54` | Active low, 10K pullup to `VDD_EXT`, 100 nF to GND |
| Volume down / log previous | `SW2` | U1B pin 55, `GPIO6` | `SB_KAE8_KEY_VOLUME_DOWN_GPIO = GPIO_PIN_NO_55` | Active low, 10K pullup to `VDD_EXT`, 100 nF to GND |
| Mode / long action | `SW3` | U1C pin 81, `GPIO8` | `SB_KAE8_KEY_MODE_GPIO = GPIO_PIN_NO_81` | Active low, 10K pullup to `VDD_EXT`, 100 nF to GND |
| Status LED | `USER_LED_1` | U1C pin 83, `GPIO9` | `SB_KAE8_STATUS_LED_GPIO = GPIO_PIN_NO_83` | Q5 transistor drives LD2 blue LED |

Recommended GPIO setup for SW1/SW2/SW3: `ql_gpio_init(pin, PIN_DIRECTION_IN, PIN_PULL_PU, PIN_LEVEL_HIGH)` and EINT falling/both-edge registration through `ql_eint_register()` after debounce policy is implemented.

## 3. Audio hardware

| Function | Schematic net/component | EG800AK pin/function | Firmware implication |
|---|---|---|---|
| Codec | U2 `ES8311` | I2C and PCM nets | Use Quectel audio path first; direct I2C register writes only through `sb_hal_i2c` if board bring-up requires it |
| Codec control | `I2C_SCL`, `I2C_SDA` | U1B pins 68/69, `I2C1_SCL/I2C1_SDA` | `ql_i2c_init()`, `ql_i2c_write()`, `ql_i2c_read()` only behind HAL |
| Digital audio | `PCM_CLK`, `PCM_DIN`, `PCM_DOUT`, `PCM_SYNC` | U1A GPIO1-4 area / PCM nets | Quectel audio subsystem should drive the PCM/I2S path |
| Speaker PA | U7 `8002A`, speaker `SPK1` | `SPK_P`, `SPK_N`, `LS_P`, `LS_N` | Audio output path |
| PA shutdown/control | `SPK_SHDN` through Q1/R8/R9/R10/R11/R12/R13/C24 | U1A pin 22, `MAIN_CTS` net used as `SPK_SHDN` | `SB_KAE8_SPK_SHDN_GPIO = GPIO_PIN_NO_22`; connect to `ql_bind_speakerpa_cb()` policy and GPIO output after pin function selection |

Recommended audio test sequence:

1. Configure speaker path with `ql_set_audio_path_speaker()`.
2. Register PA callback with `ql_bind_speakerpa_cb()`.
3. Set safe volume using `ql_set_volume()`.
4. Play a short MP3 using `ql_mp3_file_play()` or `ql_play_mp3()`.
5. Confirm `SPK_SHDN` toggles and output audio is clear.

## 4. External SPI NOR / audio assets

| Flash signal | EG800AK pin/function | Firmware symbol |
|---|---|---|
| `FLASH_SYNC` | U1C pin 74, `KP_MKOUT[1]` | `SB_KAE8_FLASH_SYNC_GPIO = GPIO_PIN_NO_74` |
| `FLASH_CLK` | U1C pin 75, `KP_MKIN[1]` | `SB_KAE8_FLASH_CLK_GPIO = GPIO_PIN_NO_75` |
| `FLASH_DOUT` | U1C pin 76, `KP_MKOUT[2]` | `SB_KAE8_FLASH_DOUT_GPIO = GPIO_PIN_NO_76` |
| `FLASH_DIN` | U1C pin 77, `KP_MKIN[2]` | `SB_KAE8_FLASH_DIN_GPIO = GPIO_PIN_NO_77` |
| `FLASH_WP` | U1C pin 79, `USIM_DET` | `SB_KAE8_FLASH_WP_GPIO = GPIO_PIN_NO_79` |
| `FLASH_RST` | U1C pin 80, `GPIO7` | `SB_KAE8_FLASH_RST_GPIO = GPIO_PIN_NO_80` |

External flash device: U8 `W25Q64JWSIQ`, 64 Mbit SPI NOR. The EG800AK SDK exposes `ql_spi_nor_init()`, `ql_spi_nor_read_id()`, `ql_spi_nor_read()`, `ql_spi_nor_write()`, `ql_spi_nor_erase_sector()`, and `ql_spi_nor_erase_chip()` in `ql_spi_nor.h`. The Phase 1 hardware bring-up must select the port macro by read-ID validation because the SDK port constants are platform groups, while the schematic names the physical FLASH_* nets.

## 5. Battery and charging

| Function | Schematic net/component | Firmware implication |
|---|---|---|
| Charger | U4 `TP4056`, USB-C VBUS through D1 `1N5817` to `V_CHRG` | Charging is hardware-managed |
| Protection | U6 `DW01A`, Q3 `FS8205` | Hardware battery protection |
| Battery sense | `BATT_VTG_SENS` | EG800AK ADC0, `SB_KAE8_ADC_BATTERY_CHANNEL = 0` |
| Divider | R24 120K from B+, R27 47K to B- | ADC voltage = VBAT × 47/(120+47) |
| Charge LED | LD1 red | Hardware charge indication |

Battery conversion basis:

```text
adc_mv = ql_adc_read(0)
vbat_mv = adc_mv * (120 + 47) / 47
```

The production battery service should apply a Li-ion percentage curve and debounce low-battery announcements.

## 6. Power rails

| Rail | Use |
|---|---|
| `VBAT` | EG800AK module power, PA/battery domain |
| `VBAT_MCU` | Module VBAT input filtering |
| `VDD_EXT` | Pullups, SIM/flash/audio control domain |
| `VDD3V3` | Generated by U5 SGM2019ADJ |
| `VDD1V8` | Codec/audio support |
| `AGND` | Audio ground domain |

## 7. Phase 1 hardware validation checklist

- Confirm `GPIO_PIN_NO_54`, `GPIO_PIN_NO_55`, `GPIO_PIN_NO_81`, `GPIO_PIN_NO_83`, and `GPIO_PIN_NO_22` can be configured through `ql_pin_set_func()` and `ql_gpio_init()` on the EG800AK-CN firmware build.
- Confirm `SPK_SHDN` enable polarity with the 8002A PA on the assembled board.
- Confirm W25Q64 read-ID through the selected `ql_spi_nor.h` port constant.
- Confirm ES8311 playback path through Quectel audio APIs before adding direct I2C register writes.
- Confirm ADC0 voltage remains within EG800AK ADC input range across full battery voltage.
