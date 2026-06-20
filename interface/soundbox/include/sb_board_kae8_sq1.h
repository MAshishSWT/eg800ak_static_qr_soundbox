/*================================================================
 * Static QR UPI Soundbox - KAE8_SQ1 Board Mapping
 * Target: Quectel EG800AK-CN QuecOpen SDK
 * Source: KAE8_SQ1_260611_Sch_01 schematic, sheets 1-3
 * Source: Quectel EG800AK Series QuecOpen GPIO Configuration V1.1
 *
 * This header records schematic-to-QuecOpen-GPIO constants for the soundbox BSP.
 * QuecOpen GPIO APIs use GPIO[x] numbers from the Quectel GPIO configuration
 * sheet. They do not use module physical pin numbers.
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

/* Active-low user keys from KAE8_SQ1 schematic.
 * Quectel GPIO Configuration V1.1 mapping:
 *   SW1 -> U1B physical pin 54, GPIO5 -> GPIO[57]
 *   SW2 -> U1B physical pin 55, GPIO6 -> GPIO[87]
 *   SW3 -> U1C physical pin 81, GPIO8 -> GPIO[8]
 */
#define SB_KAE8_KEY_VOLUME_UP_GPIO               GPIO_PIN_NO_57
#define SB_KAE8_KEY_VOLUME_DOWN_GPIO             GPIO_PIN_NO_87
#define SB_KAE8_KEY_MODE_GPIO                    GPIO_PIN_NO_8
#define SB_KAE8_KEY_ACTIVE_LEVEL                 PIN_LEVEL_LOW
#define SB_KAE8_KEY_IDLE_PULL                    PIN_PULL_PU

/* USER_LED_1 -> U1C physical pin 83, GPIO9 -> GPIO[69].
 * Q5 is a low-side transistor, so GPIO high turns LD2 on.
 */
#define SB_KAE8_STATUS_LED_GPIO                  GPIO_PIN_NO_69
#define SB_KAE8_STATUS_LED_ON_LEVEL              PIN_LEVEL_HIGH
#define SB_KAE8_STATUS_LED_OFF_LEVEL             PIN_LEVEL_LOW

/* Speaker PA shutdown/control path.
 * SPK_SHDN -> U1A physical pin 22, MAIN_CTS -> GPIO[54].
 * Signal name is SPK_SHDN; high asserts shutdown.
 */
#define SB_KAE8_SPK_SHDN_GPIO                    GPIO_PIN_NO_54
#define SB_KAE8_SPK_SHDN_ASSERT_LEVEL            PIN_LEVEL_HIGH
#define SB_KAE8_SPK_SHDN_DEASSERT_LEVEL          PIN_LEVEL_LOW

/* External W25Q64JWSIQ schematic nets on U1C.
 * These constants identify GPIO-mode equivalents only.
 * The SPI NOR service must use ql_spi_nor_* port APIs for normal NOR access.
 */
#define SB_KAE8_FLASH_SYNC_GPIO                  GPIO_PIN_NO_5  /* pin 74 KP_MKOUT[1] -> GPIO[5], SPI1_CS */
#define SB_KAE8_FLASH_CLK_GPIO                   GPIO_PIN_NO_4  /* pin 75 KP_MKIN[1]  -> GPIO[4], SPI1_CLK */
#define SB_KAE8_FLASH_DOUT_GPIO                  GPIO_PIN_NO_7  /* pin 76 KP_MKOUT[2] -> GPIO[7], SPI1_DOUT */
#define SB_KAE8_FLASH_DIN_GPIO                   GPIO_PIN_NO_6  /* pin 77 KP_MKIN[2]  -> GPIO[6], SPI1_DIN */
#define SB_KAE8_FLASH_WP_GPIO                    GPIO_PIN_NO_37 /* pin 79 USIM1_DET   -> GPIO[37] */
#define SB_KAE8_FLASH_RST_GPIO                   GPIO_PIN_NO_56 /* pin 80 GPIO7       -> GPIO[56] */

#ifdef __cplusplus
}
#endif

#endif /* SB_BOARD_KAE8_SQ1_H */
