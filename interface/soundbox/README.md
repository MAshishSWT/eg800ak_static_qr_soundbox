# Static QR UPI Soundbox - Phase 1 Application Skeleton

This module is the EG800AK-CN QuecOpen application skeleton for the Static QR UPI Soundbox.

Phase 1 provides:

- SDK-compatible `interface/soundbox/Makefile`.
- One production application entry registered through `application_init()`.
- Supervisor task creation with `ql_rtos_task_create()`.
- Event bus using `ql_rtos_queue_create()`, `ql_rtos_queue_release()`, and `ql_rtos_queue_wait()`.
- Heartbeat timer using `ql_rtos_timer_create()` and `ql_rtos_timer_start()`.
- Production logging skeleton.
- Error/status code contract.
- KAE8_SQ1 board mapping header.

This phase does not implement MQTT, audio playback, data call, OTA, SMS, filesystem, or ledger services. Those modules are attached to this event-driven skeleton by the phase-specific service packages.
