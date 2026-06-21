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

Expected:

```text
[SB][I][time] ntp sync requested server=pool.ntp.org
[SB][I][supervisor] time synced status=0 server=pool.ntp.org
[SB][I][network] rtc=YYYY-MM-DD HH:MM:SS
```

## 6. Network loss/reconnect test

Remove antenna or SIM after online. Expected:

```text
[SB][W][network] data call down
[SB][W][supervisor] network lost
```

The service should back off and retry.
