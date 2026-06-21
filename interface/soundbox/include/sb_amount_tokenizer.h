/*================================================================
 * Static QR UPI Soundbox - Amount Tokenizer
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_AMOUNT_TOKENIZER_H
#define SB_AMOUNT_TOKENIZER_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_AMOUNT_MAX_TOKENS       (48u)
#define SB_AMOUNT_MAX_RUPEES       (9999999ull)

typedef enum {
    SB_AMOUNT_TOKEN_NUMBER = 0,
    SB_AMOUNT_TOKEN_HUNDRED,
    SB_AMOUNT_TOKEN_THOUSAND,
    SB_AMOUNT_TOKEN_LAKH,
    SB_AMOUNT_TOKEN_RUPEES,
    SB_AMOUNT_TOKEN_PAISE,
    SB_AMOUNT_TOKEN_ONLY
} sb_amount_token_kind_t;

typedef struct {
    sb_amount_token_kind_t kind;
    u32 value;
} sb_amount_token_t;

typedef struct {
    sb_amount_token_t tokens[SB_AMOUNT_MAX_TOKENS];
    u32 count;
} sb_amount_token_list_t;

void sb_amount_token_list_init(sb_amount_token_list_t *list);
sb_status_t sb_amount_tokenize_paise(u64 amount_paise, sb_amount_token_list_t *out);
const char *sb_amount_token_kind_name(sb_amount_token_kind_t kind);

#ifdef __cplusplus
}
#endif

#endif /* SB_AMOUNT_TOKENIZER_H */
