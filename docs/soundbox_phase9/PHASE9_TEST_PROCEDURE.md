# Phase 9 Test Procedure

## 1. Build and boot

1. Build the package inside the EG800AK QuecOpen SDK.
2. Flash the generated image.
3. Confirm logs:

```text
[SB][I][app] starting 1.0.0-phase9-sms-factory-diag
[SB][I][mode] ready mode=production
[SB][I][serial] disabled in production mode
[SB][I][sms] disabled by config
```

## 2. Factory serial test

Build a manufacturing-only image with:

```text
-DSB_FACTORY_SERIAL_DEFAULT_ENABLED=1
```

Send over USB CDC serial:

```json
{"cmd":"diag"}
```

Expected response contains:

```json
{"status":"ok"
```

Provision MQTT config:

```json
{"cmd":"set_config","mqtt_host":"broker.example.com","mqtt_port":1883,"mqtt_client_id":"sb001","mqtt_sub_topic":"upi/sb001/pay","mqtt_pub_topic":"upi/sb001/event","http_base_url":"https://api.example.com/soundbox"}
```

Expected response:

```json
{"status":"ok","detail":"config_saved"}
```

## 3. Secure OTA key provisioning test

Send:

```json
{"cmd":"set_ota_key","key_hex":"00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff"}
```

Expected response:

```json
{"status":"ok","detail":"ota_key_saved"}
```

The key must not appear in device logs.

## 4. SMS recovery test

Enable SMS recovery from factory serial:

```json
{"cmd":"set_config","sms_recovery_enabled":1}
```

Reboot and confirm:

```text
[SB][I][sms] task started
```

Send SMS:

```json
{"cmd":"diag"}
```

Expected SMS reply contains `status":"ok`.

## 5. Lockdown test

Set production mode:

```json
{"cmd":"set_mode","mode":"production"}
```

Build production image without `SB_FACTORY_SERIAL_DEFAULT_ENABLED` and reboot.

Expected:

```text
[SB][I][serial] disabled in production mode
```

Provisioning commands must be rejected unless SMS recovery is enabled.
