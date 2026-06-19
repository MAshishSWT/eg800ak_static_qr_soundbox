# Static QR UPI Soundbox - Phase 2 BSP/HAL

This module is the EG800AK-CN QuecOpen application for the Static QR UPI Soundbox.

Phase 2 provides the Phase 1 application skeleton plus the KAE8_SQ1 board support package:

- SDK-compatible `interface/soundbox/Makefile`.
- One production application entry registered through `application_init()`.
- Supervisor task and event bus.
- KAE8_SQ1 GPIO wrapper layer using EG800AK `ql_gpio_*` and `ql_eint_*` APIs.
- Status LED control on `USER_LED_1`.
- Active-low SW1/SW2/SW3 key edge reporting through the event bus.
- Battery ADC read and divider conversion for `BATT_VTG_SENS`.
- Speaker amplifier shutdown control on `SPK_SHDN`.
- Board initialization and startup board-status event posting.

Payment MQTT, audio playback, data call, OTA, SMS, filesystem, and ledger services are attached in later phase-specific service packages. The Phase 2 code keeps Quectel APIs isolated behind BSP/HAL modules so those services can use stable board-level contracts.
