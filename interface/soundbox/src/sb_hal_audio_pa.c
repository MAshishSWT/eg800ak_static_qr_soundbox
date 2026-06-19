/*================================================================
 * Static QR UPI Soundbox - Speaker Amplifier HAL
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "sb_board_kae8_sq1.h"
#include "sb_hal_audio_pa.h"
#include "sb_hal_gpio.h"

static int s_pa_enabled = 0;

sb_status_t sb_hal_audio_pa_init(void)
{
    s_pa_enabled = 0;
    return sb_hal_gpio_output(SB_KAE8_SPK_SHDN_GPIO, SB_KAE8_SPK_SHDN_ASSERT_LEVEL);
}

sb_status_t sb_hal_audio_pa_set_enabled(int enabled)
{
    sb_status_t status;

    if (enabled != 0) {
        status = sb_hal_gpio_set(SB_KAE8_SPK_SHDN_GPIO, SB_KAE8_SPK_SHDN_DEASSERT_LEVEL);
    } else {
        status = sb_hal_gpio_set(SB_KAE8_SPK_SHDN_GPIO, SB_KAE8_SPK_SHDN_ASSERT_LEVEL);
    }

    if (status == SB_STATUS_OK) {
        s_pa_enabled = (enabled != 0) ? 1 : 0;
    }
    return status;
}

int sb_hal_audio_pa_is_enabled(void)
{
    return s_pa_enabled;
}
