/*================================================================
 * Static QR UPI Soundbox - Phase 0 Build Anchor
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *
 * This file intentionally does not register an application task. It only proves
 * that the soundbox module is integrated into the EG800AK SDK Makefile flow.
 *================================================================*/
#include "sb_phase0_contract.h"

const char *sb_phase0_contract_version(void)
{
    return SB_PHASE0_CONTRACT_VERSION_STRING;
}

unsigned int sb_phase0_contract_magic(void)
{
    return SB_PHASE0_CONTRACT_MAGIC;
}
