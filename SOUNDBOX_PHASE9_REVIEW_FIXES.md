# Soundbox Phase 9 Review Fixes

## Fixed review blockers

### 1. SMS sender allowlist

`sb_sms_service` now calls `sb_factory_diag_sms_sender_allowed()` before dispatching any SMS command. The sender must match the authorized recovery number stored in `ql_securedata` index 3.

Unauthorized SMS commands are deleted and rejected without sending a provisioning response.

### 2. SMS provisioning locked by default

`sb_factory_can_provision()` no longer permits SMS `set_config` or `set_ota_key` just because `sms_recovery_enabled=1`.

Production builds reject SMS provisioning by default.

A lab-only macro can re-enable insecure SMS provisioning for controlled testing:

```text
-DSB_ENABLE_INSECURE_SMS_RECOVERY_PROVISIONING=1
```

Do not use this macro in production.

### 3. Serial command to provision SMS authorized sender

A factory serial-only command was added:

```json
{"cmd":"set_sms_auth","phone":"+919999999999"}
```

This stores the authorized recovery sender in secure data index 3.

### 4. Supervisor log cleanup

Changed the factory-ready log label from `mode=` to `text=`.
