/*================================================================
 * Static QR UPI Soundbox - Production/Factory Mode Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "sb_crc32.h"
#include "sb_log.h"
#include "sb_mode_service.h"
#include "sb_storage_fs.h"

#define SB_MODE_MODULE_NAME "mode"
#define SB_MODE_MAGIC       (0x53424D44u)
#define SB_MODE_VERSION     (1u)

typedef struct {
    u32 magic;
    u32 version;
    u32 mode;
    u32 crc;
} sb_mode_file_t;

static ql_mutex_t s_mode_mutex = 0;
static sb_device_mode_t s_mode = SB_DEVICE_MODE_PRODUCTION;
static int s_mode_ready = 0;

static u32 sb_mode_crc(sb_mode_file_t *file)
{
    return sb_crc32_compute(file, (u32)((unsigned long)&file->crc - (unsigned long)file));
}

const char *sb_mode_name(sb_device_mode_t mode)
{
    switch (mode) {
    case SB_DEVICE_MODE_FACTORY:
        return "factory";
    case SB_DEVICE_MODE_DEBUG:
        return "debug";
    case SB_DEVICE_MODE_PRODUCTION:
    default:
        return "production";
    }
}

static sb_status_t sb_mode_save_locked(void)
{
    sb_mode_file_t file;

    file.magic = SB_MODE_MAGIC;
    file.version = SB_MODE_VERSION;
    file.mode = (u32)s_mode;
    file.crc = sb_mode_crc(&file);
    return sb_storage_fs_write_file_atomic(SB_MODE_PATH, SB_MODE_TEMP_PATH, &file, (u32)sizeof(file));
}

sb_status_t sb_mode_service_init(void)
{
    sb_mode_file_t file;
    sb_status_t status;

    if (s_mode_ready != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }
    if (ql_rtos_mutex_create(&s_mode_mutex) != 0) {
        return SB_STATUS_NO_MEMORY;
    }

    status = sb_storage_fs_read_file(SB_MODE_PATH, &file, (u32)sizeof(file));
    if ((status == SB_STATUS_OK) && (file.magic == SB_MODE_MAGIC) &&
        (file.version == SB_MODE_VERSION) && (file.crc == sb_mode_crc(&file)) &&
        (file.mode <= (u32)SB_DEVICE_MODE_DEBUG)) {
        s_mode = (sb_device_mode_t)file.mode;
    } else {
        s_mode = SB_DEVICE_MODE_PRODUCTION;
        (void)sb_mode_save_locked();
    }

    s_mode_ready = 1;
    SB_LOGI(SB_MODE_MODULE_NAME, "ready mode=%s", sb_mode_name(s_mode));
    return SB_STATUS_OK;
}

sb_status_t sb_mode_get(sb_device_mode_t *mode)
{
    if (mode == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (s_mode_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_mode_mutex, QL_WAIT_FOREVER);
    }
    *mode = s_mode;
    if (s_mode_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_mode_mutex);
    }
    return SB_STATUS_OK;
}

sb_status_t sb_mode_set(sb_device_mode_t mode)
{
    sb_status_t status;

    if (mode > SB_DEVICE_MODE_DEBUG) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (s_mode_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_mode_mutex, QL_WAIT_FOREVER);
    }
    s_mode = mode;
    status = sb_mode_save_locked();
    if (s_mode_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_mode_mutex);
    }
    return status;
}

int sb_mode_factory_access_allowed(void)
{
#ifdef SB_FACTORY_SERIAL_DEFAULT_ENABLED
    return 1;
#else
    sb_device_mode_t mode;
    if (sb_mode_get(&mode) != SB_STATUS_OK) {
        return 0;
    }
    return (mode != SB_DEVICE_MODE_PRODUCTION) ? 1 : 0;
#endif
}
