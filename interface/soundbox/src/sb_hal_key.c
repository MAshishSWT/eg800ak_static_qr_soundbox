/*================================================================
 * Static QR UPI Soundbox - Key HAL with Debounce Task
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "sb_board_kae8_sq1.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_hal_gpio.h"
#include "sb_hal_key.h"
#include "sb_log.h"

#define SB_KEY_MODULE_NAME              "key"
#define SB_KEY_TASK_STACK_BYTES         (4096u)
#define SB_KEY_TASK_PRIORITY            (18u)
#define SB_KEY_QUEUE_DEPTH              (8u)

static ql_task_t s_key_task = 0;
static ql_queue_t s_key_queue = 0;
static int s_key_last_pressed[4];
static int s_key_ready = 0;

static GPIO_PIN_NUMBER_E sb_key_to_gpio(sb_key_id_t key_id)
{
    switch (key_id) {
    case SB_KEY_VOLUME_UP:
        return SB_KAE8_KEY_VOLUME_UP_GPIO;
    case SB_KEY_VOLUME_DOWN:
        return SB_KAE8_KEY_VOLUME_DOWN_GPIO;
    case SB_KEY_MODE:
        return SB_KAE8_KEY_MODE_GPIO;
    default:
        return GPIO_PIN_NO_NOT_ASSIGNED;
    }
}

static const char *sb_key_name(sb_key_id_t key_id)
{
    switch (key_id) {
    case SB_KEY_VOLUME_UP: return "sw1_volume_up";
    case SB_KEY_VOLUME_DOWN: return "sw2_volume_down";
    case SB_KEY_MODE: return "sw3_mode";
    default: return "unknown";
    }
}

static void sb_key_irq_post(sb_key_id_t key_id)
{
    GPIO_PIN_NUMBER_E gpio = sb_key_to_gpio(key_id);
    u32 value = (u32)key_id;

    if ((s_key_queue == 0) || (gpio == GPIO_PIN_NO_NOT_ASSIGNED)) {
        return;
    }
    (void)sb_hal_gpio_disable_eint(gpio);
    if (ql_rtos_queue_release(s_key_queue, (u32)sizeof(value), (u8 *)&value, QL_NO_WAIT) != 0) {
        (void)sb_hal_gpio_enable_eint(gpio, PIN_BOTH_EDGE);
    }
}

static void sb_key_volume_up_irq(void)
{
    sb_key_irq_post(SB_KEY_VOLUME_UP);
}

static void sb_key_volume_down_irq(void)
{
    sb_key_irq_post(SB_KEY_VOLUME_DOWN);
}

static void sb_key_mode_irq(void)
{
    sb_key_irq_post(SB_KEY_MODE);
}

GPIO_PIN_NUMBER_E sb_hal_key_gpio(sb_key_id_t key_id)
{
    return sb_key_to_gpio(key_id);
}

sb_status_t sb_hal_key_get_state(sb_key_id_t key_id, sb_key_state_t *state)
{
    GPIO_PIN_NUMBER_E gpio;
    PIN_LEVEL_E level;

    if (state == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    gpio = sb_key_to_gpio(key_id);
    if (gpio == GPIO_PIN_NO_NOT_ASSIGNED) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (sb_hal_gpio_get(gpio, &level) != SB_STATUS_OK) {
        return SB_STATUS_ERROR;
    }
    state->key_id = key_id;
    state->level = level;
    state->pressed = (level == SB_KAE8_KEY_ACTIVE_LEVEL) ? 1 : 0;
    return SB_STATUS_OK;
}

static void sb_key_post_edge_event(const sb_key_state_t *state)
{
    sb_event_t event;

    if (state == 0) {
        return;
    }
    sb_event_init(&event, SB_EVENT_KEY_EDGE, SB_EVENT_SOURCE_KEY);
    event.param_u32 = (u32)state->key_id;
    event.param_s32 = (s32)state->pressed;
    (void)sb_event_set_text(&event, sb_key_name(state->key_id));
    (void)sb_event_post(&event, QL_NO_WAIT);
}

static void sb_key_task(void *argv)
{
    u32 key_value;
    sb_key_id_t key_id;
    sb_key_state_t state;
    GPIO_PIN_NUMBER_E gpio;

    (void)argv;
    SB_LOGI(SB_KEY_MODULE_NAME, "debounce task started debounce_ms=%u", SB_KAE8_KEY_DEBOUNCE_MS);
    while (1) {
        if (ql_rtos_queue_wait(s_key_queue, (u8 *)&key_value, (u32)sizeof(key_value), QL_WAIT_FOREVER) != 0) {
            continue;
        }
        key_id = (sb_key_id_t)key_value;
        gpio = sb_key_to_gpio(key_id);
        ql_rtos_task_sleep_ms(SB_KAE8_KEY_DEBOUNCE_MS);
        if (sb_hal_key_get_state(key_id, &state) == SB_STATUS_OK) {
            if ((key_id < 4) && (s_key_last_pressed[key_id] != state.pressed)) {
                s_key_last_pressed[key_id] = state.pressed;
                sb_key_post_edge_event(&state);
                SB_LOGI(SB_KEY_MODULE_NAME, "%s pressed=%d", sb_key_name(key_id), state.pressed);
            }
        }
        if (gpio != GPIO_PIN_NO_NOT_ASSIGNED) {
            (void)sb_hal_gpio_enable_eint(gpio, PIN_BOTH_EDGE);
        }
    }
}

static sb_status_t sb_hal_key_configure(sb_key_id_t key_id, sb_hal_gpio_irq_cb_t cb)
{
    GPIO_PIN_NUMBER_E gpio;
    sb_status_t status;
    sb_key_state_t state;

    gpio = sb_key_to_gpio(key_id);
    if ((gpio == GPIO_PIN_NO_NOT_ASSIGNED) || (cb == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    status = sb_hal_gpio_input(gpio, SB_KAE8_KEY_IDLE_PULL);
    if (status != SB_STATUS_OK) {
        return status;
    }
    status = sb_hal_key_get_state(key_id, &state);
    if (status == SB_STATUS_OK) {
        s_key_last_pressed[key_id] = state.pressed;
        if (state.pressed != 0) {
            SB_LOGW(SB_KEY_MODULE_NAME, "%s pressed at boot; stuck-key check window=%u ms", sb_key_name(key_id), SB_KAE8_KEY_STUCK_BOOT_MS);
        }
    }
    status = sb_hal_gpio_register_eint(gpio, PIN_BOTH_EDGE, SB_KAE8_KEY_IDLE_PULL, cb);
    if (status != SB_STATUS_OK) {
        return status;
    }
    return sb_hal_gpio_enable_eint(gpio, PIN_BOTH_EDGE);
}

sb_status_t sb_hal_key_init(void)
{
    QlOSStatus ret;
    sb_status_t status;

    if (s_key_ready != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }
    ret = ql_rtos_queue_create(&s_key_queue, (u32)sizeof(u32), SB_KEY_QUEUE_DEPTH);
    if (ret != 0) {
        s_key_queue = 0;
        return SB_STATUS_QUEUE_ERROR;
    }
    status = sb_hal_key_configure(SB_KEY_VOLUME_UP, sb_key_volume_up_irq);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_hal_key_configure(SB_KEY_VOLUME_DOWN, sb_key_volume_down_irq);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_hal_key_configure(SB_KEY_MODE, sb_key_mode_irq);
    if (status != SB_STATUS_OK) { return status; }
    ret = ql_rtos_task_create(&s_key_task,
                              SB_KEY_TASK_STACK_BYTES,
                              SB_KEY_TASK_PRIORITY,
                              "sb_key",
                              sb_key_task,
                              0);
    if (ret != 0) {
        return SB_STATUS_TASK_ERROR;
    }
    s_key_ready = 1;
    return SB_STATUS_OK;
}
