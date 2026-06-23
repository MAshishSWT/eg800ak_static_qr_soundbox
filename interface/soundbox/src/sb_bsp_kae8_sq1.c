/*================================================================
 * Static QR UPI Soundbox - KAE8_SQ1 BSP
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "sb_bsp_kae8_sq1.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_hal_adc.h"
#include "sb_hal_audio_pa.h"
#include "sb_hal_key.h"
#include "sb_hal_gpio.h"
#include "sb_board_kae8_sq1.h"
#include "sb_hal_led.h"
#include "sb_log.h"

#define SB_BSP_MODULE_NAME "bsp"

static int s_board_ready = 0;

static void sb_bsp_log_key_state(sb_key_id_t key_id, const char *name)
{
    sb_key_state_t state;
    sb_status_t status;

    status = sb_hal_key_get_state(key_id, &state);
    if (status == SB_STATUS_OK) {
        SB_LOGI(SB_BSP_MODULE_NAME, "key %s gpio=%d level=%d pressed=%d",
                name, (int)sb_hal_key_gpio(key_id), (int)state.level, state.pressed);
    } else {
        SB_LOGW(SB_BSP_MODULE_NAME, "key %s read status=%s", name, sb_status_to_string(status));
    }
}

static void sb_bsp_log_battery_state(void)
{
    sb_battery_sample_t sample;
    sb_status_t status;

    status = sb_bsp_board_read_battery(&sample);
    if (status == SB_STATUS_OK) {
        SB_LOGI(SB_BSP_MODULE_NAME, "battery adc=%umV battery=%umV percent=%u",
                sample.adc_mv, sample.battery_mv, sample.battery_percent);
    } else {
        SB_LOGW(SB_BSP_MODULE_NAME, "battery read status=%s", sb_status_to_string(status));
    }
}

static void sb_bsp_log_initial_inputs(void)
{
    sb_bsp_log_key_state(SB_KEY_VOLUME_UP, "volume_up");
    sb_bsp_log_key_state(SB_KEY_VOLUME_DOWN, "volume_down");
    sb_bsp_log_key_state(SB_KEY_MODE, "mode");
    sb_bsp_log_battery_state();
}

static sb_status_t sb_bsp_check_status(sb_status_t status, const char *name)
{
    if (status != SB_STATUS_OK && status != SB_STATUS_ALREADY_INITIALIZED) {
        SB_LOGE(SB_BSP_MODULE_NAME, "%s failed status=%s", name, sb_status_to_string(status));
        return status;
    }
    SB_LOGI(SB_BSP_MODULE_NAME, "%s ok", name);
    return SB_STATUS_OK;
}

sb_status_t sb_bsp_board_init(void)
{
    sb_status_t status;

    if (s_board_ready != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }

    status = sb_bsp_check_status(sb_hal_led_init(), "led");
    if (status != SB_STATUS_OK) {
        return status;
    }

    status = sb_bsp_check_status(sb_hal_audio_pa_init(), "speaker_pa");
    if (status != SB_STATUS_OK) {
        return status;
    }

    status = sb_bsp_check_status(sb_hal_adc_init(), "adc");
    if (status != SB_STATUS_OK) {
        return status;
    }

    status = sb_bsp_check_status(sb_hal_key_init(), "keys");
    if (status != SB_STATUS_OK) {
        return status;
    }

    s_board_ready = 1;
    sb_bsp_log_initial_inputs();
    (void)sb_bsp_board_set_status_led(1);
    return SB_STATUS_OK;
}

sb_status_t sb_bsp_board_post_status(void)
{
    sb_battery_sample_t sample;
    sb_event_t event;
    sb_status_t status;

    if (s_board_ready == 0) {
        return SB_STATUS_NOT_READY;
    }

    sb_event_init(&event, SB_EVENT_BOARD_READY, SB_EVENT_SOURCE_SYSTEM);
    (void)sb_event_post(&event, QL_NO_WAIT);

    status = sb_bsp_board_read_battery(&sample);
    if (status != SB_STATUS_OK) {
        return status;
    }

    sb_event_init(&event, SB_EVENT_BATTERY_SAMPLE, SB_EVENT_SOURCE_SYSTEM);
    event.param_u32 = sample.battery_mv;
    event.param_s32 = (s32)sample.battery_percent;
    return sb_event_post(&event, QL_NO_WAIT);
}

sb_status_t sb_bsp_board_set_status_led(int on)
{
    return sb_hal_led_set(SB_LED_STATUS, on);
}

sb_status_t sb_bsp_board_toggle_status_led(void)
{
    return sb_hal_led_toggle(SB_LED_STATUS);
}

sb_status_t sb_bsp_board_set_speaker_amp(int enabled)
{
    return sb_hal_audio_pa_set_enabled(enabled);
}

sb_status_t sb_bsp_board_read_battery(sb_battery_sample_t *sample)
{
    return sb_hal_adc_read_battery(sample);
}
