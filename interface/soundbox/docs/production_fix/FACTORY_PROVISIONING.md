# Factory Provisioning

## Step 1: Build firmware

From the QuecOpen SDK application root:

```sh
make clean
make
```

## Step 2: Provision U-drive files

Use:

```sh
python tools/soundbox_uart_ufs_push.py --port COM5 --assets Vi_mp3.zip --cert-dir certs --config config.json --dry-run
```

Remove `--dry-run` after verifying the paths. The tool sends only:

- `U:/start_tune.mp3`
- `U:/ping.mp3`
- `U:/good_bye.mp3`
- `U:/transaction_error.mp3`
- certificate files under `U:/certs`
- config file under `U:/config`

## Step 3: Pack external NOR audio

Use:

```sh
python tools/soundbox_extnor_asset_pack.py --input Vi_mp3.zip --out-dir build_assets
```

The output includes an external NOR image, manifest binary, CRC report, and language report.

## Step 4: Program or stream NOR image

Use the generated image with the production programmer, or send records through the factory diagnostic serial path if enabled in the production station.

## Step 5: Run diagnostics

```sh
python tools/soundbox_factory_diag.py --port COM5 fs_self_test
python tools/soundbox_factory_diag.py --port COM5 nor_id
python tools/soundbox_factory_diag.py --port COM5 nor_rw_test
python tools/soundbox_factory_diag.py --port COM5 play_common start_tune.mp3
python tools/soundbox_factory_diag.py --port COM5 play_lang en internet.mp3
python tools/soundbox_factory_diag.py --port COM5 key_test
python tools/soundbox_factory_diag.py --port COM5 led_test
python tools/soundbox_factory_diag.py --port COM5 cert_check
```
