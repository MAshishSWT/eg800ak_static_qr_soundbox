/*================================================================
 * Static QR UPI Soundbox - File System Storage Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_fs.h"
#include "sb_log.h"
#include "sb_storage_fs.h"

#define SB_STORAGE_MODULE_NAME "storage"
#define SB_STORAGE_ROOT_PATH   "U:/"

static int s_fs_ready = 0;

int sb_storage_fs_is_ready(void)
{
    return s_fs_ready;
}

static int sb_storage_disk_accessible(void)
{
    if (ql_fs_size(U_DISK_SYM) >= 0) {
        return 1;
    }

    if (ql_access(SB_STORAGE_ROOT_PATH, 0u) == 0) {
        return 1;
    }

    return 0;
}

static void sb_storage_print_disk_usage(const char *name, char disk_sym)
{
    int total_size;
    int used_size;
    int free_size;

    if (name == 0) {
        name = "disk";
    }

    total_size = ql_fs_size(disk_sym);
    used_size = ql_fs_used_size(disk_sym);
    free_size = ql_fs_free_size(disk_sym);

    if ((total_size < 0) || (used_size < 0) || (free_size < 0)) {
        SB_LOGW(SB_STORAGE_MODULE_NAME, "%s %c: usage unavailable total=%d used=%d free=%d",
                name, disk_sym, total_size, used_size, free_size);
        return;
    }

    SB_LOGI(SB_STORAGE_MODULE_NAME, "%s %c: total=%d used=%d free=%d bytes",
            name, disk_sym, total_size, used_size, free_size);
}

void sb_storage_fs_print_usage(void)
{
    sb_storage_print_disk_usage("userfs", U_DISK_SYM);
    sb_storage_print_disk_usage("internal", B_DISK_SYM);
}

static sb_status_t sb_storage_mkdir_if_needed(const char *path)
{
    if (path == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (path[0] == 'U' && path[1] == ':' && path[2] == '/' && path[3] == '\0') {
        return SB_STATUS_OK;
    }

    if (ql_mkdir(path, 0x777u) == 0) {
        return SB_STATUS_OK;
    }

    if (ql_access(path, 0u) == 0) {
        return SB_STATUS_OK;
    }

    return SB_STATUS_FILE_ERROR;
}

sb_status_t sb_storage_fs_init(void)
{
    int mount_ret;

    if (s_fs_ready != 0) {
        sb_storage_fs_print_usage();
        return SB_STATUS_ALREADY_INITIALIZED;
    }

    mount_ret = ql_fs_mount(U_DISK_SYM);
    if ((mount_ret != 0) && (sb_storage_disk_accessible() == 0)) {
        sb_storage_fs_print_usage();
        return SB_STATUS_FILE_ERROR;
    }

    if (sb_storage_mkdir_if_needed(SB_STORAGE_ROOT_DIR) != SB_STATUS_OK) {
        sb_storage_fs_print_usage();
        return SB_STATUS_FILE_ERROR;
    }

    if (sb_storage_mkdir_if_needed(SB_STORAGE_CONFIG_DIR) != SB_STATUS_OK) {
        sb_storage_fs_print_usage();
        return SB_STATUS_FILE_ERROR;
    }

    s_fs_ready = 1;
    sb_storage_fs_print_usage();
    return SB_STATUS_OK;
}

sb_status_t sb_storage_fs_file_exists(const char *path)
{
    if (path == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (ql_access(path, 0u) == 0) {
        return SB_STATUS_OK;
    }

    return SB_STATUS_NOT_FOUND;
}

sb_status_t sb_storage_fs_read_file(const char *path, void *buffer, u32 length)
{
    QFILE *fp;
    int read_count;

    if ((path == 0) || (buffer == 0) || (length == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (s_fs_ready == 0) {
        return SB_STATUS_NOT_READY;
    }

    fp = ql_fopen(path, "r");
    if (fp == 0) {
        return SB_STATUS_NOT_FOUND;
    }

    read_count = ql_fread(buffer, length, 1u, fp);
    (void)ql_fclose(fp);

    /* Quectel FS examples treat any negative return as failure. Some SDK
     * variants return byte count while others return object count. Do not
     * require exactly 1 here; config CRC validation protects against short or
     * corrupted records.
     */
    if (read_count < 0) {
        return SB_STATUS_FILE_ERROR;
    }

    return SB_STATUS_OK;
}

static sb_status_t sb_storage_write_file_direct(const char *path, const void *buffer, u32 length)
{
    QFILE *fp;
    int write_count;

    fp = ql_fopen(path, "w+");
    if (fp == 0) {
        return SB_STATUS_FILE_ERROR;
    }

    write_count = ql_fwrite((void *)buffer, length, 1u, fp);
    /* Quectel FS examples check ret < 0 only. Return value can be object count
     * or byte count depending on SDK build; both are success when non-negative.
     */
    if (write_count < 0) {
        (void)ql_fclose(fp);
        return SB_STATUS_FILE_ERROR;
    }

    (void)ql_fsync(fp);
    if (ql_fclose(fp) != 0) {
        return SB_STATUS_FILE_ERROR;
    }

    return SB_STATUS_OK;
}

sb_status_t sb_storage_fs_write_file_atomic(const char *final_path, const char *temp_path, const void *buffer, u32 length)
{
    sb_status_t direct_status;

    if ((final_path == 0) || (temp_path == 0) || (buffer == 0) || (length == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (s_fs_ready == 0) {
        return SB_STATUS_NOT_READY;
    }

    (void)ql_remove(temp_path);
    if (sb_storage_write_file_direct(temp_path, buffer, length) == SB_STATUS_OK) {
        (void)ql_remove(final_path);
        if (ql_rename(temp_path, final_path) == 0) {
            sb_storage_fs_print_usage();
            return SB_STATUS_OK;
        }
        (void)ql_remove(temp_path);
    }

    direct_status = sb_storage_write_file_direct(final_path, buffer, length);
    sb_storage_fs_print_usage();
    if (direct_status == SB_STATUS_OK) {
        return SB_STATUS_OK;
    }

    return SB_STATUS_FILE_ERROR;
}
