/*================================================================
 * Static QR UPI Soundbox - HTTP Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_http_client.h"
#include "ql_fs.h"
#include "ql_rtos.h"
#include "ql_ssl_hal.h"
#include "sb_business_service.h"
#include "sb_cloud_utils.h"
#include "sb_config.h"
#include "sb_demo_profile.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_http_service.h"
#include "sb_log.h"
#include "sb_network_service.h"

#define SB_HTTP_MODULE_NAME              "http"
#define SB_HTTP_CIPHER_LIST              "ALL"
#define SB_HTTP_HEALTH_PATH              "/healthpacket"
#define SB_HTTP_COMMAND_RESPONSE_PATH    "/commandresponse"
#define SB_HTTP_HEADER_JSON              "Content-Type: application/json\r\n"
#define SB_HTTP_HEADER_CLOSE             "Connection: close\r\n"
#define SB_HTTP_HOST_LEN                  (96u)

typedef struct {
    int status_code;
    int body_len;
    int completed;
    int failed;
} sb_http_response_ctx_t;

static char s_http_root_ca_path[] = "U:/http_root_ca.pem";
static ql_task_t s_http_task = 0;
static ql_mutex_t s_http_mutex = 0;
static ql_queue_t s_http_queue = 0;
static sb_http_post_request_t s_http_request_pool[SB_HTTP_QUEUE_DEPTH];
static u8 s_http_request_pool_used[SB_HTTP_QUEUE_DEPTH];
static int s_http_started = 0;
static int s_http_cert_missing_logged = 0;
static sb_config_payload_t s_http_config;
static sb_http_status_t s_http_status = {0, 0, 0, 0u, 0u};

static void sb_http_zero(void *ptr, u32 length)
{
    u32 i;
    unsigned char *p;

    if ((ptr == 0) || (length == 0u)) {
        return;
    }

    p = (unsigned char *)ptr;
    for (i = 0u; i < length; i++) {
        p[i] = 0u;
    }
}

static void sb_http_status_update(int configured, int status_code, int error, int count_success, int count_fault)
{
    if (s_http_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_http_mutex, QL_WAIT_FOREVER);
    }

    s_http_status.configured = configured;
    s_http_status.last_status_code = status_code;
    s_http_status.last_error = error;
    if (count_success != 0) {
        s_http_status.post_count++;
    }
    if (count_fault != 0) {
        s_http_status.fault_count++;
    }

    if (s_http_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_http_mutex);
    }
}

static void sb_http_post_event(sb_event_id_t id, s32 status, u32 value, const char *text)
{
    sb_event_t event;

    sb_event_init(&event, id, SB_EVENT_SOURCE_HTTP);
    event.param_s32 = status;
    event.param_u32 = value;
    if (text != 0) {
        (void)sb_event_set_text(&event, text);
    }
    (void)sb_event_post(&event, QL_NO_WAIT);
}

static int sb_http_pool_alloc(u32 *slot)
{
    u32 i;

    if (slot == 0) {
        return 0;
    }

    if (s_http_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_http_mutex, QL_WAIT_FOREVER);
    }

    for (i = 0u; i < SB_HTTP_QUEUE_DEPTH; i++) {
        if (s_http_request_pool_used[i] == 0u) {
            s_http_request_pool_used[i] = 1u;
            *slot = i;
            if (s_http_mutex != 0) {
                (void)ql_rtos_mutex_unlock(s_http_mutex);
            }
            return 1;
        }
    }

    if (s_http_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_http_mutex);
    }

    return 0;
}

static void sb_http_pool_free(u32 slot)
{
    if (slot >= SB_HTTP_QUEUE_DEPTH) {
        return;
    }

    if (s_http_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_http_mutex, QL_WAIT_FOREVER);
    }
    s_http_request_pool_used[slot] = 0u;
    if (s_http_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_http_mutex);
    }
}

static int sb_http_network_ready(void)
{
    sb_network_status_t net_status;

    if (sb_network_get_status(&net_status) != SB_STATUS_OK) {
        return 0;
    }

    return (net_status.online != 0) ? 1 : 0;
}

static int sb_http_configured(void)
{
    return (s_http_config.http_base_url[0] != '\0') ? 1 : 0;
}

static int sb_http_tls_assets_ready(void)
{
    if (sb_cloud_url_is_https(s_http_config.http_base_url) == 0) {
        return 1;
    }

    if (ql_access(s_http_root_ca_path, 0u) != 0) {
        if (s_http_cert_missing_logged == 0) {
            SB_LOGE(SB_HTTP_MODULE_NAME, "missing cert %s", s_http_root_ca_path);
            s_http_cert_missing_logged = 1;
        }
        return 0;
    }

    s_http_cert_missing_logged = 0;
    return 1;
}

static sb_status_t sb_http_build_url(char *url, u32 url_len, const char *path)
{
    if ((url == 0) || (url_len == 0u) || (path == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    url[0] = '\0';
    if (sb_cloud_append_string(url, url_len, s_http_config.http_base_url) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    if ((sb_cloud_str_len(url) > 0u) && (url[sb_cloud_str_len(url) - 1u] == '/') && (path[0] == '/')) {
        path++;
    }
    if (sb_cloud_append_string(url, url_len, path) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }

    return SB_STATUS_OK;
}


static void sb_http_extract_host(const char *url, char *host, u32 host_len)
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

static void sb_http_configure_ssl(SSLConfig *ssl_config, const char *url, char *server_name, u32 server_name_len)
{
    if (ssl_config == 0) {
        return;
    }

    sb_http_zero(ssl_config, (u32)sizeof(*ssl_config));
    ssl_config->en = (sb_cloud_url_is_https(url) != 0) ? 1u : 0u;
    if (ssl_config->en != 0u) {
        sb_http_extract_host(url, server_name, server_name_len);
        ssl_config->profileIdx = SB_HTTP_PDP_PROFILE_ID;
        ssl_config->serverName = server_name;
        ssl_config->serverPort = SB_HTTP_TLS_PORT;
        ssl_config->protocol = 0u;
        ssl_config->dbgLevel = 0u;
        ssl_config->sessionReuseEn = 0u;
        ssl_config->vsn = SSL_VSN_ALL;
        ssl_config->verify = SSL_VERIFY_MODE_REQUIRED;
        ssl_config->cert.from = SSL_CERT_FROM_FS;
        ssl_config->cert.path.rootCA = s_http_root_ca_path;
        ssl_config->cert.path.clientKey = NULL;
        ssl_config->cert.path.clientCert = NULL;
        ssl_config->cert.clientKeyPwd.data = NULL;
        ssl_config->cert.clientKeyPwd.len = 0;
        ssl_config->cipherList = SB_HTTP_CIPHER_LIST;
        ssl_config->CTRDRBGSeed.data = NULL;
        ssl_config->CTRDRBGSeed.len = 0;
    }
}

static int sb_http_response_cb(QL_HTTP_CLIENT_T *client,
                               QL_HTTP_CLIENT_EVENT_E event,
                               int status_code,
                               char *data,
                               int data_len,
                               void *private_data)
{
    sb_http_response_ctx_t *ctx = (sb_http_response_ctx_t *)private_data;

    (void)client;
    (void)data;

    if (ctx == 0) {
        return 0;
    }

    if ((event == QL_HTTP_CLIENT_EVENT_SEND_FAIL) ||
        (event == QL_HTTP_CLIENT_EVENT_RECV_HEADER_FAIL) ||
        (event == QL_HTTP_CLIENT_EVENT_SOCK_RECV_FAIL)) {
        ctx->failed = 1;
        return 0;
    }

    if (event == QL_HTTP_CLIENT_EVENT_DISCONNECTED) {
        if (ctx->completed == 0) {
            ctx->failed = 1;
        }
        return 0;
    }

    if (event == QL_HTTP_CLIENT_EVENT_RECV_HEADER_FINISHED) {
        ctx->status_code = status_code;
    } else if (event == QL_HTTP_CLIENT_EVENT_RECV_BODY) {
        if (data_len > 0) {
            ctx->body_len += data_len;
        }
    } else if (event == QL_HTTP_CLIENT_EVENT_RECV_BODY_FINISHED) {
        ctx->completed = 1;
        ctx->status_code = status_code;
    }

    return 1;
}

static sb_status_t sb_http_build_health_payload(char *payload, u32 payload_len)
{
    return sb_business_service_build_health_payload(payload, payload_len);
}

static sb_status_t sb_http_post_json(const char *path, const char *payload, u32 payload_len, sb_event_id_t success_event)
{
    QL_HTTP_CLIENT_T *client;
    QL_HTTP_CLIENT_LIST_T *header = NULL;
    QL_HTTP_CLIENT_ERR_E ret;
    SSLConfig ssl_config;
    char ssl_server_name[SB_HTTP_HOST_LEN];
    sb_http_response_ctx_t response_ctx;
    char url[SB_HTTP_URL_LEN];

    if ((path == 0) || (payload == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (sb_http_tls_assets_ready() == 0) {
        sb_http_status_update(1, 0, SB_STATUS_CONFIG_ERROR, 0, 1);
        sb_http_post_event(SB_EVENT_HTTP_FAULT, (s32)SB_STATUS_CONFIG_ERROR, 0u, "cert_missing");
        return SB_STATUS_CONFIG_ERROR;
    }

    if (sb_http_build_url(url, (u32)sizeof(url), path) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }

    client = ql_http_client_init();
    if (client == NULL) {
        return SB_STATUS_HTTP_ERROR;
    }

    sb_http_zero(&response_ctx, (u32)sizeof(response_ctx));
    sb_http_zero(ssl_server_name, (u32)sizeof(ssl_server_name));
    sb_http_configure_ssl(&ssl_config, url, ssl_server_name, (u32)sizeof(ssl_server_name));

    header = ql_http_client_list_append(header, SB_HTTP_HEADER_JSON);
    header = ql_http_client_list_append(header, SB_HTTP_HEADER_CLOSE);

    (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_PDP_CID, SB_HTTP_PDP_PROFILE_ID);
    (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_PROTOCOL_VER, QL_HTTP_CLIENT_HTTP1_1);
    (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_ENABLE_COOKIE, 0);
    (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_TOTAL_TIMEOUT_MS, SB_HTTP_TOTAL_TIMEOUT_MS);
    (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_SSL_CTX, &ssl_config);
    if (header != NULL) {
        (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_HTTPHEADER, header);
    }

    ret = ql_http_client_request(client,
                                 url,
                                 QL_HTTP_CLIENT_REQUEST_POST,
                                 QL_HTTP_CLIENT_AUTH_TYPE_NONE,
                                 NULL,
                                 NULL,
                                 (char *)payload,
                                 (int)payload_len,
                                 sb_http_response_cb,
                                 &response_ctx);

    if (header != NULL) {
        ql_http_client_list_destroy(header);
    }
    ql_http_client_release(client);

    if (ret != QL_HTTP_CLIENT_ERR_SUCCESS) {
        sb_http_status_update(1, response_ctx.status_code, (int)ret, 0, 1);
        sb_http_post_event(SB_EVENT_HTTP_FAULT, (s32)ret, 0u, "request");
        return SB_STATUS_HTTP_ERROR;
    }

    if ((response_ctx.failed != 0) ||
        (response_ctx.status_code < 200) ||
        (response_ctx.status_code >= 300)) {
        sb_http_status_update(1, response_ctx.status_code, SB_STATUS_HTTP_ERROR, 0, 1);
        sb_http_post_event(SB_EVENT_HTTP_FAULT, (s32)response_ctx.status_code, (u32)response_ctx.body_len, "response");
        return SB_STATUS_HTTP_ERROR;
    }

    sb_http_status_update(1, response_ctx.status_code, 0, 1, 0);
    sb_http_post_event(success_event, SB_STATUS_OK, (u32)response_ctx.status_code, "ok");
    return SB_STATUS_OK;
}

static void sb_http_process_queue(void)
{
    sb_http_post_request_t request;
    QlOSStatus ret;
    u32 slot = 0u;

    if (s_http_queue == 0) {
        return;
    }

    while (1) {
        ret = ql_rtos_queue_wait(s_http_queue, (u8 *)&slot, (u32)sizeof(slot), QL_NO_WAIT);
        if (ret != 0) {
            break;
        }
        if (slot >= SB_HTTP_QUEUE_DEPTH) {
            continue;
        }
        request = s_http_request_pool[slot];
        sb_http_pool_free(slot);
        if (request.type == SB_HTTP_POST_COMMAND_RESPONSE) {
            (void)sb_http_post_json(SB_HTTP_COMMAND_RESPONSE_PATH,
                                    request.payload,
                                    request.payload_len,
                                    SB_EVENT_HTTP_COMMAND_RESPONSE_DONE);
        }
    }
}

static void sb_http_task(void *argv)
{
    char payload[SB_HTTP_PAYLOAD_LEN];
    u32 elapsed_ms = 0u;
    u32 interval_ms;

    (void)argv;
    SB_LOGI(SB_HTTP_MODULE_NAME, "task started");

    while (1) {
        while (sb_http_network_ready() == 0) {
            ql_rtos_task_sleep_ms(SB_HTTP_NETWORK_WAIT_MS);
        }

        if (sb_http_configured() == 0) {
            sb_http_status_update(0, 0, SB_STATUS_CONFIG_ERROR, 0, 0);
            ql_rtos_task_sleep_ms(SB_HTTP_NETWORK_WAIT_MS);
            continue;
        }

        if (sb_http_tls_assets_ready() == 0) {
            sb_http_status_update(1, 0, SB_STATUS_CONFIG_ERROR, 0, 0);
            ql_rtos_task_sleep_ms(SB_HTTP_NETWORK_WAIT_MS);
            continue;
        }

        sb_http_process_queue();
        interval_ms = s_http_config.health_interval_sec * 1000u;
        if (interval_ms < (SB_HTTP_MIN_INTERVAL_SEC * 1000u)) {
            interval_ms = SB_HTTP_MIN_INTERVAL_SEC * 1000u;
        }

        elapsed_ms += SB_HTTP_NETWORK_WAIT_MS;
        if (elapsed_ms >= interval_ms) {
            elapsed_ms = 0u;
            if (sb_http_build_health_payload(payload, (u32)sizeof(payload)) == SB_STATUS_OK) {
                (void)sb_http_post_json(SB_HTTP_HEALTH_PATH,
                                        payload,
                                        sb_cloud_str_len(payload),
                                        SB_EVENT_HTTP_HEALTH_DONE);
            }
        }

        ql_rtos_task_sleep_ms(SB_HTTP_NETWORK_WAIT_MS);
    }
}

sb_status_t sb_http_service_init(const sb_config_payload_t *config)
{
    QlOSStatus ret;

    if (s_http_started != 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }

    if (config == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    s_http_config = *config;
    sb_demo_expand_config_runtime(&s_http_config);
    ret = ql_rtos_mutex_create(&s_http_mutex);
    if (ret != 0) {
        return SB_STATUS_NO_MEMORY;
    }

    ret = ql_rtos_queue_create(&s_http_queue, (u32)sizeof(u32), SB_HTTP_QUEUE_DEPTH);
    if (ret != 0) {
        return SB_STATUS_QUEUE_ERROR;
    }

    ret = ql_rtos_task_create(&s_http_task,
                              SB_HTTP_TASK_STACK_SIZE_BYTES,
                              SB_HTTP_TASK_PRIORITY,
                              "sb_http",
                              sb_http_task,
                              0);
    if (ret != 0) {
        return SB_STATUS_TASK_ERROR;
    }

    s_http_started = 1;
    return SB_STATUS_OK;
}

sb_status_t sb_http_service_post_command_response(const char *payload, u32 payload_len)
{
    sb_http_post_request_t request;
    u32 i;
    u32 copy_len;
    u32 slot = 0u;

    if ((payload == 0) || (s_http_queue == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (sb_http_pool_alloc(&slot) == 0) {
        return SB_STATUS_QUEUE_ERROR;
    }

    sb_http_zero(&request, (u32)sizeof(request));
    request.type = SB_HTTP_POST_COMMAND_RESPONSE;
    copy_len = payload_len;
    if (copy_len >= SB_HTTP_PAYLOAD_LEN) {
        copy_len = SB_HTTP_PAYLOAD_LEN - 1u;
    }
    for (i = 0u; i < copy_len; i++) {
        request.payload[i] = payload[i];
    }
    request.payload[copy_len] = '\0';
    request.payload_len = copy_len;

    s_http_request_pool[slot] = request;
    if (ql_rtos_queue_release(s_http_queue, (u32)sizeof(slot), (u8 *)&slot, QL_NO_WAIT) != 0) {
        sb_http_pool_free(slot);
        return SB_STATUS_QUEUE_ERROR;
    }

    return SB_STATUS_OK;
}

sb_status_t sb_http_get_status(sb_http_status_t *status)
{
    if (status == 0) {
        return SB_STATUS_INVALID_PARAM;
    }

    if (s_http_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_http_mutex, QL_WAIT_FOREVER);
    }
    *status = s_http_status;
    if (s_http_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_http_mutex);
    }

    return SB_STATUS_OK;
}
