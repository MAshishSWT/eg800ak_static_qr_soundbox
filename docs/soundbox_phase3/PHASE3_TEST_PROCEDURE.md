# Phase 3 Test Procedure

## 1. Static source inspection

Run from the package root:

```sh
grep -R "application_init" interface/soundbox --include='*.c' --include='*.h'
grep -R "strcpy\|strcat\|sprintf\|gets(" interface/soundbox || true
grep -R "SENSITIVE_MATERIAL_MARKER" interface/soundbox docs/soundbox_phase3 || true
```

Expected result:

- Exactly one active `application_init()` in `interface/soundbox/src/sb_app_main.c`.
- No unsafe fixed-buffer string construction calls.
- No embedded sensitive material.

## 2. Build registration inspection

Run:

```sh
grep -n "COMMPILE_DIRS" Makefile
sed -n '1,140p' interface/soundbox/Makefile
```

Expected result:

- Root build uses only `interface/soundbox`.
- Soundbox Makefile lists `sb_crc32.c`, `sb_storage_fs.c`, `sb_storage_nor.c`, and `sb_config.c`.

## 3. Vendor build

Use the EG800AK QuecOpen build environment:

```bat
build.bat clean
build.bat app
```

Expected result:

- No duplicate application entry.
- No missing file-system, SPI NOR, or config symbols.
- No Quectel example application is compiled into the production soundbox image.

## 4. Boot and first-run storage test

Expected logs:

```text
[SB][I][app] starting 1.0.0-phase3-storage-config
[SB][I][bsp] led ok
[SB][I][bsp] speaker_pa ok
[SB][I][bsp] adc ok
[SB][I][bsp] keys ok
[SB][I][config] defaults committed
[SB][I][supervisor] storage ready nor_status=<status>
[SB][I][supervisor] config ready seq=1 source=0
```

Expected file state:

```text
U:/soundbox/config/config_a.bin
```

## 5. Reboot persistence test

Reboot the board without deleting files.

Expected logs:

```text
[SB][I][config] loaded slot=0 seq=1
[SB][I][supervisor] config ready seq=1 source=1
```

## 6. A/B slot update test

Call `sb_config_get()`, change a non-sensitive test field such as `volume_percent`, then call `sb_config_commit()`.

Expected result:

- Active sequence increments.
- Active slot alternates between A and B.
- Reboot loads the highest valid sequence.

## 7. CRC recovery test

Corrupt one slot file using the factory/debug file path and reboot.

Expected result:

- The valid slot is selected.
- Corrupt slot is rejected by CRC validation.
- If both slots are invalid or absent, defaults are committed safely.

## 8. SPI NOR abstraction test

Call `sb_storage_nor_get_info()` after boot.

Expected result:

- If the external NOR is detected, `ready=1` and a three-byte ID is available.
- If the driver cannot identify the chip on supported ports, the config service continues using file-system storage and reports the probe status in logs.
