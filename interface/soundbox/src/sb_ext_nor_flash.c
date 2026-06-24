/*================================================================
 * Static QR UPI Soundbox - External W25Q64 NOR Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_gpio.h"
#include "ql_rtos.h"
#include "ql_spi_nor.h"
#include "sb_board_kae8_sq1.h"
#include "sb_crc32.h"
#include "sb_cloud_utils.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_ext_nor_flash.h"
#include "sb_log.h"
#include "sb_storage_fs.h"

#define SB_EXT_NOR_MODULE_NAME          "ext_nor"
#define SB_EXT_NOR_CHUNK_MAX            (4096u)
#define SB_EXT_NOR_W25Q_MFG_ID          (0xEFu)
#define SB_EXT_NOR_W25Q64_CAP_ID        (0x17u)

static sb_ext_nor_status_t s_nor_status;

static void sb_ext_nor_post(sb_event_id_t id, sb_status_t status, u32 value, const char *text)
{
    sb_event_t event;
    sb_event_init(&event, id, SB_EVENT_SOURCE_STORAGE);
    event.param_s32 = (s32)status;
    event.param_u32 = value;
    if (text != 0) {
        (void)sb_event_set_text(&event, text);
    }
    (void)sb_event_post(&event, QL_NO_WAIT);
}

static void sb_ext_nor_save_diag(const char *result, sb_status_t status)
{
    char text[160];
    u32 pos = 0u;
    text[0] = '\0';
    (void)sb_cloud_append_string(text, (u32)sizeof(text), "result=");
    (void)sb_cloud_append_string(text, (u32)sizeof(text), (result != 0) ? result : "unknown");
    (void)sb_cloud_append_string(text, (u32)sizeof(text), "\nstatus=");
    if (status < 0) {
        (void)sb_cloud_append_string(text, (u32)sizeof(text), "-");
        (void)sb_cloud_append_u32(text, (u32)sizeof(text), (u32)(-status));
    } else {
        (void)sb_cloud_append_u32(text, (u32)sizeof(text), (u32)status);
    }
    (void)sb_cloud_append_string(text, (u32)sizeof(text), "\njedec=");
    pos = sb_cloud_str_len(text);
    if ((pos + 10u) < (u32)sizeof(text)) {
        static const char hex[] = "0123456789ABCDEF";
        text[pos++] = hex[(s_nor_status.manufacturer_id >> 4) & 0x0Fu];
        text[pos++] = hex[s_nor_status.manufacturer_id & 0x0Fu];
        text[pos++] = ' ';
        text[pos++] = hex[(s_nor_status.memory_type >> 4) & 0x0Fu];
        text[pos++] = hex[s_nor_status.memory_type & 0x0Fu];
        text[pos++] = ' ';
        text[pos++] = hex[(s_nor_status.capacity_id >> 4) & 0x0Fu];
        text[pos++] = hex[s_nor_status.capacity_id & 0x0Fu];
        text[pos++] = '\n';
        text[pos] = '\0';
    }
    (void)sb_storage_fs_write_text_atomic(SB_STORAGE_DIAG_NOR_PATH, text);
}

static sb_status_t sb_ext_nor_range_check(u32 address, u32 length)
{
    if (length == 0u) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (address >= SB_KAE8_NOR_SIZE_BYTES) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (length > (SB_KAE8_NOR_SIZE_BYTES - address)) {
        return SB_STATUS_INVALID_PARAM;
    }
    return SB_STATUS_OK;
}

static sb_status_t sb_ext_nor_drive_control_pins(void)
{
    if (ql_gpio_init(SB_KAE8_NOR_WP_GPIO, PIN_DIRECTION_OUT, PIN_PULL_DISABLE, SB_KAE8_NOR_CTRL_ACTIVE_LEVEL) != 0) {
        return SB_STATUS_GPIO_ERROR;
    }
    if (ql_gpio_init(SB_KAE8_NOR_RST_GPIO, PIN_DIRECTION_OUT, PIN_PULL_DISABLE, SB_KAE8_NOR_CTRL_ACTIVE_LEVEL) != 0) {
        return SB_STATUS_GPIO_ERROR;
    }
    return SB_STATUS_OK;
}

int sb_ext_nor_flash_is_ready(void)
{
    return s_nor_status.ready;
}

sb_status_t sb_ext_nor_flash_get_status(sb_ext_nor_status_t *status)
{
    if (status == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    *status = s_nor_status;
    return SB_STATUS_OK;
}

sb_status_t sb_ext_nor_flash_read(u32 address, void *buffer, u32 length)
{
    u32 offset = 0u;
    unsigned char *dst = (unsigned char *)buffer;

    if ((buffer == 0) || (sb_ext_nor_range_check(address, length) != SB_STATUS_OK)) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (s_nor_status.ready == 0) {
        return SB_STATUS_NOT_READY;
    }
    while (offset < length) {
        u32 chunk = length - offset;
        unsigned int ret;
        if (chunk > SB_EXT_NOR_CHUNK_MAX) {
            chunk = SB_EXT_NOR_CHUNK_MAX;
        }
        ret = ql_spi_nor_read(SB_KAE8_NOR_PORT, dst + offset, address + offset, (unsigned short)chunk);
        if (ret != QUEC_SSPOperationNoError) {
            s_nor_status.last_error = ret;
            return SB_STATUS_FLASH_ERROR;
        }
        offset += chunk;
    }
    return SB_STATUS_OK;
}

sb_status_t sb_ext_nor_flash_write(u32 address, const void *buffer, u32 length)
{
    u32 offset = 0u;
    unsigned char *src = (unsigned char *)buffer;

    if ((buffer == 0) || (sb_ext_nor_range_check(address, length) != SB_STATUS_OK)) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (s_nor_status.ready == 0) {
        return SB_STATUS_NOT_READY;
    }
    while (offset < length) {
        u32 page_left = SB_KAE8_NOR_PAGE_SIZE_BYTES - ((address + offset) % SB_KAE8_NOR_PAGE_SIZE_BYTES);
        u32 chunk = length - offset;
        unsigned int ret;
        if (chunk > page_left) {
            chunk = page_left;
        }
        if (chunk > SB_EXT_NOR_CHUNK_MAX) {
            chunk = SB_EXT_NOR_CHUNK_MAX;
        }
        ret = ql_spi_nor_write(SB_KAE8_NOR_PORT, src + offset, address + offset, (unsigned short)chunk);
        if (ret != QUEC_SSPOperationNoError) {
            s_nor_status.last_error = ret;
            return SB_STATUS_FLASH_ERROR;
        }
        offset += chunk;
    }
    return SB_STATUS_OK;
}

sb_status_t sb_ext_nor_flash_erase_sector(u32 address)
{
    unsigned int ret;

    if ((address % SB_KAE8_NOR_SECTOR_SIZE_BYTES) != 0u) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (sb_ext_nor_range_check(address, SB_KAE8_NOR_SECTOR_SIZE_BYTES) != SB_STATUS_OK) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (s_nor_status.ready == 0) {
        return SB_STATUS_NOT_READY;
    }
    ret = ql_spi_nor_erase_sector(SB_KAE8_NOR_PORT, address);
    if (ret != QUEC_SSPOperationNoError) {
        s_nor_status.last_error = ret;
        return SB_STATUS_FLASH_ERROR;
    }
    return SB_STATUS_OK;
}

sb_status_t sb_ext_nor_flash_factory_rw_test(void)
{
    unsigned char pattern[256];
    unsigned char readback[256];
    u32 i;
    u32 crc_pattern;
    u32 crc_readback;
    sb_status_t status;

    if (s_nor_status.ready == 0) {
        return SB_STATUS_NOT_READY;
    }
    for (i = 0u; i < (u32)sizeof(pattern); i++) {
        pattern[i] = (unsigned char)((i * 37u) ^ 0xA5u);
        readback[i] = 0u;
    }
    status = sb_ext_nor_flash_erase_sector(SB_KAE8_NOR_FACTORY_TEST_ADDR);
    if (status != SB_STATUS_OK) {
        sb_ext_nor_save_diag("factory_erase_fail", status);
        return status;
    }
    status = sb_ext_nor_flash_write(SB_KAE8_NOR_FACTORY_TEST_ADDR, pattern, (u32)sizeof(pattern));
    if (status != SB_STATUS_OK) {
        sb_ext_nor_save_diag("factory_write_fail", status);
        return status;
    }
    status = sb_ext_nor_flash_read(SB_KAE8_NOR_FACTORY_TEST_ADDR, readback, (u32)sizeof(readback));
    if (status != SB_STATUS_OK) {
        sb_ext_nor_save_diag("factory_read_fail", status);
        return status;
    }
    crc_pattern = sb_crc32_compute(pattern, (u32)sizeof(pattern));
    crc_readback = sb_crc32_compute(readback, (u32)sizeof(readback));
    s_nor_status.health_crc = crc_readback;
    if (crc_pattern != crc_readback) {
        sb_ext_nor_save_diag("factory_crc_fail", SB_STATUS_CRC_ERROR);
        return SB_STATUS_CRC_ERROR;
    }
    sb_ext_nor_save_diag("factory_pass", SB_STATUS_OK);
    return SB_STATUS_OK;
}

sb_status_t sb_ext_nor_flash_init(void)
{
    unsigned char *id;
    unsigned char probe[16];
    unsigned int ret;
    sb_status_t status;

    if (s_nor_status.ready != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }
    s_nor_status.ready = 0;
    s_nor_status.jedec_valid = 0;
    s_nor_status.size_bytes = SB_KAE8_NOR_SIZE_BYTES;
    s_nor_status.last_error = 0u;

    status = sb_ext_nor_drive_control_pins();
    if (status != SB_STATUS_OK) {
        s_nor_status.last_error = (u32)status;
        return status;
    }
    ret = ql_spi_nor_init(SB_KAE8_NOR_PORT, SB_KAE8_NOR_CLK);
    if (ret != QUEC_SSPOperationNoError) {
        s_nor_status.last_error = ret;
        SB_LOGE(SB_EXT_NOR_MODULE_NAME, "init failed port=%d ret=%u di_do_swap=%u", SB_KAE8_NOR_PORT, ret, SB_NOR_DI_DO_INTERCHANGED);
        sb_ext_nor_save_diag("init_fail", SB_STATUS_FLASH_ERROR);
        return SB_STATUS_FLASH_ERROR;
    }
    id = ql_spi_nor_read_id(SB_KAE8_NOR_PORT);
    if (id == 0) {
        sb_ext_nor_save_diag("id_null", SB_STATUS_FLASH_ERROR);
        return SB_STATUS_FLASH_ERROR;
    }
    s_nor_status.manufacturer_id = id[0];
    s_nor_status.memory_type = id[1];
    s_nor_status.capacity_id = id[2];
    if ((id[0] == SB_EXT_NOR_W25Q_MFG_ID) && (id[2] == SB_EXT_NOR_W25Q64_CAP_ID)) {
        s_nor_status.jedec_valid = 1;
    }
    if (s_nor_status.jedec_valid == 0) {
        SB_LOGE(SB_EXT_NOR_MODULE_NAME, "unexpected JEDEC id=%02X %02X %02X port=%d di_do_swap=%u",
                id[0], id[1], id[2], SB_KAE8_NOR_PORT, SB_NOR_DI_DO_INTERCHANGED);
        sb_ext_nor_save_diag("jedec_fail", SB_STATUS_FLASH_ERROR);
        return SB_STATUS_FLASH_ERROR;
    }
    s_nor_status.ready = 1;
    status = sb_ext_nor_flash_read(SB_KAE8_NOR_MANIFEST_ADDR, probe, (u32)sizeof(probe));
    if (status != SB_STATUS_OK) {
        s_nor_status.ready = 0;
        sb_ext_nor_save_diag("probe_fail", status);
        return status;
    }
    SB_LOGI(SB_EXT_NOR_MODULE_NAME, "ready jedec=%02X %02X %02X size=%u port=%d",
            s_nor_status.manufacturer_id, s_nor_status.memory_type, s_nor_status.capacity_id,
            s_nor_status.size_bytes, SB_KAE8_NOR_PORT);
    sb_ext_nor_save_diag("boot_pass", SB_STATUS_OK);
    sb_ext_nor_post(SB_EVENT_NOR_READY, SB_STATUS_OK, s_nor_status.size_bytes, "ready");
    return SB_STATUS_OK;
}
