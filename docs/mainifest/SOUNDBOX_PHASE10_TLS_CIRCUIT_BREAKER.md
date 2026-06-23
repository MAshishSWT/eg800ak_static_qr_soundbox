# Phase 10 TLS Circuit Breaker

This package prevents repeated TLS handshake failures from continuously entering the Quectel SSL/HTTP stack.

## Observed log

```text
[FOTA_EXAM][SSLHandshake, 760] socket connected
[FOTA_EXAM][SSLHandshake, 765] *** failed !!! net_set_nonblock returned -0x1
[SB][W][mqtt] network connect failed rc=-1
```

## Interpretation

The default certificate files are now created correctly. The remaining failure is the MQTT TLS handshake/connect path. Repeated TLS connect attempts can stress the Quectel network stack and may lead to reset after some time.

## Fix

- MQTT now counts consecutive connect/TLS failures.
- After 3 failures, MQTT enters a 10-minute cooldown before retrying.
- HTTP transport failures also enter a 10-minute cooldown after 3 failures.
- The circuit breaker protects the demo from continuous SSL retry loops while keeping network, audio, keys, serial provisioning, and raw NOR storage alive.

## Expected log

```text
[SB][W][mqtt] network connect failed rc=-1
[SB][W][mqtt] connect circuit breaker failures=3 cooldown_ms=600000
```

The device should continue running and should not reset due to repeated TLS attempts.
