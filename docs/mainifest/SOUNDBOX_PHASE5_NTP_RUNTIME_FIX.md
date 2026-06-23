# Phase 5 NTP Runtime Fix

## Runtime observation

The device reached SIM ready, network registered, data call ready and RTC valid, but NTP server setup returned generic failure:

```text
[SB][W][time] ntp server set failed ret=1
[SB][I][network] rtc=2026-06-21 16:03:26
```

## Fix

`sb_time_start_ntp_sync()` now:

1. Sets PDP CID first with `ql_ntp_set_cid()`.
2. Tries a bounded NTP server fallback list:
   - configured/default server
   - `time.google.com`
   - `ntp.aliyun.com`
   - `time.windows.com`
3. Starts NTP with `ql_ntp_sync_ex()` when a server is accepted.
4. If all NTP server setup attempts fail but `ql_rtc_get_time()` already returns a valid NITZ/network time, posts `SB_EVENT_TIME_SYNCED` with `rtc_nitz_ntp_server` so later phases can proceed with valid RTC.

The `ql_ntp_init()` call remains removed because this SDK exports no `ql_ntp_init` symbol and Quectel `example_ntp.c` does not call it.
