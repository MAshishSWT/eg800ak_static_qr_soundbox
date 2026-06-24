# Strong Command Prompt for Deep Analysis and Updated Source Package Generation

EXPOSE:
You are a senior embedded firmware engineer for Quectel EG800AK-CN QuecOpen SDK. Deep-dive the attached source package `1.0.0-phase21-essential-audio-mqttfs.zip` and all attached reference documents before changing code:

1. `EG800AK_QuecOpen_SDK_documents(5).zip`
2. `KAE8_SQ1_260611_Sch_01(6).pdf`
3. `Quectel_EG800AK_Series_QuecOpen_GPIO_Configuration_V1.1(3).xlsx`
4. the current phase21 source package

Generate a new ready-to-build source package named `1.0.0-phase22-critical-audio-mqttfix.zip` for EG800AK-CN KAE8_SQ1 hardware. Do not produce pseudo code, placeholders, TODOs, or incomplete stubs. Preserve the existing QuecOpen project layout, coding style, file headers, function headers, and Makefile structure. Keep all changes minimal, deterministic, and production-safe.

Deep analysis requirements:
- Compare the board schematic and EG800AK GPIO configuration table against `sb_board_kae8_sq1.h`, BSP, GPIO, key, LED, ADC, speaker PA, ES8311 codec, SPI/NOR, serial, MQTT, SSL, file-system, and audio modules.
- Confirm these mappings: SW1/SW2/SW3, USER_LED_1, SPK_SHDN, ADC0 battery divider, I2C pins for ES8311, PCM pins for codec, and W25Q64 SPI NOR pins.
- Verify that FLASH_SYNC/FLASH_CLK/FLASH_DOUT/FLASH_DIN use EG800AK hardware SPI1 alternate function and that FLASH_WP/FLASH_RST are GPIO control lines. Do not use bit-bang SPI.
- Compare all QuecOpen API usage with the uploaded Quectel documents: GPIO, ADC, Audio, I2C, File System, SSL, MQTT, Data Call, Serial Port, RTOS, and SPI NOR.
- Analyze the boot logs and observed failures: missing/non-playing audio prompts, MQTT TLS connection failure, certificate file mode, U: filesystem size limit, essential audio transfer, SIM/network disturbance, and non-functioning prompt features.

Mandatory fixes:
1. Versioning/build consistency
   - Update version string, README, docs, and SB_PHASE_NUMBER consistently for phase22.

2. Audio asset correctness
   - Fix all filename mismatches between firmware and `tools/audio.zip`. In particular, use `device_unregistered.mp3` and do not reference missing `unregistered_device.mp3`, `single_txn.mp3`, or `transactions.mp3` unless those files are provided.
   - Implement robust audio path handling: firmware should prefer `U:/audio/<language>/<file>.mp3`, but for the supplied demo zip it must also accept `U:/audio/<file>.mp3` fallback.
   - Ensure boot/start tune, no SIM, internet, no internet, no MQTT, no transactions, battery low, setup/unregistered, provider, transaction error, daily summary, last transaction, and health prompts are handled gracefully.
   - The supplied `tools/audio.zip` does not contain numeric amount MP3s. Do not let the device become silent if amount number tokens are absent. Add a safe fallback transaction prompt that plays at least prefix + provider + thankyou and logs the missing numeric asset. Keep the full amount path intact when number assets are available.
   - Do not copy all audio files by default. Keep default transfer essential-only and verify total U: usage remains safe.

3. UART audio transfer tool
   - Update `tools/soundbox_uart_ufs_push.py` to transfer only essential demo prompts by default.
   - Add dry-run mode to print selected files, target paths, sizes, and total bytes without opening UART.
   - Keep chunk validation compatible with firmware JSON line size: max 1024 raw bytes per chunk; recommend 512 bytes.
   - Make the tool work with both a flat `audio.zip` and a folder tree.
   - Emit clear errors for missing source, empty selection, device reject, CRC mismatch, short write, and timeout.

4. MQTT/TLS and certificates
   - Use SSL_CERT_FROM_FS for MQTT TLS.
   - Ensure root CA, client cert, and client key files are created on UFS only if missing.
   - MQTT readiness must check actual UFS file existence, not just embedded PEM buffers.
   - Remove duplicate logs and add useful status logs for TLS file mode.
   - Preserve reconnection/backoff/circuit-breaker behavior.

5. File-system robustness
   - Confirm U: mount and usage logging.
   - Ensure file upload creates parent directories and writes/flushes/closes safely.
   - Do not allow serial file upload outside `U:/audio` for audio provisioning.
   - Keep config A/B slot CRC handling unchanged unless a real bug is found.

6. Business/MQTT handling
   - Verify payment message parsing and dispatch to audio service.
   - Do not block boot if audio asset store is empty; log missing assets cleanly.
   - Ensure command topic suffix `/cmd` works.
   - Preserve health publishing.

7. Security and production discipline
   - Do not add insecure SMS provisioning unless explicitly macro-gated.
   - Do not expose arbitrary file write outside audio provisioning.
   - Do not print private key content in logs.
   - Do not introduce heap-heavy logic, recursion, large stack buffers, or blocking loops inside interrupt callbacks.

Deliverables:
- Updated complete source package zip.
- A change report listing every modified file and reason.
- A build/run test procedure.
- A PC-side Python audio transfer command and dry-run command.
- A review checklist that confirms no pseudo code, no TODO, no missing source files, no wrong phase number, no bit-bang NOR, and no references to missing MP3 filenames.

Acceptance criteria:
- Source package keeps original QuecOpen directory structure and Makefile compatibility.
- Phase banner logs `1.0.0-phase22-critical-audio-mqttfix`.
- `SB_PHASE_NUMBER` equals 22.
- Essential audio dry-run selects only demo-required prompts and remains below U: capacity.
- Setup prompt references `device_unregistered.mp3`.
- Daily summary references existing transaction prompt files.
- MQTT TLS cert mode uses UFS file paths.
- If number MP3 files are missing, transaction audio does not fail silently; fallback prompt plays and logs the missing asset.
- No raw private key material is printed in logs or report.
