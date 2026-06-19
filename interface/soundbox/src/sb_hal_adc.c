/*================================================================
 * Static QR UPI Soundbox - ADC/Battery HAL
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_adc.h"
#include "sb_board_kae8_sq1.h"
#include "sb_hal_adc.h"

#define SB_BATTERY_EMPTY_MV  (3300u)
#define SB_BATTERY_FULL_MV   (4200u)

static int s_adc_ready = 0;

u32 sb_hal_adc_battery_percent_from_mv(u32 battery_mv)
{
    u32 range_mv;
    u32 percent;

    if (battery_mv <= SB_BATTERY_EMPTY_MV) {
        return 0u;
    }

    if (battery_mv >= SB_BATTERY_FULL_MV) {
        return 100u;
    }

    range_mv = SB_BATTERY_FULL_MV - SB_BATTERY_EMPTY_MV;
    percent = ((battery_mv - SB_BATTERY_EMPTY_MV) * 100u) / range_mv;
    if (percent > 100u) {
        percent = 100u;
    }
    return percent;
}

sb_status_t sb_hal_adc_init(void)
{
    if (s_adc_ready != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }

    if (ql_adc_init() != 0) {
        return SB_STATUS_ERROR;
    }

    s_adc_ready = 1;
    return SB_STATUS_OK;
}

sb_status_t sb_hal_adc_read_battery(sb_battery_sample_t *sample)
{
    unsigned short adc_mv = 0u;
    u32 battery_mv;

    if (sample == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (s_adc_ready == 0) {
        return SB_STATUS_NOT_READY;
    }

    if (ql_adc_read((unsigned char)SB_KAE8_ADC_BATTERY_CHANNEL, &adc_mv) != ADC_READ_SUCCESS) {
        return SB_STATUS_ERROR;
    }

    battery_mv = ((u32)adc_mv * (SB_KAE8_BATTERY_R_HIGH_OHM + SB_KAE8_BATTERY_R_LOW_OHM)) /
                 SB_KAE8_BATTERY_R_LOW_OHM;

    sample->adc_mv = (u32)adc_mv;
    sample->battery_mv = battery_mv;
    sample->battery_percent = sb_hal_adc_battery_percent_from_mv(battery_mv);
    return SB_STATUS_OK;
}
