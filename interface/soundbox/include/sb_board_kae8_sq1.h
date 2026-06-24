/*================================================================
 * Static QR UPI Soundbox - KAE8_SQ1_260611 Board Mapping
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *
 * Board facts are taken from KAE8_SQ1_260611_Sch_01 and the
 * Quectel EG800AK GPIO configuration sheet. GPIO[x] values below
 * are QuecOpen GPIO numbers, not the EG800AK package pin numbers.
 *================================================================*/
#ifndef SB_BOARD_KAE8_SQ1_H
#define SB_BOARD_KAE8_SQ1_H

#include "ql_gpio.h"
#include "ql_spi_nor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_BOARD_NAME                           "KAE8_SQ1_260611"
#define SB_BOARD_TARGET                         "EG800AK-CN"

/* Battery divider: BATT_VTG_SENS -> EG800AK ADC0, 120K high / 47K low. */
#define SB_KAE8_ADC_BATTERY_CHANNEL             (0u)
#define SB_KAE8_BATTERY_R_HIGH_OHM              (120000u)
#define SB_KAE8_BATTERY_R_LOW_OHM               (47000u)

/* Active-low keys from sheet 3. */
#define SB_KAE8_KEY_VOLUME_UP_GPIO              GPIO_PIN_NO_57 /* SW1, pin54 GPIO5 -> GPIO[57] */
#define SB_KAE8_KEY_VOLUME_DOWN_GPIO            GPIO_PIN_NO_87 /* SW2, pin55 GPIO6 -> GPIO[87] */
#define SB_KAE8_KEY_MODE_GPIO                   GPIO_PIN_NO_8  /* SW3, pin81 GPIO8 -> GPIO[8]  */
#define SB_KAE8_KEY_ACTIVE_LEVEL                PIN_LEVEL_LOW
#define SB_KAE8_KEY_IDLE_PULL                   PIN_PULL_PU
#define SB_KAE8_KEY_DEBOUNCE_MS                 (40u)
#define SB_KAE8_KEY_STUCK_BOOT_MS               (1500u)

/* One status LED: USER_LED_1 -> pin83 GPIO9 -> GPIO[69], Q5 low-side driver. */
#define SB_KAE8_STATUS_LED_GPIO                 GPIO_PIN_NO_69
#define SB_KAE8_STATUS_LED_ON_LEVEL             PIN_LEVEL_HIGH
#define SB_KAE8_STATUS_LED_OFF_LEVEL            PIN_LEVEL_LOW

/* Speaker amplifier shutdown/control: SPK_SHDN -> pin22 MAIN_CTS -> GPIO[54].
 * The schematic signal name is shutdown; the production runtime validates the
 * actual polarity by audio diagnostics. Default: high = shutdown asserted.
 */
#define SB_KAE8_SPK_SHDN_GPIO                   GPIO_PIN_NO_54
#define SB_KAE8_SPK_SHDN_ASSERT_LEVEL           PIN_LEVEL_HIGH
#define SB_KAE8_SPK_SHDN_DEASSERT_LEVEL         PIN_LEVEL_LOW

/* ES8311 codec. GPIO sheet states external audio design uses CI2C on pins 66/67. */
#define SB_KAE8_ES8311_I2C_NAME                 "CI2C"
#define SB_KAE8_ES8311_I2C_SDA_PIN              (66u)
#define SB_KAE8_ES8311_I2C_SCL_PIN              (67u)
#define SB_KAE8_ES8311_I2C_SDA_GPIO             GPIO_PIN_NO_50
#define SB_KAE8_ES8311_I2C_SCL_GPIO             GPIO_PIN_NO_49
#define SB_KAE8_ES8311_ADDR_7BIT                (0x18u)

/* PCM/I2S nets from sheet 1/2. */
#define SB_KAE8_PCM_DOUT_NET                    "PCM_DOUT"
#define SB_KAE8_PCM_DIN_NET                     "PCM_DIN"
#define SB_KAE8_PCM_SYNC_NET                    "PCM_SYNC"
#define SB_KAE8_PCM_CLK_NET                     "PCM_CLK"

/* External W25Q64JWSIQ NOR.
 * SPI1 uses pins 74-77. FLASH_WP is USIM_DET pin79 -> GPIO[37].
 * FLASH_RST/HOLD is GPIO7 pin80 -> GPIO[56]. Both are driven high before
 * ql_spi_nor_init(). DI/DO interchange is a board routing requirement;
 * firmware treats a valid JEDEC-ID read as the acceptance test.
 */
#define SB_KAE8_NOR_WP_GPIO                     GPIO_PIN_NO_37
#define SB_KAE8_NOR_RST_GPIO                    GPIO_PIN_NO_56
#define SB_KAE8_NOR_CTRL_ACTIVE_LEVEL           PIN_LEVEL_HIGH
#define SB_KAE8_NOR_PORT                        EXTERNAL_NORFLASH_PORT4_7
#define SB_KAE8_NOR_CLK                         _APBC_SSP_FNCLKSEL_13MHZ_
#define SB_KAE8_NOR_SIZE_BYTES                  (8u * 1024u * 1024u)
#define SB_KAE8_NOR_SECTOR_SIZE_BYTES           (4096u)
#define SB_KAE8_NOR_PAGE_SIZE_BYTES             (256u)
#define SB_KAE8_NOR_MANIFEST_ADDR               (0x000000u)
#define SB_KAE8_NOR_MANIFEST_RESERVED_BYTES     (192u * 1024u)
#define SB_KAE8_NOR_ASSET_BASE_ADDR             (SB_KAE8_NOR_MANIFEST_RESERVED_BYTES)
#define SB_KAE8_NOR_FACTORY_TEST_ADDR           (SB_KAE8_NOR_SIZE_BYTES - SB_KAE8_NOR_SECTOR_SIZE_BYTES)
#define SB_NOR_DI_DO_INTERCHANGED               (1u)

#ifdef __cplusplus
}
#endif

#endif /* SB_BOARD_KAE8_SQ1_H */
