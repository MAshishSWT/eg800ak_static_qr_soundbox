# Soundbox Phase 10 Package Manifest

Package: `EG800AK_Soundbox_Phase10_Audio_Assets_Business_Logic_Source_Package.zip`

Phase 10 integrates `audio_assets.zip` business logic and one-LED adaptation into the Phase 9 review-fixed production baseline.

See:

- `docs/soundbox_phase10/PHASE10_README.md`
- `docs/soundbox_phase10/PHASE10_TEST_PROCEDURE.md`
- `docs/soundbox_phase10/PHASE10_ACCEPTANCE_CHECKLIST.md`
- `docs/soundbox_phase10/PHASE10_FILE_MANIFEST.md`

## Review fix note

- `SOUNDBOX_PHASE10_REVIEW_FIXES.md`

## External NOR SPI4 fix

- `interface/soundbox/include/sb_board_kae8_sq1.h`
- `interface/soundbox/src/sb_bsp_kae8_sq1.c`
- `interface/soundbox/src/sb_extnor.c`
- `SOUNDBOX_PHASE10_EXTNOR_SPI4_FIX.md`

## Raw asset pack UART/FTP update

Added:
- interface/soundbox/include/sb_asset_pack_loader.h
- interface/soundbox/src/sb_asset_pack_loader.c
- tools/soundbox_asset_pack_builder.py
- tools/soundbox_uart_asset_push.py
- SOUNDBOX_PHASE10_RAWPACK_UART_FTP.md

Modified:
- interface/soundbox/Makefile
- interface/soundbox/include/sb_app.h
- interface/soundbox/src/sb_audio_asset.c
- interface/soundbox/src/sb_audio_prompt_logic.c
- interface/soundbox/src/sb_audio_asset_store.c
- interface/soundbox/src/sb_factory_diag.c
- docs/soundbox_phase10/PHASE10_README.md
- docs/soundbox_phase10/PHASE10_TEST_PROCEDURE.md

## MQTT demo default update

- `interface/soundbox/include/sb_demo_profile.h`
- `interface/soundbox/src/sb_demo_profile.c`
- `interface/soundbox/src/sb_config.c`
- `interface/soundbox/src/sb_mqtt_service.c`
- `interface/soundbox/src/sb_http_service.c`
- `interface/soundbox/src/sb_payment_processor.c`
- `interface/soundbox/src/sb_business_service.c`
- `interface/soundbox/src/sb_command_dispatcher.c`
- `SOUNDBOX_PHASE10_MQTT_DEMO_DEFAULTS.md`

## UART fast asset-pack transfer update

- `interface/soundbox/include/sb_asset_pack_loader.h`
- `interface/soundbox/include/sb_serial_service.h`
- `interface/soundbox/include/sb_app.h`
- `tools/soundbox_uart_asset_push.py`
- `SOUNDBOX_PHASE10_UART_FASTPACK.md`

## Root certificate path update

- `interface/soundbox/include/sb_demo_profile.h`
- `interface/soundbox/src/sb_mqtt_service.c`
- `interface/soundbox/src/sb_http_service.c`
- `interface/soundbox/include/sb_app.h`
- `SOUNDBOX_PHASE10_ROOT_CERT_PATH_FIX.md`

## UART asset provisioning enablement

- `interface/soundbox/Makefile`
- `interface/soundbox/src/sb_serial_service.c`
- `interface/soundbox/src/sb_factory_diag.c`
- `interface/soundbox/include/sb_app.h`
- `tools/soundbox_uart_asset_push.py`
- `SOUNDBOX_PHASE10_UART_ASSET_PROVISION.md`

## UART RX line accumulation update

- `interface/soundbox/src/sb_serial_service.c`
- `interface/soundbox/include/sb_serial_service.h`
- `interface/soundbox/include/sb_app.h`
- `SOUNDBOX_PHASE10_UART_RXLINE_FIX.md`

## Main UART asset provisioning update

- `interface/soundbox/Makefile`
- `interface/soundbox/src/sb_serial_service.c`
- `interface/soundbox/include/sb_app.h`
- `SOUNDBOX_PHASE10_MAIN_UART_ASSET_PROVISION.md`

## Phase 10 Default Certificate Creation

Added in this package:

- interface/soundbox/include/sb_default_certs.h
- interface/soundbox/src/sb_default_certs.c
- SOUNDBOX_PHASE10_DEFAULT_CERTS.md

Updated in this package:

- interface/soundbox/Makefile
- interface/soundbox/include/sb_app.h
- interface/soundbox/src/sb_app_main.c
