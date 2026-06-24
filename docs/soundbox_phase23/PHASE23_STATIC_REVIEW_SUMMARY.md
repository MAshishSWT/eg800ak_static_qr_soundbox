# Phase 23 Static Review Summary

Package: `eg800ak_soundbox_phase23_production_fix_ready_to_build.zip`
Firmware version: `1.0.0-phase23-production-fix`
Board: `KAE8_SQ1_260611` / EG800AK-CN

## Scope completed

- External W25Q64 NOR support added through QuecOpen SPI NOR APIs.
- Internal U-drive policy hardened for root common MP3 files, certs, config, ledger, logs, diagnostics, and cache.
- Two-tier audio asset architecture added: four common files on U-drive root and language MP3 files in external NOR.
- Asset manifest and external NOR packer added.
- MQTT TLS certificate path handling, PEM validation, time gate, and single ready event path updated.
- HTTPS registration/health service added.
- One-LED pattern engine retained and documented for single `USER_LED_1` hardware.
- GPIO key HAL rewritten with ISR-safe debounce task and active-low key validation.
- Factory diagnostics expanded for FS, NOR, audio, key, LED, MQTT, HTTP, and certificate checks.
- Documentation added under `interface/soundbox/docs/production_fix/`.

## Static checks performed in the sandbox

- Forbidden-token source scan passed for production-facing source and tools.
- Disabled-feature macro scan passed.
- Old `U:/audio` dependency scan passed.
- Python tooling compile check passed.
- `soundbox_extnor_asset_pack.py` successfully generated a NOR image from the provided MP3 ZIP with 1370 language entries.

## Hardware/toolchain validation required

The sandbox does not contain the QuecOpen ARM compiler, EG800AK SDK build host, modem, PCB, SIM, or cloud credentials. Therefore these checks must be performed on the actual development setup:

1. QuecOpen clean build.
2. SPI4/W25Q64 JEDEC ID read with the DI/DO interchange board quirk enabled.
3. External NOR destructive factory test on a blank or disposable test sector.
4. ES8311 I2C/PCM playback verification.
5. `SPK_SHDN` polarity verification on the 8002A amplifier.
6. U-drive file write/read/rename verification on the target.
7. MQTT TLS connect and subscribe with production certificates and valid RTC/NTP time.
8. HTTPS Kiotel registration and health API validation.
9. SW1/SW2/SW3 short/long press verification.
10. Single LED status pattern verification.

## Acceptance gate

Use `interface/soundbox/docs/production_fix/TEST_PLAN.md` and `tools/soundbox_factory_diag.py` before enabling pilot devices.
