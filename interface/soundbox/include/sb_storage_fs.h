/*================================================================
 * Static QR UPI Soundbox - U-drive File System Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_STORAGE_FS_H
#define SB_STORAGE_FS_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_STORAGE_DISK_TEXT              "U:"
#define SB_STORAGE_ROOT_DIR               "U:/"
#define SB_STORAGE_CONFIG_DIR             "U:/config"
#define SB_STORAGE_LEDGER_DIR             "U:/ledger"
#define SB_STORAGE_CERT_DIR               "U:/certs"
#define SB_STORAGE_LOG_DIR                "U:/logs"
#define SB_STORAGE_DIAG_DIR               "U:/diag"
#define SB_STORAGE_CACHE_DIR              "U:/cache"

#define SB_STORAGE_CONFIG_SLOT_A_PATH     "U:/config/sb_cfg_a.bin"
#define SB_STORAGE_CONFIG_SLOT_B_PATH     "U:/config/sb_cfg_b.bin"
#define SB_STORAGE_CONFIG_TEMP_PATH       "U:/config/sb_cfg_tmp.bin"
#define SB_STORAGE_DIAG_FS_PATH           "U:/diag/fs_health.txt"
#define SB_STORAGE_DIAG_NOR_PATH          "U:/diag/nor_health.txt"
#define SB_STORAGE_AUDIO_CACHE_PATH       "U:/cache/asset_play.mp3"

sb_status_t sb_storage_fs_init(void);
sb_status_t sb_storage_fs_self_test(void);
sb_status_t sb_storage_fs_mkdir_recursive(const char *path);
sb_status_t sb_storage_fs_read_file(const char *path, void *buffer, u32 length);
sb_status_t sb_storage_fs_read_file_partial(const char *path, void *buffer, u32 max_length, u32 *actual_length);
sb_status_t sb_storage_fs_write_file_atomic(const char *final_path,
                                            const char *temp_path,
                                            const void *buffer,
                                            u32 length);
sb_status_t sb_storage_fs_write_text_atomic(const char *final_path, const char *text);
sb_status_t sb_storage_fs_file_exists(const char *path);
void sb_storage_fs_print_usage(void);
int sb_storage_fs_is_ready(void);
int sb_storage_fs_path_is_absolute_u(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* SB_STORAGE_FS_H */
