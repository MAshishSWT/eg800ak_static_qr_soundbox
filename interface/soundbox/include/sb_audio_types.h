/*================================================================
 * Static QR UPI Soundbox - Audio Types
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_AUDIO_TYPES_H
#define SB_AUDIO_TYPES_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_AUDIO_PATH_LEN             (128u)
#define SB_AUDIO_SCRIPT_MAX_ITEMS     (72u)
#define SB_AUDIO_LANG_CODE_LEN        (4u)
#define SB_AUDIO_PROVIDER_NAME_LEN    (16u)

/* Supported product audio languages. Each language maps to one asset directory
 * under U:/audio/<code>/.
 */
typedef enum {
    SB_AUDIO_LANG_EN = 0,
    SB_AUDIO_LANG_HI,
    SB_AUDIO_LANG_MR,
    SB_AUDIO_LANG_GU,
    SB_AUDIO_LANG_BN,
    SB_AUDIO_LANG_KN,
    SB_AUDIO_LANG_ML,
    SB_AUDIO_LANG_TA,
    SB_AUDIO_LANG_TE,
    SB_AUDIO_LANG_PA,
    SB_AUDIO_LANG_COUNT
} sb_audio_language_t;

/* Payment provider prompt selector. */
typedef enum {
    SB_AUDIO_PROVIDER_OTHER = 0,
    SB_AUDIO_PROVIDER_PAYTM,
    SB_AUDIO_PROVIDER_PHONEPE,
    SB_AUDIO_PROVIDER_GPAY,
    SB_AUDIO_PROVIDER_BHIM,
    SB_AUDIO_PROVIDER_COUNT
} sb_audio_provider_t;

/* Device and transaction prompt identifiers. */
typedef enum {
    SB_AUDIO_PROMPT_POWER_ON = 0,
    SB_AUDIO_PROMPT_READY,
    SB_AUDIO_PROMPT_SETUP,
    SB_AUDIO_PROMPT_NO_SIM,
    SB_AUDIO_PROMPT_NO_NETWORK,
    SB_AUDIO_PROMPT_NO_INTERNET,
    SB_AUDIO_PROMPT_NO_MQTT,
    SB_AUDIO_PROMPT_BATTERY_LOW,
    SB_AUDIO_PROMPT_TRANSACTION_ERROR,
    SB_AUDIO_PROMPT_PAYMENT_RECEIVED,
    SB_AUDIO_PROMPT_COUNT
} sb_audio_prompt_id_t;

typedef struct {
    char path[SB_AUDIO_PATH_LEN];
} sb_audio_script_item_t;

typedef struct {
    sb_audio_script_item_t items[SB_AUDIO_SCRIPT_MAX_ITEMS];
    u32 count;
} sb_audio_script_t;

const char *sb_audio_language_code(sb_audio_language_t language);
const char *sb_audio_language_asset_code(sb_audio_language_t language);
const char *sb_audio_provider_name(sb_audio_provider_t provider);
const char *sb_audio_prompt_name(sb_audio_prompt_id_t prompt);
sb_audio_language_t sb_audio_language_from_code(const char *code);

#ifdef __cplusplus
}
#endif

#endif /* SB_AUDIO_TYPES_H */
