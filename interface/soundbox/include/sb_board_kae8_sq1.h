/*================================================================
 * Static QR UPI Soundbox - KAE8_SQ1 Board Mapping
 * Target: Quectel EG800AK-CN QuecOpen SDK
 * Source: KAE8_SQ1_260611_Sch_01 schematic, sheets 1-3
 *
 * This header records schematic-to-firmware constants for Phase 0.
 * It contains no runtime side effects.
 *================================================================*/
#ifndef SB_BOARD_KAE8_SQ1_H
#define SB_BOARD_KAE8_SQ1_H

#include "ql_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ADC */
#define SB_KAE8_ADC_BATTERY_CHANNEL              (0u) /* BATT_VTG_SENS -> EG800AK ADC0, U1A pin 9 */
#define SB_KAE8_BATTERY_R_HIGH_OHM               (120000u)
#define SB_KAE8_BATTERY_R_LOW_OHM                (47000u)

/* Active-low user keys from sheet 3, routed on sheet 1. */
#define SB_KAE8_KEY_VOLUME_UP_GPIO               GPIO_PIN_NO_54 /* SW1 -> U1B GPIO5, pin 54 */
#define SB_KAE8_KEY_VOLUME_DOWN_GPIO             GPIO_PIN_NO_55 /* SW2 -> U1B GPIO6, pin 55 */
#define SB_KAE8_KEY_MODE_GPIO                    GPIO_PIN_NO_81 /* SW3 -> U1C GPIO8, pin 81 */
#define SB_KAE8_KEY_ACTIVE_LEVEL                 PIN_LEVEL_LOW
#define SB_KAE8_KEY_IDLE_PULL                    PIN_PULL_PU

/* User LED. Q5 transistor polarity must be validated on first board test. */
#define SB_KAE8_STATUS_LED_GPIO                  GPIO_PIN_NO_83 /* USER_LED_1 -> U1C GPIO9, pin 83 */

/* Speaker PA shutdown/control path. Net SPK_SHDN routes through Q1 to U7 8002A. */
#define SB_KAE8_SPK_SHDN_GPIO                    GPIO_PIN_NO_22 /* SPK_SHDN -> U1A MAIN_CTS, pin 22 */

/* External W25Q64JWSIQ schematic nets on U1C. Port macro is selected during SPI NOR read-ID bring-up. */
#define SB_KAE8_FLASH_SYNC_GPIO                  GPIO_PIN_NO_74 /* FLASH_SYNC -> U1C KP_MKOUT[1] */
#define SB_KAE8_FLASH_CLK_GPIO                   GPIO_PIN_NO_75 /* FLASH_CLK  -> U1C KP_MKIN[1] */
#define SB_KAE8_FLASH_DOUT_GPIO                  GPIO_PIN_NO_76 /* FLASH_DOUT -> U1C KP_MKOUT[2] */
#define SB_KAE8_FLASH_DIN_GPIO                   GPIO_PIN_NO_77 /* FLASH_DIN  -> U1C KP_MKIN[2] */
#define SB_KAE8_FLASH_WP_GPIO                    GPIO_PIN_NO_79 /* FLASH_WP   -> U1C USIM_DET */
#define SB_KAE8_FLASH_RST_GPIO                   GPIO_PIN_NO_80 /* FLASH_RST  -> U1C GPIO7 */

#ifdef __cplusplus
}
#endif

#endif /* SB_BOARD_KAE8_SQ1_H */
