# Static QR UPI Soundbox Phase 0 Integration Anchor

This directory contains the Phase 0 build anchor and stable public contract headers for the EG800AK-CN Static QR UPI Soundbox application.

Phase 0 scope is migration architecture, EC200U business-logic inventory, EG800AK API replacement mapping, KAE8_SQ1 hardware mapping, build non-regression, and stable naming for Phase 1 implementation. The build anchor compiles without registering an application task and without calling `application_init()`, so the original EG800AK example startup behavior is not changed by Phase 0.

## Contents

- `include/sb_phase0_contract.h` exposes the Phase 0 contract/version symbols.
- `include/sb_board_kae8_sq1.h` captures the verified KAE8_SQ1 schematic net-to-EG800AK mapping available from the PDF schematic.
- `src/sb_phase0_build_anchor.c` provides a small buildable translation unit so the `interface/soundbox` module can be registered in the SDK Makefile flow.
- `Makefile` follows the EG800AK SDK interface module style and includes `config/common/makefile.mk`.

## Policy for Phase 1 code generation

- Add exactly one production `application_init()` entry point when the soundbox supervisor is implemented.
- Keep Quectel SDK API calls inside HAL/platform service modules.
- Do not modify `common/include`, `common/lib`, or Quectel example source files.
- Use typed queues/events for cross-module communication.
- Keep secrets out of logs and out of source files.

Detailed architecture, API mapping, hardware mapping, review closure, test procedure, and acceptance checklist are in `docs/soundbox_phase0/`.
