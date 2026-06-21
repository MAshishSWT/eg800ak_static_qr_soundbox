# Phase 8 Acceptance Checklist

| Requirement | Result |
|---|---:|
| Builds inside EG800AK SDK | Required |
| OTA task starts | Required |
| Signed manifest parsing works | Required |
| HMAC key is read from securedata | Required |
| No hardcoded OTA key | Required |
| Bad signature rejected | Required |
| SHA-256 hash verified | Required |
| Wrong size rejected | Required |
| Firmware image written through Quectel FOTA API | Required |
| FOTA update flag set only after verify | Required |
| Audio-pack staged through temp file | Required |
| Audio-pack activated by atomic rename | Required |
| OTA progress events posted | Required |
| OTA failure events posted | Required |
| No unsafe string APIs | Required |
| Quectel common headers/libraries untouched | Required |
