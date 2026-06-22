/*================================================================
 * Static QR UPI Soundbox - External NOR Audio Asset Pack Loader
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *
 * This module writes a complete SBAS raw audio asset pack to the KAE8
 * external W25Q64 NOR. The pack preserves folder paths in its index and is
 * later consumed by sb_audio_asset_store without mounting a filesystem.
 *================================================================*/
#include "ql_ftp_client.h"
#include "sb_asset_pack_loader.h"
#include "sb_crc32.h"
#include "sb_extnor.h"
#include "sb_log.h"

#define SB_ASSET_PACK_MODULE_NAME "asset_loader"

typedef struct {
    u32 received;
    u32 expected_size;
    u32 expected_crc32;
    u32 crc;
    sb_status_t status;
} sb_asset_ftp_ctx_t;

static sb_asset_pack_loader_status_t s_loader;

static void sb_asset_loader_set_error(sb_status_t status)
{
    s_loader.last_error = (u32)((s32)status);
}

static int sb_asset_hex_value(char c)
{
    if ((c >= '0') && (c <= '9')) {
        return (int)(c - '0');
    }
    if ((c >= 'a') && (c <= 'f')) {
        return (int)(c - 'a' + 10);
    }
    if ((c >= 'A') && (c <= 'F')) {
        return (int)(c - 'A' + 10);
    }
    return -1;
}

static u32 sb_asset_text_len(const char *text)
{
    u32 len = 0u;

    if (text == 0) {
        return 0u;
    }
    while (text[len] != '\0') {
        len++;
    }
    return len;
}

static sb_status_t sb_asset_pack_erase(u32 bytes)
{
    u32 address;
    u32 erase_bytes;
    sb_status_t status;

    if (bytes == 0u) {
        return SB_STATUS_INVALID_PARAM;
    }
    erase_bytes = (bytes + (SB_EXTNOR_SECTOR_SIZE_BYTES - 1u)) & ~(SB_EXTNOR_SECTOR_SIZE_BYTES - 1u);
    for (address = 0u; address < erase_bytes; address += SB_EXTNOR_SECTOR_SIZE_BYTES) {
        status = sb_extnor_erase_sector(address);
        if (status != SB_STATUS_OK) {
            SB_LOGW(SB_ASSET_PACK_MODULE_NAME, "erase failed addr=%u status=%s", address, sb_status_to_string(status));
            return status;
        }
    }
    return SB_STATUS_OK;
}

sb_status_t sb_asset_pack_loader_init(void)
{
    s_loader.active = 0;
    s_loader.expected_size = 0u;
    s_loader.expected_crc32 = 0u;
    s_loader.received_size = 0u;
    s_loader.running_crc32 = SB_CRC32_INITIAL_VALUE;
    s_loader.finalized_crc32 = 0u;
    s_loader.last_error = 0u;
    return SB_STATUS_OK;
}

sb_status_t sb_asset_pack_loader_begin(u32 expected_size, u32 expected_crc32, u32 erase)
{
    sb_extnor_info_t info;
    sb_status_t status;

    if ((expected_size == 0u) || (expected_size > SB_ASSET_PACK_MAX_BYTES)) {
        sb_asset_loader_set_error(SB_STATUS_INVALID_PARAM);
        return SB_STATUS_INVALID_PARAM;
    }
    status = sb_extnor_get_info(&info);
    if ((status != SB_STATUS_OK) || (info.ready == 0)) {
        sb_asset_loader_set_error(SB_STATUS_NOT_READY);
        return SB_STATUS_NOT_READY;
    }
    if ((info.capacity_bytes != 0u) && (expected_size > info.capacity_bytes)) {
        sb_asset_loader_set_error(SB_STATUS_INVALID_PARAM);
        return SB_STATUS_INVALID_PARAM;
    }
    if (erase != 0u) {
        status = sb_asset_pack_erase(expected_size);
        if (status != SB_STATUS_OK) {
            sb_asset_loader_set_error(status);
            return status;
        }
    }

    s_loader.active = 1;
    s_loader.expected_size = expected_size;
    s_loader.expected_crc32 = expected_crc32;
    s_loader.received_size = 0u;
    s_loader.running_crc32 = SB_CRC32_INITIAL_VALUE;
    s_loader.finalized_crc32 = 0u;
    s_loader.last_error = 0u;
    SB_LOGI(SB_ASSET_PACK_MODULE_NAME, "begin size=%u crc32=%08x erase=%u", expected_size, expected_crc32, erase);
    return SB_STATUS_OK;
}

sb_status_t sb_asset_pack_loader_write(u32 offset, const u8 *data, u32 length)
{
    sb_status_t status;

    if ((data == 0) || (length == 0u)) {
        sb_asset_loader_set_error(SB_STATUS_INVALID_PARAM);
        return SB_STATUS_INVALID_PARAM;
    }
    if (s_loader.active == 0) {
        sb_asset_loader_set_error(SB_STATUS_INVALID_STATE);
        return SB_STATUS_INVALID_STATE;
    }
    if (offset != s_loader.received_size) {
        sb_asset_loader_set_error(SB_STATUS_INVALID_PARAM);
        return SB_STATUS_INVALID_PARAM;
    }
    if (length > (s_loader.expected_size - s_loader.received_size)) {
        sb_asset_loader_set_error(SB_STATUS_INVALID_PARAM);
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_extnor_write(offset, data, length);
    if (status != SB_STATUS_OK) {
        sb_asset_loader_set_error(status);
        return status;
    }
    s_loader.running_crc32 = sb_crc32_update(s_loader.running_crc32, data, length);
    s_loader.received_size += length;
    return SB_STATUS_OK;
}

sb_status_t sb_asset_pack_loader_end(void)
{
    u32 magic;
    u8 header[SB_ASSET_PACK_HEADER_BYTES];
    sb_status_t status;

    if (s_loader.active == 0) {
        sb_asset_loader_set_error(SB_STATUS_INVALID_STATE);
        return SB_STATUS_INVALID_STATE;
    }
    if (s_loader.received_size != s_loader.expected_size) {
        sb_asset_loader_set_error(SB_STATUS_INVALID_STATE);
        return SB_STATUS_INVALID_STATE;
    }

    s_loader.finalized_crc32 = s_loader.running_crc32 ^ 0xFFFFFFFFu;
    if ((s_loader.expected_crc32 != 0u) && (s_loader.finalized_crc32 != s_loader.expected_crc32)) {
        s_loader.active = 0;
        sb_asset_loader_set_error(SB_STATUS_CRC_ERROR);
        return SB_STATUS_CRC_ERROR;
    }

    status = sb_extnor_read(0u, header, (u32)sizeof(header));
    if (status != SB_STATUS_OK) {
        s_loader.active = 0;
        sb_asset_loader_set_error(status);
        return status;
    }
    magic = ((u32)header[0]) | (((u32)header[1]) << 8) | (((u32)header[2]) << 16) | (((u32)header[3]) << 24);
    if (magic != SB_ASSET_PACK_MAGIC) {
        s_loader.active = 0;
        sb_asset_loader_set_error(SB_STATUS_INVALID_PARAM);
        return SB_STATUS_INVALID_PARAM;
    }

    s_loader.active = 0;
    s_loader.last_error = 0u;
    SB_LOGI(SB_ASSET_PACK_MODULE_NAME, "end size=%u crc32=%08x", s_loader.received_size, s_loader.finalized_crc32);
    return SB_STATUS_OK;
}

sb_status_t sb_asset_pack_loader_abort(void)
{
    s_loader.active = 0;
    return SB_STATUS_OK;
}

sb_status_t sb_asset_pack_loader_status(sb_asset_pack_loader_status_t *status)
{
    if (status == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    *status = s_loader;
    return SB_STATUS_OK;
}

sb_status_t sb_asset_pack_loader_write_hex(u32 offset, const char *hex_data, u32 *written_bytes)
{
    u8 buffer[SB_ASSET_PACK_UART_HEX_MAX_BYTES];
    u32 hex_len;
    u32 byte_len;
    u32 i;

    if ((hex_data == 0) || (written_bytes == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    *written_bytes = 0u;
    hex_len = sb_asset_text_len(hex_data);
    if ((hex_len == 0u) || ((hex_len & 1u) != 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    byte_len = hex_len / 2u;
    if (byte_len > (u32)sizeof(buffer)) {
        return SB_STATUS_INVALID_PARAM;
    }
    for (i = 0u; i < byte_len; i++) {
        int hi = sb_asset_hex_value(hex_data[i * 2u]);
        int lo = sb_asset_hex_value(hex_data[(i * 2u) + 1u]);
        if ((hi < 0) || (lo < 0)) {
            return SB_STATUS_INVALID_PARAM;
        }
        buffer[i] = (u8)(((u32)hi << 4u) | (u32)lo);
    }
    if (sb_asset_pack_loader_write(offset, buffer, byte_len) != SB_STATUS_OK) {
        return (sb_status_t)((s32)s_loader.last_error);
    }
    *written_bytes = byte_len;
    return SB_STATUS_OK;
}

static int sb_asset_ftp_write_cb(unsigned char *data, int data_len, void *user_data)
{
    sb_asset_ftp_ctx_t *ctx = (sb_asset_ftp_ctx_t *)user_data;
    sb_status_t status;

    if ((ctx == 0) || (data == 0) || (data_len <= 0)) {
        return 0;
    }
    if (((u32)data_len) > (ctx->expected_size - ctx->received)) {
        ctx->status = SB_STATUS_INVALID_PARAM;
        return 0;
    }
    status = sb_extnor_write(ctx->received, data, (u32)data_len);
    if (status != SB_STATUS_OK) {
        ctx->status = status;
        return 0;
    }
    ctx->crc = sb_crc32_update(ctx->crc, data, (u32)data_len);
    ctx->received += (u32)data_len;
    ctx->status = SB_STATUS_OK;
    return data_len;
}

sb_status_t sb_asset_pack_loader_ftp_get(const char *host,
                                          const char *user,
                                          const char *password,
                                          const char *remote_file,
                                          u32 cid,
                                          u32 expected_size,
                                          u32 expected_crc32)
{
    void *client;
    int ret;
    sb_asset_ftp_ctx_t ctx;
    sb_status_t status;

    if ((host == 0) || (user == 0) || (password == 0) || (remote_file == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    status = sb_asset_pack_loader_begin(expected_size, expected_crc32, 1u);
    if (status != SB_STATUS_OK) {
        return status;
    }

    ctx.received = 0u;
    ctx.expected_size = expected_size;
    ctx.expected_crc32 = expected_crc32;
    ctx.crc = SB_CRC32_INITIAL_VALUE;
    ctx.status = SB_STATUS_OK;

    client = ql_ftp_client_new();
    if (client == 0) {
        (void)sb_asset_pack_loader_abort();
        return SB_STATUS_NO_MEMORY;
    }
    ret = ql_ftp_client_setopt(client, QL_FTP_CLIENT_OPT_PDP_CID, (int)cid);
    if (ret != 0) {
        ql_ftp_client_release(client);
        (void)sb_asset_pack_loader_abort();
        return SB_STATUS_NETWORK_ERROR;
    }
    ret = ql_ftp_client_open(client, (char *)host, (char *)user, (char *)password);
    if (ret != 0) {
        ql_ftp_client_release(client);
        (void)sb_asset_pack_loader_abort();
        return SB_STATUS_NETWORK_ERROR;
    }

    ret = ql_ftp_client_get(client, (char *)remote_file, 0, sb_asset_ftp_write_cb, &ctx);
    (void)ql_ftp_client_close(client);
    ql_ftp_client_release(client);

    if ((ret != 0) || (ctx.status != SB_STATUS_OK)) {
        (void)sb_asset_pack_loader_abort();
        return (ctx.status != SB_STATUS_OK) ? ctx.status : SB_STATUS_NETWORK_ERROR;
    }
    if (ctx.received != expected_size) {
        (void)sb_asset_pack_loader_abort();
        return SB_STATUS_INVALID_STATE;
    }
    ctx.crc ^= 0xFFFFFFFFu;
    if ((expected_crc32 != 0u) && (ctx.crc != expected_crc32)) {
        (void)sb_asset_pack_loader_abort();
        return SB_STATUS_CRC_ERROR;
    }

    s_loader.received_size = ctx.received;
    s_loader.running_crc32 = ctx.crc ^ 0xFFFFFFFFu;
    return sb_asset_pack_loader_end();
}
