# EG800AK Static QR UPI Soundbox - Phase 0 File Manifest

Phase 0 is a migration architecture and build-anchor phase. The package does not modify Quectel SDK `common/include` headers, `common/lib` libraries, or existing Quectel example source files.

## New files added

| Path | Purpose |
|---|---|
| `docs/soundbox_phase0/PHASE0_README.md` | Phase 0 summary, integration/build/test instructions, references, and assumptions. |
| `docs/soundbox_phase0/MIGRATION_ARCHITECTURE.md` | Production architecture, module layering, task/event model, data model, OTA/security strategy. |
| `docs/soundbox_phase0/EG800AK_API_REPLACEMENT_MATRIX.md` | EG800AK QuecOpen API/header/example replacement map for EC200U platform APIs. |
| `docs/soundbox_phase0/EC200U_BUSINESS_LOGIC_INVENTORY.md` | EC200U source inventory and reusable business logic extraction plan. |
| `docs/soundbox_phase0/KAE8_SQ1_HARDWARE_MAPPING.md` | Hardware mapping from the KAE8_SQ1 schematic for EG800AK-CN soundbox hardware. |
| `docs/soundbox_phase0/PHASE0_TEST_PROCEDURE.md` | Phase 0 validation and Phase 1 bring-up test procedure. |
| `docs/soundbox_phase0/PHASE0_ACCEPTANCE_CHECKLIST.md` | Phase 0 acceptance status and Phase 1 entry gates. |
| `docs/soundbox_phase0/PHASE0_REVIEW_FIXES.md` | Review comment closure summary. |
| `interface/soundbox/README.md` | Phase 0 soundbox integration anchor description. |
| `interface/soundbox/Makefile` | EG800AK SDK-compatible soundbox module Makefile. |
| `interface/soundbox/include/sb_phase0_contract.h` | Stable Phase 0 public contract/version header. |
| `interface/soundbox/include/sb_board_kae8_sq1.h` | KAE8_SQ1 board mapping constants from the schematic. |
| `interface/soundbox/src/sb_phase0_build_anchor.c` | Buildable Phase 0 anchor source; no `application_init()` registration. |

## Existing file modified

| Path | Change |
|---|---|
| `Makefile` | Adds `interface/soundbox` to `COMMPILE_DIRS` so the Phase 0 anchor source compiles in the SDK Makefile flow. |

## Build integration for Phase 0

The soundbox module is now build-integrated as a Phase 0 anchor. It contributes one small C translation unit and two public headers. It does not start a task, register an application, modify Quectel common headers, or modify Quectel common libraries.
