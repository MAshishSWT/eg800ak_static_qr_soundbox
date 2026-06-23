# Static QR UPI Soundbox Phase 5 Package Manifest

Phase 5 adds SIM, network registration, PDP data-call, CSQ, RTC and NTP services.

Link-fix note: `ql_ntp_init()` was removed because it is declared in the SDK header but not exported by the EG800AK SDK library. The implementation follows Quectel `interface/ntp/example_ntp.c`.

See `docs/soundbox_phase5/PHASE5_FILE_MANIFEST.md` and `SOUNDBOX_PHASE5_LINKFIX.md`.
