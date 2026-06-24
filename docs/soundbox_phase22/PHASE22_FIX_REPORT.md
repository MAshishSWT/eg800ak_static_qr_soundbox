# Phase 22 Critical Fix Report - EG800AK Static QR UPI Soundbox

Source baseline: 1.0.0-phase21-essential-audio-mqttfs
Output package: 1.0.0-phase22-critical-audio-mqttfix

## Hardware facts used
- KAE8_SQ1 sheet 1: MAIN_CTS is routed to SPK_SHDN; GPIO5/GPIO6/GPIO8 are routed to SW1/SW2/SW3; GPIO9 is routed to USER_LED_1.
- KAE8_SQ1 sheet 1 and EG800AK GPIO table: FLASH_SYNC/CLK/DOUT/DIN are routed to KP_MKOUT[1]/KP_MKIN[1]/KP_MKOUT[2]/KP_MKIN[2], which are hardware SPI1 CS/CLK/DOUT/DIN alternate functions; FLASH_WP and FLASH_RST are GPIO-only control lines.
- KAE8_SQ1 sheet 2: W25Q64JWSIQ is connected as an external SPI NOR flash and ES8311 is the external audio codec.
- KAE8_SQ1 sheet 3: BATT_VTG_SENS uses a 120K/47K divider into ADC0.

## Fixes applied
1. Version and phase alignment
   - Updated SB_APP_VERSION_STRING to 1.0.0-phase22-critical-audio-mqttfix.
   - Updated SB_PHASE_NUMBER from 18 to 22.
   - Updated README version and phase notes.

2. Audio prompt path fixes
   - Fixed setup prompt mismatch from unregistered_device.mp3 to device_unregistered.mp3.
   - Added flat UFS fallback: if audio/en/file.mp3 is missing, firmware also checks audio/file.mp3.
   - Added transaction fallback script: if numeric amount tokens are missing, the firmware still plays prefix + provider + thankyou instead of becoming silent.
   - Fixed daily-summary filenames to match supplied audio.zip: transaction.mp3 and transaction_s.mp3.

3. Audio UART transfer tool improvements
   - Extended essential list to include setup, health, summary, transaction scale words, and fallback prompt assets.
   - Added --dry-run to verify file count and total bytes before opening UART.
   - Kept default mode as essential-only to stay below U: capacity.

4. MQTT/TLS fixes
   - MQTT TLS readiness now checks actual UFS certificate files instead of only embedded PEM buffers.
   - Removed duplicate MQTT boot log line.
   - Updated certificate source comment to match FS mode.

## Remaining known limitation
The supplied tools/audio.zip does not include numeric amount files such as 1.mp3, 2.mp3, 10.mp3, 20.mp3, etc. Therefore full amount speech cannot be verified using only this asset zip. Phase 22 prevents silence by falling back to a minimal transaction prompt. For production, provide the complete numeric audio library and transfer the required number tokens.

## Recommended smoke test
1. Build and flash the package.
2. Boot log must show: starting 1.0.0-phase22-critical-audio-mqttfix.
3. Run audio dry-run from PC.
4. Transfer essential audio.
5. Reboot and verify start_tune.mp3 plays.
6. Verify setup/no SIM/no internet/no MQTT prompts.
7. Publish a demo MQTT payment JSON and verify that at minimum prefix/provider/thankyou plays if number prompts are absent.
8. Check MQTT certificate files are created: U:/mqtt_root_ca.pem, U:/mqtt_client.crt, U:/mqtt_client.key.
