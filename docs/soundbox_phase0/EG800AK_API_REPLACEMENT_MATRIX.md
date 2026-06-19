# EG800AK API Replacement Matrix

This matrix maps the EC200U application APIs to EG800AK APIs confirmed in `ql_application_eg800ak.zip`. Do not use EC200U-only APIs in the EG800AK port.

## 1. Application startup and RTOS

| EC200U source pattern | EG800AK replacement | Header | Example reference | Notes |
|---|---|---|---|---|
| `ql_rtos_task_create()` from EC200U OSI layer | `ql_rtos_task_create(ql_task_t *taskRef, u32 stackSize, u8 priority, char *taskName, void (*taskStart)(void *), void *argv, ...)` | `common/include/ql_rtos.h` | `interface/os/example_rtos.c` | Same concept, but use exact EG800AK signature and priorities. |
| EC200U global flags for module state | `ql_rtos_queue_create()`, `ql_rtos_queue_release()`, `ql_rtos_queue_wait()` | `ql_rtos.h` | `example_rtos.c` | Use typed events; avoid shared mutable flags. |
| `ql_rtos_semaphore_create/wait/release` EC200U usage | `ql_rtos_semaphore_create()`, `ql_rtos_semaphore_wait()`, `ql_rtos_semaphore_release()` | `ql_rtos.h` | `example_rtos.c`, `example_httpclient_perform.c` | Use for async callbacks only. |
| EC200U periodic system tick thread | `ql_rtos_timer_create()`, `ql_rtos_timer_start()`, `ql_rtos_get_systicks()` | `ql_rtos.h` | `interface/time/example_timer.c`, `example_wtd.c` | Prefer timers/events over busy periodic loops. |
| EC200U app init | `application_init(app_entry, app_name, stack_size_kib, startup_prio)` | `ql_application.h` | Many examples | Add one production supervisor entry only. |

## 2. GPIO, keys, LED, speaker PA

| EC200U source pattern | EG800AK replacement | Header | Example reference | Notes |
|---|---|---|---|---|
| `ql_pin_set_gpio()` / `ql_pin_set_drving()` | `ql_pin_set_func()`, `ql_gpio_set_drv_level()` where needed | `ql_gpio.h` | `interface/driver/example_gpio.c` | Verify pin function per EG800AK pinmux. |
| GPIO output set/clear | `ql_gpio_init()`, `ql_gpio_set_level()` | `ql_gpio.h` | `example_gpio.c` | Use for LED and speaker PA shutdown. |
| GPIO input read | `ql_gpio_get_level()` | `ql_gpio.h` | `example_gpio.c` | Use for key polling or validation. |
| Key interrupt | `ql_eint_register()`, `ql_eint_enable()`, `ql_eint_disable()` | `ql_gpio.h` | `example_rtos.c`, `example_eint.c` | ISR should push a small event to a queue and re-enable EINT. |
| PWRKEY callback | `ql_pwrkey_register_irq()`, `ql_pwrkey_intc_enable()` | `ql_power.h` | `interface/driver/example_pwrkey.c` | Use only for PWRKEY, not SW1/SW2/SW3 unless hardware requires. |

## 3. Battery ADC

| EC200U source pattern | EG800AK replacement | Header | Example reference | Notes |
|---|---|---|---|---|
| Battery voltage read | `ql_adc_init()`, `ql_adc_read(channel, &mv)`, `ql_adc_read_raw_value()` | `ql_adc.h` | `interface/driver/example_adc.c` | KAE8 `BATT_VTG_SENS` is on EG800AK ADC0 according to schematic. |
| Battery percent calculation | Keep business logic concept, rewrite in `sb_battery_service` | `ql_adc.h` | `example_adc.c` | Use calibration table and resistor divider formula, not hard-coded EC200U values. |

KAE8 divider from schematic: `B+ -> R24 120K -> BATT_VTG_SENS -> R27 47K -> B-`. The measured ADC voltage is approximately `VBAT * 47 / (120 + 47) = VBAT * 0.2814`.

## 4. Audio and codec

| EC200U source pattern | EG800AK replacement | Header | Example reference | Notes |
|---|---|---|---|---|
| `ql_set_audio_path_speaker()` | `ql_set_audio_path_speaker()` | `ql_audio.h` | `interface/audio/example_mp3.c` | Confirm speaker path with ES8311/8002A hardware. |
| `ql_set_volume()` | `ql_set_volume(AUDIOHAL_SPK_LEVEL_T vol)` and `ql_get_volume()` | `ql_audio.h` | `example_mp3.c` | Keep volume state in config manager. |
| MP3 file play | `ql_play_mp3()`, `ql_stop_mp3_play()` or `ql_mp3_file_play()` / `ql_mp3_file_stop()` | `ql_audio.h` | `example_mp3.c`, `example_play_audio.c` | Select one API family and wrap it in `sb_hal_audio`. |
| Speaker PA enable | `ql_bind_speakerpa_cb(ql_cb_on_speakerpa cb)` plus board GPIO | `ql_audio.h`, `ql_gpio.h` | `example_mp3.c` | Map callback to `SPK_SHDN` control on KAE8. |
| PCM stream | `ql_pcm_write()`, `ql_pcm_play_init()`, `ql_pcm_play()` if required | `ql_audio.h` | `example_pcm.c`, `example_play_audio.c` | Prefer MP3 file playback for prompt assets. |
| ES8311 direct I2C | `ql_i2c_init()`, `ql_i2c_write()`, `ql_i2c_read()` only if direct codec setup is required | `ql_iic.h` | `interface/driver/example_iic.c` | Do not write codec registers unless Quectel audio path requires board-level setup. |

## 5. File system and external NOR

| EC200U source pattern | EG800AK replacement | Header | Example reference | Notes |
|---|---|---|---|---|
| `ql_fs` file access | `ql_fopen()`, `ql_fread()`, `ql_fwrite()`, `ql_fclose()`, `ql_fseek()`, `ql_remove()`, `ql_rename()`, `ql_mkdir()`, `ql_opendir()` | `common/include/fs/ql_fs.h` | `interface/fs/example_fs.c` | Use for config, ledger, manifest, audio asset files where mounted. |
| EC200U `ql_api_spi6_ext_nor_flash` | `ql_spi_nor_init()`, `ql_spi_nor_read_id()`, `ql_spi_nor_read()`, `ql_spi_nor_write()`, `ql_spi_nor_erase_sector()`, `ql_spi_nor_erase_chip()` | `ql_spi_nor.h` | `interface/flash/example_spi_nor.c` | KAE8 has W25Q64JWSIQ on FLASH_* nets. Validate selected port. |
| Raw flash partition APIs | `ql_norflash_get_addr()`, `ql_norflash_do_erase()`, `ql_norflash_read_id()` | `ql_flash.h` | `interface/flash/example_flashspeed.c` | Use only for confirmed partitions. |

## 6. SIM, network, data call

| EC200U source pattern | EG800AK replacement | Header | Example reference | Notes |
|---|---|---|---|---|
| `ql_sim_get_card_status_phy(nSim, ...)` | `ql_sim_get_card_status(int *card_status)` | `ql_sim.h` | `interface/network/sim/example_sim.c` | Confirm status enum values from EG800AK header. |
| IMSI/ICCID | `ql_sim_get_imsi()`, `ql_sim_get_iccid()` | `ql_sim.h` | `example_sim.c` | Store only if product policy requires. |
| Network wait | `ql_network_register_wait(timeout_s)` | `ql_data_call.h` | `interface/network/data_call/example_datacall.c`, `interface/mqtt/example_mqtt.c` | Use before data call. |
| CSQ | `ql_nw_get_csq(int *csq)` | `ql_nw.h` | `interface/network/nw/example_nw.c` | Use for signal announcement. |
| Registration info | `ql_nw_get_reg_status()` | `ql_nw.h` | `example_nw.c` | Use for diagnostics. |
| Data call start | `ql_start_data_call(profile_idx, ip_version, apn_name, username, password, auth_type)` | `ql_data_call.h` | `example_datacall.c` | EG800AK signature has no SIM ID argument. |
| Data call info | `ql_get_data_call_info(profile_idx, ip_version, &info)` | `ql_data_call.h` | `example_datacall.c` | Use to validate IP/DNS. |
| Data call stop | `ql_stop_data_call(profile_idx, ip_version)` | `ql_data_call.h` | `example_datacall.c` | Use during recovery. |
| WAN callback | `ql_wan_start(nw_status_cb)` | `ql_data_call.h` | `example_datacall.c`, `example_mqtt.c` | Use to observe state changes. |
| Auto connect | `ql_set_auto_connect(profile_idx, TRUE)` | `ql_data_call.h` | `example_datacall.c` | Controlled by config policy. |

## 7. MQTT

| EC200U source pattern | EG800AK replacement | Header | Example reference | Notes |
|---|---|---|---|---|
| `ql_mqtt_client_init()` | `MQTTClientInit()` with `NetworkInit()` | `common/include/mqtt/MQTTClient.h`, `MQTTQlRTOS.h` | `interface/mqtt/example_mqtt.c` | EG800AK uses Paho-style MQTT in this SDK package. |
| `ql_mqtt_connect()` | `NetworkConnect()` then `MQTTConnect()` | MQTT headers | `example_mqtt.c` | Configure TLS through `SSLConfig`. |
| `ql_mqtt_sub_unsub()` | `MQTTSubscribe()` and `MQTTUnsubscribe()` | `MQTTClient.h` | `example_mqtt.c` | Callback receives `MessageData *`. |
| `ql_mqtt_publish()` | `MQTTPublish()` | `MQTTClient.h` | `example_mqtt.c` | Use bounded publish buffers. |
| `ql_mqtt_set_inpub_callback()` | Subscribe callback parameter in `MQTTSubscribe()` | `MQTTClient.h` | `example_mqtt.c` | Copy payload immediately; callback-owned memory must not be retained. |
| MQTT async task mode | `MQTTStartTask()` when `MQTT_TASK` is enabled | `MQTTUserConfig.h`, `MQTTClient.h` | `example_mqtt.c` | Otherwise call `MQTTYield()` in service loop. |
| TLS cert buffers | `SSLConfig` fields | `ql_ssl_hal.h` | `example_mqtt.c`, `interface/ssl/example_ssl.c` | Production should load certs from secure store/files. |

## 8. HTTP/HTTPS

| EC200U source pattern | EG800AK replacement | Header | Example reference | Notes |
|---|---|---|---|---|
| Raw HTTP request string over socket | `ql_http_client_init()` + `ql_http_client_setopt()` + `ql_http_client_perform()` | `ql_http_client.h` | `interface/http/example_httpclient.c`, `example_httpclient_perform.c` | Do not manually format full HTTP request unless socket-level implementation is deliberately selected. |
| Response callback | `QL_HTTP_CLIENT_OPT_RESPONSE_FUNC` with callback | `ql_http_client.h` | `example_httpclient_perform.c` | Callback handles header/body/disconnect events. |
| POST payload upload | `QL_HTTP_CLIENT_OPT_REQUEST_METHOD`, `QL_HTTP_CLIENT_OPT_UPLOAD_FUNC`, `QL_HTTP_CLIENT_OPT_UPLOAD_PARAM`, `QL_HTTP_CLIENT_OPT_UPLOAD_LEN` | `ql_http_client.h` | `example_httpclient_perform.c` | Use for health and command response. |
| HTTP headers | `ql_http_client_list_append()` and `QL_HTTP_CLIENT_OPT_HTTPHEADER` | `ql_http_client.h` | `example_httpclient_perform.c` | Add content type, auth token, device headers. |
| HTTPS/TLS | `QL_HTTP_CLIENT_OPT_SSL_CTX` with `SSLConfig` | `ql_http_client.h`, `ql_ssl_hal.h` | `example_httpclient_perform.c`, `example_ssl.c` | Do not hard-code production keys. |

## 9. RTC and NTP

| EC200U source pattern | EG800AK replacement | Header | Example reference | Notes |
|---|---|---|---|---|
| `ql_api_rtc.h` time get/set | `ql_rtc_get_time()`, `ql_rtc_set_time()`, `ql_rtc_get_time_ex()` | `ql_rtc.h` | `interface/time/example_rtc.c` | Use for date reset and timestamps. |
| `ql_ntp_client.h` | `ql_ntp_set_server()`, `ql_ntp_set_cid()`, `ql_ntp_sync_ex(cb)` | `ql_ntp.h` | `interface/ntp/example_ntp.c` | Use after data call. |
| NITZ | `ql_nw_get_nitz_time_info()` | `ql_nw.h` | `interface/network/nw/example_nw.c` | Use before NTP where available. |

## 10. SMS recovery/config

| EC200U source pattern | EG800AK replacement | Header | Example reference | Notes |
|---|---|---|---|---|
| SMS event callback | `ql_sms_add_event_handler(handler, context)` | `ql_sms.h` | `interface/network/sms/example_sms.c` | Disabled by default in production. |
| Read received SMS | `ql_search_sms_text_message(index, &payload)` | `ql_sms.h` | `example_sms.c` | Validate allowlist and command signature/password. |
| Send SMS | `ql_sms_send_text_msg()` | `ql_sms.h` | `example_sms.c` | Optional recovery acknowledgement. |

## 11. Device identity and secure storage

| EC200U source pattern | EG800AK replacement | Header | Example reference | Notes |
|---|---|---|---|---|
| `ql_dev_get_imei()` | `ql_dev_get_imei(char *imei, unsigned int imei_len)` | `ql_dev.h` | `interface/dev/example_dev.c` | Use as default serial only if product policy allows. |
| Firmware version | `ql_dev_get_firmware_version()` / product app version from config | `ql_dev.h` | `example_dev.c` | Distinguish module FW and application FW. |
| `ql_cust_nvm_fread/fwrite()` | Use `ql_securedata_read/store()` for secrets and `ql_fs`/flash-backed A/B config for normal config | `ql_securedata.h`, `fs/ql_fs.h` | `interface/securedata/example_securedata.c`, `interface/fs/example_fs.c` | `ql_cust_nvm_*` not present in EG800AK package. |

## 12. Firmware OTA

| EC200U source pattern | EG800AK replacement | Header | Example reference | Notes |
|---|---|---|---|---|
| FTP FOTA flow | Prefer HTTPS manifest + `ql_fota_firmware_download()` or image-write flow | `ql_fota.h` | `interface/fota/example_http_fota.c`, `example_fota.c` | Use manifest hash/signature verification before applying. |
| Progress | `ql_fota_get_progress()` and callback type `qlFotaProgressCB_t` | `ql_fota.h` | `example_http_fota.c` | Report status via HTTP command response. |
| Verify downloaded image | `ql_fota_image_verify()` / `ql_fota_image_verify_without_setflag()` | `ql_fota.h` | `example_fota.c` | Use before setting update flag. |

## 13. Watchdog and power recovery

| EC200U source pattern | EG800AK replacement | Header | Example reference | Notes |
|---|---|---|---|---|
| Watchdog enable/feed | `ql_wtd_timeoutperiod_set()`, `ql_wtd_faultwake_enable()`, `ql_wtd_enable()`, `ql_wtd_feed()` | `ql_wtd.h` | `interface/platform/example_wtd.c` | Feed only from supervisor health policy. |
| Reset | `ql_power_reset()` / `ql_power_reset_fast()` | `ql_power.h` | `interface/driver/example_power.c` | Use only through controlled fault policy. |
| Powerdown | `ql_power_down()` | `ql_power.h` | `example_power.c` | Not normally used for soundbox except factory/test. |

## 14. APIs not to carry over from EC200U

Do not use these EC200U APIs in the EG800AK port unless they are present in the target EG800AK SDK package:

- `ql_api_osi.h`
- `ql_api_datacall.h`
- `ql_api_nw.h`
- `ql_api_sim.h`
- `ql_mqttclient.h`
- `ql_ssl.h`
- `ql_api_dev.h`
- `ql_api_rtc.h`
- `ql_ntp_client.h`
- `ql_api_spi6_ext_nor_flash.h`
- `ql_cust_nvm_fread()`
- `ql_cust_nvm_fwrite()`
- `ql_bind_sim_and_profile()`
- EC200U-specific GPIO identifiers such as `GPIO_10`, `GPIO_20`, `GPIO_19` without EG800AK mapping validation.
