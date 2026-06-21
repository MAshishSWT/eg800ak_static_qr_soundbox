/*================================================================
 * Static QR UPI Soundbox - OTA SHA-256 / HMAC Helpers
 * Target: Quectel EG800AK-CN QuecOpen SDK
 *================================================================*/
#include "sb_ota_crypto.h"

#define SB_SHA256_ROTLEFT(a,b)   (((a) << (b)) | ((a) >> (32u - (b))))
#define SB_SHA256_ROTRIGHT(a,b)  (((a) >> (b)) | ((a) << (32u - (b))))
#define SB_SHA256_CH(x,y,z)      (((x) & (y)) ^ (~(x) & (z)))
#define SB_SHA256_MAJ(x,y,z)     (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SB_SHA256_EP0(x)         (SB_SHA256_ROTRIGHT(x,2u) ^ SB_SHA256_ROTRIGHT(x,13u) ^ SB_SHA256_ROTRIGHT(x,22u))
#define SB_SHA256_EP1(x)         (SB_SHA256_ROTRIGHT(x,6u) ^ SB_SHA256_ROTRIGHT(x,11u) ^ SB_SHA256_ROTRIGHT(x,25u))
#define SB_SHA256_SIG0(x)        (SB_SHA256_ROTRIGHT(x,7u) ^ SB_SHA256_ROTRIGHT(x,18u) ^ ((x) >> 3u))
#define SB_SHA256_SIG1(x)        (SB_SHA256_ROTRIGHT(x,17u) ^ SB_SHA256_ROTRIGHT(x,19u) ^ ((x) >> 10u))

static const u32 s_sha256_k[64] = {
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
    0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
    0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
    0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
    0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
    0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
};

static void sb_ota_crypto_zero(void *ptr, u32 len)
{
    u32 i;
    u8 *p = (u8 *)ptr;

    if (p == 0) {
        return;
    }
    for (i = 0u; i < len; i++) {
        p[i] = 0u;
    }
}

static void sb_ota_sha256_transform(sb_ota_sha256_ctx_t *ctx, const u8 data[64])
{
    u32 a,b,c,d,e,f,g,h,i,j,t1,t2,m[64];

    for (i = 0u, j = 0u; i < 16u; ++i, j += 4u) {
        m[i] = ((u32)data[j] << 24u) | ((u32)data[j + 1u] << 16u) | ((u32)data[j + 2u] << 8u) | ((u32)data[j + 3u]);
    }
    for (; i < 64u; ++i) {
        m[i] = SB_SHA256_SIG1(m[i - 2u]) + m[i - 7u] + SB_SHA256_SIG0(m[i - 15u]) + m[i - 16u];
    }

    a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2]; d = ctx->state[3];
    e = ctx->state[4]; f = ctx->state[5]; g = ctx->state[6]; h = ctx->state[7];

    for (i = 0u; i < 64u; ++i) {
        t1 = h + SB_SHA256_EP1(e) + SB_SHA256_CH(e,f,g) + s_sha256_k[i] + m[i];
        t2 = SB_SHA256_EP0(a) + SB_SHA256_MAJ(a,b,c);
        h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }

    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

void sb_ota_sha256_init(sb_ota_sha256_ctx_t *ctx)
{
    if (ctx == 0) {
        return;
    }
    sb_ota_crypto_zero(ctx, (u32)sizeof(*ctx));
    ctx->state[0] = 0x6a09e667u; ctx->state[1] = 0xbb67ae85u; ctx->state[2] = 0x3c6ef372u; ctx->state[3] = 0xa54ff53au;
    ctx->state[4] = 0x510e527fu; ctx->state[5] = 0x9b05688cu; ctx->state[6] = 0x1f83d9abu; ctx->state[7] = 0x5be0cd19u;
}

sb_status_t sb_ota_sha256_update(sb_ota_sha256_ctx_t *ctx, const u8 *data, u32 len)
{
    u32 i;

    if ((ctx == 0) || ((data == 0) && (len > 0u))) {
        return SB_STATUS_INVALID_PARAM;
    }
    for (i = 0u; i < len; ++i) {
        ctx->data[ctx->data_len] = data[i];
        ctx->data_len++;
        if (ctx->data_len == 64u) {
            sb_ota_sha256_transform(ctx, ctx->data);
            ctx->bit_len += 512u;
            ctx->data_len = 0u;
        }
    }
    return SB_STATUS_OK;
}

sb_status_t sb_ota_sha256_finish(sb_ota_sha256_ctx_t *ctx, u8 hash[32])
{
    u32 i;
    u64 bit_len;

    if ((ctx == 0) || (hash == 0)) {
        return SB_STATUS_INVALID_PARAM;
    }

    i = ctx->data_len;
    if (ctx->data_len < 56u) {
        ctx->data[i++] = 0x80u;
        while (i < 56u) {
            ctx->data[i++] = 0x00u;
        }
    } else {
        ctx->data[i++] = 0x80u;
        while (i < 64u) {
            ctx->data[i++] = 0x00u;
        }
        sb_ota_sha256_transform(ctx, ctx->data);
        sb_ota_crypto_zero(ctx->data, 56u);
    }

    bit_len = ctx->bit_len + ((u64)ctx->data_len * 8u);
    ctx->data[63] = (u8)(bit_len);
    ctx->data[62] = (u8)(bit_len >> 8u);
    ctx->data[61] = (u8)(bit_len >> 16u);
    ctx->data[60] = (u8)(bit_len >> 24u);
    ctx->data[59] = (u8)(bit_len >> 32u);
    ctx->data[58] = (u8)(bit_len >> 40u);
    ctx->data[57] = (u8)(bit_len >> 48u);
    ctx->data[56] = (u8)(bit_len >> 56u);
    sb_ota_sha256_transform(ctx, ctx->data);

    for (i = 0u; i < 4u; ++i) {
        hash[i]      = (u8)((ctx->state[0] >> (24u - i * 8u)) & 0x000000ffu);
        hash[i + 4u]  = (u8)((ctx->state[1] >> (24u - i * 8u)) & 0x000000ffu);
        hash[i + 8u]  = (u8)((ctx->state[2] >> (24u - i * 8u)) & 0x000000ffu);
        hash[i + 12u] = (u8)((ctx->state[3] >> (24u - i * 8u)) & 0x000000ffu);
        hash[i + 16u] = (u8)((ctx->state[4] >> (24u - i * 8u)) & 0x000000ffu);
        hash[i + 20u] = (u8)((ctx->state[5] >> (24u - i * 8u)) & 0x000000ffu);
        hash[i + 24u] = (u8)((ctx->state[6] >> (24u - i * 8u)) & 0x000000ffu);
        hash[i + 28u] = (u8)((ctx->state[7] >> (24u - i * 8u)) & 0x000000ffu);
    }
    return SB_STATUS_OK;
}

sb_status_t sb_ota_hmac_sha256(const u8 *key, u32 key_len, const u8 *data, u32 data_len, u8 out[32])
{
    u8 ipad[64];
    u8 opad[64];
    u8 inner[32];
    sb_ota_sha256_ctx_t ctx;
    u32 i;

    if ((key == 0) || (data == 0) || (out == 0) || (key_len == 0u) || (key_len > 64u)) {
        return SB_STATUS_INVALID_PARAM;
    }
    for (i = 0u; i < 64u; i++) {
        u8 k = (i < key_len) ? key[i] : 0u;
        ipad[i] = (u8)(k ^ 0x36u);
        opad[i] = (u8)(k ^ 0x5cu);
    }
    sb_ota_sha256_init(&ctx);
    (void)sb_ota_sha256_update(&ctx, ipad, 64u);
    (void)sb_ota_sha256_update(&ctx, data, data_len);
    (void)sb_ota_sha256_finish(&ctx, inner);

    sb_ota_sha256_init(&ctx);
    (void)sb_ota_sha256_update(&ctx, opad, 64u);
    (void)sb_ota_sha256_update(&ctx, inner, 32u);
    return sb_ota_sha256_finish(&ctx, out);
}

int sb_ota_digest_equal(const u8 *a, const u8 *b, u32 len)
{
    u32 i;
    u8 diff = 0u;

    if ((a == 0) || (b == 0)) {
        return 0;
    }
    for (i = 0u; i < len; i++) {
        diff = (u8)(diff | (u8)(a[i] ^ b[i]));
    }
    return (diff == 0u) ? 1 : 0;
}
