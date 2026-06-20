/*================================================================
 * Static QR UPI Soundbox - KAE8 External NOR Flash Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "ql_spi.h"
#include "sb_board_kae8_sq1.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_extnor.h"
#include "sb_hal_gpio.h"
#include "sb_log.h"

#define SB_EXTNOR_MODULE_NAME             "extnor"
#define SB_EXTNOR_PAGE_SIZE_BYTES         (256u)
#define SB_EXTNOR_STATUS_BUSY_MASK        (0x01u)
#define SB_EXTNOR_READY_POLL_DELAY_MS     (2u)
#define SB_EXTNOR_READY_POLL_LIMIT        (2500u)

#define SB_EXTNOR_CMD_READ_ID             (0x9Fu)
#define SB_EXTNOR_CMD_READ_STATUS         (0x05u)
#define SB_EXTNOR_CMD_WRITE_ENABLE        (0x06u)
#define SB_EXTNOR_CMD_READ_DATA           (0x03u)
#define SB_EXTNOR_CMD_PAGE_PROGRAM        (0x02u)
#define SB_EXTNOR_CMD_SECTOR_ERASE_4K     (0x20u)

typedef struct {
    SPI_PORT_E port;
    SPI_MODE_E mode;
    SPI_CLK_E clock;
} sb_extnor_probe_t;

static const sb_extnor_probe_t s_probe_table[] = {
    /* EG800AK/1605 GPIO4-GPIO7 route according to SDK SPI example notes. */
    {SPI_PORT1, SPI_MODE0, SPI_CLK_812_5KHZ},
    {SPI_PORT1, SPI_MODE3, SPI_CLK_812_5KHZ},
    {SPI_PORT1, SPI_MODE0, SPI_CLK_1_625MHZ},
    {SPI_PORT1, SPI_MODE3, SPI_CLK_1_625MHZ},

    /* Keep this valid generic SPI fallback because ql_spi.h enum comments mark
     * SPI_PORT2 as GPIO4-GPIO7 on some SDK/header revisions.
     */
    {SPI_PORT2, SPI_MODE0, SPI_CLK_812_5KHZ},
    {SPI_PORT2, SPI_MODE3, SPI_CLK_812_5KHZ},
    {SPI_PORT2, SPI_MODE0, SPI_CLK_1_625MHZ},
    {SPI_PORT2, SPI_MODE3, SPI_CLK_1_625MHZ}
};

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

    status = sb_hal_gpio_output(SB_KAE8_FLASH_WP_GPIO, PIN_LEVEL_HIGH);
    if (status != SB_STATUS_OK) {
        SB_LOGW(SB_EXTNOR_MODULE_NAME, "flash wp gpio status=%s", sb_status_to_string(status));
        return status;
    }

    status = sb_hal_gpio_output(SB_KAE8_FLASH_RST_GPIO, PIN_LEVEL_HIGH);
    if (status != SB_STATUS_OK) {
        SB_LOGW(SB_EXTNOR_MODULE_NAME, "flash hold/rst gpio status=%s", sb_status_to_string(status));
        return status;
    }

    ql_rtos_task_sleep_ms(5u);
    return SB_STATUS_OK;
}

static void sb_extnor_addr_to_bytes(u32 address, unsigned char *out)
{
    if (out == 0) {
        return;
    }

    out[0] = (unsigned char)((address >> 16u) & 0xFFu);
    out[1] = (unsigned char)((address >> 8u) & 0xFFu);
    out[2] = (unsigned char)(address & 0xFFu);
}

static sb_status_t sb_extnor_spi_read_id_raw(SPI_PORT_E port, SPI_MODE_E mode, SPI_CLK_E clock,
                                             unsigned char *raw, unsigned char *id)
{
    unsigned char tx[4] = {SB_EXTNOR_CMD_READ_ID, 0xFFu, 0xFFu, 0xFFu};
    unsigned char rx[4] = {0u, 0u, 0u, 0u};

    if ((raw == 0) || (id == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (ql_spi_init(port, mode, clock) != 0) {
        return SB_STATUS_FLASH_ERROR;
    }

    ql_rtos_task_sleep_ms(2u);

    if (ql_spi_write_read(port, rx, tx, (unsigned int)sizeof(tx)) != 0) {
        return SB_STATUS_FLASH_ERROR;
    }

    raw[0] = rx[0];
    raw[1] = rx[1];
    raw[2] = rx[2];
    raw[3] = rx[3];

    id[0] = rx[1];
    id[1] = rx[2];
    id[2] = rx[3];

    /* Some Quectel SPI implementations can return only the read phase bytes. */
    if (sb_extnor_id_is_valid(id) == 0) {
        id[0] = rx[0];
        id[1] = rx[1];
        id[2] = rx[2];
    }

    return SB_STATUS_OK;
}

static sb_status_t sb_extnor_spi_write_enable(void)
{
    unsigned char tx[1] = {SB_EXTNOR_CMD_WRITE_ENABLE};

    if (ql_spi_write((SPI_PORT_E)s_extnor_info.port, tx, (unsigned int)sizeof(tx)) != 0) {
        return SB_STATUS_FLASH_ERROR;
    }

    return SB_STATUS_OK;
}

static sb_status_t sb_extnor_spi_read_status(unsigned char *status_reg)
{
    unsigned char tx[2] = {SB_EXTNOR_CMD_READ_STATUS, 0xFFu};
    unsigned char rx[2] = {0u, 0u};

    if (status_reg == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (ql_spi_write_read((SPI_PORT_E)s_extnor_info.port, rx, tx, (unsigned int)sizeof(tx)) != 0) {
        return SB_STATUS_FLASH_ERROR;
    }

    *status_reg = rx[1];
    return SB_STATUS_OK;
}

static sb_status_t sb_extnor_wait_ready(void)
{
    u32 i;
    unsigned char status_reg;
    sb_status_t status;

    for (i = 0u; i < SB_EXTNOR_READY_POLL_LIMIT; i++) {
        status = sb_extnor_spi_read_status(&status_reg);
        if (status != SB_STATUS_OK) {
            return status;
        }

        if ((status_reg & SB_EXTNOR_STATUS_BUSY_MASK) == 0u) {
            return SB_STATUS_OK;
        }

        ql_rtos_task_sleep_ms(SB_EXTNOR_READY_POLL_DELAY_MS);
    }

    return SB_STATUS_TIMEOUT;
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
    unsigned int i;
    unsigned char raw[4] = {0u, 0u, 0u, 0u};
    unsigned char id[SB_EXTNOR_ID_LEN] = {0u, 0u, 0u};
    sb_status_t status;

    if (s_extnor_info.ready != 0) {
        (void)sb_extnor_post_status_event(SB_STATUS_ALREADY_INITIALIZED);
        return SB_STATUS_ALREADY_INITIALIZED;
    }

    status = sb_extnor_prepare_control_pins();
    if (status != SB_STATUS_OK) {
        (void)sb_extnor_post_status_event(status);
        return status;
    }

    for (i = 0u; i < (u32)(sizeof(s_probe_table) / sizeof(s_probe_table[0])); i++) {
        status = sb_extnor_spi_read_id_raw(s_probe_table[i].port, s_probe_table[i].mode, s_probe_table[i].clock,
                                           raw, id);
        if (status != SB_STATUS_OK) {
            SB_LOGW(SB_EXTNOR_MODULE_NAME, "probe port=%d mode=%d clk=%d status=%s",
                    (int)s_probe_table[i].port, (int)s_probe_table[i].mode, (int)s_probe_table[i].clock,
                    sb_status_to_string(status));
            continue;
        }

        SB_LOGI(SB_EXTNOR_MODULE_NAME,
                "probe port=%d mode=%d clk=%d raw=%02x %02x %02x %02x id=%02x %02x %02x",
                (int)s_probe_table[i].port, (int)s_probe_table[i].mode, (int)s_probe_table[i].clock,
                raw[0], raw[1], raw[2], raw[3], id[0], id[1], id[2]);

        if (sb_extnor_id_is_valid(id) != 0) {
            s_extnor_info.port = (int)s_probe_table[i].port;
            s_extnor_info.mode = (int)s_probe_table[i].mode;
            s_extnor_info.clock = (int)s_probe_table[i].clock;
            s_extnor_info.id[0] = id[0];
            s_extnor_info.id[1] = id[1];
            s_extnor_info.id[2] = id[2];
            s_extnor_info.capacity_bytes = sb_extnor_capacity_from_jedec_id(id);
            s_extnor_info.ready = 1;

            SB_LOGI(SB_EXTNOR_MODULE_NAME,
                    "ready spi_port=%d mode=%d clk=%d id=%02x %02x %02x capacity=%u bytes",
                    s_extnor_info.port, s_extnor_info.mode, s_extnor_info.clock,
                    s_extnor_info.id[0], s_extnor_info.id[1], s_extnor_info.id[2],
                    s_extnor_info.capacity_bytes);

            (void)sb_extnor_post_status_event(SB_STATUS_OK);
            return SB_STATUS_OK;
        }
    }

    SB_LOGE(SB_EXTNOR_MODULE_NAME, "no valid jedec id on SPI1/SPI2 GPIO4-GPIO7 candidates");
    (void)sb_extnor_post_status_event(SB_STATUS_NOT_FOUND);
    return SB_STATUS_NOT_FOUND;
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
    unsigned char tx[4u + SB_EXTNOR_PAGE_SIZE_BYTES];
    unsigned char rx[4u + SB_EXTNOR_PAGE_SIZE_BYTES];
    unsigned char *dst;
    u32 remaining;
    u32 offset;
    u32 chunk;
    u32 i;
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
        chunk = (remaining > SB_EXTNOR_PAGE_SIZE_BYTES) ? SB_EXTNOR_PAGE_SIZE_BYTES : remaining;
        tx[0] = SB_EXTNOR_CMD_READ_DATA;
        sb_extnor_addr_to_bytes(address + offset, &tx[1]);
        for (i = 0u; i < chunk; i++) {
            tx[4u + i] = 0xFFu;
        }

        if (ql_spi_write_read((SPI_PORT_E)s_extnor_info.port, rx, tx, 4u + chunk) != 0) {
            return SB_STATUS_FLASH_ERROR;
        }

        for (i = 0u; i < chunk; i++) {
            dst[offset + i] = rx[4u + i];
        }

        offset += chunk;
        remaining -= chunk;
    }

    return SB_STATUS_OK;
}

sb_status_t sb_extnor_write(u32 address, const void *buffer, u32 length)
{
    unsigned char tx[4u + SB_EXTNOR_PAGE_SIZE_BYTES];
    const unsigned char *src;
    u32 remaining;
    u32 offset;
    u32 page_room;
    u32 chunk;
    u32 i;
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
        page_room = SB_EXTNOR_PAGE_SIZE_BYTES - ((address + offset) % SB_EXTNOR_PAGE_SIZE_BYTES);
        chunk = (remaining > page_room) ? page_room : remaining;
        if (chunk > SB_EXTNOR_PAGE_SIZE_BYTES) {
            chunk = SB_EXTNOR_PAGE_SIZE_BYTES;
        }

        status = sb_extnor_spi_write_enable();
        if (status != SB_STATUS_OK) {
            return status;
        }

        tx[0] = SB_EXTNOR_CMD_PAGE_PROGRAM;
        sb_extnor_addr_to_bytes(address + offset, &tx[1]);
        for (i = 0u; i < chunk; i++) {
            tx[4u + i] = src[offset + i];
        }

        if (ql_spi_write((SPI_PORT_E)s_extnor_info.port, tx, 4u + chunk) != 0) {
            return SB_STATUS_FLASH_ERROR;
        }

        status = sb_extnor_wait_ready();
        if (status != SB_STATUS_OK) {
            return status;
        }

        offset += chunk;
        remaining -= chunk;
    }

    return SB_STATUS_OK;
}

sb_status_t sb_extnor_erase_sector(u32 address)
{
    unsigned char tx[4];
    sb_status_t status;

    if ((address % SB_EXTNOR_SECTOR_SIZE_BYTES) != 0u) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (s_extnor_info.ready == 0) {
        return SB_STATUS_NOT_READY;
    }

    if ((s_extnor_info.capacity_bytes != 0u) && (address >= s_extnor_info.capacity_bytes)) {
        return SB_STATUS_INVALID_PARAM;
    }

    status = sb_extnor_spi_write_enable();
    if (status != SB_STATUS_OK) {
        return status;
    }

    tx[0] = SB_EXTNOR_CMD_SECTOR_ERASE_4K;
    sb_extnor_addr_to_bytes(address, &tx[1]);

    if (ql_spi_write((SPI_PORT_E)s_extnor_info.port, tx, (unsigned int)sizeof(tx)) != 0) {
        return SB_STATUS_FLASH_ERROR;
    }

    return sb_extnor_wait_ready();
}
