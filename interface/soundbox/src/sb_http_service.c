/*================================================================
 * Static QR UPI Soundbox - HTTPS Registration and Health Service
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "ql_http_client.h"
#include "ql_rtos.h"
#include "ql_ssl_hal.h"
#include "sb_business_service.h"
#include "sb_cloud_utils.h"
#include "sb_config.h"
#include "sb_default_certs.h"
#include "sb_event.h"
#include "sb_event_bus.h"
#include "sb_http_service.h"
#include "sb_log.h"
#include "sb_network_service.h"
#include "sb_storage_fs.h"
#include "sb_time_service.h"

#define SB_HTTP_MODULE_NAME "http"

static ql_task_t s_http_task = 0;
static ql_mutex_t s_http_mutex = 0;
static int s_http_started = 0;
static sb_config_payload_t s_http_config;
static sb_http_status_t s_http_status;
static char s_http_response[SB_HTTP_RESPONSE_LEN];
static u32 s_http_response_used;

static void sb_http_zero(void *ptr, u32 length)
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

const char *sb_http_state_name(sb_http_state_t state)
{
    switch (state) {
    case SB_HTTP_STATE_WAIT_NETWORK: return "wait_network";
    case SB_HTTP_STATE_REGISTERING: return "registering";
    case SB_HTTP_STATE_REGISTERED: return "registered";
    case SB_HTTP_STATE_UNREGISTERED: return "unregistered";
    case SB_HTTP_STATE_BACKOFF: return "backoff";
    case SB_HTTP_STATE_STOPPED:
    default: return "stopped";
    }
}

static void sb_http_set_state(sb_http_state_t state, int registered, int status_code, int error)
{
    if (s_http_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_http_mutex, QL_WAIT_FOREVER);
    }
    s_http_status.state = state;
    s_http_status.registered = registered;
    s_http_status.last_status_code = status_code;
    s_http_status.last_error = error;
    if (s_http_mutex != 0) {
        (void)ql_rtos_mutex_unlock(s_http_mutex);
    }
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

static void sb_http_post_event(sb_event_id_t id, sb_status_t status, u32 value, const char *text)
{
    sb_event_t event;
    sb_event_init(&event, id, SB_EVENT_SOURCE_HTTP);
    event.param_s32 = (s32)status;
    event.param_u32 = value;
    if (text != 0) {
        (void)sb_event_set_text(&event, text);
    }
    (void)sb_event_post(&event, QL_NO_WAIT);
}

static int sb_http_network_ready(void)
{
    sb_network_status_t net;
    if (sb_network_get_status(&net) != SB_STATUS_OK) {
        return 0;
    }
    return (net.online != 0) ? 1 : 0;
}

static int sb_http_time_ready(void)
{
    sb_time_status_t time_status;
    if (sb_time_get_rtc(&time_status) != SB_STATUS_OK) {
        return 0;
    }
    return (time_status.rtc_valid != 0) ? 1 : 0;
}

static int sb_http_response_cb(QL_HTTP_CLIENT_T *client,
                               QL_HTTP_CLIENT_EVENT_E event,
                               int status_code,
                               char *data,
                               int data_len,
                               void *private_data)
{
    (void)client;
    (void)private_data;
    if (event == QL_HTTP_CLIENT_EVENT_RECV_HEADER_FINISHED) {
        s_http_status.last_status_code = status_code;
        return 1;
    }
    if (event == QL_HTTP_CLIENT_EVENT_RECV_BODY) {
        if ((data != 0) && (data_len > 0)) {
            u32 copy_len = (u32)data_len;
            u32 i;
            if (copy_len > ((u32)sizeof(s_http_response) - 1u - s_http_response_used)) {
                copy_len = (u32)sizeof(s_http_response) - 1u - s_http_response_used;
            }
            for (i = 0u; i < copy_len; i++) {
                s_http_response[s_http_response_used + i] = data[i];
            }
            s_http_response_used += copy_len;
            s_http_response[s_http_response_used] = '\0';
        }
        return 1;
    }
    if ((event == QL_HTTP_CLIENT_EVENT_SEND_FAIL) ||
        (event == QL_HTTP_CLIENT_EVENT_RECV_HEADER_FAIL) ||
        (event == QL_HTTP_CLIENT_EVENT_DISCONNECTED)) {
        return 0;
    }
    return 1;
}

static sb_status_t sb_http_build_url(char *url, u32 url_len, const char *path)
{
    if ((url == 0) || (url_len == 0u) || (path == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    url[0] = '\0';
    if (sb_cloud_append_string(url, url_len, "https://") != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_cloud_append_string(url, url_len, s_http_config.mqtt_host) != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    return sb_cloud_append_string(url, url_len, path);
}

static sb_status_t sb_http_build_registration_payload(char *body, u32 body_len)
{
    char health[SB_HTTP_BODY_LEN];
    sb_status_t status;
    body[0] = '\0';
    health[0] = '\0';
    status = sb_business_service_build_health_payload(health, (u32)sizeof(health));
    if (status != SB_STATUS_OK) {
        return status;
    }
    if (sb_cloud_append_string(body, body_len, "{\"deviceId\":") != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_cloud_append_json_string(body, body_len, s_http_config.device_id) != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_cloud_append_string(body, body_len, ",\"firmware\":\"1.0.0-phase23-production-fix-v6\",\"hardware\":\"KAE8_SQ1_260611\",\"health\":") != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    if (sb_cloud_append_string(body, body_len, health) != SB_STATUS_OK) { return SB_STATUS_NO_MEMORY; }
    return sb_cloud_append_string(body, body_len, "}");
}

static sb_status_t sb_http_request_json(const char *path, const char *payload)
{
    QL_HTTP_CLIENT_T *client;
    QL_HTTP_CLIENT_LIST_T *headers = NULL;
    QL_HTTP_CLIENT_ERR_E ret;
    char url[SB_HTTP_URL_LEN];
    SSLConfig ssl_config;

    if ((path == 0) || (payload == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (sb_http_build_url(url, (u32)sizeof(url), path) != SB_STATUS_OK) {
        return SB_STATUS_NO_MEMORY;
    }
    sb_http_zero(&ssl_config, (u32)sizeof(ssl_config));
    ssl_config.en = 1u;
    ssl_config.profileIdx = SB_HTTP_PDP_PROFILE_ID;
    ssl_config.serverName = s_http_config.mqtt_host;
    ssl_config.serverPort = SB_HTTP_TLS_PORT;
    ssl_config.vsn = SSL_VSN_ALL;
    ssl_config.verify = SSL_VERIFY_MODE_REQUIRED;
    ssl_config.cert.from = SSL_CERT_FROM_FS;
    ssl_config.cert.path.rootCA = SB_HTTP_ROOT_CA_PATH;
    ssl_config.cipherList = "ALL";

    s_http_response_used = 0u;
    s_http_response[0] = '\0';
    client = ql_http_client_init();
    if (client == 0) {
        return SB_STATUS_HTTP_ERROR;
    }
    headers = ql_http_client_list_append(headers, "Content-Type: application/json\r\n");
    headers = ql_http_client_list_append(headers, "Connection: close\r\n");
    (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_PDP_CID, SB_HTTP_PDP_PROFILE_ID);
    (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_HTTPHEADER, headers);
    (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_SSL_CTX, &ssl_config);
    (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_RECV_TIMEOUT_MS, 30000);
    ret = ql_http_client_request(client, url, QL_HTTP_CLIENT_REQUEST_POST,
                                 QL_HTTP_CLIENT_AUTH_TYPE_NONE, NULL, NULL,
                                 (char *)payload, (int)sb_cloud_str_len(payload),
                                 sb_http_response_cb, NULL);
    ql_http_client_list_destroy(headers);
    ql_http_client_release(client);
    if (ret != QL_HTTP_CLIENT_ERR_SUCCESS) {
        return SB_STATUS_HTTP_ERROR;
    }
    return SB_STATUS_OK;
}

static int sb_http_response_registered(void)
{
    if ((sb_cloud_find(s_http_response, "\"kiotel\":true") != 0) ||
        (sb_cloud_find(s_http_response, "\"registered\":true") != 0) ||
        (sb_cloud_find(s_http_response, "TRUE") != 0)) {
        return 1;
    }
    return 0;
}

static int sb_http_response_unregistered(void)
{
    if ((sb_cloud_find(s_http_response, "\"kiotel\":false") != 0) ||
        (sb_cloud_find(s_http_response, "\"registered\":false") != 0) ||
        (sb_cloud_find(s_http_response, "FALSE") != 0)) {
        return 1;
    }
    return 0;
}

static sb_status_t sb_http_registration_cycle(void)
{
    char body[SB_HTTP_BODY_LEN];
    sb_status_t status;

    status = sb_http_build_registration_payload(body, (u32)sizeof(body));
    if (status != SB_STATUS_OK) {
        return status;
    }
    status = sb_http_request_json("/api/soundbox/register", body);
    if (status != SB_STATUS_OK) {
        return status;
    }
    if (s_http_mutex != 0) {
        (void)ql_rtos_mutex_lock(s_http_mutex, QL_WAIT_FOREVER);
        s_http_status.request_count++;
        (void)ql_rtos_mutex_unlock(s_http_mutex);
    }
    if (sb_http_response_registered() != 0) {
        sb_http_set_state(SB_HTTP_STATE_REGISTERED, 1, s_http_status.last_status_code, 0);
        sb_http_post_event(SB_EVENT_HTTP_REGISTERED, SB_STATUS_OK, 1u, "registered");
        return SB_STATUS_OK;
    }
    if (sb_http_response_unregistered() != 0) {
        sb_http_set_state(SB_HTTP_STATE_UNREGISTERED, 0, s_http_status.last_status_code, 0);
        sb_http_post_event(SB_EVENT_HTTP_UNREGISTERED, SB_STATUS_SECURITY_ERROR, 0u, "unregistered");
        return SB_STATUS_SECURITY_ERROR;
    }
    return SB_STATUS_HTTP_ERROR;
}

static void sb_http_task(void *argv)
{
    u32 backoff = SB_HTTP_BACKOFF_BASE_MS;
    sb_status_t status;
    (void)argv;
    SB_LOGI(SB_HTTP_MODULE_NAME, "task started");
    while (1) {
        sb_http_set_state(SB_HTTP_STATE_WAIT_NETWORK, 0, 0, 0);
        while ((sb_http_network_ready() == 0) || (sb_http_time_ready() == 0)) {
            ql_rtos_task_sleep_ms(2000u);
        }
        sb_http_set_state(SB_HTTP_STATE_REGISTERING, 0, 0, 0);
        status = sb_http_registration_cycle();
        if (status == SB_STATUS_OK) {
            backoff = SB_HTTP_BACKOFF_BASE_MS;
            ql_rtos_task_sleep_ms(SB_HTTP_HEALTH_INTERVAL_MS);
            continue;
        }
        if (s_http_mutex != 0) {
            (void)ql_rtos_mutex_lock(s_http_mutex, QL_WAIT_FOREVER);
            s_http_status.fail_count++;
            (void)ql_rtos_mutex_unlock(s_http_mutex);
        }
        sb_http_set_state(SB_HTTP_STATE_BACKOFF, 0, s_http_status.last_status_code, (int)status);
        sb_http_post_event(SB_EVENT_HTTP_FAILED, status, s_http_status.fail_count, "request_failed");
        ql_rtos_task_sleep_ms(backoff);
        if (backoff < SB_HTTP_BACKOFF_MAX_MS) {
            backoff *= 2u;
            if (backoff > SB_HTTP_BACKOFF_MAX_MS) {
                backoff = SB_HTTP_BACKOFF_MAX_MS;
            }
        }
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
    sb_http_zero(&s_http_status, (u32)sizeof(s_http_status));
    if (ql_rtos_mutex_create(&s_http_mutex) != 0) {
        return SB_STATUS_NO_MEMORY;
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
