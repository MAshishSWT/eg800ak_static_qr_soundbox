/*================================================================
 * Static QR UPI Soundbox - Transaction Ledger
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_rtos.h"
#include "sb_crc32.h"
#include "sb_log.h"
#include "sb_storage_fs.h"
#include "sb_transaction_ledger.h"

#define SB_LEDGER_MODULE_NAME     "ledger"
#define SB_LEDGER_MAGIC           (0x53424C47u)
#define SB_LEDGER_VERSION         (1u)

typedef struct {
    u32 magic;
    u32 version;
    u32 write_index;
    u32 count;
    sb_daily_summary_t daily;
    sb_transaction_record_t records[SB_LEDGER_MAX_RECORDS];
    u32 crc;
} sb_ledger_file_t;

static sb_ledger_file_t s_ledger;
static ql_mutex_t s_ledger_mutex = 0;
static int s_ledger_ready = 0;

static void sb_ledger_zero(void *ptr, u32 length)
{
    u32 i;
    unsigned char *p = (unsigned char *)ptr;
    if (p == 0) {
        return;
    }
    for (i = 0u; i < length; i++) {
        p[i] = 0u;
    }
}

static void sb_ledger_init_defaults(void)
{
    sb_ledger_zero(&s_ledger, (u32)sizeof(s_ledger));
    s_ledger.magic = SB_LEDGER_MAGIC;
    s_ledger.version = SB_LEDGER_VERSION;
}

static u32 sb_ledger_crc_without_crc(void)
{
    return sb_crc32_compute(&s_ledger, (u32)(((unsigned long)&s_ledger.crc) - ((unsigned long)&s_ledger)));
}

static sb_status_t sb_ledger_save(void)
{
    s_ledger.crc = sb_ledger_crc_without_crc();
    return sb_storage_fs_write_file_atomic(SB_LEDGER_PATH, SB_LEDGER_TEMP_PATH, &s_ledger, (u32)sizeof(s_ledger));
}

static sb_status_t sb_ledger_load(void)
{
    sb_status_t status;
    u32 crc;

    status = sb_storage_fs_read_file(SB_LEDGER_PATH, &s_ledger, (u32)sizeof(s_ledger));
    if (status != SB_STATUS_OK) {
        sb_ledger_init_defaults();
        return sb_ledger_save();
    }

    if ((s_ledger.magic != SB_LEDGER_MAGIC) || (s_ledger.version != SB_LEDGER_VERSION)) {
        sb_ledger_init_defaults();
        return sb_ledger_save();
    }

    crc = sb_ledger_crc_without_crc();
    if (crc != s_ledger.crc) {
        sb_ledger_init_defaults();
        return sb_ledger_save();
    }

    return SB_STATUS_OK;
}

static int sb_ledger_tx_id_equal(const char *a, const char *b)
{
    u32 i;
    if ((a == 0) || (b == 0)) {
        return 0;
    }
    for (i = 0u; i < SB_LEDGER_TX_ID_LEN; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
        if (a[i] == '\0') {
            return 1;
        }
    }
    return 1;
}

static int sb_ledger_is_duplicate(const char *tx_id)
{
    u32 i;
    for (i = 0u; i < s_ledger.count; i++) {
        if (sb_ledger_tx_id_equal(s_ledger.records[i].tx_id, tx_id) != 0) {
            return 1;
        }
    }
    return 0;
}

u32 sb_transaction_date_from_rtc(const ql_rtc_time_t *rtc)
{
    if (rtc == 0) {
        return 0u;
    }
    return ((u32)rtc->tm_year * 10000u) + ((u32)rtc->tm_mon * 100u) + (u32)rtc->tm_mday;
}

u32 sb_transaction_time_from_rtc(const ql_rtc_time_t *rtc)
{
    if (rtc == 0) {
        return 0u;
    }
    return ((u32)rtc->tm_hour * 10000u) + ((u32)rtc->tm_min * 100u) + (u32)rtc->tm_sec;
}

sb_status_t sb_transaction_ledger_init(void)
{
    sb_status_t status;

    if (s_ledger_ready != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }
    if (ql_rtos_mutex_create(&s_ledger_mutex) != 0) {
        return SB_STATUS_NO_MEMORY;
    }
    status = sb_ledger_load();
    if (status != SB_STATUS_OK) {
        return status;
    }
    s_ledger_ready = 1;
    SB_LOGI(SB_LEDGER_MODULE_NAME, "ready count=%u daily_count=%u", s_ledger.count, s_ledger.daily.count);
    return SB_STATUS_OK;
}

sb_status_t sb_transaction_ledger_reset_daily_if_needed(u32 date_yyyymmdd)
{
    if (s_ledger_ready == 0) {
        return SB_STATUS_NOT_READY;
    }
    if (date_yyyymmdd == 0u) {
        return SB_STATUS_INVALID_PARAM;
    }
    (void)ql_rtos_mutex_lock(s_ledger_mutex, QL_WAIT_FOREVER);
    if (s_ledger.daily.date_yyyymmdd != date_yyyymmdd) {
        s_ledger.daily.date_yyyymmdd = date_yyyymmdd;
        s_ledger.daily.count = 0u;
        s_ledger.daily.total_paise = 0u;
        (void)sb_ledger_save();
    }
    (void)ql_rtos_mutex_unlock(s_ledger_mutex);
    return SB_STATUS_OK;
}

sb_status_t sb_transaction_ledger_add(const sb_transaction_record_t *record, int *duplicate)
{
    u32 slot;

    if ((record == 0) || (duplicate == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (s_ledger_ready == 0) {
        return SB_STATUS_NOT_READY;
    }

    *duplicate = 0;
    (void)ql_rtos_mutex_lock(s_ledger_mutex, QL_WAIT_FOREVER);
    if (sb_ledger_is_duplicate(record->tx_id) != 0) {
        *duplicate = 1;
        (void)ql_rtos_mutex_unlock(s_ledger_mutex);
        return SB_STATUS_OK;
    }

    if ((s_ledger.daily.date_yyyymmdd != record->date_yyyymmdd) && (record->date_yyyymmdd != 0u)) {
        s_ledger.daily.date_yyyymmdd = record->date_yyyymmdd;
        s_ledger.daily.count = 0u;
        s_ledger.daily.total_paise = 0u;
    }

    slot = s_ledger.write_index % SB_LEDGER_MAX_RECORDS;
    s_ledger.records[slot] = *record;
    s_ledger.write_index = (s_ledger.write_index + 1u) % SB_LEDGER_MAX_RECORDS;
    if (s_ledger.count < SB_LEDGER_MAX_RECORDS) {
        s_ledger.count++;
    }
    s_ledger.daily.count++;
    s_ledger.daily.total_paise += record->amount_paise;
    (void)sb_ledger_save();
    (void)ql_rtos_mutex_unlock(s_ledger_mutex);
    return SB_STATUS_OK;
}

sb_status_t sb_transaction_ledger_get_last(sb_transaction_record_t *record)
{
    u32 index;
    if ((record == 0) || (s_ledger_ready == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    (void)ql_rtos_mutex_lock(s_ledger_mutex, QL_WAIT_FOREVER);
    if (s_ledger.count == 0u) {
        (void)ql_rtos_mutex_unlock(s_ledger_mutex);
        return SB_STATUS_NOT_FOUND;
    }
    index = (s_ledger.write_index == 0u) ? (SB_LEDGER_MAX_RECORDS - 1u) : (s_ledger.write_index - 1u);
    *record = s_ledger.records[index];
    (void)ql_rtos_mutex_unlock(s_ledger_mutex);
    return SB_STATUS_OK;
}

sb_status_t sb_transaction_ledger_get_daily(sb_daily_summary_t *summary)
{
    if ((summary == 0) || (s_ledger_ready == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    (void)ql_rtos_mutex_lock(s_ledger_mutex, QL_WAIT_FOREVER);
    *summary = s_ledger.daily;
    (void)ql_rtos_mutex_unlock(s_ledger_mutex);
    return SB_STATUS_OK;
}
