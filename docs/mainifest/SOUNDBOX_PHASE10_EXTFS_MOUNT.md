# Phase 10 External NOR Filesystem Mount Integration

This package integrates the QuecOpen external SPI NOR filesystem API into the
Phase 10 audio asset store.

## Runtime order

1. Initialize the KAE8 W25Q64 external NOR using SPI NOR port 4.
2. Drive `FLASH_WP` GPIO37 high and `FLASH_RST` GPIO56 high.
3. Mount the external SPI NOR filesystem as `C:` using `qextfs_init()`.
4. If `C:` mount succeeds, audio logical paths are resolved directly as:
   - `audio/common/start_tune.mp3` -> `C:/audio/common/start_tune.mp3`
   - `audio/en/alerts/no_SIM.mp3` -> `C:/audio/en/alerts/no_SIM.mp3`
5. If `C:` mount fails, the firmware remains boot-safe and falls back to:
   - `U:/audio/...` debug assets
   - raw SBAS external-NOR indexed asset pack staged to `U:/sb_play.mp3`

## Mount parameters

```c
qextfs_init('C', "external_fs", format_flag, EXTERNAL_NORFLASH_PORT4_7, 0, 0x00800000);
```

If `external_fs` is not available in the active Quectel partition layout, the
code also tries:

```c
qextfs_init('C', "ext_reserved", format_flag, EXTERNAL_NORFLASH_PORT4_7, 0, 0x00800000);
```

## Format control

By default the firmware does not auto-format the external filesystem:

```c
SB_AUDIO_STORE_EXTFS_FORMAT_ON_MOUNT = 0
```

For a one-time lab format build, add this compile define only once:

```text
-DSB_AUDIO_STORE_EXTFS_FORMAT_ON_MOUNT=1
```

After a successful format, rebuild without this macro. Keeping it enabled will
reformat the external NOR region on every boot.

## Expected logs when mount succeeds

```text
[SB][I][asset_store] ready backend=extfs disk=C root=C:/audio part=external_fs size=8388608
```

## Expected logs when mount is not supported by the active flash layout

```text
[SB][W][asset_store] extfs mount failed disk=C part=external_fs ...
[SB][W][asset_store] external fs unavailable, using raw extnor asset index capacity=8388608
```

This fallback is intentional and keeps the product boot-safe.
