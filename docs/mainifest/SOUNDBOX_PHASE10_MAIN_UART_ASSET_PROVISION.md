# Phase 10 Main UART Asset Provisioning

## Change

Serial asset provisioning is now bound to the EG800AK Main UART instead of USB CDC.

## SDK port

```c
#define SB_SERIAL_PORT QL_MAIN_UART_PORT
#define SB_SERIAL_BAUD QL_UART_BAUD_921600
```

## Wiring

- PC USB-TTL TX -> EG800AK MAIN_RX
- PC USB-TTL RX -> EG800AK MAIN_TX
- PC USB-TTL GND -> board GND
- Use TTL logic level compatible with the EG800AK board IO. Do not connect RS232 voltage directly.
- Flow control is disabled.

## Expected boot log

```text
[SB][I][serial] enabled for asset provisioning
[SB][I][serial] task started port=main_uart baud=921600
[SB][I][supervisor] serial ready text=main_uart
```

## Transfer command

```bash
python tools/soundbox_uart_asset_push.py COM16 Vi_mp3.sbas --chunk 512 --baud 921600 --timeout 20 --retries 3
```
