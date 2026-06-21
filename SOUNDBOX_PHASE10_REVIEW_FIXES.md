# Soundbox Phase 10 Review Fixes

The external-NOR hardware-interface item is intentionally deferred to the final hardware-dependent phase. The remaining Phase 10 review comments were fixed in this package.

## Fixed items

1. Automatic no-SIM audio alert is now wired from `SB_EVENT_SIM_FAULT`.
2. Automatic internet OK audio alert is now wired from `SB_EVENT_DATACALL_READY`.
3. Automatic no-internet audio alert is now wired from network/data-call fault events.
4. Automatic no-MQTT audio alert is now wired from MQTT fault/disconnect events.
5. Battery-low audio alert is now wired when the reported battery threshold crosses 15% or 10%.
6. Shutdown/goodbye tune is now connected to the controlled `power_down` command path.
7. Alert prompts are rate-gated so the same fault does not enqueue repeatedly every heartbeat.

## Files changed

- `interface/soundbox/src/sb_supervisor.c`
- `interface/soundbox/src/sb_command_dispatcher.c`

## External NOR note

The current external NOR JEDEC issue remains non-blocking and is deferred. The firmware still boots safely and uses `U:/audio` debug fallback while the hardware-dependent SPI/NOR issue is investigated.
