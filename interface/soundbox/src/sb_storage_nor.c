/*================================================================
 * Static QR UPI Soundbox - External SPI NOR Abstraction
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_spi_nor.h"
#include "sb_storage_nor.h"

static const int s_candidate_ports[] = {
    EXTERNAL_NORFLASH_PORT16_19,
    EXTERNAL_NORFLASH_PORT33_36_SSP0,
    EXTERNAL_NORFLASH_PORT33_36,
    EXTERNAL_NORFLASH_PORT12_15_SSP2,
    EXTERNAL_NORFLASH_PORT4_7
};

static sb_nor_info_t s_nor_info = { -1, {0u, 0u, 0u}, 0 };

static int sb_nor_id_is_valid(const unsigned char *id)
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

int sb_storage_nor_is_ready(void)
{
    return s_nor_info.ready;
}

sb_status_t sb_storage_nor_init(void)
{
    unsigned int i;
    unsigned char *id;

    if (s_nor_info.ready != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }

    for (i = 0u; i < (u32)(sizeof(s_candidate_ports) / sizeof(s_candidate_ports[0])); i++) {
        if (ql_spi_nor_init(s_candidate_ports[i], _APBC_SSP_FNCLKSEL_1_625MHZ_) != 0u) {
            continue;
        }

        id = ql_spi_nor_read_id(s_candidate_ports[i]);
        if (sb_nor_id_is_valid(id) != 0) {
            s_nor_info.port = s_candidate_ports[i];
            s_nor_info.id[0] = id[0];
            s_nor_info.id[1] = id[1];
            s_nor_info.id[2] = id[2];
            s_nor_info.ready = 1;
            return SB_STATUS_OK;
        }
    }

    return SB_STATUS_NOT_FOUND;
}

sb_status_t sb_storage_nor_get_info(sb_nor_info_t *info)
{
    if (info == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    info->port = s_nor_info.port;
    info->id[0] = s_nor_info.id[0];
    info->id[1] = s_nor_info.id[1];
    info->id[2] = s_nor_info.id[2];
    info->ready = s_nor_info.ready;
    return (s_nor_info.ready != 0) ? SB_STATUS_OK : SB_STATUS_NOT_READY;
}

sb_status_t sb_storage_nor_read(u32 address, void *buffer, u32 length)
{
    if ((buffer == 0) || (length == 0u) || (length > SB_NOR_MAX_XFER_BYTES)) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (s_nor_info.ready == 0) {
        return SB_STATUS_NOT_READY;
    }

    if (ql_spi_nor_read(s_nor_info.port, (unsigned char *)buffer, address, (unsigned short)length) != 0u) {
        return SB_STATUS_FLASH_ERROR;
    }

    return SB_STATUS_OK;
}

sb_status_t sb_storage_nor_write(u32 address, const void *buffer, u32 length)
{
    if ((buffer == 0) || (length == 0u) || (length > SB_NOR_MAX_XFER_BYTES)) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (s_nor_info.ready == 0) {
        return SB_STATUS_NOT_READY;
    }

    if (ql_spi_nor_write(s_nor_info.port, (unsigned char *)buffer, address, (unsigned short)length) != 0u) {
        return SB_STATUS_FLASH_ERROR;
    }

    return SB_STATUS_OK;
}

sb_status_t sb_storage_nor_erase_sector(u32 address)
{
    if (s_nor_info.ready == 0) {
        return SB_STATUS_NOT_READY;
    }

    if (ql_spi_nor_erase_sector(s_nor_info.port, address) != 0u) {
        return SB_STATUS_FLASH_ERROR;
    }

    return SB_STATUS_OK;
}
