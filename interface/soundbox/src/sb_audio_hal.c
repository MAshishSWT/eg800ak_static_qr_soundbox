/*================================================================
 * Static QR UPI Soundbox - EG800AK Audio HAL
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_audio.h"
#include "ql_rtos.h"
#include "sb_audio_codec_es8311.h"
#include "sb_audio_hal.h"
#include "sb_bsp_kae8_sq1.h"
#include "sb_log.h"

#define SB_AUDIO_HAL_MODULE_NAME       "audio_hal"
#define SB_AUDIO_HAL_LEVEL_MAX         (11u)
#define SB_AUDIO_HAL_MIN_TIMEOUT_MS    (1000u)
#define SB_AUDIO_HAL_MAX_TIMEOUT_MS    (60000u)

static ql_sem_t s_mp3_done_sem = 0;
static volatile int s_mp3_last_status = -1;
static int s_audio_ready = 0;
static u32 s_volume_percent = 70u;

static AUDIOHAL_SPK_LEVEL_T sb_audio_hal_percent_to_level(u32 volume_percent)
{
    u32 level;

    if (volume_percent > 100u) {
        volume_percent = 100u;
    }

    level = ((volume_percent * SB_AUDIO_HAL_LEVEL_MAX) + 50u) / 100u;
    if (level > SB_AUDIO_HAL_LEVEL_MAX) {
        level = SB_AUDIO_HAL_LEVEL_MAX;
    }

    return (AUDIOHAL_SPK_LEVEL_T)level;
}

static int sb_audio_hal_timeout_us(u32 timeout_ms)
{
    if (timeout_ms < SB_AUDIO_HAL_MIN_TIMEOUT_MS) {
        timeout_ms = SB_AUDIO_HAL_MIN_TIMEOUT_MS;
    }
    if (timeout_ms > SB_AUDIO_HAL_MAX_TIMEOUT_MS) {
        timeout_ms = SB_AUDIO_HAL_MAX_TIMEOUT_MS;
    }

    return (int)(timeout_ms * 1000u);
}

static void sb_audio_hal_mp3_event_cb(Mp3PlayEventType type, int value)
{
    if (type != MP3_PLAYBACK_EVENT_STATUS) {
        return;
    }

    s_mp3_last_status = value;
    if ((value == MP3_PLAYBACK_STATUS_ENDED) ||
        (value == MP3_PLAYBACK_STATUS_TIMEOUT) ||
        (value == MP3_PLAYBACK_STATUS_FILE_READED)) {
        if (s_mp3_done_sem != 0) {
            (void)ql_rtos_semaphore_release(s_mp3_done_sem);
        }
    }
}

static sb_status_t sb_audio_hal_wait_audio_ready(void)
{
    u32 waited_ms = 0u;

    while (ql_get_audio_state() == 0) {
        if (waited_ms >= SB_AUDIO_HAL_READY_TIMEOUT_MS) {
            return SB_STATUS_TIMEOUT;
        }
        ql_rtos_task_sleep_ms(50u);
        waited_ms += 50u;
    }

    return SB_STATUS_OK;
}

sb_status_t sb_audio_hal_init(u32 volume_percent)
{
    sb_status_t status;
    ql_codecpcm_config pcm_config;

    if (s_audio_ready != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }

    status = sb_audio_hal_wait_audio_ready();
    if (status != SB_STATUS_OK) {
        SB_LOGW(SB_AUDIO_HAL_MODULE_NAME, "audio core not ready status=%s", sb_status_to_string(status));
        return status;
    }

    if (s_mp3_done_sem == 0) {
        if (ql_rtos_semaphore_create(&s_mp3_done_sem, 0u) != 0) {
            return SB_STATUS_NO_MEMORY;
        }
    }

    pcm_config.pcmbclk = PCM_FS_64;
    pcm_config.sample = PCM_CODEC_SAMPLE_16000;
    pcm_config.is_i2s = 0;
    ql_codec_choose(AUD_EXTERNAL_CODEC, &pcm_config);
    status = sb_audio_codec_es8311_open();
    if (status != SB_STATUS_OK) {
        SB_LOGW(SB_AUDIO_HAL_MODULE_NAME,
                "external codec status=%s, using internal codec path",
                sb_status_to_string(status));
        ql_codec_choose(AUD_INTERNAL_CODEC, NULL);
        ql_set_audio_path_speaker();
    }

    status = sb_audio_hal_set_volume_percent(volume_percent);
    if (status != SB_STATUS_OK) {
        return status;
    }

    s_audio_ready = 1;
    SB_LOGI(SB_AUDIO_HAL_MODULE_NAME, "ready volume=%u level=%d",
            s_volume_percent, ql_get_volume());
    return SB_STATUS_OK;
}

sb_status_t sb_audio_hal_set_volume_percent(u32 volume_percent)
{
    AUDIOHAL_SPK_LEVEL_T level;

    if (volume_percent > 100u) {
        volume_percent = 100u;
    }

    level = sb_audio_hal_percent_to_level(volume_percent);
    ql_set_volume(level);
    s_volume_percent = volume_percent;
    return SB_STATUS_OK;
}

sb_status_t sb_audio_hal_play_mp3_file(const char *path, u32 timeout_ms)
{
    Mp3PlaybackHandle handle = 0u;
    Mp3PlayConfigInfo config;
    int ret;
    QlOSStatus wait_status;

    if ((path == 0) || (path[0] == '\0')) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (s_audio_ready == 0) {
        return SB_STATUS_NOT_READY;
    }

    {
        unsigned char *cfg_bytes = (unsigned char *)&config;
        u32 i;
        for (i = 0u; i < (u32)sizeof(config); i++) {
            cfg_bytes[i] = 0u;
        }
    }
    config.listener = sb_audio_hal_mp3_event_cb;
    config.listener2 = 0;
    config.timeout = sb_audio_hal_timeout_us(timeout_ms);
    config.sync_mode = 0;

    s_mp3_last_status = -1;
    (void)sb_bsp_board_set_speaker_amp(1);

    ret = ql_play_mp3((char *)path, &handle, &config);
    if (ret != 0) {
        (void)sb_bsp_board_set_speaker_amp(0);
        SB_LOGW(SB_AUDIO_HAL_MODULE_NAME, "mp3 start failed ret=%d path=%s", ret, path);
        return SB_STATUS_FILE_ERROR;
    }

    wait_status = ql_rtos_semaphore_wait(s_mp3_done_sem, timeout_ms + 1000u);
    if (wait_status != 0) {
        (void)ql_stop_mp3_play(handle, 1);
        (void)sb_bsp_board_set_speaker_amp(0);
        SB_LOGW(SB_AUDIO_HAL_MODULE_NAME, "mp3 wait timeout path=%s", path);
        return SB_STATUS_TIMEOUT;
    }

    (void)sb_bsp_board_set_speaker_amp(0);

    if ((s_mp3_last_status == MP3_PLAYBACK_STATUS_ENDED) ||
        (s_mp3_last_status == MP3_PLAYBACK_STATUS_FILE_READED)) {
        return SB_STATUS_OK;
    }

    if (s_mp3_last_status == MP3_PLAYBACK_STATUS_TIMEOUT) {
        return SB_STATUS_TIMEOUT;
    }

    return SB_STATUS_OK;
}

int sb_audio_hal_is_ready(void)
{
    return s_audio_ready;
}
