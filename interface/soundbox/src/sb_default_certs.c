/*================================================================
 * Static QR UPI Soundbox - Demo Default Certificate Buffers
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *
 * These PEM blocks are migrated from the provided EC200U-CN-AA demo
 * application source. Phase 22 writes them once to UFS certificate files
 * because the EG800AK MQTT stack is configured for SSL_CERT_FROM_FS.
 *================================================================*/
#include "sb_default_certs.h"
#include "ql_fs.h"
#include "sb_storage_fs.h"
#include "sb_log.h"

#define SB_DEFAULT_CERTS_MODULE_NAME "cert_defaults"
#define SB_MQTT_ROOT_CA_PATH         "U:/certs/mqtt_root_ca.pem"
#define SB_MQTT_CLIENT_CRT_PATH      "U:/certs/mqtt_client.crt"
#define SB_MQTT_CLIENT_KEY_PATH      "U:/certs/mqtt_client.key"

static const char s_default_root_ca_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
    "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
    "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
    "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
    "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
    "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
    "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
    "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
    "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
    "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
    "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
    "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
    "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
    "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
    "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
    "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
    "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
    "rqXRfboQnoZsG4q5WTP468SQvvG5\n"
    "-----END CERTIFICATE-----\n";

static const char s_default_client_crt_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDWjCCAkKgAwIBAgIVAIot/lU1IqHFsipnxmVI6IPzsB+zMA0GCSqGSIb3DQEB\n"
    "CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t\n"
    "IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0yMzEwMDUxMjQy\n"
    "NDRaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh\n"
    "dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDS7/al1wuZYinmxmhU\n"
    "SZavaMxZZ/YLeIPSMmqhHVStcgZhBguNbeIXdSQtqlHpOGEjLHg0eYr4diHv2FUj\n"
    "S4AmlFCbvJuCs6BZfvisdNRDtWxcvVVTs9jkAemetWLcLTOkVIbKWvslBBOHOmcW\n"
    "BVIw6SxZ3ClEsazjbh9GCTlmQF5uyhJeGm3tNUQIhrQuuB9UUgEF3IIq0q4cHgsI\n"
    "xXo6Jm73ng1yAVSlV5Eqm5WkTnSrCfrRbU5GwcINDv7q17/fTGJVj7iQll5ppXMB\n"
    "Zf2XDLarDnCBfeMyNEhnzxypaOmHXv4prLfGl+4oHyN2nkj23XOP5y1DI9SZ/cMc\n"
    "CEaBAgMBAAGjYDBeMB8GA1UdIwQYMBaAFLy9YmFV9qxR+/FM1ZQ0W7ri1m4vMB0G\n"
    "A1UdDgQWBBQg98kVR2IM3pyPobcqjia3VORGJjAMBgNVHRMBAf8EAjAAMA4GA1Ud\n"
    "DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAYrDZ9b2Zy1avVnYjZjUuUp9z\n"
    "6VrWYuQx4HGcxeL48yzAhynKLk1PbePnfVVxKiMpjR0vwhVfv+t0CFs9cx/3DqTg\n"
    "xh1vOtMW2e+rTabNmsJ8Wrmp7S6ZN21xomZLgPQOdZv0MaNu7DJLuvgU29W+A+Tp\n"
    "AVPt0+MFReJkb/NXpWVQj0wA+3ZonwDLyoIdfdLLV/hZs9HZMJxWvhAtjl7LipSd\n"
    "+W5DCb6q7C1nmI81EUvXrz544KahpzGLsrxb5PmjTto5EPhF/8DKut382mYoaVo2\n"
    "rgDRfoWVPP6mdKoDL/9r6v4/16x3e00M0S4YTqa3y9QzlcwvDmd7Q5u6j6PU4A==\n"
    "-----END CERTIFICATE-----\n";

static const char s_default_client_key_pem[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEpAIBAAKCAQEA0u/2pdcLmWIp5sZoVEmWr2jMWWf2C3iD0jJqoR1UrXIGYQYL\n"
    "jW3iF3UkLapR6ThhIyx4NHmK+HYh79hVI0uAJpRQm7ybgrOgWX74rHTUQ7VsXL1V\n"
    "U7PY5AHpnrVi3C0zpFSGylr7JQQThzpnFgVSMOksWdwpRLGs424fRgk5ZkBebsoS\n"
    "Xhpt7TVECIa0LrgfVFIBBdyCKtKuHB4LCMV6OiZu954NcgFUpVeRKpuVpE50qwn6\n"
    "0W1ORsHCDQ7+6te/30xiVY+4kJZeaaVzAWX9lwy2qw5wgX3jMjRIZ88cqWjph17+\n"
    "Kay3xpfuKB8jdp5I9t1zj+ctQyPUmf3DHAhGgQIDAQABAoIBAEcGW6N9pD8ySW+W\n"
    "MPzohl9rxxr3oWY9Xw2Awlyblq6MLU+wDGXDsRQ68kKaDdicjkCcsaDhqAR/KnAr\n"
    "BIfM3UqgOAAoYqiAuSZJVYwkDnD5/pG/gqKbdRVY+4PgCXifk7rqZzRtrBDLrZOj\n"
    "+5/zjLhBWxiqxA6IUPGHW+pmIrilqVSk6WZr0OERGyozgu14Lty6kP0F2aEQSOAT\n"
    "UD7yq0oBn6zZv2EyvHI2+CyQg6jL1qPCPGhtg3hnL3RrfPhm9cMOAI3mXV+yTjo3\n"
    "VFbKBW1C+hY/bubEz8/0734m3hySzicYGJor9IDKaZm71gE9jrb/wVPy+Y7ww0sU\n"
    "WTeMsD0CgYEA79TeC4w5+VW+gfmS7WrIwNUl9VokHPhwBO9v6whmdlK2QfJCRIOP\n"
    "OAVMKTnVd9UM3uoSlVia4a7u5E5ec0pRFloJpXw3bXWV2OPKPRa2AtgF+aCvCS3i\n"
    "aSwUrXug9iOAMseBW8gBkXjf22UouSAYUPYpAKiCOTTuYS9PFUSb4FsCgYEA4Sht\n"
    "H1cuhuNJ1g4V/3uoyNsm1BJhNLqMN2kejvlRlxEiILfkvnfJ/mexijcjRSgdRMiC\n"
    "LXdQkzPHxvoEmhs+GIfZt9OvPW1Zkm3EHjzXfXaenXi0PYClV6caMaFuPj341LRa\n"
    "yi56c8c6r03hPxyUy59SAqHsgWpCXybMGImR61MCgYAU1zugZ0QWbaQLaWOiK/hc\n"
    "AWm2A2pF9jTNyPzBwM4elBtwaZvmlkQYyyUOJA9vxVzD9jU9MyDqYagywLimbhvT\n"
    "xGk4Ly4l3eTynwDRBCs23gzO3262bn1RRFpbpPWczgLy5rFQB3ZTbNyPFSBPzgEu\n"
    "7+UqQHpofDgGy/SkVXzyJwKBgQDbc4+IMG0Ew5s/qtL+BATqX89ke9WKkxf/GdHT\n"
    "AGXpm6VrxlCI5DBYabC9SihySzrbw6I9tkEueBWLdjvuCAdp2V72sLfoYyAefCXe\n"
    "YuaVPnwyd2cVWsHMwO3i2bidqcFRb8fdr5diKRilrH6SCMVuAYbpUfE14d925lWe\n"
    "xNcYSQKBgQClCllJwGMQPwnuJQPT37UZ+vfWrOt1Gmj7PyDzPLFOZ9lusDxQfbeN\n"
    "6wHnCHo5GmSeABVyhA+zQnYMOeDvb/wX0EQpsH+7BHkmoN4vHmenbrCwEuJEr9TY\n"
    "Yl4hXTVhNu2r2zbaqSriEuKzcQOuwf6VVIiRDPHm2fwqUtq19bTpdQ==\n"
    "-----END RSA PRIVATE KEY-----\n";

const char *sb_default_certs_mqtt_root_ca(void)
{
    return s_default_root_ca_pem;
}

const char *sb_default_certs_mqtt_client_crt(void)
{
    return s_default_client_crt_pem;
}

const char *sb_default_certs_mqtt_client_key(void)
{
    return s_default_client_key_pem;
}


static sb_status_t sb_default_cert_write_file(const char *path, const char *data)
{
    QFILE *fp;
    u32 len;
    int written;

    if ((path == 0) || (data == 0) || (data[0] == '\0')) {
        return SB_STATUS_INVALID_PARAM;
    }
    if (ql_access(path, 0u) == 0) {
        return SB_STATUS_ALREADY_INITIALIZED;
    }
    if (sb_storage_fs_mkdir_recursive(SB_STORAGE_CERT_DIR) != SB_STATUS_OK) {
        return SB_STATUS_FILE_ERROR;
    }

    fp = ql_fopen(path, "w+");
    if (fp == 0) {
        return SB_STATUS_FILE_ERROR;
    }
    len = 0u;
    while (data[len] != '\0') {
        len++;
    }
    written = ql_fwrite((void *)data, 1u, len, fp);
    (void)ql_fsync(fp);
    (void)ql_fclose(fp);
    if (written != (int)len) {
        (void)ql_remove(path);
        return SB_STATUS_FILE_ERROR;
    }
    SB_LOGI(SB_DEFAULT_CERTS_MODULE_NAME, "created %s bytes=%u", path, len);
    return SB_STATUS_OK;
}

static void sb_default_cert_log_status(const char *name, sb_status_t status)
{
    if ((status == SB_STATUS_OK) || (status == SB_STATUS_ALREADY_INITIALIZED)) {
        return;
    }
    SB_LOGW(SB_DEFAULT_CERTS_MODULE_NAME, "%s create status=%s", name, sb_status_to_string(status));
}

sb_status_t sb_default_certs_ensure_mqtt_files(void)
{
    sb_status_t status;
    sb_status_t final_status = SB_STATUS_OK;

    status = sb_default_cert_write_file(SB_MQTT_ROOT_CA_PATH, s_default_root_ca_pem);
    sb_default_cert_log_status("mqtt_root_ca", status);
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) { final_status = status; }

    status = sb_default_cert_write_file(SB_MQTT_CLIENT_CRT_PATH, s_default_client_crt_pem);
    sb_default_cert_log_status("mqtt_client_crt", status);
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) { final_status = status; }

    status = sb_default_cert_write_file(SB_MQTT_CLIENT_KEY_PATH, s_default_client_key_pem);
    sb_default_cert_log_status("mqtt_client_key", status);
    if ((status != SB_STATUS_OK) && (status != SB_STATUS_ALREADY_INITIALIZED)) { final_status = status; }

    return final_status;
}
