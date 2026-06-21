/*================================================================
 * Static QR UPI Soundbox - Audio Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "sb_audio_asset.h"
#include "sb_audio_hal.h"
#include "sb_audio_script.h"
#include "sb_audio_service.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_log.h"

#define SB_AUDIO_SERVICE_MODULE_NAME       "audio"
#define SB_AUDIO_SCRIPT_ITEM_TIMEOUT_MS    (12000u)

typedef enum {
    SB_AUDIO_REQ_PROMPT = 0,
    SB_AUDIO_REQ_AMOUNT,
    SB_AUDIO_REQ_VOLUME
} sb_audio_request_type_t;

typedef struct {
    sb_audio_request_type_t type;
    sb_audio_language_t language;
    sb_audio_provider_t provider;
    sb_audio_prompt_id_t prompt;
    u64 amount_paise;
    u32 volume_percent;
} sb_audio_request_t;

static ql_task_t s_audio_task = 0;
static ql_queue_t s_audio_queue = 0;
static int s_audio_started = 0;
static sb_audio_language_t s_default_language = SB_AUDIO_LANG_EN;
static u32 s_default_volume_percent = 70u;

static void sb_audio_post_event(sb_event_id_t id, sb_status_t status, const char *text)
{
    sb_event_t event;

    sb_event_init(&event, id, SB_EVENT_SOURCE_AUDIO);
    event.param_s32 = (s32)status;
    if (text != 0) {
        (void)sb_event_set_text(&event, text);
    }
    (void)sb_event_post(&event, QL_NO_WAIT);
}

static sb_status_t sb_audio_service_enqueue(const sb_audio_request_t *request)
{
    if (request == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (s_audio_queue == 0) {
        return SB_STATUS_NOT_READY;
    }

    if (ql_rtos_queue_release(s_audio_queue,
                              (u32)sizeof(*request),
                              (u8 *)request,
                              QL_NO_WAIT) != 0) {
        return SB_STATUS_QUEUE_ERROR;
    }

    return SB_STATUS_OK;
}

static sb_status_t sb_audio_play_script(const sb_audio_script_t *script)
{
    sb_status_t status;
    char missing[SB_AUDIO_PATH_LEN];
    u32 i;

    if (script == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (script->count == 0u) {
        return SB_STATUS_INVALID_PARAM;
    }

    missing[0] = '\0';
    status = sb_audio_asset_validate_script(script, missing, (u32)sizeof(missing));
    if (status != SB_STATUS_OK) {
        SB_LOGW(SB_AUDIO_SERVICE_MODULE_NAME, "missing asset=%s", missing);
        sb_audio_post_event(SB_EVENT_AUDIO_FAULT, status, missing);
        return status;
    }

    sb_audio_post_event(SB_EVENT_AUDIO_PLAY_STARTED, SB_STATUS_OK, script->items[0].path);

    for (i = 0u; i < script->count; i++) {
        status = sb_audio_hal_play_mp3_file(script->items[i].path, SB_AUDIO_SCRIPT_ITEM_TIMEOUT_MS);
        if (status != SB_STATUS_OK) {
            SB_LOGW(SB_AUDIO_SERVICE_MODULE_NAME,
                    "play status=%s path=%s",
                    sb_status_to_string(status),
                    script->items[i].path);
            sb_audio_post_event(SB_EVENT_AUDIO_FAULT, status, script->items[i].path);
            return status;
        }
    }

    sb_audio_post_event(SB_EVENT_AUDIO_PLAY_DONE, SB_STATUS_OK, "done");
    return SB_STATUS_OK;
}

static void sb_audio_service_handle_request(const sb_audio_request_t *request)
{
    sb_audio_script_t script;
    sb_status_t status;

    if (request == 0) {
        return;
    }

    if (request->type == SB_AUDIO_REQ_VOLUME) {
        status = sb_audio_hal_set_volume_percent(request->volume_percent);
        SB_LOGI(SB_AUDIO_SERVICE_MODULE_NAME,
                "volume percent=%u status=%s",
                request->volume_percent,
                sb_status_to_string(status));
        if (status != SB_STATUS_OK) {
            sb_audio_post_event(SB_EVENT_AUDIO_FAULT, status, "volume");
        }
        return;
    }

    if (request->type == SB_AUDIO_REQ_PROMPT) {
        status = sb_audio_script_build_status(request->language, request->prompt, &script);
    } else if (request->type == SB_AUDIO_REQ_AMOUNT) {
        status = sb_audio_script_build_amount_received(request->language,
                                                       request->provider,
                                                       request->amount_paise,
                                                       &script);
    } else {
        status = SB_STATUS_INVALID_PARAM;
    }

    if (status != SB_STATUS_OK) {
        SB_LOGW(SB_AUDIO_SERVICE_MODULE_NAME, "script build status=%s", sb_status_to_string(status));
        sb_audio_post_event(SB_EVENT_AUDIO_FAULT, status, "script");
        return;
    }

    SB_LOGI(SB_AUDIO_SERVICE_MODULE_NAME, "script items=%u", script.count);
    (void)sb_audio_play_script(&script);
}

static void sb_audio_task(void *argv)
{
    sb_audio_request_t request;
    sb_status_t status;

    (void)argv;

    status = sb_audio_hal_init(s_default_volume_percent);
    if ((status == SB_STATUS_OK) || (status == SB_STATUS_ALREADY_INITIALIZED)) {
        SB_LOGI(SB_AUDIO_SERVICE_MODULE_NAME,
                "ready lang=%s volume=%u",
                sb_audio_language_code(s_default_language),
                s_default_volume_percent);
        sb_audio_post_event(SB_EVENT_AUDIO_READY, SB_STATUS_OK, "ready");
    } else {
        SB_LOGW(SB_AUDIO_SERVICE_MODULE_NAME, "hal init status=%s", sb_status_to_string(status));
        sb_audio_post_event(SB_EVENT_AUDIO_FAULT, status, "hal_init");
    }

    while (1) {
        if (ql_rtos_queue_wait(s_audio_queue,
                               (u8 *)&request,
                               (u32)sizeof(request),
                               QL_WAIT_FOREVER) == 0) {
            if (sb_audio_hal_is_ready() == 0) {
                status = sb_audio_hal_init(s_default_volume_percent);
                if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) {
                    SB_LOGW(SB_AUDIO_SERVICE_MODULE_NAME, "hal retry status=%s", sb_status_to_string(status));
                    sb_audio_post_event(SB_EVENT_AUDIO_FAULT, status, "hal_retry");
                    continue;
                }
            }
            sb_audio_service_handle_request(&request);
        }
    }
}

sb_status_t sb_audio_service_init(sb_audio_language_t language, u32 volume_percent)
{
    QlOSStatus ret;

    if (s_audio_started != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }

    if ((language < 0) || (language >= SB_AUDIO_LANG_COUNT)) {
        language = SB_AUDIO_LANG_EN;
    }

    if (volume_percent > 100u) {
        volume_percent = 100u;
    }

    s_default_language = language;
    s_default_volume_percent = volume_percent;

    ret = ql_rtos_queue_create(&s_audio_queue,
                               (u32)sizeof(sb_audio_request_t),
                               SB_AUDIO_SERVICE_QUEUE_DEPTH);
    if (ret != 0) {
        s_audio_queue = 0;
        return SB_STATUS_QUEUE_ERROR;
    }

    ret = ql_rtos_task_create(&s_audio_task,
                              SB_AUDIO_SERVICE_TASK_STACK_BYTES,
                              SB_AUDIO_SERVICE_TASK_PRIORITY,
                              "sb_audio",
                              sb_audio_task,
                              0);
    if (ret != 0) {
        (void)ql_rtos_queue_delete(s_audio_queue);
        s_audio_queue = 0;
        s_audio_task = 0;
        return SB_STATUS_TASK_ERROR;
    }

    s_audio_started = 1;
    return SB_STATUS_OK;
}

sb_status_t sb_audio_service_play_prompt(sb_audio_language_t language, sb_audio_prompt_id_t prompt)
{
    sb_audio_request_t request;

    if ((prompt < 0) || (prompt >= SB_AUDIO_PROMPT_COUNT)) {
        return SB_STATUS_INVALID_PARAM;
    }

    request.type = SB_AUDIO_REQ_PROMPT;
    request.language = language;
    request.provider = SB_AUDIO_PROVIDER_OTHER;
    request.prompt = prompt;
    request.amount_paise = 0ull;
    request.volume_percent = 0u;
    return sb_audio_service_enqueue(&request);
}

sb_status_t sb_audio_service_play_amount(sb_audio_language_t language,
                                          sb_audio_provider_t provider,
                                          u64 amount_paise)
{
    sb_audio_request_t request;

    if ((provider < 0) || (provider >= SB_AUDIO_PROVIDER_COUNT)) {
        provider = SB_AUDIO_PROVIDER_OTHER;
    }

    request.type = SB_AUDIO_REQ_AMOUNT;
    request.language = language;
    request.provider = provider;
    request.prompt = SB_AUDIO_PROMPT_PAYMENT_RECEIVED;
    request.amount_paise = amount_paise;
    request.volume_percent = 0u;
    return sb_audio_service_enqueue(&request);
}

sb_status_t sb_audio_service_set_volume(u32 volume_percent)
{
    sb_audio_request_t request;

    if (volume_percent > 100u) {
        volume_percent = 100u;
    }

    request.type = SB_AUDIO_REQ_VOLUME;
    request.language = s_default_language;
    request.provider = SB_AUDIO_PROVIDER_OTHER;
    request.prompt = SB_AUDIO_PROMPT_READY;
    request.amount_paise = 0ull;
    request.volume_percent = volume_percent;
    return sb_audio_service_enqueue(&request);
}
