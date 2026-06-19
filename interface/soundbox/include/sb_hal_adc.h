/*================================================================
 * Static QR UPI Soundbox - ADC/Battery HAL
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_HAL_ADC_H
#define SB_HAL_ADC_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    u32 adc_mv;
    u32 battery_mv;
    u32 battery_percent;
} sb_battery_sample_t;

sb_status_t sb_hal_adc_init(void);
sb_status_t sb_hal_adc_read_battery(sb_battery_sample_t *sample);
u32 sb_hal_adc_battery_percent_from_mv(u32 battery_mv);

#ifdef __cplusplus
}
#endif

#endif /* SB_HAL_ADC_H */
