# Phase 8 Review Fixes

This package fixes the strict Phase 8 review comments.

## Fixes applied

1. Audio OTA target allowlist
   - Audio OTA target path must start with `U:/audio/`.
   - `..` and backslash are rejected.
   - OTA can no longer overwrite config, ledger, certificate, or arbitrary U: files.

2. Rollback-safe audio activation
   - Existing audio files are renamed to backup before activation.
   - New files are renamed into place only after temp write succeeds.
   - If any step fails, previous files are restored.

3. Complete audio pack semantics
   - Added Soundbox Audio Pack binary format with magic `SBAP`.
   - Supports up to 16 files per signed pack.
   - Each file has an allowlisted `U:/audio/...` target path.
   - Whole pack is SHA-256 checked and HMAC-signed before extraction.

4. Single signed audio asset mode retained
   - If manifest contains `target_path`, it updates one signed asset safely.
