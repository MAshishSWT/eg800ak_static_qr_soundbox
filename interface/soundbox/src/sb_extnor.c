/*================================================================
 * Static QR UPI Soundbox - KAE8 External NOR Flash Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *
 * Hardware note for KAE8_SQ1:
 * - FLASH_DIN / FLASH_DOUT board GPIO mapping is intentionally swapped in
 *   sb_board_kae8_sq1.h per hardware bring-up instruction.
 * - FLASH_WP and FLASH_RST are driven high before the SPI NOR controller is
 *   initialized.
 * - This implementation uses Quectel SPI-NOR port 4
 *   EXTERNAL_NORFLASH_PORT4_7 instead of generic SPI_PORT1/SPI_PORT2 probing.
 *================================================================*/
#include "ql_rtos.h"
#include "ql_spi_nor.h"
#include "sb_board_kae8_sq1.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_extnor.h"
#include "sb_hal_gpio.h"
#include "sb_log.h"

#define SB_EXTNOR_MODULE_NAME          "extnor"
#define SB_EXTNOR_PORT                 EXTERNAL_NORFLASH_PORT4_7
#define SB_EXTNOR_CLOCK                _APBC_SSP_FNCLKSEL_812_5KHZ_
#define SB_EXTNOR_MAX_CHUNK_BYTES      (65535u)

static sb_extnor_info_t s_extnor_info = {
    -1,
    -1,
    -1,
    {0u, 0u, 0u},
    0u,
    0
};

static int sb_extnor_id_is_valid(const unsigned char *id)
{
    if (id == 0) {
        return 0;
    }
    if ((id[0] == 0x00u) && (id[1] == 0x00u) && (id[2] == 0x00u)) {
        return 0;
    }
    if ((id[0] == 0xFFu) && (id[1] == 0xFFu) && (id[2] == 0xFFu)) {
        return 0;
    }
    return 1;
}

static u32 sb_extnor_capacity_from_jedec_id(const unsigned char *id)
{
    u32 i;
    u32 density_code = 0u;

    if (sb_extnor_id_is_valid(id) == 0) {
        return 0u;
    }

    /* ql_spi_nor_read_id() on this EG800AK SDK can return the three ID bytes
     * in controller/native order instead of conventional JEDEC order. Accept
     * any byte in the known serial-NOR density-code range. Examples:
     *   EF 40 17 -> W25Q64, 8 MB
     *   17 60 EF -> same information returned in reversed/controller order
     */
    for (i = 0u; i < SB_EXTNOR_ID_LEN; i++) {
        if (((u32)id[i] >= 0x14u) && ((u32)id[i] <= 0x1Du)) {
            density_code = (u32)id[i];
            break;
        }
    }

    if (density_code == 0u) {
        return 0u;
    }

    if (density_code >= 31u) {
        return 0u;
    }

    return 1u << density_code;
}

static sb_status_t sb_extnor_prepare_control_pins(void)
{
    sb_status_t status;

    status = sb_hal_gpio_output(SB_KAE8_FLASH_WP_GPIO, PIN_LEVEL_HIGH);
    if (status != SB_STATUS_OK) {
        SB_LOGW(SB_EXTNOR_MODULE_NAME, "flash wp high failed status=%s", sb_status_to_string(status));
        return status;
    }

    status = sb_hal_gpio_output(SB_KAE8_FLASH_RST_GPIO, PIN_LEVEL_HIGH);
    if (status != SB_STATUS_OK) {
        SB_LOGW(SB_EXTNOR_MODULE_NAME, "flash rst high failed status=%s", sb_status_to_string(status));
        return status;
    }

    ql_rtos_task_sleep_ms(10u);
    return SB_STATUS_OK;
}

static sb_status_t sb_extnor_check_range(u32 address, u32 length)
{
    if ((length == 0u) || (length > SB_EXTNOR_MAX_XFER_BYTES)) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (s_extnor_info.ready == 0) {
        return SB_STATUS_NOT_READY;
    }
    if (s_extnor_info.capacity_bytes != 0u) {
        if (address >= s_extnor_info.capacity_bytes) {
            return SB_STATUS_INVALID_PARAM;
        }
        if (length > (s_extnor_info.capacity_bytes - address)) {
            return SB_STATUS_INVALID_PARAM;
        }
    }
    return SB_STATUS_OK;
}

int sb_extnor_is_ready(void)
{
    return s_extnor_info.ready;
}

sb_status_t sb_extnor_post_status_event(sb_status_t status)
{
    sb_event_t event;

    sb_event_init(&event,
                  ((status == SB_STATUS_OK) || (status == SB_STATUS_ALREADY_INITIALIZED)) ?
                  SB_EVENT_EXTNOR_READY : SB_EVENT_EXTNOR_FAULT,
                  SB_EVENT_SOURCE_STORAGE);
    event.param_u32 = s_extnor_info.capacity_bytes;
    event.param_s32 = (s32)status;
    return sb_event_post(&event, QL_NO_WAIT);
}

sb_status_t sb_extnor_init(void)
{
    unsigned char *id_ptr;
    sb_status_t status;
    unsigned int ret;

    if (s_extnor_info.ready != 0) {
        (void)sb_extnor_post_status_event(SB_STATUS_ALREADY_INITIALIZED);
        return SB_STATUS_ALREADY_INITIALIZED;
    }

    status = sb_extnor_prepare_control_pins();
    if (status != SB_STATUS_OK) {
        (void)sb_extnor_post_status_event(status);
        return status;
    }

    SB_LOGI(SB_EXTNOR_MODULE_NAME,
            "control wp_gpio=%d rst_gpio=%d high, spi_nor_port=%d clk=%d dout_gpio=%d din_gpio=%d",
            (int)SB_KAE8_FLASH_WP_GPIO,
            (int)SB_KAE8_FLASH_RST_GPIO,
            (int)SB_EXTNOR_PORT,
            (int)SB_EXTNOR_CLOCK,
            (int)SB_KAE8_FLASH_DOUT_GPIO,
            (int)SB_KAE8_FLASH_DIN_GPIO);

    ret = ql_spi_nor_init(SB_EXTNOR_PORT, SB_EXTNOR_CLOCK);
    if (ret != 0u) {
        SB_LOGE(SB_EXTNOR_MODULE_NAME, "spi nor init failed port=%d ret=%u", (int)SB_EXTNOR_PORT, ret);
        (void)sb_extnor_post_status_event(SB_STATUS_FLASH_ERROR);
        return SB_STATUS_FLASH_ERROR;
    }

    id_ptr = ql_spi_nor_read_id(SB_EXTNOR_PORT);
    if (id_ptr == 0) {
        SB_LOGE(SB_EXTNOR_MODULE_NAME, "spi nor read id returned null port=%d", (int)SB_EXTNOR_PORT);
        (void)sb_extnor_post_status_event(SB_STATUS_NOT_FOUND);
        return SB_STATUS_NOT_FOUND;
    }

    SB_LOGI(SB_EXTNOR_MODULE_NAME,
            "probe spi_nor_port=%d id=%02x %02x %02x",
            (int)SB_EXTNOR_PORT, id_ptr[0], id_ptr[1], id_ptr[2]);

    if (sb_extnor_id_is_valid(id_ptr) == 0) {
        SB_LOGE(SB_EXTNOR_MODULE_NAME, "no valid jedec id on SPI NOR port 4 GPIO4-GPIO7");
        (void)sb_extnor_post_status_event(SB_STATUS_NOT_FOUND);
        return SB_STATUS_NOT_FOUND;
    }

    s_extnor_info.port = SB_EXTNOR_PORT;
    s_extnor_info.mode = 0;
    s_extnor_info.clock = SB_EXTNOR_CLOCK;
    s_extnor_info.id[0] = id_ptr[0];
    s_extnor_info.id[1] = id_ptr[1];
    s_extnor_info.id[2] = id_ptr[2];
    s_extnor_info.capacity_bytes = sb_extnor_capacity_from_jedec_id(id_ptr);
    s_extnor_info.ready = 1;

    SB_LOGI(SB_EXTNOR_MODULE_NAME,
            "ready spi_nor_port=%d clk=%d id=%02x %02x %02x capacity=%u bytes",
            s_extnor_info.port, s_extnor_info.clock,
            s_extnor_info.id[0], s_extnor_info.id[1], s_extnor_info.id[2],
            s_extnor_info.capacity_bytes);

    (void)sb_extnor_post_status_event(SB_STATUS_OK);
    return SB_STATUS_OK;
}

sb_status_t sb_extnor_get_info(sb_extnor_info_t *info)
{
    if (info == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    *info = s_extnor_info;
    return (s_extnor_info.ready != 0) ? SB_STATUS_OK : SB_STATUS_NOT_READY;
}

sb_status_t sb_extnor_read(u32 address, void *buffer, u32 length)
{
    unsigned char *dst;
    u32 remaining;
    u32 offset;
    u32 chunk;
    unsigned int ret;
    sb_status_t status;

    if (buffer == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    status = sb_extnor_check_range(address, length);
    if (status != SB_STATUS_OK) {
        return status;
    }

    dst = (unsigned char *)buffer;
    remaining = length;
    offset = 0u;
    while (remaining != 0u) {
        chunk = (remaining > SB_EXTNOR_MAX_CHUNK_BYTES) ? SB_EXTNOR_MAX_CHUNK_BYTES : remaining;
        ret = ql_spi_nor_read(SB_EXTNOR_PORT, &dst[offset], address + offset, (unsigned short)chunk);
        if (ret != 0u) {
            SB_LOGW(SB_EXTNOR_MODULE_NAME, "read addr=%u len=%u ret=%u", address + offset, chunk, ret);
            return SB_STATUS_FLASH_ERROR;
        }
        offset += chunk;
        remaining -= chunk;
    }
    return SB_STATUS_OK;
}

sb_status_t sb_extnor_write(u32 address, const void *buffer, u32 length)
{
    const unsigned char *src;
    u32 remaining;
    u32 offset;
    u32 chunk;
    unsigned int ret;
    sb_status_t status;

    if (buffer == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    status = sb_extnor_check_range(address, length);
    if (status != SB_STATUS_OK) {
        return status;
    }

    src = (const unsigned char *)buffer;
    remaining = length;
    offset = 0u;
    while (remaining != 0u) {
        chunk = (remaining > SB_EXTNOR_MAX_CHUNK_BYTES) ? SB_EXTNOR_MAX_CHUNK_BYTES : remaining;
        ret = ql_spi_nor_write(SB_EXTNOR_PORT, (unsigned char *)&src[offset], address + offset, (unsigned short)chunk);
        if (ret != 0u) {
            SB_LOGW(SB_EXTNOR_MODULE_NAME, "write addr=%u len=%u ret=%u", address + offset, chunk, ret);
            return SB_STATUS_FLASH_ERROR;
        }
        offset += chunk;
        remaining -= chunk;
    }
    return SB_STATUS_OK;
}

sb_status_t sb_extnor_erase_sector(u32 address)
{
    unsigned int ret;

    if ((address % SB_EXTNOR_SECTOR_SIZE_BYTES) != 0u) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (s_extnor_info.ready == 0) {
        return SB_STATUS_NOT_READY;
    }
    if ((s_extnor_info.capacity_bytes != 0u) && (address >= s_extnor_info.capacity_bytes)) {
        return SB_STATUS_INVALID_PARAM;
    }

    ret = ql_spi_nor_erase_sector(SB_EXTNOR_PORT, address);
    if (ret != 0u) {
        SB_LOGW(SB_EXTNOR_MODULE_NAME, "erase sector addr=%u ret=%u", address, ret);
        return SB_STATUS_FLASH_ERROR;
    }
    return SB_STATUS_OK;
}
