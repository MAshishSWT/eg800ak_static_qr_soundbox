/*================================================================
 * Static QR UPI Soundbox - Phase 0 Contract
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *
 * This header defines stable Phase 0 metadata used to prove that the
 * soundbox module is build-integrated without registering a runtime app.
 *================================================================*/
#ifndef SB_PHASE0_CONTRACT_H
#define SB_PHASE0_CONTRACT_H

#ifdef __cplusplus
extern "C" {
#endif

#define SB_PHASE0_CONTRACT_MAGIC          (0x53425030u) /* "SBP0" */
#define SB_PHASE0_CONTRACT_VERSION_MAJOR  (0u)
#define SB_PHASE0_CONTRACT_VERSION_MINOR  (2u)
#define SB_PHASE0_CONTRACT_VERSION_PATCH  (0u)
#define SB_PHASE0_CONTRACT_VERSION_STRING "0.2.0-phase0-review-fixed"

const char *sb_phase0_contract_version(void);
unsigned int sb_phase0_contract_magic(void);

#ifdef __cplusplus
}
#endif

#endif /* SB_PHASE0_CONTRACT_H */
