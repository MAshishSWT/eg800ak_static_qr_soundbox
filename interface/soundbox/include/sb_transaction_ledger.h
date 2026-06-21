/*================================================================
 * Static QR UPI Soundbox - Transaction Ledger
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_TRANSACTION_LEDGER_H
#define SB_TRANSACTION_LEDGER_H

#include "ql_type.h"
#include "ql_rtc.h"
#include "sb_audio_types.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_LEDGER_TX_ID_LEN          (40u)
#define SB_LEDGER_PROVIDER_LEN       (16u)
#define SB_LEDGER_MAX_RECORDS        (32u)
#define SB_LEDGER_PATH               "U:/sb_ledger.bin"
#define SB_LEDGER_TEMP_PATH          "U:/sb_ledger.tmp"

typedef struct {
    char tx_id[SB_LEDGER_TX_ID_LEN];
    u64 amount_paise;
    u32 date_yyyymmdd;
    u32 time_hhmmss;
    sb_audio_provider_t provider;
} sb_transaction_record_t;

typedef struct {
    u32 date_yyyymmdd;
    u32 count;
    u64 total_paise;
} sb_daily_summary_t;

sb_status_t sb_transaction_ledger_init(void);
sb_status_t sb_transaction_ledger_add(const sb_transaction_record_t *record, int *duplicate);
sb_status_t sb_transaction_ledger_get_last(sb_transaction_record_t *record);
sb_status_t sb_transaction_ledger_get_daily(sb_daily_summary_t *summary);
sb_status_t sb_transaction_ledger_reset_daily_if_needed(u32 date_yyyymmdd);
u32 sb_transaction_date_from_rtc(const ql_rtc_time_t *rtc);
u32 sb_transaction_time_from_rtc(const ql_rtc_time_t *rtc);

#ifdef __cplusplus
}
#endif

#endif /* SB_TRANSACTION_LEDGER_H */
