# Phase 8 FOTA Link Fix

The EG800AK LTE01R07A16_C_SDK_A V03 header declares `ql_fota_image_verify_without_setflag()` and `ql_fota_set_update_flag()`, but the linked `ql_common_api.lib` in this SDK does not export those symbols.

The firmware OTA activation path now uses the exported and Quectel-example-verified function:

```c
ql_fota_image_verify(ctx)
```

This call is executed only after:

1. Manifest HMAC-SHA256 signature verification succeeds.
2. HTTP/HTTPS download completes.
3. Download size matches manifest size.
4. SHA-256 of downloaded bytes matches the manifest hash.
5. FOTA image flush succeeds.

This removes the application link failure while preserving rollback-safe activation using the SDK-supported FOTA verify/update path.
