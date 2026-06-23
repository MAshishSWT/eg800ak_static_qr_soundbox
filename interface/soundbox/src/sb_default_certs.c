/*================================================================
 * Static QR UPI Soundbox - Demo Default Certificate Buffers
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *
 * These PEM blocks are migrated from the provided EC200U-CN-AA demo
 * application source and are provided to the Quectel SSL layer directly
 * through SSL_CERT_FROM_BUF. No certificate files are created.
 *================================================================*/
#include "sb_default_certs.h"


static const char s_default_root_ca_pem[] =
    "-----BEGIN CERTIFICATE-----\r\n"
    "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\r\n"
    "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\r\n"
    "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\r\n"
    "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\r\n"
    "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\r\n"
    "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\r\n"
    "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\r\n"
    "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\r\n"
    "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\r\n"
    "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\r\n"
    "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\r\n"
    "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\r\n"
    "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\r\n"
    "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\r\n"
    "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\r\n"
    "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\r\n"
    "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\r\n"
    "rqXRfboQnoZsG4q5WTP468SQvvG5\r\n"
    "-----END CERTIFICATE-----\r\n";

static const char s_default_client_crt_pem[] =
    "-----BEGIN CERTIFICATE-----\r\n"
    "MIIDWjCCAkKgAwIBAgIVAIot/lU1IqHFsipnxmVI6IPzsB+zMA0GCSqGSIb3DQEB\r\n"
    "CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t\r\n"
    "IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0yMzEwMDUxMjQy\r\n"
    "NDRaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh\r\n"
    "dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDS7/al1wuZYinmxmhU\r\n"
    "SZavaMxZZ/YLeIPSMmqhHVStcgZhBguNbeIXdSQtqlHpOGEjLHg0eYr4diHv2FUj\r\n"
    "S4AmlFCbvJuCs6BZfvisdNRDtWxcvVVTs9jkAemetWLcLTOkVIbKWvslBBOHOmcW\r\n"
    "BVIw6SxZ3ClEsazjbh9GCTlmQF5uyhJeGm3tNUQIhrQuuB9UUgEF3IIq0q4cHgsI\r\n"
    "xXo6Jm73ng1yAVSlV5Eqm5WkTnSrCfrRbU5GwcINDv7q17/fTGJVj7iQll5ppXMB\r\n"
    "Zf2XDLarDnCBfeMyNEhnzxypaOmHXv4prLfGl+4oHyN2nkj23XOP5y1DI9SZ/cMc\r\n"
    "CEaBAgMBAAGjYDBeMB8GA1UdIwQYMBaAFLy9YmFV9qxR+/FM1ZQ0W7ri1m4vMB0G\r\n"
    "A1UdDgQWBBQg98kVR2IM3pyPobcqjia3VORGJjAMBgNVHRMBAf8EAjAAMA4GA1Ud\r\n"
    "DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAYrDZ9b2Zy1avVnYjZjUuUp9z\r\n"
    "6VrWYuQx4HGcxeL48yzAhynKLk1PbePnfVVxKiMpjR0vwhVfv+t0CFs9cx/3DqTg\r\n"
    "xh1vOtMW2e+rTabNmsJ8Wrmp7S6ZN21xomZLgPQOdZv0MaNu7DJLuvgU29W+A+Tp\r\n"
    "AVPt0+MFReJkb/NXpWVQj0wA+3ZonwDLyoIdfdLLV/hZs9HZMJxWvhAtjl7LipSd\r\n"
    "+W5DCb6q7C1nmI81EUvXrz544KahpzGLsrxb5PmjTto5EPhF/8DKut382mYoaVo2\r\n"
    "rgDRfoWVPP6mdKoDL/9r6v4/16x3e00M0S4YTqa3y9QzlcwvDmd7Q5u6j6PU4A==\r\n"
    "-----END CERTIFICATE-----\r\n";

static const char s_default_client_key_pem[] =
    "-----BEGIN RSA PRIVATE KEY-----\r\n"
    "MIIEpAIBAAKCAQEA0u/2pdcLmWIp5sZoVEmWr2jMWWf2C3iD0jJqoR1UrXIGYQYL\r\n"
    "jW3iF3UkLapR6ThhIyx4NHmK+HYh79hVI0uAJpRQm7ybgrOgWX74rHTUQ7VsXL1V\r\n"
    "U7PY5AHpnrVi3C0zpFSGylr7JQQThzpnFgVSMOksWdwpRLGs424fRgk5ZkBebsoS\r\n"
    "Xhpt7TVECIa0LrgfVFIBBdyCKtKuHB4LCMV6OiZu954NcgFUpVeRKpuVpE50qwn6\r\n"
    "0W1ORsHCDQ7+6te/30xiVY+4kJZeaaVzAWX9lwy2qw5wgX3jMjRIZ88cqWjph17+\r\n"
    "Kay3xpfuKB8jdp5I9t1zj+ctQyPUmf3DHAhGgQIDAQABAoIBAEcGW6N9pD8ySW+W\r\n"
    "MPzohl9rxxr3oWY9Xw2Awlyblq6MLU+wDGXDsRQ68kKaDdicjkCcsaDhqAR/KnAr\r\n"
    "BIfM3UqgOAAoYqiAuSZJVYwkDnD5/pG/gqKbdRVY+4PgCXifk7rqZzRtrBDLrZOj\r\n"
    "+5/zjLhBWxiqxA6IUPGHW+pmIrilqVSk6WZr0OERGyozgu14Lty6kP0F2aEQSOAT\r\n"
    "UD7yq0oBn6zZv2EyvHI2+CyQg6jL1qPCPGhtg3hnL3RrfPhm9cMOAI3mXV+yTjo3\r\n"
    "VFbKBW1C+hY/bubEz8/0734m3hySzicYGJor9IDKaZm71gE9jrb/wVPy+Y7ww0sU\r\n"
    "WTeMsD0CgYEA79TeC4w5+VW+gfmS7WrIwNUl9VokHPhwBO9v6whmdlK2QfJCRIOP\r\n"
    "OAVMKTnVd9UM3uoSlVia4a7u5E5ec0pRFloJpXw3bXWV2OPKPRa2AtgF+aCvCS3i\r\n"
    "aSwUrXug9iOAMseBW8gBkXjf22UouSAYUPYpAKiCOTTuYS9PFUSb4FsCgYEA4Sht\r\n"
    "H1cuhuNJ1g4V/3uoyNsm1BJhNLqMN2kejvlRlxEiILfkvnfJ/mexijcjRSgdRMiC\r\n"
    "LXdQkzPHxvoEmhs+GIfZt9OvPW1Zkm3EHjzXfXaenXi0PYClV6caMaFuPj341LRa\r\n"
    "yi56c8c6r03hPxyUy59SAqHsgWpCXybMGImR61MCgYAU1zugZ0QWbaQLaWOiK/hc\r\n"
    "AWm2A2pF9jTNyPzBwM4elBtwaZvmlkQYyyUOJA9vxVzD9jU9MyDqYagywLimbhvT\r\n"
    "xGk4Ly4l3eTynwDRBCs23gzO3262bn1RRFpbpPWczgLy5rFQB3ZTbNyPFSBPzgEu\r\n"
    "7+UqQHpofDgGy/SkVXzyJwKBgQDbc4+IMG0Ew5s/qtL+BATqX89ke9WKkxf/GdHT\r\n"
    "AGXpm6VrxlCI5DBYabC9SihySzrbw6I9tkEueBWLdjvuCAdp2V72sLfoYyAefCXe\r\n"
    "YuaVPnwyd2cVWsHMwO3i2bidqcFRb8fdr5diKRilrH6SCMVuAYbpUfE14d925lWe\r\n"
    "xNcYSQKBgQClCllJwGMQPwnuJQPT37UZ+vfWrOt1Gmj7PyDzPLFOZ9lusDxQfbeN\r\n"
    "6wHnCHo5GmSeABVyhA+zQnYMOeDvb/wX0EQpsH+7BHkmoN4vHmenbrCwEuJEr9TY\r\n"
    "Yl4hXTVhNu2r2zbaqSriEuKzcQOuwf6VVIiRDPHm2fwqUtq19bTpdQ==\r\n"
    "-----END RSA PRIVATE KEY-----\r\n";

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
