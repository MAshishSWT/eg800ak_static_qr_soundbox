/*================================================================
 * Static QR UPI Soundbox - ES8311 Codec Support
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *
 * The register sequence is adapted from Quectel's EG800AK QuecOpen
 * interface/audio/example_codec_choose.c ES8311 example and wrapped as a
 * production service helper.
 *================================================================*/
#include "ql_iic.h"
#include "ql_rtos.h"
#include "sb_audio_codec_es8311.h"
#include "sb_log.h"

#define SB_ES8311_MODULE_NAME       "es8311"
#define SB_ES8311_I2C_NO            (0u)
#define SB_ES8311_I2C_FAST_MODE     (0u)
#define SB_ES8311_I2C_ADDR          (0x18u)
#define SB_ES8311_REG_DEVICE_ID     (0xFDu)
#define SB_ES8311_DEVICE_ID_VALUE   (0x83u)

#define SB_ES8311_RATIO_PCM_FS_64   (1u)
#define SB_ES8311_FORMAT_DSPA       (0x03u)
#define SB_ES8311_FORMAT_LEN_16     (0x03u)
#define SB_ES8311_SCLK_DIV          (4u)
#define SB_ES8311_SCLK_INV          (0u)
#define SB_ES8311_MS_SLAVE          (0u)
#define SB_ES8311_MCLK_PIN          (0u)
#define SB_ES8311_VDDA_1V8          (1u)
#define SB_ES8311_ADC_CH1           (1u)
#define SB_ES8311_DAC_LEFT          (0u)
#define SB_ES8311_ADC_PGA_GAIN      (8u)
#define SB_ES8311_ADC_VOLUME_0DB    (191u)
#define SB_ES8311_DAC_VOLUME_0DB    (191u)
#define SB_ES8311_DMIC_OFF          (0u)
#define SB_ES8311_ADC2DAC_OFF       (0u)
#define SB_ES8311_DAC_HP_OFF        (0u)

static sb_status_t sb_es8311_write_reg(unsigned char reg, unsigned char value)
{
    unsigned char data = value;
    unsigned char retry;
    unsigned short status;

    for (retry = 0u; retry < 5u; retry++) {
        status = ql_i2c_write(SB_ES8311_I2C_NO, SB_ES8311_I2C_ADDR, reg, &data, 1);
        if (status == 0u) {
            return SB_STATUS_OK;
        }
        ql_rtos_task_sleep_ms(2u);
    }

    return SB_STATUS_ERROR;
}

static sb_status_t sb_es8311_read_reg(unsigned char reg, unsigned char *value)
{
    unsigned char retry;
    unsigned short status;

    if (value == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    for (retry = 0u; retry < 5u; retry++) {
        status = ql_i2c_read(SB_ES8311_I2C_NO, SB_ES8311_I2C_ADDR, reg, value, 1);
        if (status == 0u) {
            return SB_STATUS_OK;
        }
        ql_rtos_task_sleep_ms(2u);
    }

    return SB_STATUS_NOT_FOUND;
}

static sb_status_t sb_es8311_init_registers(void)
{
    sb_status_t status;
    unsigned char reg06;

    reg06 = (unsigned char)((SB_ES8311_SCLK_INV << 5u) + SB_ES8311_SCLK_DIV - 1u);

    status = sb_es8311_write_reg(0x45u, 0x00u);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x01u, 0x30u);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x02u, 0x10u);
    if (status != SB_STATUS_OK) { return status; }

    /* PCM_FS_64 / 16-bit DSP-A slave mode matches the Quectel ES8311 example
     * for the EG800AK external codec path.
     */
    status = sb_es8311_write_reg(0x02u, 0x18u);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x03u, 0x20u);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x16u, 0x20u);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x04u, 0x20u);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x05u, 0x00u);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x06u, reg06);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x07u, 0x00u);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x08u, 0x3Fu);
    if (status != SB_STATUS_OK) { return status; }

    status = sb_es8311_write_reg(0x09u, (unsigned char)((SB_ES8311_DAC_LEFT << 7u) + SB_ES8311_FORMAT_DSPA + (SB_ES8311_FORMAT_LEN_16 << 2u)));
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x0Au, (unsigned char)(SB_ES8311_FORMAT_DSPA + (SB_ES8311_FORMAT_LEN_16 << 2u)));
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x0Bu, 0x00u);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x0Cu, 0x00u);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x10u, (unsigned char)((0x1Cu * SB_ES8311_DAC_HP_OFF) + (0x60u * SB_ES8311_VDDA_1V8) + 0x03u));
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x11u, 0x7Fu);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x00u, (unsigned char)(0x80u + (SB_ES8311_MS_SLAVE << 6u)));
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x0Du, 0x01u);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x01u, (unsigned char)(0x3Fu + (SB_ES8311_MCLK_PIN << 7u)));
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x14u, (unsigned char)((SB_ES8311_DMIC_OFF << 6u) + (SB_ES8311_ADC_CH1 << 4u) + SB_ES8311_ADC_PGA_GAIN));
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x12u, 0x28u);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x13u, (unsigned char)(0x00u + (SB_ES8311_DAC_HP_OFF << 4u)));
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x0Eu, 0x02u);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x0Fu, 0x44u);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x15u, 0x00u);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x1Bu, 0x0Au);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x1Cu, 0x6Au);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x37u, 0x08u);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x44u, (unsigned char)(SB_ES8311_ADC2DAC_OFF << 7u));
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x17u, SB_ES8311_ADC_VOLUME_0DB);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_es8311_write_reg(0x32u, SB_ES8311_DAC_VOLUME_0DB);

    return status;
}

sb_status_t sb_audio_codec_es8311_open(void)
{
    unsigned char id = 0u;
    sb_status_t status;

    if (ql_i2c_init(SB_ES8311_I2C_NO, SB_ES8311_I2C_FAST_MODE) != 0) {
        SB_LOGW(SB_ES8311_MODULE_NAME, "i2c init failed");
        return SB_STATUS_ERROR;
    }

    status = sb_es8311_read_reg(SB_ES8311_REG_DEVICE_ID, &id);
    if (status != SB_STATUS_OK) {
        SB_LOGW(SB_ES8311_MODULE_NAME, "device id read status=%s", sb_status_to_string(status));
        return status;
    }

    if (id != SB_ES8311_DEVICE_ID_VALUE) {
        SB_LOGW(SB_ES8311_MODULE_NAME, "unexpected id=0x%02x", id);
        return SB_STATUS_NOT_FOUND;
    }

    status = sb_es8311_init_registers();
    if (status == SB_STATUS_OK) {
        SB_LOGI(SB_ES8311_MODULE_NAME, "external codec ready id=0x%02x", id);
    } else {
        SB_LOGW(SB_ES8311_MODULE_NAME, "register init status=%s", sb_status_to_string(status));
    }

    return status;
}
