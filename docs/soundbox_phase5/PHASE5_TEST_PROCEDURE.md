# Phase 5 Test Procedure

## 1. Build test

Run the Quectel SDK build and confirm `interface/soundbox` builds.

## 2. No SIM test

Boot without SIM. Expected:

```text
[SB][W][network] sim not ready
[SB][W][supervisor] sim fault
```

The app must continue running.

## 3. SIM inserted/network registered test

Boot with an active SIM and antenna connected. Expected:

```text
[SB][I][network] sim ready
[SB][I][supervisor] network registered
[SB][I][supervisor] csq=<0-31 or 99>
```

## 4. Data call test

Expected:

```text
[SB][I][network] online
[SB][I][supervisor] data call ready cid=1 status=0
```

## 5. NTP/RTC test

Expected, when NTP server setup succeeds:

```text
[SB][I][time] ntp sync requested server=<selected-server>
[SB][I][supervisor] time synced status=0 server=<selected-server>
[SB][I][network] rtc=YYYY-MM-DD HH:MM:SS
```

Acceptable fallback, when NTP server setup fails but NITZ/RTC is already valid:

```text
[SB][I][time] rtc fallback accepted reason=rtc_nitz_ntp_server time=YYYY-MM-DD HH:MM:SS
[SB][I][supervisor] time synced status=0 server=rtc_nitz_ntp_server
```

## 6. Network loss/reconnect test

Remove antenna or SIM after online. Expected:

```text
[SB][W][network] data call down
[SB][W][supervisor] network lost
```

The service should back off and retry.
