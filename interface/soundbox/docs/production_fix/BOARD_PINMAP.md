# KAE8_SQ1_260611 Board Pin Map

| Schematic net | EG800AK pin | GPIO / function | Active level | Firmware macro | Validation method |
|---|---:|---|---|---|---|
| SW1 | 57 | GPIO[57] | Active low, pull-up | `SB_KAE8_KEY_VOLUME_UP_PIN` | Press key, expect `KEY_EDGE` then volume-up action |
| SW2 | 87 | GPIO[87] | Active low, pull-up | `SB_KAE8_KEY_VOLUME_DOWN_PIN` | Press key, expect `KEY_EDGE` then volume-down action |
| SW3 | 82 | GPIO[8] | Active low, pull-up | `SB_KAE8_KEY_MODE_PIN` | Press key, expect mode/action event |
| USER_LED_1 | 83 | GPIO[69] | Logical high at firmware LED level | `SB_KAE8_LED_USER_PIN` | Run `led_test`, observe single LED pattern |
| SPK_SHDN | 22 | GPIO[54] via MAIN_CTS alternate GPIO | Verify board polarity | `SB_KAE8_SPK_SHDN_GPIO` | Run `play_common`, confirm audio only when PA enabled |
| BATT_VTG_SENS | ADC0 | ADC0 | Analog divider 120K/47K | `SB_KAE8_BATT_ADC_CHANNEL` | Compare ADC derived voltage with bench meter |
| I2C_SCL / I2C_SDA | 67 / 66 | CI2C SCL/SDA | I2C pull-up | `SB_KAE8_ES8311_I2C_*` | ES8311 register read/write pass |
| PCM_CLK / PCM_SYNC / PCM_DIN / PCM_DOUT | PCM pins | Digital audio | PCM/I2S timing | PCM net macros | MP3 playback without underrun |
| FLASH_SYNC | KP_MKOUT[1] | SPI1 CS | Active low | `SB_KAE8_NOR_PORT` | NOR ID test |
| FLASH_CLK | KP_MKIN[1] | SPI1 CLK | Clock | `SB_KAE8_NOR_PORT` | NOR ID test |
| FLASH_DOUT / FLASH_DIN | KP_MKOUT[2] / KP_MKIN[2] | SPI data | DI/DO interchanged in board quirk | `SB_NOR_DI_DO_INTERCHANGED` | JEDEC-ID and CRC read/write test |
| FLASH_WP / FLASH_RST | GPIO / hardware control | Pull high for operation | High | Board macros | NOR status register read |

Only one user-visible LED exists on this board. Status colors from early product notes are mapped to single-LED timing patterns.
