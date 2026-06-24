/*================================================================
 * Static QR UPI Soundbox - External NOR Audio Manifest
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "sb_audio_asset_manifest.h"
#include "sb_board_kae8_sq1.h"
#include "sb_cloud_utils.h"
#include "sb_crc32.h"
#include "sb_ext_nor_flash.h"
#include "sb_log.h"

#define SB_AUDIO_MANIFEST_MODULE_NAME "asset_manifest"

static sb_audio_manifest_header_t s_header;
static sb_audio_manifest_entry_t s_entries[SB_AUDIO_MANIFEST_MAX_ENTRIES];
static sb_audio_manifest_status_t s_status;

static void sb_manifest_zero(void *ptr, u32 length)
{
    u32 i;
    unsigned char *p = (unsigned char *)ptr;
    if (p == 0) {
        return;
    }
    for (i = 0u; i < length; i++) {
        p[i] = 0u;
    }
}

static int sb_manifest_text_equal(const char *a, const char *b)
{
    return sb_cloud_text_equal(a, b);
}

static sb_status_t sb_manifest_read_entries(u32 entry_count)
{
    u32 bytes = entry_count * (u32)sizeof(sb_audio_manifest_entry_t);
    u32 crc_calc;

    if (entry_count > SB_AUDIO_MANIFEST_MAX_ENTRIES) {
        return SB_STATUS_INVALID_STATE;
    }
    sb_manifest_zero(s_entries, (u32)sizeof(s_entries));
    if (bytes == 0u) {
        return SB_STATUS_NOT_FOUND;
    }
    if (sb_ext_nor_flash_read(SB_KAE8_NOR_MANIFEST_ADDR + (u32)sizeof(sb_audio_manifest_header_t),
                              s_entries, bytes) != SB_STATUS_OK) {
        return SB_STATUS_FLASH_ERROR;
    }
    crc_calc = sb_crc32_compute(s_entries, bytes);
    if (crc_calc != s_header.manifest_crc32) {
        SB_LOGE(SB_AUDIO_MANIFEST_MODULE_NAME, "crc mismatch calc=%u stored=%u", crc_calc, s_header.manifest_crc32);
        return SB_STATUS_CRC_ERROR;
    }
    return SB_STATUS_OK;
}

sb_status_t sb_audio_manifest_init(void)
{
    sb_status_t status;

    if (s_status.ready != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }
    sb_manifest_zero(&s_header, (u32)sizeof(s_header));
    sb_manifest_zero(&s_status, (u32)sizeof(s_status));
    if (sb_ext_nor_flash_is_ready() == 0) {
        s_status.last_error = (u32)SB_STATUS_NOT_READY;
        return SB_STATUS_NOT_READY;
    }
    status = sb_ext_nor_flash_read(SB_KAE8_NOR_MANIFEST_ADDR, &s_header, (u32)sizeof(s_header));
    if (status != SB_STATUS_OK) {
        s_status.last_error = (u32)status;
        return status;
    }
    if ((s_header.magic != SB_AUDIO_MANIFEST_MAGIC) ||
        (s_header.version != SB_AUDIO_MANIFEST_VERSION) ||
        (s_header.header_size != (u32)sizeof(sb_audio_manifest_header_t)) ||
        (s_header.entry_size != (u32)sizeof(sb_audio_manifest_entry_t)) ||
        (s_header.assets_base_addr != SB_KAE8_NOR_ASSET_BASE_ADDR)) {
        SB_LOGW(SB_AUDIO_MANIFEST_MODULE_NAME,
                "manifest invalid magic=%u version=%u entries=%u",
                s_header.magic, s_header.version, s_header.entry_count);
        s_status.last_error = (u32)SB_STATUS_INVALID_STATE;
        return SB_STATUS_INVALID_STATE;
    }
    status = sb_manifest_read_entries(s_header.entry_count);
    if (status != SB_STATUS_OK) {
        s_status.last_error = (u32)status;
        return status;
    }
    s_status.ready = 1;
    s_status.entry_count = s_header.entry_count;
    s_status.last_error = 0u;
    SB_LOGI(SB_AUDIO_MANIFEST_MODULE_NAME, "ready entries=%u", s_status.entry_count);
    return SB_STATUS_OK;
}

sb_status_t sb_audio_manifest_get_status(sb_audio_manifest_status_t *status)
{
    if (status == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    *status = s_status;
    return SB_STATUS_OK;
}

sb_status_t sb_audio_manifest_find(const char *logical_path, sb_audio_manifest_entry_t *entry)
{
    u32 i;

    if ((logical_path == 0) || (entry == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (s_status.ready == 0) {
        return SB_STATUS_NOT_READY;
    }
    for (i = 0u; i < s_status.entry_count; i++) {
        if (sb_manifest_text_equal(s_entries[i].logical_path, logical_path) != 0) {
            *entry = s_entries[i];
            return SB_STATUS_OK;
        }
    }
    return SB_STATUS_NOT_FOUND;
}

static sb_status_t sb_manifest_try_file_variant(const char *logical_path,
                                                const char *from,
                                                const char *to,
                                                sb_audio_manifest_entry_t *entry)
{
    char path[SB_AUDIO_MANIFEST_PATH_LEN];
    u32 i;
    u32 j;
    u32 from_len = sb_cloud_str_len(from);

    if ((logical_path == 0) || (from == 0) || (to == 0) || (entry == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    path[0] = '\0';
    for (i = 0u; logical_path[i] != '\0'; i++) {
        int match = 1;
        for (j = 0u; j < from_len; j++) {
            if (logical_path[i + j] != from[j]) {
                match = 0;
                break;
            }
        }
        if (match != 0) {
            break;
        }
    }
    if (logical_path[i] == '\0') {
        return SB_STATUS_NOT_FOUND;
    }
    for (j = 0u; (j < i) && (j + 1u < (u32)sizeof(path)); j++) {
        path[j] = logical_path[j];
    }
    path[j] = '\0';
    if (sb_cloud_append_string(path, (u32)sizeof(path), to) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(path, (u32)sizeof(path), logical_path + i + from_len) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    return sb_audio_manifest_find(path, entry);
}

sb_status_t sb_audio_manifest_find_with_fallback(const char *logical_path, sb_audio_manifest_entry_t *entry)
{
    sb_status_t status;
    char english_path[SB_AUDIO_MANIFEST_PATH_LEN];
    const char *slash1;
    const char *slash2;

    status = sb_audio_manifest_find(logical_path, entry);
    if (status == SB_STATUS_OK) {
        return status;
    }
    status = sb_manifest_try_file_variant(logical_path, "transaction.mp3", "transaction_s.mp3", entry);
    if (status == SB_STATUS_OK) { return status; }
    status = sb_manifest_try_file_variant(logical_path, "transaction_s.mp3", "transaction.mp3", entry);
    if (status == SB_STATUS_OK) { return status; }
    status = sb_manifest_try_file_variant(logical_path, "rupees.mp3", "ruppes.mp3", entry);
    if (status == SB_STATUS_OK) { return status; }
    status = sb_manifest_try_file_variant(logical_path, "transaction.mp3", "trasnsactions.mp3", entry);
    if (status == SB_STATUS_OK) { return status; }
    status = sb_manifest_try_file_variant(logical_path, "transaction_s.mp3", "trasnsactions.mp3", entry);
    if (status == SB_STATUS_OK) { return status; }

    if (sb_cloud_has_prefix(logical_path, "audio/") == 0) {
        return SB_STATUS_NOT_FOUND;
    }
    slash1 = logical_path + 6;
    slash2 = slash1;
    while ((*slash2 != '\0') && (*slash2 != '/')) {
        slash2++;
    }
    if ((*slash2 != '/') || (*(slash2 + 1) == '\0')) {
        return SB_STATUS_NOT_FOUND;
    }
    english_path[0] = '\0';
    if (sb_cloud_append_string(english_path, (u32)sizeof(english_path), "audio/en/") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(english_path, (u32)sizeof(english_path), slash2 + 1) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    return sb_audio_manifest_find(english_path, entry);
}
