/*================================================================
 * Static QR UPI Soundbox - Audio Asset Store Abstraction
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *
 * Production assets are addressed by logical paths such as
 * audio/en/alerts/no_mqtt.mp3. If a raw external-NOR asset pack is
 * present, only the selected asset is staged to U:/sb_play.mp3 before
 * playback because QuecOpen MP3 playback accepts filesystem paths. If the
 * external NOR is absent or not yet provisioned, U:/audio is used only as a
 * debug/small-asset fallback; full asset libraries must not be copied to U:.
 *================================================================*/
#include "ql_fs.h"
#include "sb_audio_asset_store.h"
#include "sb_cloud_utils.h"
#include "sb_crc32.h"
#include "sb_extnor.h"
#include "sb_log.h"

#define SB_AUDIO_STORE_MODULE_NAME       "asset_store"
#define SB_AUDIO_STORE_PACK_MAGIC        (0x53424153u) /* "SBAS" */
#define SB_AUDIO_STORE_PACK_HEADER_BYTES (12u)
#define SB_AUDIO_STORE_INDEX_ENTRY_BYTES (112u)
#define SB_AUDIO_STORE_INDEX_PATH_BYTES  (96u)
#define SB_AUDIO_STORE_MAX_INDEX_FILES   (256u)
#define SB_AUDIO_STORE_READ_CHUNK_BYTES  (512u)

typedef struct {
    char path[SB_AUDIO_STORE_INDEX_PATH_BYTES];
    u32 offset;
    u32 length;
    u32 crc32;
} sb_audio_store_entry_t;

static sb_audio_store_status_t s_store_status;

static void sb_store_zero(void *data, u32 length)
{
    u32 i;
    unsigned char *p = (unsigned char *)data;

    if (p == 0) {
        return;
    }
    for (i = 0u; i < length; i++) {
        p[i] = 0u;
    }
}

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

static sb_status_t sb_store_read_u32(u32 address, u32 *value)
{
    unsigned char raw[4];
    sb_status_t status;

    if (value == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    status = sb_extnor_read(address, raw, (u32)sizeof(raw));
    if (status != SB_STATUS_OK) {
        return status;
    }
    *value = ((u32)raw[0]) | (((u32)raw[1]) << 8) | (((u32)raw[2]) << 16) | (((u32)raw[3]) << 24);
    return SB_STATUS_OK;
}

static int sb_store_entry_path_equal(const char *a, const char *b)
{
    u32 i;

    if ((a == 0) || (b == 0)) {
        return 0;
    }
    for (i = 0u; i < SB_AUDIO_STORE_INDEX_PATH_BYTES; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
        if (a[i] == '\0') {
            return 1;
        }
    }
    return (b[SB_AUDIO_STORE_INDEX_PATH_BYTES - 1u] == '\0') ? 1 : 0;
}

static sb_status_t sb_store_find_extnor_entry(const char *logical_path, sb_audio_store_entry_t *entry)
{
    u32 magic;
    u32 version;
    u32 count;
    u32 i;
    u32 address;
    unsigned char raw[SB_AUDIO_STORE_INDEX_ENTRY_BYTES];
    sb_status_t status;

    if ((logical_path == 0) || (entry == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (s_store_status.extnor_available == 0) {
        return SB_STATUS_NOT_FOUND;
    }

    status = sb_store_read_u32(0u, &magic);
    if (status == SB_STATUS_OK) {
        status = sb_store_read_u32(4u, &version);
    }
    if (status == SB_STATUS_OK) {
        status = sb_store_read_u32(8u, &count);
    }
    if ((status != SB_STATUS_OK) || (magic != SB_AUDIO_STORE_PACK_MAGIC) ||
        (version == 0u) || (count == 0u) || (count > SB_AUDIO_STORE_MAX_INDEX_FILES)) {
        return SB_STATUS_NOT_FOUND;
    }

    for (i = 0u; i < count; i++) {
        address = SB_AUDIO_STORE_PACK_HEADER_BYTES + (i * SB_AUDIO_STORE_INDEX_ENTRY_BYTES);
        status = sb_extnor_read(address, raw, (u32)sizeof(raw));
        if (status != SB_STATUS_OK) {
            return status;
        }
        sb_store_zero(entry, (u32)sizeof(*entry));
        sb_store_copy(entry->path, (u32)sizeof(entry->path), (const char *)raw);
        entry->offset = ((u32)raw[96]) | (((u32)raw[97]) << 8) | (((u32)raw[98]) << 16) | (((u32)raw[99]) << 24);
        entry->length = ((u32)raw[100]) | (((u32)raw[101]) << 8) | (((u32)raw[102]) << 16) | (((u32)raw[103]) << 24);
        entry->crc32 = ((u32)raw[104]) | (((u32)raw[105]) << 8) | (((u32)raw[106]) << 16) | (((u32)raw[107]) << 24);
        if (sb_store_entry_path_equal(entry->path, logical_path) != 0) {
            if ((entry->length == 0u) || (entry->length > SB_AUDIO_STORE_MAX_STAGE_BYTES)) {
                return SB_STATUS_INVALID_PARAM;
            }
            return SB_STATUS_OK;
        }
    }
    return SB_STATUS_NOT_FOUND;
}

static sb_status_t sb_store_stage_extnor_entry(const sb_audio_store_entry_t *entry,
                                               char *play_path,
                                               u32 play_path_len)
{
    QFILE *file;
    unsigned char chunk[SB_AUDIO_STORE_READ_CHUNK_BYTES];
    u32 remaining;
    u32 offset;
    u32 crc;
    sb_status_t status;

    if ((entry == 0) || (play_path == 0) || (play_path_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    if ((entry->length == 0u) || (entry->length > SB_AUDIO_STORE_MAX_STAGE_BYTES)) {
        return SB_STATUS_INVALID_PARAM;
    }

    (void)ql_remove(SB_AUDIO_STORE_STAGE_PATH);
    file = ql_fopen(SB_AUDIO_STORE_STAGE_PATH, "w+");
    if (file == 0) {
        return SB_STATUS_FILE_ERROR;
    }

    remaining = entry->length;
    offset = 0u;
    crc = SB_CRC32_INITIAL_VALUE;
    while (remaining != 0u) {
        u32 chunk_len = (remaining > SB_AUDIO_STORE_READ_CHUNK_BYTES) ? SB_AUDIO_STORE_READ_CHUNK_BYTES : remaining;
        status = sb_extnor_read(entry->offset + offset, chunk, chunk_len);
        if (status != SB_STATUS_OK) {
            (void)ql_fclose(file);
            (void)ql_remove(SB_AUDIO_STORE_STAGE_PATH);
            return status;
        }
        if (ql_fwrite(chunk, chunk_len, 1u, file) != 1) {
            (void)ql_fclose(file);
            (void)ql_remove(SB_AUDIO_STORE_STAGE_PATH);
            return SB_STATUS_FILE_ERROR;
        }
        crc = sb_crc32_update(crc, chunk, chunk_len);
        remaining -= chunk_len;
        offset += chunk_len;
    }
    crc ^= 0xFFFFFFFFu;
    if ((entry->crc32 != 0u) && (entry->crc32 != crc)) {
        (void)ql_fclose(file);
        (void)ql_remove(SB_AUDIO_STORE_STAGE_PATH);
        return SB_STATUS_CRC_ERROR;
    }
    (void)ql_fsync(file);
    (void)ql_fclose(file);

    sb_store_copy(play_path, play_path_len, SB_AUDIO_STORE_STAGE_PATH);
    s_store_status.staged_count++;
    return SB_STATUS_OK;
}

const char *sb_audio_asset_store_backend_name(sb_audio_store_backend_t backend)
{
    switch (backend) {
    case SB_AUDIO_STORE_BACKEND_EXTNOR:
        return "extnor";
    case SB_AUDIO_STORE_BACKEND_UFS:
    default:
        return "ufs";
    }
}

sb_status_t sb_audio_asset_store_init(void)
{
    sb_extnor_info_t extnor_info;
    sb_status_t status;

    s_store_status.ready = 1;
    s_store_status.extnor_available = 0;
    s_store_status.missing_count = 0u;
    s_store_status.staged_count = 0u;
    s_store_status.backend = SB_AUDIO_STORE_BACKEND_UFS;

    status = sb_extnor_get_info(&extnor_info);
    if ((status == SB_STATUS_OK) && (extnor_info.ready != 0)) {
        s_store_status.extnor_available = 1;
        s_store_status.backend = SB_AUDIO_STORE_BACKEND_EXTNOR;
        SB_LOGI(SB_AUDIO_STORE_MODULE_NAME, "ready backend=extnor capacity=%u", extnor_info.capacity_bytes);
    } else {
        SB_LOGW(SB_AUDIO_STORE_MODULE_NAME, "external nor unavailable, using U: debug fallback");
    }
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
    char ufs_path[SB_AUDIO_PATH_LEN];
    sb_audio_store_entry_t entry;

    if (sb_store_logical_to_ufs(logical_path, ufs_path, (u32)sizeof(ufs_path)) != SB_STATUS_OK) {
        return 0;
    }
    if (ql_access(ufs_path, 0u) == 0) {
        return 1;
    }
    if (sb_store_find_extnor_entry(logical_path, &entry) == SB_STATUS_OK) {
        return 1;
    }
    return 0;
}

sb_status_t sb_audio_asset_store_prepare_play_path(const char *logical_path,
                                                    char *play_path,
                                                    u32 play_path_len)
{
    char ufs_path[SB_AUDIO_PATH_LEN];
    sb_audio_store_entry_t entry;
    sb_status_t status;

    if ((logical_path == 0) || (play_path == 0) || (play_path_len == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_store_logical_to_ufs(logical_path, ufs_path, (u32)sizeof(ufs_path));
    if (status != SB_STATUS_OK) {
        return status;
    }

    if (ql_access(ufs_path, 0u) == 0) {
        sb_store_copy(play_path, play_path_len, ufs_path);
        return SB_STATUS_OK;
    }

    status = sb_store_find_extnor_entry(logical_path, &entry);
    if (status == SB_STATUS_OK) {
        return sb_store_stage_extnor_entry(&entry, play_path, play_path_len);
    }

    s_store_status.missing_count++;
    return SB_STATUS_NOT_FOUND;
}
