/*================================================================
 * Static QR UPI Soundbox - Two-tier Audio Asset Store
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_fs.h"
#include "sb_audio_asset_manifest.h"
#include "sb_audio_asset_store.h"
#include "sb_cloud_utils.h"
#include "sb_crc32.h"
#include "sb_ext_nor_flash.h"
#include "sb_log.h"
#include "sb_storage_fs.h"

#define SB_AUDIO_STORE_MODULE_NAME       "asset_store"
#define SB_AUDIO_STORE_COPY_CHUNK        (1024u)

static sb_audio_store_status_t s_store_status;

static sb_status_t sb_store_common_to_ufs(const char *logical_path, char *ufs_path, u32 ufs_path_len);

static void sb_store_copy(char *dst, u32 dst_len, const char *src)
{
    sb_cloud_copy_string(dst, dst_len, src);
}

static int sb_store_ufs_file_present(const char *path, u32 *size_out)
{
    QFILE *fp;
    int size;

    if (size_out != 0) {
        *size_out = 0u;
    }
    if ((path == 0) || (path[0] == '\0')) {
        return 0;
    }

    /* ql_access() may return stale/not-mounted results on some EG800AK
     * LittleFS builds immediately after factory serial upload. A real fopen
     * plus fsize is a stronger proof that the audio decoder can consume it.
     */
    fp = ql_fopen(path, "rb");
    if (fp == 0) {
        return 0;
    }
    size = ql_fsize(fp);
    (void)ql_fclose(fp);
    if (size <= 0) {
        return 0;
    }
    if (size_out != 0) {
        *size_out = (u32)size;
    }
    return 1;
}

static void sb_store_log_common_files(void)
{
    static const char *const files[] = {
        "start_tune.mp3",
        "ping.mp3",
        "good_bye.mp3",
        "transaction_error.mp3"
    };
    u32 i;

    for (i = 0u; i < (u32)(sizeof(files) / sizeof(files[0])); i++) {
        char path[SB_AUDIO_PATH_LEN];
        u32 size = 0u;
        if (sb_store_common_to_ufs(files[i], path, (u32)sizeof(path)) != SB_STATUS_OK) {
            continue;
        }
        if (sb_store_ufs_file_present(path, &size) != 0) {
            SB_LOGI(SB_AUDIO_STORE_MODULE_NAME, "common asset present path=%s size=%u", path, size);
        } else {
            SB_LOGW(SB_AUDIO_STORE_MODULE_NAME, "common asset missing path=%s", path);
        }
    }
}

static int sb_store_is_common_root_file(const char *logical_path)
{
    if (logical_path == 0) {
        return 0;
    }
    if ((sb_cloud_text_equal(logical_path, "start_tune.mp3") != 0) ||
        (sb_cloud_text_equal(logical_path, "ping.mp3") != 0) ||
        (sb_cloud_text_equal(logical_path, "good_bye.mp3") != 0) ||
        (sb_cloud_text_equal(logical_path, "transaction_error.mp3") != 0)) {
        return 1;
    }
    return 0;
}

static sb_status_t sb_store_common_to_ufs(const char *logical_path, char *ufs_path, u32 ufs_path_len)
{
    if ((logical_path == 0) || (ufs_path == 0) || (ufs_path_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (sb_store_is_common_root_file(logical_path) == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    ufs_path[0] = '\0';
    if (sb_cloud_append_string(ufs_path, ufs_path_len, "U:/") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    return sb_cloud_append_string(ufs_path, ufs_path_len, logical_path);
}

const char *sb_audio_asset_store_backend_name(sb_audio_store_backend_t backend)
{
    switch (backend) {
    case SB_AUDIO_STORE_BACKEND_EXT_NOR:
        return "ext_nor";
    case SB_AUDIO_STORE_BACKEND_UFS_ROOT:
    default:
        return "ufs_root";
    }
}

sb_status_t sb_audio_asset_store_init(void)
{
    sb_status_t status;
    s_store_status.ready = 1;
    s_store_status.nor_ready = sb_ext_nor_flash_is_ready();
    s_store_status.manifest_ready = 0;
    s_store_status.missing_count = 0u;
    s_store_status.backend = SB_AUDIO_STORE_BACKEND_EXT_NOR;

    status = sb_audio_manifest_init();
    if ((status == SB_STATUS_OK) || (status == SB_STATUS_ALREADY_INITIALIZED)) {
        s_store_status.manifest_ready = 1;
    } else {
        SB_LOGW(SB_AUDIO_STORE_MODULE_NAME, "manifest init status=%s", sb_status_to_string(status));
    }
    SB_LOGI(SB_AUDIO_STORE_MODULE_NAME, "ready common=%s lang=%s nor=%d manifest=%d",
            SB_AUDIO_STORE_UFS_ROOT, SB_AUDIO_STORE_EXT_ROOT,
            s_store_status.nor_ready, s_store_status.manifest_ready);
    sb_store_log_common_files();
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
    sb_audio_manifest_entry_t entry;

    if (sb_store_common_to_ufs(logical_path, path, (u32)sizeof(path)) == SB_STATUS_OK) {
        return sb_store_ufs_file_present(path, 0);
    }
    if (sb_audio_manifest_find_with_fallback(logical_path, &entry) == SB_STATUS_OK) {
        return 1;
    }
    return 0;
}

static sb_status_t sb_store_copy_nor_to_cache(const sb_audio_manifest_entry_t *entry)
{
    static unsigned char chunk[SB_AUDIO_STORE_COPY_CHUNK];
    QFILE *fp;
    u32 offset = 0u;
    u32 crc = 0u;
    sb_status_t status = SB_STATUS_OK;

    if (entry == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (sb_storage_fs_mkdir_recursive(SB_STORAGE_CACHE_DIR) != SB_STATUS_OK) {
        return SB_STATUS_FILE_ERROR;
    }
    (void)ql_remove(SB_STORAGE_AUDIO_CACHE_PATH);
    fp = ql_fopen(SB_STORAGE_AUDIO_CACHE_PATH, "w+");
    if (fp == 0) {
        return SB_STATUS_FILE_ERROR;
    }
    while (offset < entry->length) {
        u32 copy_len = entry->length - offset;
        int written;
        if (copy_len > SB_AUDIO_STORE_COPY_CHUNK) {
            copy_len = SB_AUDIO_STORE_COPY_CHUNK;
        }
        status = sb_ext_nor_flash_read(entry->offset + offset, chunk, copy_len);
        if (status != SB_STATUS_OK) {
            break;
        }
        written = ql_fwrite(chunk, 1u, copy_len, fp);
        if (written != (int)copy_len) {
            status = SB_STATUS_FILE_ERROR;
            break;
        }
        offset += copy_len;
    }
    if (status == SB_STATUS_OK) {
        if (ql_fsync(fp) != 0) {
            status = SB_STATUS_FILE_ERROR;
        }
    }
    if (ql_fclose(fp) != 0) {
        status = SB_STATUS_FILE_ERROR;
    }
    if (status != SB_STATUS_OK) {
        (void)ql_remove(SB_STORAGE_AUDIO_CACHE_PATH);
        return status;
    }

    /* Verify cache by reading it back in chunks with running CRC over the same data. */
    {
        QFILE *rfp = ql_fopen(SB_STORAGE_AUDIO_CACHE_PATH, "r");
        if (rfp == 0) {
            return SB_STATUS_FILE_ERROR;
        }
        crc = 0xFFFFFFFFu;
        while (1) {
            int n = ql_fread(chunk, 1u, (u32)sizeof(chunk), rfp);
            if (n < 0) {
                (void)ql_fclose(rfp);
                return SB_STATUS_FILE_ERROR;
            }
            if (n == 0) {
                break;
            }
            crc = sb_crc32_update(crc, chunk, (u32)n);
        }
        (void)ql_fclose(rfp);
        crc ^= 0xFFFFFFFFu;
    }
    if (crc != entry->crc32) {
        (void)ql_remove(SB_STORAGE_AUDIO_CACHE_PATH);
        SB_LOGW(SB_AUDIO_STORE_MODULE_NAME, "cache crc mismatch path=%s calc=%u expected=%u",
                entry->logical_path, crc, entry->crc32);
        return SB_STATUS_CRC_ERROR;
    }
    return SB_STATUS_OK;
}

sb_status_t sb_audio_asset_store_prepare_play_path(const char *logical_path,
                                                   char *play_path,
                                                   u32 play_path_len)
{
    char ufs_path[SB_AUDIO_PATH_LEN];
    sb_audio_manifest_entry_t entry;
    sb_status_t status;

    if ((logical_path == 0) || (play_path == 0) || (play_path_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    play_path[0] = '\0';
    status = sb_store_common_to_ufs(logical_path, ufs_path, (u32)sizeof(ufs_path));
    if (status == SB_STATUS_OK) {
        u32 size = 0u;
        if (sb_store_ufs_file_present(ufs_path, &size) != 0) {
            SB_LOGI(SB_AUDIO_STORE_MODULE_NAME, "prepare common path=%s size=%u", ufs_path, size);
            sb_store_copy(play_path, play_path_len, ufs_path);
            s_store_status.backend = SB_AUDIO_STORE_BACKEND_UFS_ROOT;
            return SB_STATUS_OK;
        }
        SB_LOGW(SB_AUDIO_STORE_MODULE_NAME, "common asset not found path=%s", ufs_path);
        s_store_status.missing_count++;
        return SB_STATUS_NOT_FOUND;
    }
    status = sb_audio_manifest_find_with_fallback(logical_path, &entry);
    if (status != SB_STATUS_OK) {
        s_store_status.missing_count++;
        return status;
    }
    status = sb_store_copy_nor_to_cache(&entry);
    if (status != SB_STATUS_OK) {
        s_store_status.missing_count++;
        return status;
    }
    sb_store_copy(play_path, play_path_len, SB_STORAGE_AUDIO_CACHE_PATH);
    s_store_status.backend = SB_AUDIO_STORE_BACKEND_EXT_NOR;
    return SB_STATUS_OK;
}
