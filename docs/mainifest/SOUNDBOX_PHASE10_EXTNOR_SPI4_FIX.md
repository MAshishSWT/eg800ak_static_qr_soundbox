# Phase 10 External NOR SPI4 Fix

Applied hardware bring-up changes requested for KAE8_SQ1 external W25Q64 NOR flash.

## Changes

1. Interchanged the board GPIO constants for external flash data lines:
   - `SB_KAE8_FLASH_DOUT_GPIO` now maps to `GPIO_PIN_NO_6`.
   - `SB_KAE8_FLASH_DIN_GPIO` now maps to `GPIO_PIN_NO_7`.

2. `SB_KAE8_FLASH_WP_GPIO` and `SB_KAE8_FLASH_RST_GPIO` are driven high by default during board initialization.

3. External NOR service now uses Quectel SPI NOR port 4:
   - `EXTERNAL_NORFLASH_PORT4_7`
   - `_APBC_SSP_FNCLKSEL_812_5KHZ_`

4. Generic `SPI_PORT1` / `SPI_PORT2` probing was removed from `sb_extnor.c`.

## Expected boot log

```text
[SB][I][bsp] flash_wp_high ok
[SB][I][bsp] flash_rst_high ok
[SB][I][extnor] control wp_gpio=37 rst_gpio=56 high, spi_nor_port=4 ...
[SB][I][extnor] probe spi_nor_port=4 id=xx xx xx
```

If the flash still returns invalid ID, the app remains non-blocking and continues boot.

## Runtime capacity decode follow-up

The first SPI4 boot returned `id=17 60 ef`, which is valid W25Q64-class information in controller/native byte order. Capacity decode now accepts any byte in the serial-NOR density-code range and reports 0x17 as 8 MB.

Expected corrected log:

```text
[SB][I][extnor] probe spi_nor_port=4 id=17 60 ef
[SB][I][extnor] ready spi_nor_port=4 clk=6 id=17 60 ef capacity=8388608 bytes
```
