# Phase 10 MQTT Demo Defaults

This package ports the EC200U-CN-AA demo MQTT/cloud defaults into the EG800AK soundbox firmware without hardcoding private certificates or private keys.

## Defaults applied

- APN: `m2misafe`
- Provider: `vi`
- Client code: `client1`
- Model number: `ENPAY4G`
- MQTT host: `a3pzee9xiwvo6-ats.iot.us-east-1.amazonaws.com`
- MQTT port: `8883`
- MQTT client ID template: `device-{imei}`
- Payment subscribe topic template: `kiotel/client1/sb/{imei}`
- Demo publish topic template: `kiotel/client1/sb/{imei}/health`
- HTTP/DMS base URL: `https://103.75.249.183/dms-OMADM`
- HTTP health endpoint: `/healthpacket`
- HTTP command response endpoint: `/commandresponse`

`{imei}` is expanded at runtime using `ql_dev_get_imei()`.

## MQTT payload support

The payment processor now accepts both the existing JSON payment format and the EC200U legacy hash format:

```text
<amount>#<language>#<provider>
```

Example:

```text
12.50#en#paytm
```

The parser converts the amount to paise, maps the provider, stores the transaction in the ledger, and plays the transaction announcement in the language from the payload.

Legacy non-payment payloads beginning with `conf#` or `play#` are ignored as non-payment payloads instead of playing transaction-error audio.

## Health payload

Periodic HTTP/MQTT health payload now follows the EC200U DMS JSON shape:

```json
{
  "soundboxSerial": "<imei>",
  "healthParams": {
    "networkStrength": "<csq>",
    "batteryLevel": "<percent>",
    "powerConnected": true,
    "lastTransactionAmount": "<rupees.paise>"
  }
}
```

## TLS certificates

The EC200U source used certificate buffers. This package does not hardcode certificates, private keys, passwords, or tokens. For AWS IoT demo, load these files through secure provisioning:

- `U:/mqtt_root_ca.pem`
- `U:/mqtt_client.crt`
- `U:/mqtt_client.key`
- `U:/http_root_ca.pem`
