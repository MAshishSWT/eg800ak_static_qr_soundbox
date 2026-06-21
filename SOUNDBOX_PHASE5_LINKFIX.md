# Phase 5 Link Fix

## Issue

The Quectel EG800AK SDK header `ql_ntp.h` declares `ql_ntp_init()`, but the linked `ql_common_api.lib` in this SDK package does not export that symbol. The application link failed with:

```text
undefined reference to `ql_ntp_init`
```

## Fix

Removed the `ql_ntp_init()` call from `sb_time_service_init()`. The implementation now follows `interface/ntp/example_ntp.c`, which uses `ql_ntp_set_server()`, `ql_ntp_set_cid()` and `ql_ntp_sync_ex()` without calling `ql_ntp_init()`.

## Modified file

```text
interface/soundbox/src/sb_time_service.c
```
