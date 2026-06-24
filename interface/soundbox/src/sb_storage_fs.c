/*================================================================
 * Static QR UPI Soundbox - U-drive File System Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_fs.h"
#include "sb_crc32.h"
#include "sb_log.h"
#include "sb_storage_fs.h"

#define SB_STORAGE_MODULE_NAME        "storage"
#define SB_STORAGE_TEST_PATH          "U:/diag/fs_self_test.bin"
#define SB_STORAGE_TMP_SUFFIX         ".tmp"

static int s_fs_ready = 0;

int sb_storage_fs_is_ready(void)
{
    return s_fs_ready;
}

int sb_storage_fs_path_is_absolute_u(const char *path)
{
    if (path == 0) {
        return 0;
    }
    return ((path[0] == 'U') && (path[1] == ':') && (path[2] == '/')) ? 1 : 0;
}

static u32 sb_storage_strlen(const char *text)
{
    u32 len = 0u;
    if (text == 0) {
        return 0u;
    }
    while (text[len] != '\0') {
        len++;
    }
    return len;
}

static sb_status_t sb_storage_validate_path(const char *path)
{
    if ((path == 0) || (path[0] == '\0')) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (sb_storage_fs_path_is_absolute_u(path) == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    return SB_STATUS_OK;
}

static int sb_storage_disk_accessible(void)
{
    if (ql_fs_size(U_DISK_SYM) >= 0) {
        return 1;
    }
    return (ql_access(SB_STORAGE_ROOT_DIR, 0u) == 0) ? 1 : 0;
}

static void sb_storage_print_disk_usage(const char *name, char disk_sym)
{
    int total_size = ql_fs_size(disk_sym);
    int used_size = ql_fs_used_size(disk_sym);
    int free_size = ql_fs_free_size(disk_sym);

    if (name == 0) {
        name = "disk";
    }
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
}

sb_status_t sb_storage_fs_mkdir_recursive(const char *path)
{
    char partial[128];
    u32 i;
    u32 len;

    if (sb_storage_validate_path(path) != SB_STATUS_OK) {
        return SB_STATUS_INVALID_PARAM;
    }
    len = sb_storage_strlen(path);
    if (len >= (u32)sizeof(partial)) {
        return SB_STATUS_NO_MEMORY;
    }
    if ((len == 3u) && (path[0] == 'U') && (path[1] == ':') && (path[2] == '/')) {
        return SB_STATUS_OK;
    }

    partial[0] = 'U';
    partial[1] = ':';
    partial[2] = '/';
    partial[3] = '\0';

    for (i = 3u; i <= len; i++) {
        partial[i] = path[i];
        if ((path[i] == '/') || (path[i] == '\0')) {
            if (i > 3u) {
                partial[i] = '\0';
                if ((ql_access(partial, 0u) != 0) && (ql_mkdir(partial, 0x777u) != 0)) {
                    if (ql_access(partial, 0u) != 0) {
                        return SB_STATUS_FILE_ERROR;
                    }
                }
                if (path[i] == '/') {
                    partial[i] = '/';
                }
            }
        }
    }
    return SB_STATUS_OK;
}

static sb_status_t sb_storage_make_parent_dir(const char *file_path)
{
    char dir[128];
    u32 i;
    u32 last_slash = 0u;
    u32 len;

    if (sb_storage_validate_path(file_path) != SB_STATUS_OK) {
        return SB_STATUS_INVALID_PARAM;
    }
    len = sb_storage_strlen(file_path);
    if (len >= (u32)sizeof(dir)) {
        return SB_STATUS_NO_MEMORY;
    }
    for (i = 3u; i < len; i++) {
        if (file_path[i] == '/') {
            last_slash = i;
        }
    }
    if (last_slash == 0u) {
        return SB_STATUS_OK;
    }
    for (i = 0u; i < last_slash; i++) {
        dir[i] = file_path[i];
    }
    dir[last_slash] = '\0';
    return sb_storage_fs_mkdir_recursive(dir);
}

sb_status_t sb_storage_fs_file_exists(const char *path)
{
    QFILE *fp;

    if (sb_storage_validate_path(path) != SB_STATUS_OK) {
        return SB_STATUS_INVALID_PARAM;
    }

    /* EG800AK LittleFS builds have shown ql_access() inconsistencies after
     * serial provisioning. Use ql_fopen() as the authoritative existence
     * probe because the MP3 player also consumes files through fopen/fread.
     */
    fp = ql_fopen(path, "rb");
    if (fp == 0) {
        return SB_STATUS_NOT_FOUND;
    }
    (void)ql_fclose(fp);
    return SB_STATUS_OK;
}

sb_status_t sb_storage_fs_read_file(const char *path, void *buffer, u32 length)
{
    QFILE *fp;
    int read_count;

    if ((buffer == 0) || (length == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (sb_storage_validate_path(path) != SB_STATUS_OK) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (s_fs_ready == 0) {
        return SB_STATUS_NOT_READY;
    }
    fp = ql_fopen(path, "r");
    if (fp == 0) {
        return SB_STATUS_NOT_FOUND;
    }
    read_count = ql_fread(buffer, 1u, length, fp);
    (void)ql_fclose(fp);
    if (read_count != (int)length) {
        return SB_STATUS_FILE_ERROR;
    }
    return SB_STATUS_OK;
}

sb_status_t sb_storage_fs_read_file_partial(const char *path, void *buffer, u32 max_length, u32 *actual_length)
{
    QFILE *fp;
    int read_count;

    if ((buffer == 0) || (max_length == 0u) || (actual_length == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    *actual_length = 0u;
    if (sb_storage_validate_path(path) != SB_STATUS_OK) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (s_fs_ready == 0) {
        return SB_STATUS_NOT_READY;
    }
    fp = ql_fopen(path, "r");
    if (fp == 0) {
        return SB_STATUS_NOT_FOUND;
    }
    read_count = ql_fread(buffer, 1u, max_length, fp);
    (void)ql_fclose(fp);
    if (read_count < 0) {
        return SB_STATUS_FILE_ERROR;
    }
    *actual_length = (u32)read_count;
    return SB_STATUS_OK;
}

static sb_status_t sb_storage_write_file_direct(const char *path, const void *buffer, u32 length)
{
    QFILE *fp;
    int write_count;

    if ((path == 0) || (buffer == 0) || (length == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (sb_storage_make_parent_dir(path) != SB_STATUS_OK) {
        return SB_STATUS_FILE_ERROR;
    }
    fp = ql_fopen(path, "w+");
    if (fp == 0) {
        return SB_STATUS_FILE_ERROR;
    }
    write_count = ql_fwrite((void *)buffer, 1u, length, fp);
    if (write_count != (int)length) {
        (void)ql_fclose(fp);
        return SB_STATUS_FILE_ERROR;
    }
    if (ql_fsync(fp) != 0) {
        (void)ql_fclose(fp);
        return SB_STATUS_FILE_ERROR;
    }
    if (ql_fclose(fp) != 0) {
        return SB_STATUS_FILE_ERROR;
    }
    return SB_STATUS_OK;
}

sb_status_t sb_storage_fs_write_file_atomic(const char *final_path,
                                            const char *temp_path,
                                            const void *buffer,
                                            u32 length)
{
    if ((buffer == 0) || (length == 0u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    if ((sb_storage_validate_path(final_path) != SB_STATUS_OK) ||
        (sb_storage_validate_path(temp_path) != SB_STATUS_OK)) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (s_fs_ready == 0) {
        return SB_STATUS_NOT_READY;
    }
    if (sb_storage_make_parent_dir(final_path) != SB_STATUS_OK) {
        return SB_STATUS_FILE_ERROR;
    }
    (void)ql_remove(temp_path);
    if (sb_storage_write_file_direct(temp_path, buffer, length) != SB_STATUS_OK) {
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

sb_status_t sb_storage_fs_write_text_atomic(const char *final_path, const char *text)
{
    char temp_path[160];
    u32 len;
    u32 i;

    if ((final_path == 0) || (text == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    len = sb_storage_strlen(final_path);
    if ((len + (u32)sizeof(SB_STORAGE_TMP_SUFFIX)) >= (u32)sizeof(temp_path)) {
        return SB_STATUS_NO_MEMORY;
    }
    for (i = 0u; i < len; i++) {
        temp_path[i] = final_path[i];
    }
    temp_path[len] = '\0';
    for (i = 0u; i < (u32)sizeof(SB_STORAGE_TMP_SUFFIX); i++) {
        temp_path[len + i] = SB_STORAGE_TMP_SUFFIX[i];
    }
    return sb_storage_fs_write_file_atomic(final_path, temp_path, text, sb_storage_strlen(text));
}

sb_status_t sb_storage_fs_self_test(void)
{
    static const unsigned char pattern[] = {
        0x53u, 0x42u, 0x46u, 0x53u, 0x2du, 0x50u, 0x32u, 0x33u,
        0x10u, 0x32u, 0x54u, 0x76u, 0x98u, 0xbau, 0xdcu, 0xfeu
    };
    unsigned char readback[sizeof(pattern)];
    u32 crc_w;
    u32 crc_r;
    sb_status_t status;

    status = sb_storage_fs_write_file_atomic(SB_STORAGE_TEST_PATH, "U:/diag/fs_self_test.tmp",
                                             pattern, (u32)sizeof(pattern));
    if (status != SB_STATUS_OK) {
        return status;
    }
    status = sb_storage_fs_read_file(SB_STORAGE_TEST_PATH, readback, (u32)sizeof(readback));
    if (status != SB_STATUS_OK) {
        return status;
    }
    crc_w = sb_crc32_compute(pattern, (u32)sizeof(pattern));
    crc_r = sb_crc32_compute(readback, (u32)sizeof(readback));
    (void)ql_remove(SB_STORAGE_TEST_PATH);
    if (crc_w != crc_r) {
        return SB_STATUS_CRC_ERROR;
    }
    (void)sb_storage_fs_write_text_atomic(SB_STORAGE_DIAG_FS_PATH, "fs_self_test=pass\n");
    return SB_STATUS_OK;
}

sb_status_t sb_storage_fs_init(void)
{
    int mount_ret;
    sb_status_t status;

    if (s_fs_ready != 0) {
        sb_storage_fs_print_usage();
        return SB_STATUS_ALREADY_INITIALIZED;
    }
    mount_ret = ql_fs_mount(U_DISK_SYM);
    if ((mount_ret != 0) && (sb_storage_disk_accessible() == 0)) {
        sb_storage_fs_print_usage();
        return SB_STATUS_FILE_ERROR;
    }
    s_fs_ready = 1;
    status = sb_storage_fs_mkdir_recursive(SB_STORAGE_CONFIG_DIR);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_storage_fs_mkdir_recursive(SB_STORAGE_LEDGER_DIR);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_storage_fs_mkdir_recursive(SB_STORAGE_CERT_DIR);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_storage_fs_mkdir_recursive(SB_STORAGE_LOG_DIR);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_storage_fs_mkdir_recursive(SB_STORAGE_DIAG_DIR);
    if (status != SB_STATUS_OK) { return status; }
    status = sb_storage_fs_mkdir_recursive(SB_STORAGE_CACHE_DIR);
    if (status != SB_STATUS_OK) { return status; }
    sb_storage_fs_print_usage();
    status = sb_storage_fs_self_test();
    if (status != SB_STATUS_OK) {
        SB_LOGW(SB_STORAGE_MODULE_NAME, "self_test status=%s", sb_status_to_string(status));
        return status;
    }
    SB_LOGI(SB_STORAGE_MODULE_NAME, "ready paths config=%s certs=%s diag=%s", SB_STORAGE_CONFIG_DIR, SB_STORAGE_CERT_DIR, SB_STORAGE_DIAG_DIR);
    return SB_STATUS_OK;
}
