/*================================================================
 * Static QR UPI Soundbox - EG800AK Audio HAL
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_audio.h"
#include "ql_fs.h"
#include "ql_rtos.h"
#include "sb_audio_codec_es8311.h"
#include "sb_audio_hal.h"
#include "sb_bsp_kae8_sq1.h"
#include "sb_log.h"

#define SB_AUDIO_HAL_MODULE_NAME       "audio_hal"
#define SB_AUDIO_HAL_LEVEL_MAX         (11u)
#define SB_AUDIO_HAL_MIN_TIMEOUT_MS    (1000u)
#define SB_AUDIO_HAL_MAX_TIMEOUT_MS    (60000u)
#define SB_AUDIO_HAL_STREAM_CHUNK      (512u)
#define SB_AUDIO_HAL_OLD_API_RETRY_MS  (150u)

static ql_sem_t s_mp3_done_sem = 0;
static volatile int s_mp3_last_status = -1;
static int s_audio_ready = 0;
static u32 s_volume_percent = 70u;
static signed short s_audio_dsp_gain_table[12] = {-36, -31, -27, -24, -21, -18, -15, -12, -9, -6, -3, 0};
static volatile int s_old_player_last_state = AUD_PLAYER_ERROR;

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

static u32 sb_audio_hal_clamp_timeout_ms(u32 timeout_ms)
{
    if (timeout_ms < SB_AUDIO_HAL_MIN_TIMEOUT_MS) {
        timeout_ms = SB_AUDIO_HAL_MIN_TIMEOUT_MS;
    }
    if (timeout_ms > SB_AUDIO_HAL_MAX_TIMEOUT_MS) {
        timeout_ms = SB_AUDIO_HAL_MAX_TIMEOUT_MS;
    }

    return timeout_ms;
}

static u32 sb_audio_hal_estimate_mp3_ms(u32 file_size)
{
    u32 duration_ms;

    /* Supplied prompts are low-bitrate mono MP3 files. A conservative
     * 16 kbit/s estimate prevents fallback stream playback from being
     * stopped before the decoder drains its buffer. */
    duration_ms = ((file_size * 8u * 1000u) / 16000u) + 1200u;
    if (duration_ms < 1500u) {
        duration_ms = 1500u;
    }
    if (duration_ms > 8000u) {
        duration_ms = 8000u;
    }
    return duration_ms;
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

static void sb_audio_hal_player_cb(char *data, int len, enum_aud_player_state state)
{
    (void)data;
    (void)len;
    s_old_player_last_state = state;

    if ((state == AUD_PLAYER_ERROR) || (state == AUD_PLAYER_FINISHED)) {
        if (s_mp3_done_sem != 0) {
            (void)ql_rtos_semaphore_release(s_mp3_done_sem);
        }
    }
}

static void sb_audio_hal_pa_cb(unsigned int on)
{
    (void)sb_bsp_board_set_speaker_amp(on ? 1 : 0);
}

static void sb_audio_hal_apply_quec_sample_gain_table(void)
{
    (void)ql_set_rxcodec_gain_table(1u, 0, 36u, 1u, 1u, 0u, 2u);
    (void)ql_set_rxcodec_gain_table(1u, 1, 36u, 1u, 1u, 0u, 2u);
    (void)ql_set_rxcodec_gain_table(1u, 2, 36u, 1u, 2u, 0u, 2u);
    (void)ql_set_rxcodec_gain_table(1u, 3, 36u, 1u, 2u, 0u, 2u);
    (void)ql_set_rxcodec_gain_table(1u, 4, 36u, 1u, 3u, 0u, 2u);
    (void)ql_set_rxcodec_gain_table(1u, 5, 36u, 1u, 3u, 1u, 2u);
    (void)ql_set_rxcodec_gain_table(1u, 6, 36u, 1u, 4u, 1u, 2u);
    (void)ql_set_rxcodec_gain_table(1u, 7, 36u, 1u, 4u, 1u, 2u);
    (void)ql_set_rxcodec_gain_table(1u, 8, 36u, 1u, 4u, 1u, 2u);
    (void)ql_set_rxcodec_gain_table(1u, 9, 36u, 1u, 4u, 1u, 2u);
    (void)ql_set_rxcodec_gain_table(1u, 10, 36u, 1u, 4u, 1u, 2u);
    (void)ql_set_rxcodec_gain_table(1u, 11, 36u, 1u, 4u, 1u, 2u);
    (void)ql_set_dsp_gain_table(1u, s_audio_dsp_gain_table);
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

static sb_status_t sb_audio_hal_probe_mp3_file(const char *path, u32 *file_size_out)
{
    QFILE *fp;
    unsigned char head[4] = {0u, 0u, 0u, 0u};
    int size;
    int read_len;

    if ((path == 0) || (file_size_out == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    fp = ql_fopen(path, "rb");
    if (fp == 0) {
        SB_LOGW(SB_AUDIO_HAL_MODULE_NAME, "mp3 open failed path=%s", path);
        return SB_STATUS_NOT_FOUND;
    }

    size = ql_fsize(fp);
    if (size <= 0) {
        (void)ql_fclose(fp);
        SB_LOGW(SB_AUDIO_HAL_MODULE_NAME, "mp3 invalid size=%d path=%s", size, path);
        return SB_STATUS_FILE_ERROR;
    }

    read_len = ql_fread(head, 1u, (size_t)sizeof(head), fp);
    (void)ql_fclose(fp);
    if (read_len <= 0) {
        SB_LOGW(SB_AUDIO_HAL_MODULE_NAME, "mp3 header read failed path=%s", path);
        return SB_STATUS_FILE_ERROR;
    }

    *file_size_out = (u32)size;
    SB_LOGI(SB_AUDIO_HAL_MODULE_NAME,
            "mp3 probe path=%s size=%u head=%02X %02X %02X %02X",
            path, *file_size_out, head[0], head[1], head[2], head[3]);
    return SB_STATUS_OK;
}

static sb_status_t sb_audio_hal_play_mp3_stream_from_file(const char *path,
                                                          u32 timeout_ms,
                                                          u32 file_size)
{
    QFILE *fp;
    Mp3PlaybackHandle handle = 0u;
    Mp3PlayConfigInfo config;
    unsigned char buffer[SB_AUDIO_HAL_STREAM_CHUNK];
    int ret;
    u32 sleep_ms;

    {
        unsigned char *cfg_bytes = (unsigned char *)&config;
        u32 i;
        for (i = 0u; i < (u32)sizeof(config); i++) {
            cfg_bytes[i] = 0u;
        }
    }
    config.listener = sb_audio_hal_mp3_event_cb;
    config.listener2 = 0;
    config.timeout = 0;
    config.sync_mode = 0;

    fp = ql_fopen(path, "rb");
    if (fp == 0) {
        return SB_STATUS_NOT_FOUND;
    }

    s_mp3_last_status = -1;
    ret = ql_play_mp3(0, &handle, &config);
    if (ret != 0) {
        (void)ql_fclose(fp);
        SB_LOGW(SB_AUDIO_HAL_MODULE_NAME, "mp3 stream open failed ret=%d path=%s", ret, path);
        return SB_STATUS_FILE_ERROR;
    }

    for (;;) {
        int read_len = ql_fread(buffer, 1u, (size_t)sizeof(buffer), fp);
        if (read_len < 0) {
            (void)ql_fclose(fp);
            (void)ql_stop_mp3_play(handle, 0);
            return SB_STATUS_FILE_ERROR;
        }
        if (read_len == 0) {
            break;
        }
        ret = ql_play_mp3_stream_buffer(handle, buffer, read_len);
        if (ret != 0) {
            (void)ql_fclose(fp);
            (void)ql_stop_mp3_play(handle, 0);
            SB_LOGW(SB_AUDIO_HAL_MODULE_NAME, "mp3 stream write failed ret=%d path=%s", ret, path);
            return SB_STATUS_FILE_ERROR;
        }
        ql_rtos_task_sleep_ms(10u);
    }

    (void)ql_fclose(fp);
    sleep_ms = sb_audio_hal_estimate_mp3_ms(file_size);
    if (sleep_ms > timeout_ms) {
        sleep_ms = timeout_ms;
    }
    ql_rtos_task_sleep_ms(sleep_ms);
    (void)ql_stop_mp3_play(handle, 1);
    SB_LOGI(SB_AUDIO_HAL_MODULE_NAME, "mp3 stream playback finished path=%s wait_ms=%u", path, sleep_ms);
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

    (void)ql_audio_play_init(sb_audio_hal_player_cb);
    ql_bind_speakerpa_cb(sb_audio_hal_pa_cb);
    ql_set_audio_path_speaker();
    sb_audio_hal_apply_quec_sample_gain_table();

    pcm_config.pcmbclk = PCM_FS_64;
    pcm_config.sample = PCM_CODEC_SAMPLE_16000;
    pcm_config.is_i2s = 0;
    ql_codec_choose(AUD_EXTERNAL_CODEC, &pcm_config);
    status = sb_audio_codec_es8311_open();
    if (status != SB_STATUS_OK) {
        SB_LOGE(SB_AUDIO_HAL_MODULE_NAME,
                "external codec required on KAE8, status=%s",
                sb_status_to_string(status));
        return status;
    }

    status = sb_audio_hal_set_volume_percent(volume_percent);
    if (status != SB_STATUS_OK) {
        return status;
    }

    s_audio_ready = 1;
    SB_LOGI(SB_AUDIO_HAL_MODULE_NAME, "ready volume=%u", s_volume_percent);
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

static sb_status_t sb_audio_hal_play_mp3_old_file_api(const char *path, u32 timeout_ms, u32 file_size)
{
    int ret;
    QlOSStatus wait_status;

    (void)file_size;
    if ((path == 0) || (path[0] == '\0')) {
        return SB_STATUS_INVALID_PARAM;
    }

    s_old_player_last_state = AUD_PLAYER_ERROR;
    (void)sb_bsp_board_set_speaker_amp(1);
    ret = ql_mp3_file_play((char *)path);
    if (ret == -2) {
        ql_mp3_file_stop();
        ql_rtos_task_sleep_ms(SB_AUDIO_HAL_OLD_API_RETRY_MS);
        ret = ql_mp3_file_play((char *)path);
    }
    if (ret != 0) {
        (void)sb_bsp_board_set_speaker_amp(0);
        SB_LOGW(SB_AUDIO_HAL_MODULE_NAME, "mp3 old file API start failed ret=%d path=%s", ret, path);
        return SB_STATUS_FILE_ERROR;
    }

    wait_status = ql_rtos_semaphore_wait(s_mp3_done_sem, timeout_ms + 1000u);
    ql_mp3_file_stop();
    (void)sb_bsp_board_set_speaker_amp(0);

    if (wait_status != 0) {
        SB_LOGW(SB_AUDIO_HAL_MODULE_NAME, "mp3 old file API wait timeout path=%s", path);
        return SB_STATUS_TIMEOUT;
    }
    if (s_old_player_last_state == AUD_PLAYER_FINISHED) {
        SB_LOGI(SB_AUDIO_HAL_MODULE_NAME, "mp3 old file API finished path=%s", path);
        return SB_STATUS_OK;
    }
    if (s_old_player_last_state == AUD_PLAYER_ERROR) {
        SB_LOGW(SB_AUDIO_HAL_MODULE_NAME, "mp3 old file API decoder error path=%s", path);
        return SB_STATUS_FILE_ERROR;
    }

    SB_LOGI(SB_AUDIO_HAL_MODULE_NAME, "mp3 old file API state=%d path=%s", s_old_player_last_state, path);
    return SB_STATUS_OK;
}

sb_status_t sb_audio_hal_play_mp3_file(const char *path, u32 timeout_ms)
{
    Mp3PlaybackHandle handle = 0u;
    Mp3PlayConfigInfo config;
    int ret;
    QlOSStatus wait_status;
    u32 file_size = 0u;
    sb_status_t status;

    if ((path == 0) || (path[0] == '\0')) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (s_audio_ready == 0) {
        return SB_STATUS_NOT_READY;
    }

    timeout_ms = sb_audio_hal_clamp_timeout_ms(timeout_ms);
    status = sb_audio_hal_probe_mp3_file(path, &file_size);
    if (status != SB_STATUS_OK) {
        return status;
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
    config.timeout = 0;
    config.sync_mode = 0;

    s_mp3_last_status = -1;
    (void)sb_bsp_board_set_speaker_amp(1);

    ret = ql_play_mp3((char *)path, &handle, &config);
    if (ret != 0) {
        SB_LOGW(SB_AUDIO_HAL_MODULE_NAME,
                "mp3 file API start failed ret=%d path=%s, trying old API fallback",
                ret, path);
        status = sb_audio_hal_play_mp3_old_file_api(path, timeout_ms, file_size);
        if (status == SB_STATUS_OK) {
            return SB_STATUS_OK;
        }
        SB_LOGW(SB_AUDIO_HAL_MODULE_NAME,
                "mp3 old file API fallback failed status=%s path=%s, trying stream fallback",
                sb_status_to_string(status), path);
        status = sb_audio_hal_play_mp3_stream_from_file(path, timeout_ms, file_size);
        (void)sb_bsp_board_set_speaker_amp(0);
        if (status == SB_STATUS_OK) {
            return SB_STATUS_OK;
        }
        SB_LOGW(SB_AUDIO_HAL_MODULE_NAME, "mp3 stream fallback failed status=%s path=%s",
                sb_status_to_string(status), path);
        return status;
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

    SB_LOGW(SB_AUDIO_HAL_MODULE_NAME, "mp3 abnormal status=%d path=%s", s_mp3_last_status, path);
    return SB_STATUS_FILE_ERROR;
}

int sb_audio_hal_is_ready(void)
{
    return s_audio_ready;
}
