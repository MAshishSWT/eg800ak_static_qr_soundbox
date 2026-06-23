/*================================================================
 * Static QR UPI Soundbox - UFS-Only Audio Asset Store
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_fs.h"
#include "sb_audio_asset_store.h"
#include "sb_cloud_utils.h"
#include "sb_log.h"

#define SB_AUDIO_STORE_MODULE_NAME "asset_store"

static sb_audio_store_status_t s_store_status;

static void sb_store_copy(char *dst, u32 dst_len, const char *src)
{
    sb_cloud_copy_string(dst, dst_len, src);
}

static int sb_store_is_ufs_path(const char *path)
{
    return sb_cloud_has_prefix(path, "U:/");
}

static int sb_store_is_logical_audio_path(const char *path)
{
    return sb_cloud_has_prefix(path, SB_AUDIO_STORE_LOGICAL_ROOT "/");
}

static sb_status_t sb_store_logical_to_ufs(const char *logical_path,
                                           char *ufs_path,
                                           u32 ufs_path_len)
{
    if ((logical_path == 0) || (ufs_path == 0) || (ufs_path_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    ufs_path[0] = '\0';
    if (sb_store_is_ufs_path(logical_path) != 0) {
        sb_store_copy(ufs_path, ufs_path_len, logical_path);
        return SB_STATUS_OK;
    }
    if (sb_store_is_logical_audio_path(logical_path) == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (sb_cloud_append_string(ufs_path, ufs_path_len, "U:/") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    return sb_cloud_append_string(ufs_path, ufs_path_len, logical_path);
}

const char *sb_audio_asset_store_backend_name(sb_audio_store_backend_t backend)
{
    (void)backend;
    return "ufs";
}

sb_status_t sb_audio_asset_store_init(void)
{
    s_store_status.ready = 1;
    s_store_status.missing_count = 0u;
    s_store_status.backend = SB_AUDIO_STORE_BACKEND_UFS;
    SB_LOGI(SB_AUDIO_STORE_MODULE_NAME, "ready backend=ufs root=%s", SB_AUDIO_STORE_UFS_ROOT);
    return SB_STATUS_OK;
}

sb_status_t sb_audio_asset_store_get_status(sb_audio_store_status_t *status)
{
    if (status == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    *status = s_store_status;
    return SB_STATUS_OK;
}

int sb_audio_asset_store_exists(const char *logical_path)
{
    char path[SB_AUDIO_PATH_LEN];

    if (sb_store_logical_to_ufs(logical_path, path, (u32)sizeof(path)) == SB_STATUS_OK) {
        return (ql_access(path, 0u) == 0) ? 1 : 0;
    }
    return 0;
}

sb_status_t sb_audio_asset_store_prepare_play_path(const char *logical_path,
                                                    char *play_path,
                                                    u32 play_path_len)
{
    char path[SB_AUDIO_PATH_LEN];
    sb_status_t status;

    if ((logical_path == 0) || (play_path == 0) || (play_path_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_store_logical_to_ufs(logical_path, path, (u32)sizeof(path));
    if (status != SB_STATUS_OK) {
        return status;
    }
    if (ql_access(path, 0u) == 0) {
        sb_store_copy(play_path, play_path_len, path);
        return SB_STATUS_OK;
    }

    s_store_status.missing_count++;
    return SB_STATUS_NOT_FOUND;
}
