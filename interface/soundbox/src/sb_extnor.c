/*================================================================
 * Static QR UPI Soundbox - KAE8 External NOR Flash Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "ql_spi_nor.h"
#include "sb_board_kae8_sq1.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_extnor.h"
#include "sb_hal_gpio.h"
#include "sb_log.h"

#define SB_EXTNOR_MODULE_NAME       "extnor"
#define SB_EXTNOR_PORT              EXTERNAL_NORFLASH_PORT4_7
#define SB_EXTNOR_CLOCK             _APBC_SSP_FNCLKSEL_1_625MHZ_

static sb_extnor_info_t s_extnor_info = {
    SB_EXTNOR_PORT,
    SB_EXTNOR_CLOCK,
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
    u32 density_code;

    if (sb_extnor_id_is_valid(id) == 0) {
        return 0u;
    }

    density_code = (u32)id[2];
    if ((density_code < 0x14u) || (density_code > 0x1Du)) {
        return 0u;
    }

    /* JEDEC density code is expressed in bits. Example: 0x17 = 64 Mbit = 8 MB. */
    return 1u << (density_code - 3u);
}

static sb_status_t sb_extnor_prepare_control_pins(void)
{
    sb_status_t status;

    /* W25Q-series /WP and /HOLD-/RESET style pins must be deasserted high.
     * Do not configure CLK/DIN/DOUT/CS here; ql_spi_nor_init owns the SPI pins.
     */
    status = sb_hal_gpio_output(SB_KAE8_FLASH_WP_GPIO, PIN_LEVEL_HIGH);
    if (status != SB_STATUS_OK) {
        SB_LOGW(SB_EXTNOR_MODULE_NAME, "flash wp gpio status=%s", sb_status_to_string(status));
        return status;
    }

    status = sb_hal_gpio_output(SB_KAE8_FLASH_RST_GPIO, PIN_LEVEL_HIGH);
    if (status != SB_STATUS_OK) {
        SB_LOGW(SB_EXTNOR_MODULE_NAME, "flash rst gpio status=%s", sb_status_to_string(status));
        return status;
    }

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
    unsigned char *id;
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

    /* KAE8_SQ1 external flash is routed to FLASH_CLK/SYNC/DIN/DOUT on GPIO[4..7].
     * Use only this exact port. Do not scan other ports; unsupported ports caused
     * INVALID_GPIO_MFPR_ADDR on EG800AK-CN during earlier bring-up.
     */
    ret = ql_spi_nor_init(SB_EXTNOR_PORT, SB_EXTNOR_CLOCK);
    if (ret != QUEC_SSPOperationNoError) {
        SB_LOGE(SB_EXTNOR_MODULE_NAME, "init port=%d ret=0x%x", SB_EXTNOR_PORT, ret);
        (void)sb_extnor_post_status_event(SB_STATUS_FLASH_ERROR);
        return SB_STATUS_FLASH_ERROR;
    }

    id = ql_spi_nor_read_id(SB_EXTNOR_PORT);
    if (sb_extnor_id_is_valid(id) == 0) {
        SB_LOGE(SB_EXTNOR_MODULE_NAME, "invalid id on port=%d", SB_EXTNOR_PORT);
        (void)sb_extnor_post_status_event(SB_STATUS_NOT_FOUND);
        return SB_STATUS_NOT_FOUND;
    }

    s_extnor_info.id[0] = id[0];
    s_extnor_info.id[1] = id[1];
    s_extnor_info.id[2] = id[2];
    s_extnor_info.capacity_bytes = sb_extnor_capacity_from_jedec_id(id);
    s_extnor_info.ready = 1;

    SB_LOGI(SB_EXTNOR_MODULE_NAME,
            "ready port=%d id=%02x %02x %02x capacity=%u bytes",
            s_extnor_info.port,
            s_extnor_info.id[0], s_extnor_info.id[1], s_extnor_info.id[2],
            s_extnor_info.capacity_bytes);

    if ((s_extnor_info.capacity_bytes != 0u) &&
        ((s_extnor_info.capacity_bytes < SB_EXTNOR_EXPECTED_MIN_BYTES) ||
         (s_extnor_info.capacity_bytes > SB_EXTNOR_EXPECTED_MAX_BYTES))) {
        SB_LOGW(SB_EXTNOR_MODULE_NAME, "capacity outside expected range=%u", s_extnor_info.capacity_bytes);
    }

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
    sb_status_t status;

    if (buffer == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_extnor_check_range(address, length);
    if (status != SB_STATUS_OK) {
        return status;
    }

    if (ql_spi_nor_read(SB_EXTNOR_PORT, (unsigned char *)buffer, address, (unsigned short)length) != QUEC_SSPOperationNoError) {
        return SB_STATUS_FLASH_ERROR;
    }

    return SB_STATUS_OK;
}

sb_status_t sb_extnor_write(u32 address, const void *buffer, u32 length)
{
    sb_status_t status;

    if (buffer == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_extnor_check_range(address, length);
    if (status != SB_STATUS_OK) {
        return status;
    }

    if (ql_spi_nor_write(SB_EXTNOR_PORT, (unsigned char *)buffer, address, (unsigned short)length) != QUEC_SSPOperationNoError) {
        return SB_STATUS_FLASH_ERROR;
    }

    return SB_STATUS_OK;
}

sb_status_t sb_extnor_erase_sector(u32 address)
{
    if ((address % SB_EXTNOR_SECTOR_SIZE_BYTES) != 0u) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (s_extnor_info.ready == 0) {
        return SB_STATUS_NOT_READY;
    }

    if ((s_extnor_info.capacity_bytes != 0u) && (address >= s_extnor_info.capacity_bytes)) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (ql_spi_nor_erase_sector(SB_EXTNOR_PORT, address) != QUEC_SSPOperationNoError) {
        return SB_STATUS_FLASH_ERROR;
    }

    return SB_STATUS_OK;
}
