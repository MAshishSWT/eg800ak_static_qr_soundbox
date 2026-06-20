/*================================================================
 * Static QR UPI Soundbox - File System Storage Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_fs.h"
#include "sb_storage_fs.h"

static int s_fs_ready = 0;

int sb_storage_fs_is_ready(void)
{
    return s_fs_ready;
}

static sb_status_t sb_storage_mkdir_if_needed(const char *path)
{
    if (path == 0) {
        return SB_STATUS_INVALID_PARAM;
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
    if (s_fs_ready != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }

    if (ql_fs_mount(U_DISK_SYM) != 0) {
        return SB_STATUS_FILE_ERROR;
    }

    if (sb_storage_mkdir_if_needed(SB_STORAGE_ROOT_DIR) != SB_STATUS_OK) {
        return SB_STATUS_FILE_ERROR;
    }

    if (sb_storage_mkdir_if_needed(SB_STORAGE_CONFIG_DIR) != SB_STATUS_OK) {
        return SB_STATUS_FILE_ERROR;
    }

    s_fs_ready = 1;
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

    if (read_count != 1) {
        return SB_STATUS_FILE_ERROR;
    }

    return SB_STATUS_OK;
}

sb_status_t sb_storage_fs_write_file_atomic(const char *final_path, const char *temp_path, const void *buffer, u32 length)
{
    QFILE *fp;
    int write_count;

    if ((final_path == 0) || (temp_path == 0) || (buffer == 0) || (length == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (s_fs_ready == 0) {
        return SB_STATUS_NOT_READY;
    }

    (void)ql_remove(temp_path);
    fp = ql_fopen(temp_path, "w+");
    if (fp == 0) {
        return SB_STATUS_FILE_ERROR;
    }

    write_count = ql_fwrite((void *)buffer, length, 1u, fp);
    if (write_count != 1) {
        (void)ql_fclose(fp);
        (void)ql_remove(temp_path);
        return SB_STATUS_FILE_ERROR;
    }

    if (ql_fsync(fp) != 0) {
        (void)ql_fclose(fp);
        (void)ql_remove(temp_path);
        return SB_STATUS_FILE_ERROR;
    }

    if (ql_fclose(fp) != 0) {
        (void)ql_remove(temp_path);
        return SB_STATUS_FILE_ERROR;
    }

    (void)ql_remove(final_path);
    if (ql_rename(temp_path, final_path) != 0) {
        (void)ql_remove(temp_path);
        return SB_STATUS_FILE_ERROR;
    }

    return SB_STATUS_OK;
}
