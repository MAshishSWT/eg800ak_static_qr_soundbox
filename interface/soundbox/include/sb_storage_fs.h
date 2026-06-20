/*================================================================
 * Static QR UPI Soundbox - File System Storage Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#ifndef SB_STORAGE_FS_H
#define SB_STORAGE_FS_H

#include "ql_type.h"
#include "sb_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SB_STORAGE_DISK_TEXT          "U:"
#define SB_STORAGE_ROOT_DIR           "U:/soundbox"
#define SB_STORAGE_CONFIG_DIR         "U:/soundbox/config"
#define SB_STORAGE_CONFIG_SLOT_A_PATH "U:/soundbox/config/config_a.bin"
#define SB_STORAGE_CONFIG_SLOT_B_PATH "U:/soundbox/config/config_b.bin"
#define SB_STORAGE_CONFIG_TEMP_PATH   "U:/soundbox/config/config_tmp.bin"

sb_status_t sb_storage_fs_init(void);
sb_status_t sb_storage_fs_read_file(const char *path, void *buffer, u32 length);
sb_status_t sb_storage_fs_write_file_atomic(const char *final_path, const char *temp_path, const void *buffer, u32 length);
sb_status_t sb_storage_fs_file_exists(const char *path);
int sb_storage_fs_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* SB_STORAGE_FS_H */
