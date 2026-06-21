/*================================================================
 * Static QR UPI Soundbox - OTA Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_fota.h"
#include "ql_fs.h"
#include "ql_http_client.h"
#include "ql_rtos.h"
#include "ql_ssl_hal.h"
#include "sb_cloud_utils.h"
#include "sb_config.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_log.h"
#include "sb_network_service.h"
#include "sb_ota_crypto.h"
#include "sb_ota_manifest.h"
#include "sb_ota_service.h"
#include "sb_storage_fs.h"

#define SB_OTA_MODULE_NAME        "ota"
#define SB_OTA_CIPHER_LIST        "ALL"
#define SB_OTA_HOST_LEN           (96u)
#define SB_OTA_WRITE_BUF_LEN      (64u)

typedef struct {
    sb_ota_manifest_t manifest;
    sb_ota_sha256_ctx_t sha;
    unsigned char expected_sha[32];
    unsigned char actual_sha[32];
    qlFotaImgProcCtxPtr fota_ctx;
    QFILE *file;
    u32 received;
    int completed;
    int failed;
    int status_code;
} sb_ota_download_ctx_t;

static char s_ota_root_ca_path[] = "U:/certs/ota_root_ca.pem";
static ql_task_t s_ota_task = 0;
static ql_mutex_t s_ota_mutex = 0;
static ql_queue_t s_ota_queue = 0;
static char s_ota_manifest_pool[SB_OTA_QUEUE_DEPTH][SB_OTA_MANIFEST_JSON_LEN];
static u8 s_ota_manifest_pool_used[SB_OTA_QUEUE_DEPTH];
static int s_ota_started = 0;
static sb_config_payload_t s_ota_config;
static sb_ota_status_t s_ota_status = {SB_OTA_STATE_IDLE, SB_OTA_KIND_FIRMWARE, 0u, 0, {0}};

static void sb_ota_zero(void *ptr, u32 len)
{
    u32 i;
    unsigned char *p = (unsigned char *)ptr;

    if (p == 0) {
        return;
    }
    for (i = 0u; i < len; i++) {
        p[i] = 0u;
    }
}

const char *sb_ota_state_name(sb_ota_state_t state)
{
    switch (state) {
    case SB_OTA_STATE_IDLE:
        return "idle";
    case SB_OTA_STATE_QUEUED:
        return "queued";
    case SB_OTA_STATE_VERIFY_MANIFEST:
        return "verify_manifest";
    case SB_OTA_STATE_DOWNLOADING:
        return "downloading";
    case SB_OTA_STATE_VERIFY_IMAGE:
        return "verify_image";
    case SB_OTA_STATE_STAGED:
        return "staged";
    case SB_OTA_STATE_FAULT:
        return "fault";
    default:
        return "unknown";
    }
}

static void sb_ota_set_status(sb_ota_state_t state, const sb_ota_manifest_t *manifest, u32 progress, int error)
{
    if (s_ota_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_ota_mutex, QL_WAIT_FOREVER);
    }

    s_ota_status.state = state;
    s_ota_status.progress_percent = progress;
    s_ota_status.last_error = error;
    if (manifest != 0) {
        s_ota_status.kind = manifest->kind;
        sb_cloud_copy_string(s_ota_status.version, SB_OTA_VERSION_LEN, manifest->version);
    }

    if (s_ota_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_ota_mutex);
    }
}

static void sb_ota_post_event(sb_event_id_t id, s32 status, u32 value, const char *text)
{
    sb_event_t event;

    sb_event_init(&event, id, SB_EVENT_SOURCE_OTA);
    event.param_s32 = status;
    event.param_u32 = value;
    if (text != 0) {
        (void)sb_event_set_text(&event, text);
    }
    (void)sb_event_post(&event, QL_NO_WAIT);
}

static int sb_ota_pool_alloc(u32 *slot)
{
    u32 i;

    if (slot == 0) {
        return 0;
    }
    if (s_ota_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_ota_mutex, QL_WAIT_FOREVER);
    }
    for (i = 0u; i < SB_OTA_QUEUE_DEPTH; i++) {
        if (s_ota_manifest_pool_used[i] == 0u) {
            s_ota_manifest_pool_used[i] = 1u;
            *slot = i;
            if (s_ota_mutex != 0) {
                (void)ql_rtos_mutex_unlock(s_ota_mutex);
            }
            return 1;
        }
    }
    if (s_ota_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_ota_mutex);
    }
    return 0;
}

static void sb_ota_pool_free(u32 slot)
{
    if (slot >= SB_OTA_QUEUE_DEPTH) {
        return;
    }
    if (s_ota_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_ota_mutex, QL_WAIT_FOREVER);
    }
    s_ota_manifest_pool_used[slot] = 0u;
    if (s_ota_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_ota_mutex);
    }
}

static int sb_ota_network_ready(void)
{
    sb_network_status_t net;

    if (sb_network_get_status(&net) != SB_STATUS_OK) {
        return 0;
    }
    return (net.online != 0) ? 1 : 0;
}

static void sb_ota_extract_host(const char *url, char *host, u32 host_len)
{
    u32 i = 0u;
    u32 pos = 0u;

    if ((host == 0) || (host_len == 0u)) {
        return;
    }
    host[0] = '\0';
    if (url == 0) {
        return;
    }
    if (sb_cloud_has_prefix(url, "https://") != 0) {
        i = 8u;
    } else if (sb_cloud_has_prefix(url, "http://") != 0) {
        i = 7u;
    }
    while ((url[i] != '\0') && (url[i] != '/') && (url[i] != ':') && (pos + 1u < host_len)) {
        host[pos++] = url[i++];
    }
    host[pos] = '\0';
}

static void sb_ota_configure_ssl(SSLConfig *ssl_config, const char *url, char *server_name, u32 server_name_len)
{
    if (ssl_config == 0) {
        return;
    }

    sb_ota_zero(ssl_config, (u32)sizeof(*ssl_config));
    ssl_config->en = (sb_cloud_url_is_https(url) != 0) ? 1u : 0u;
    if (ssl_config->en != 0u) {
        sb_ota_extract_host(url, server_name, server_name_len);
        ssl_config->profileIdx = SB_OTA_PDP_PROFILE_ID;
        ssl_config->serverName = server_name;
        ssl_config->serverPort = SB_OTA_TLS_PORT;
        ssl_config->protocol = 0u;
        ssl_config->dbgLevel = 0u;
        ssl_config->sessionReuseEn = 0u;
        ssl_config->vsn = SSL_VSN_ALL;
        ssl_config->verify = SSL_VERIFY_MODE_REQUIRED;
        ssl_config->cert.from = SSL_CERT_FROM_FS;
        ssl_config->cert.path.rootCA = s_ota_root_ca_path;
        ssl_config->cert.path.clientKey = NULL;
        ssl_config->cert.path.clientCert = NULL;
        ssl_config->cert.clientKeyPwd.data = NULL;
        ssl_config->cert.clientKeyPwd.len = 0;
        ssl_config->cipherList = SB_OTA_CIPHER_LIST;
        ssl_config->CTRDRBGSeed.data = NULL;
        ssl_config->CTRDRBGSeed.len = 0;
    }
}

static int sb_ota_http_cb(QL_HTTP_CLIENT_T *client,
                          QL_HTTP_CLIENT_EVENT_E event,
                          int status_code,
                          char *data,
                          int data_len,
                          void *private_data)
{
    sb_ota_download_ctx_t *ctx = (sb_ota_download_ctx_t *)private_data;
    int write_ret = 0;

    (void)client;

    if (ctx == 0) {
        return 0;
    }

    if ((event == QL_HTTP_CLIENT_EVENT_SEND_FAIL) ||
        (event == QL_HTTP_CLIENT_EVENT_RECV_HEADER_FAIL) ||
        (event == QL_HTTP_CLIENT_EVENT_SOCK_RECV_FAIL)) {
        ctx->failed = 1;
        return 0;
    }

    if (event == QL_HTTP_CLIENT_EVENT_RECV_HEADER_FINISHED) {
        ctx->status_code = status_code;
        return (status_code == 200) ? 1 : 0;
    }

    if (event == QL_HTTP_CLIENT_EVENT_RECV_BODY) {
        if ((data == 0) || (data_len <= 0)) {
            return 1;
        }
        if (sb_ota_sha256_update(&ctx->sha, (const u8 *)data, (u32)data_len) != SB_STATUS_OK) {
            ctx->failed = 1;
            return 0;
        }
        if (ctx->manifest.kind == SB_OTA_KIND_FIRMWARE) {
            write_ret = ql_fota_image_write(ctx->fota_ctx, (void *)data, data_len);
            if (write_ret != 0) {
                ctx->failed = 1;
                return 0;
            }
        } else {
            write_ret = ql_fwrite(data, (u32)data_len, 1u, ctx->file);
            if (write_ret < 0) {
                ctx->failed = 1;
                return 0;
            }
        }
        ctx->received += (u32)data_len;
        if (ctx->manifest.size_bytes > 0u) {
            u32 percent = (ctx->received * 100u) / ctx->manifest.size_bytes;
            if (percent > 100u) {
                percent = 100u;
            }
            sb_ota_set_status(SB_OTA_STATE_DOWNLOADING, &ctx->manifest, percent, 0);
            sb_ota_post_event(SB_EVENT_OTA_PROGRESS, SB_STATUS_OK, percent, ctx->manifest.type);
        }
        return 1;
    }

    if (event == QL_HTTP_CLIENT_EVENT_RECV_BODY_FINISHED) {
        ctx->completed = 1;
        ctx->status_code = status_code;
        return 1;
    }

    if (event == QL_HTTP_CLIENT_EVENT_DISCONNECTED) {
        if (ctx->completed == 0) {
            ctx->failed = 1;
        }
        return 0;
    }

    return 1;
}

static sb_status_t sb_ota_download_http(sb_ota_download_ctx_t *ctx)
{
    QL_HTTP_CLIENT_T *client;
    QL_HTTP_CLIENT_ERR_E ret;
    SSLConfig ssl_config;
    char ssl_server_name[SB_OTA_HOST_LEN];

    if (ctx == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    client = ql_http_client_init();
    if (client == NULL) {
        return SB_STATUS_HTTP_ERROR;
    }

    sb_ota_zero(ssl_server_name, (u32)sizeof(ssl_server_name));
    sb_ota_configure_ssl(&ssl_config, ctx->manifest.url, ssl_server_name, (u32)sizeof(ssl_server_name));

    (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_ASYN, 0);
    (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_PDP_CID, SB_OTA_PDP_PROFILE_ID);
    (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_PROTOCOL_VER, QL_HTTP_CLIENT_HTTP1_1);
    (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_TOTAL_TIMEOUT_MS, SB_OTA_HTTP_TIMEOUT_MS);
    (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_SSL_CTX, &ssl_config);

    ret = ql_http_client_request(client,
                                 ctx->manifest.url,
                                 QL_HTTP_CLIENT_REQUEST_GET,
                                 QL_HTTP_CLIENT_AUTH_TYPE_NONE,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 sb_ota_http_cb,
                                 ctx);
    ql_http_client_release(client);

    if (ret != QL_HTTP_CLIENT_ERR_SUCCESS) {
        return SB_STATUS_HTTP_ERROR;
    }
    if ((ctx->failed != 0) || (ctx->completed == 0) || (ctx->status_code != 200)) {
        return SB_STATUS_HTTP_ERROR;
    }
    return SB_STATUS_OK;
}

static sb_status_t sb_ota_prepare_download(sb_ota_download_ctx_t *ctx, const sb_ota_manifest_t *manifest)
{
    sb_status_t status;

    if ((ctx == 0) || (manifest == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    sb_ota_zero(ctx, (u32)sizeof(*ctx));
    ctx->manifest = *manifest;

    status = sb_ota_manifest_sha_hex_to_bytes(manifest->sha256_hex, ctx->expected_sha);
    if (status != SB_STATUS_OK) {
        return status;
    }

    sb_ota_sha256_init(&ctx->sha);

    if (manifest->kind == SB_OTA_KIND_FIRMWARE) {
        (void)ql_fota_set_package_path(SB_OTA_FW_PACKAGE_PATH);
        ctx->fota_ctx = ql_fota_init();
        if (ctx->fota_ctx == NULL) {
                        return SB_STATUS_OTA_ERROR;
        }
    } else {
        (void)ql_remove(SB_OTA_AUDIO_TEMP_PATH);
        ctx->file = ql_fopen(SB_OTA_AUDIO_TEMP_PATH, "w+");
        if (ctx->file == NULL) {
                        return SB_STATUS_FILE_ERROR;
        }
    }
    return SB_STATUS_OK;
}

static void sb_ota_cleanup_download(sb_ota_download_ctx_t *ctx)
{
    if (ctx == 0) {
        return;
    }
    if (ctx->file != NULL) {
        (void)ql_fclose(ctx->file);
        ctx->file = NULL;
    }
    if (ctx->fota_ctx != NULL) {
        ql_fota_deinit(ctx->fota_ctx);
        ctx->fota_ctx = NULL;
    }
    }




static sb_status_t sb_ota_finish_hash(sb_ota_download_ctx_t *ctx)
{
    if (ctx == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (ctx->manifest.size_bytes != ctx->received) {
        return SB_STATUS_HASH_ERROR;
    }
    if (sb_ota_sha256_finish(&ctx->sha, ctx->actual_sha) != SB_STATUS_OK) {
        return SB_STATUS_HASH_ERROR;
    }
    if (sb_ota_digest_equal(ctx->expected_sha, ctx->actual_sha, 32u) == 0) {
        return SB_STATUS_HASH_ERROR;
    }
    return SB_STATUS_OK;
}

static sb_status_t sb_ota_activate_firmware(sb_ota_download_ctx_t *ctx)
{
    int ret;
    int file_size = 0;
    int is_dfota = 0;

    if ((ctx == 0) || (ctx->fota_ctx == NULL)) {
        return SB_STATUS_INVALID_PARAM;
    }

    ret = ql_fota_image_flush(ctx->fota_ctx);
    if (ret != 0) {
        return SB_STATUS_OTA_ERROR;
    }
    ret = ql_fota_image_verify_without_setflag(ctx->fota_ctx, &file_size, &is_dfota);
    if (ret != 0) {
        return SB_STATUS_OTA_ERROR;
    }
    ret = ql_fota_set_update_flag(file_size, is_dfota);
    if (ret != 0) {
        return SB_STATUS_OTA_ERROR;
    }
    return SB_STATUS_OK;
}

static sb_status_t sb_ota_activate_audio_pack(sb_ota_download_ctx_t *ctx)
{
    char state[160];
    const char *target;

    if (ctx == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (ctx->file != NULL) {
        (void)ql_fsync(ctx->file);
        if (ql_fclose(ctx->file) != 0) {
            ctx->file = NULL;
            return SB_STATUS_FILE_ERROR;
        }
        ctx->file = NULL;
    }

    target = (ctx->manifest.target_path[0] != '\0') ? ctx->manifest.target_path : SB_OTA_AUDIO_ACTIVE_PATH;
    (void)ql_remove(target);
    if (ql_rename(SB_OTA_AUDIO_TEMP_PATH, target) != 0) {
        return SB_STATUS_FILE_ERROR;
    }

    state[0] = '\0';
    if (sb_cloud_append_string(state, (u32)sizeof(state), "{\"version\":") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_json_string(state, (u32)sizeof(state), ctx->manifest.version) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(state, (u32)sizeof(state), ",\"target\":") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_json_string(state, (u32)sizeof(state), target) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if (sb_cloud_append_string(state, (u32)sizeof(state), "}") != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }

    return sb_storage_fs_write_file_atomic(SB_OTA_AUDIO_STATE_PATH,
                                           "U:/sb_audio_state.tmp",
                                           state,
                                           sb_cloud_str_len(state));
}

static sb_status_t sb_ota_process_manifest(const char *manifest_json)
{
    sb_ota_manifest_t manifest;
    sb_ota_download_ctx_t ctx;
    sb_status_t status;

    if (manifest_json == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (sb_ota_network_ready() == 0) {
        return SB_STATUS_NETWORK_ERROR;
    }

    status = sb_ota_manifest_parse(manifest_json, &manifest);
    if (status != SB_STATUS_OK) {
        return status;
    }

    sb_ota_set_status(SB_OTA_STATE_VERIFY_MANIFEST, &manifest, 0u, 0);
    status = sb_ota_manifest_verify_signature(&manifest);
    if (status != SB_STATUS_OK) {
        return status;
    }

    sb_ota_post_event(SB_EVENT_OTA_STARTED, SB_STATUS_OK, 0u, manifest.type);
    sb_ota_set_status(SB_OTA_STATE_DOWNLOADING, &manifest, 0u, 0);

    status = sb_ota_prepare_download(&ctx, &manifest);
    if (status != SB_STATUS_OK) {
        return status;
    }
    status = sb_ota_download_http(&ctx);
    if (status == SB_STATUS_OK) {
        sb_ota_set_status(SB_OTA_STATE_VERIFY_IMAGE, &manifest, 100u, 0);
        status = sb_ota_finish_hash(&ctx);
    }
    if (status == SB_STATUS_OK) {
        if (manifest.kind == SB_OTA_KIND_FIRMWARE) {
            status = sb_ota_activate_firmware(&ctx);
        } else {
            status = sb_ota_activate_audio_pack(&ctx);
        }
    }

    if (status == SB_STATUS_OK) {
        sb_ota_set_status(SB_OTA_STATE_STAGED, &manifest, 100u, 0);
        sb_ota_post_event(SB_EVENT_OTA_STAGED, SB_STATUS_OK, manifest.kind, manifest.version);
    } else {
        sb_ota_set_status(SB_OTA_STATE_FAULT, &manifest, 0u, (int)status);
        sb_ota_post_event(SB_EVENT_OTA_FAILED, (s32)status, manifest.kind, manifest.type);
    }

    sb_ota_cleanup_download(&ctx);
    return status;
}

static void sb_ota_task(void *argv)
{
    u32 slot = 0u;
    QlOSStatus ret;

    (void)argv;
    SB_LOGI(SB_OTA_MODULE_NAME, "task started");

    while (1) {
        ret = ql_rtos_queue_wait(s_ota_queue, (u8 *)&slot, (u32)sizeof(slot), QL_WAIT_FOREVER);
        if (ret != 0) {
            ql_rtos_task_sleep_ms(1000u);
            continue;
        }
        if (slot >= SB_OTA_QUEUE_DEPTH) {
            continue;
        }
        (void)sb_ota_process_manifest(s_ota_manifest_pool[slot]);
        sb_ota_pool_free(slot);
    }
}

sb_status_t sb_ota_service_init(const sb_config_payload_t *config)
{
    QlOSStatus ret;

    if (s_ota_started != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }
    if (config == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    s_ota_config = *config;
    (void)s_ota_config;

    if (ql_rtos_mutex_create(&s_ota_mutex) != 0) {
        return SB_STATUS_NO_MEMORY;
    }
    ret = ql_rtos_queue_create(&s_ota_queue, (u32)sizeof(u32), SB_OTA_QUEUE_DEPTH);
    if (ret != 0) {
        return SB_STATUS_QUEUE_ERROR;
    }
    ret = ql_rtos_task_create(&s_ota_task,
                              SB_OTA_TASK_STACK_SIZE_BYTES,
                              SB_OTA_TASK_PRIORITY,
                              "sb_ota",
                              sb_ota_task,
                              0);
    if (ret != 0) {
        return SB_STATUS_TASK_ERROR;
    }

    s_ota_started = 1;
    return SB_STATUS_OK;
}

sb_status_t sb_ota_service_start_from_manifest_json(const char *manifest_json)
{
    u32 slot = 0u;
    u32 len;

    if ((manifest_json == 0) || (s_ota_queue == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (sb_ota_pool_alloc(&slot) == 0) {
        return SB_STATUS_QUEUE_ERROR;
    }

    sb_cloud_copy_string(s_ota_manifest_pool[slot], SB_OTA_MANIFEST_JSON_LEN, manifest_json);
    len = sb_cloud_str_len(s_ota_manifest_pool[slot]);
    if ((len == 0u) || (len >= (SB_OTA_MANIFEST_JSON_LEN - 1u))) {
        sb_ota_pool_free(slot);
        return SB_STATUS_INVALID_PARAM;
    }

    sb_ota_set_status(SB_OTA_STATE_QUEUED, 0, 0u, 0);
    if (ql_rtos_queue_release(s_ota_queue, (u32)sizeof(slot), (u8 *)&slot, QL_NO_WAIT) != 0) {
        sb_ota_pool_free(slot);
        return SB_STATUS_QUEUE_ERROR;
    }
    return SB_STATUS_OK;
}

sb_status_t sb_ota_get_status(sb_ota_status_t *status)
{
    if (status == 0) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (s_ota_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_ota_mutex, QL_WAIT_FOREVER);
    }
    *status = s_ota_status;
    if (s_ota_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_ota_mutex);
    }
    return SB_STATUS_OK;
}
