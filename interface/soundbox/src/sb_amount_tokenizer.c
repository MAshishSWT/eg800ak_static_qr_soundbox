/*================================================================
 * Static QR UPI Soundbox - Amount Tokenizer
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "sb_amount_tokenizer.h"

static sb_status_t sb_amount_append(sb_amount_token_list_t *list,
                                    sb_amount_token_kind_t kind,
                                    u32 value)
{
    if (list == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (list->count >= SB_AMOUNT_MAX_TOKENS) {
        return SB_STATUS_NO_MEMORY;
    }

    list->tokens[list->count].kind = kind;
    list->tokens[list->count].value = value;
    list->count++;
    return SB_STATUS_OK;
}

static sb_status_t sb_amount_append_under_100(sb_amount_token_list_t *list, u32 value)
{
    sb_status_t status;
    u32 tens;
    u32 ones;

    if (value > 99u) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (value <= 20u) {
        return sb_amount_append(list, SB_AMOUNT_TOKEN_NUMBER, value);
    }

    tens = (value / 10u) * 10u;
    ones = value % 10u;

    status = sb_amount_append(list, SB_AMOUNT_TOKEN_NUMBER, tens);
    if (status != SB_STATUS_OK) {
        return status;
    }

    if (ones != 0u) {
        status = sb_amount_append(list, SB_AMOUNT_TOKEN_NUMBER, ones);
    }

    return status;
}

static sb_status_t sb_amount_append_under_1000(sb_amount_token_list_t *list, u32 value)
{
    sb_status_t status;
    u32 hundreds;
    u32 rem;

    if (value > 999u) {
        return SB_STATUS_INVALID_PARAM;
    }

    hundreds = value / 100u;
    rem = value % 100u;

    if (hundreds != 0u) {
        status = sb_amount_append(list, SB_AMOUNT_TOKEN_NUMBER, hundreds);
        if (status != SB_STATUS_OK) {
            return status;
        }

        status = sb_amount_append(list, SB_AMOUNT_TOKEN_HUNDRED, 0u);
        if (status != SB_STATUS_OK) {
            return status;
        }
    }

    if (rem != 0u) {
        status = sb_amount_append_under_100(list, rem);
        if (status != SB_STATUS_OK) {
            return status;
        }
    }

    return SB_STATUS_OK;
}

static sb_status_t sb_amount_append_rupees(sb_amount_token_list_t *list, u64 rupees)
{
    sb_status_t status;
    u32 crore;
    u32 lakh;
    u32 thousand;
    u32 remainder;

    if (rupees == 0u) {
        return sb_amount_append(list, SB_AMOUNT_TOKEN_NUMBER, 0u);
    }

    if (rupees > SB_AMOUNT_MAX_RUPEES) {
        return SB_STATUS_UNSUPPORTED;
    }

    crore = (u32)(rupees / 10000000ull);
    remainder = (u32)(rupees % 10000000ull);
    lakh = remainder / 100000u;
    remainder = remainder % 100000u;
    thousand = remainder / 1000u;
    remainder = remainder % 1000u;

    if (crore != 0u) {
        status = sb_amount_append_under_100(list, crore);
        if (status != SB_STATUS_OK) {
            return status;
        }
        status = sb_amount_append(list, SB_AMOUNT_TOKEN_CRORE, 0u);
        if (status != SB_STATUS_OK) {
            return status;
        }
    }

    if (lakh != 0u) {
        status = sb_amount_append_under_100(list, lakh);
        if (status != SB_STATUS_OK) {
            return status;
        }
        status = sb_amount_append(list, SB_AMOUNT_TOKEN_LAKH, 0u);
        if (status != SB_STATUS_OK) {
            return status;
        }
    }

    if (thousand != 0u) {
        status = sb_amount_append_under_100(list, thousand);
        if (status != SB_STATUS_OK) {
            return status;
        }
        status = sb_amount_append(list, SB_AMOUNT_TOKEN_THOUSAND, 0u);
        if (status != SB_STATUS_OK) {
            return status;
        }
    }

    if (remainder != 0u) {
        status = sb_amount_append_under_1000(list, remainder);
        if (status != SB_STATUS_OK) {
            return status;
        }
    }

    return SB_STATUS_OK;
}

void sb_amount_token_list_init(sb_amount_token_list_t *list)
{
    u32 i;

    if (list == 0) {
        return;
    }

    list->count = 0u;
    for (i = 0u; i < SB_AMOUNT_MAX_TOKENS; i++) {
        list->tokens[i].kind = SB_AMOUNT_TOKEN_NUMBER;
        list->tokens[i].value = 0u;
    }
}

sb_status_t sb_amount_tokenize_paise(u64 amount_paise, sb_amount_token_list_t *out)
{
    sb_status_t status;
    u64 rupees;
    u32 paise;

    if (out == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    sb_amount_token_list_init(out);

    rupees = amount_paise / 100ull;
    paise = (u32)(amount_paise % 100ull);

    status = sb_amount_append_rupees(out, rupees);
    if (status != SB_STATUS_OK) {
        return status;
    }

    status = sb_amount_append(out, SB_AMOUNT_TOKEN_RUPEES, 0u);
    if (status != SB_STATUS_OK) {
        return status;
    }

    if (paise != 0u) {
        status = sb_amount_append_under_100(out, paise);
        if (status != SB_STATUS_OK) {
            return status;
        }
        status = sb_amount_append(out, SB_AMOUNT_TOKEN_PAISE, 0u);
        if (status != SB_STATUS_OK) {
            return status;
        }
    }

    return sb_amount_append(out, SB_AMOUNT_TOKEN_ONLY, 0u);
}

const char *sb_amount_token_kind_name(sb_amount_token_kind_t kind)
{
    switch (kind) {
    case SB_AMOUNT_TOKEN_NUMBER:
        return "number";
    case SB_AMOUNT_TOKEN_HUNDRED:
        return "hundred";
    case SB_AMOUNT_TOKEN_THOUSAND:
        return "thousand";
    case SB_AMOUNT_TOKEN_LAKH:
        return "lakh";
    case SB_AMOUNT_TOKEN_CRORE:
        return "crore";
    case SB_AMOUNT_TOKEN_RUPEES:
        return "rupees";
    case SB_AMOUNT_TOKEN_PAISE:
        return "paise";
    case SB_AMOUNT_TOKEN_AND:
        return "and";
    case SB_AMOUNT_TOKEN_ONLY:
        return "only";
    default:
        return "unknown";
    }
}
