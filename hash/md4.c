#include <linux/module.h>
#include <crypto/internal/hash.h>
#include <linux/kernel.h>
#include <linux/string.h>

#define LKE_MD4_DIGEST_SIZE 16
#define LKE_MD4_HMAC_BLOCK_SIZE 64
#define LKE_MD4_BLOCK_WORDS 16
#define LKE_MD4_HASH_WORDS  4

struct lke_md4_ctx {
    u32 hash[LKE_MD4_HASH_WORDS];
    u32 block[LKE_MD4_BLOCK_WORDS];
    u64 byte_count;
};


static inline u32 lshift(u32 x, unsigned int s){
    return ( x << s ) | (x >> (32 - s));
}

static inline u32 F(u32 x, u32 y, u32 z){
    return  (x & y) | ((~x) & z);
}

static inline u32 G(u32 x, u32 y, u32 z){
    return ( x & y ) | ( x & z ) | (y & z );
}

static inline u32 H(u32 x, u32 y, u32 z){
    return x ^ y ^ z;
}

#define ROUND1(a,b,c,d,k,s) (a = lshift(a + F(b,c,d) + k, s))
#define ROUND2(a,b,c,d,k,s) (a = lshift(a + G(b,c,d) + k + (u32)0x5A827999, s))
#define ROUND3(a,b,c,d,k,s) (a = lshift(a + H(b,c,d) + k + (u32)0x6ED9EBA1, s))

static void lke_md4_transform(u32 *hash, u32 const *in){
    u32 a, b, c, d;

    a = hash[0];
    b = hash[1];
    c = hash[2];
    d = hash[3];

    ROUND1(a, b, c, d, in[0], 3);
    ROUND1(d, a, b, c, in[1], 7);
    ROUND1(c, d, a, b, in[2], 11);
    ROUND1(b, c, d, a, in[3], 19);
    ROUND1(a, b, c, d, in[4], 3);
    ROUND1(d, a, b, c, in[5], 7);
    ROUND1(c, d, a, b, in[6], 11);
    ROUND1(b, c, d, a, in[7], 19);
    ROUND1(a, b, c, d, in[8], 3);
    ROUND1(d, a, b, c, in[9], 7);
    ROUND1(c, d, a, b, in[10], 11);
    ROUND1(b, c, d, a, in[11], 19);
    ROUND1(a, b, c, d, in[12], 3);
    ROUND1(d, a, b, c, in[13], 7);
    ROUND1(c, d, a, b, in[14], 11);
    ROUND1(b, c, d, a, in[15], 19);

    ROUND2(a, b, c, d,in[ 0], 3);
    ROUND2(d, a, b, c, in[4], 5);
    ROUND2(c, d, a, b, in[8], 9);
    ROUND2(b, c, d, a, in[12], 13);
    ROUND2(a, b, c, d, in[1], 3);
    ROUND2(d, a, b, c, in[5], 5);
    ROUND2(c, d, a, b, in[9], 9);
    ROUND2(b, c, d, a, in[13], 13);
    ROUND2(a, b, c, d, in[2], 3);
    ROUND2(d, a, b, c, in[6], 5);
    ROUND2(c, d, a, b, in[10], 9);
    ROUND2(b, c, d, a, in[14], 13);
    ROUND2(a, b, c, d, in[3], 3);
    ROUND2(d, a, b, c, in[7], 5);
    ROUND2(c, d, a, b, in[11], 9);
    ROUND2(b, c, d, a, in[15], 13);

    ROUND3(a, b, c, d,in[ 0], 3);
    ROUND3(d, a, b, c, in[8], 9);
    ROUND3(c, d, a, b, in[4], 11);
    ROUND3(b, c, d, a, in[12], 15);
    ROUND3(a, b, c, d, in[2], 3);
    ROUND3(d, a, b, c, in[10], 9);
    ROUND3(c, d, a, b, in[6], 11);
    ROUND3(b, c, d, a, in[14], 15);
    ROUND3(a, b, c, d, in[1], 3);
    ROUND3(d, a, b, c, in[9], 9);
    ROUND3(c, d, a, b, in[5], 11);
    ROUND3(b, c, d, a, in[13], 15);
    ROUND3(a, b, c, d, in[3], 3);
    ROUND3(d, a, b, c, in[11], 9);
    ROUND3(c, d, a, b, in[7], 11);
    ROUND3(b, c, d, a, in[15], 15);

    hash[0] += a;
    hash[1] += b;
    hash[2] += c;
    hash[3] += d;
}

static inline void lke_md4_transform_helper(struct lke_md4_ctx* ctx){
    le32_to_cpu_array(ctx->block, ARRAY_SIZE(ctx->block));
    lke_md4_transform(ctx->hash, ctx->block);
}

static int lke_md4_init(struct shash_desc *desc){
    struct lke_md4_ctx *lke_md4_ctx_p= shash_desc_ctx(desc);

    lke_md4_ctx_p -> hash[0] = 0x67452301;
    lke_md4_ctx_p -> hash[1] = 0xefcdab89;
    lke_md4_ctx_p -> hash[2] = 0x98badcfe;
    lke_md4_ctx_p -> hash[3] = 0x10325476;
    lke_md4_ctx_p -> byte_count = 0;
    return 0;
}

static int lke_md4_update(struct shash_desc* desc, const u8 *data, unsigned int len){
    int result;
    struct lke_md4_ctx *lke_md4_ctx_p = shash_desc_ctx(desc);
    unsigned int avail_len = sizeof(lke_md4_ctx_p->block) - (lke_md4_ctx_p->byte_count & 0x3F);
    unsigned int offset = lke_md4_ctx_p->byte_count & 0x3F;
    lke_md4_ctx_p -> byte_count += len;
    if( avail_len > len ){
        memcpy((char *)lke_md4_ctx_p->block + offset, data, len);
        result = 0;
        goto out;
    }
    memcpy((char *)lke_md4_ctx_p->block + offset, data, avail_len);
    lke_md4_transform_helper(lke_md4_ctx_p);
    len -= avail_len;
    data += avail_len;
    while(len >= sizeof(lke_md4_ctx_p->block)){
        memcpy(lke_md4_ctx_p->block, data, sizeof(lke_md4_ctx_p->block));
        lke_md4_transform_helper(lke_md4_ctx_p);
        len -= sizeof(lke_md4_ctx_p -> block);
        data += sizeof(lke_md4_ctx_p -> block);
    }
    memcpy(lke_md4_ctx_p->block, data, len);
    result = 0;
out:
    return result;
}

static int lke_md4_final(struct shash_desc *desc, u8 *out){
    int result = 0;
    struct lke_md4_ctx *lke_md4_ctx_p = shash_desc_ctx(desc);
    unsigned int avail_len = sizeof(lke_md4_ctx_p->block) - (lke_md4_ctx_p->byte_count & 0x3F);
    unsigned int offset = lke_md4_ctx_p->byte_count & 0x3F;
    char* p = (char *)lke_md4_ctx_p -> block + offset;
    unsigned int padding;
    *p++ = 0x80;
    offset += 1;
    avail_len -= 1;
    if(avail_len < 8 ){
        memset(p, 0, avail_len);
        lke_md4_transform_helper(lke_md4_ctx_p);
        padding = 56;
        p = (char *)lke_md4_ctx_p -> block;
    }else{
        padding = avail_len - 8;
    }
    memset(p, 0, padding);
    p += padding;
    lke_md4_ctx_p -> block[14] = lke_md4_ctx_p -> byte_count << 3;
    lke_md4_ctx_p -> block[15] = lke_md4_ctx_p -> byte_count >> 29;
    le32_to_cpu_array(lke_md4_ctx_p->block, ((sizeof(lke_md4_ctx_p -> block) - 2 * sizeof(u32))) / sizeof(u32));
    lke_md4_transform(lke_md4_ctx_p->hash, lke_md4_ctx_p->block);
    cpu_to_le32_array(lke_md4_ctx_p->hash, ARRAY_SIZE(lke_md4_ctx_p->hash));
    memcpy(out, lke_md4_ctx_p->hash, sizeof(lke_md4_ctx_p->hash));
    // memset(out, 0, sizeof(lke_md4_ctx_p->hash));
    memset(lke_md4_ctx_p, 0, sizeof(*lke_md4_ctx_p));
    return result;
}

static struct shash_alg lke_md4 = {
    .digestsize = LKE_MD4_DIGEST_SIZE,
    .descsize = sizeof(struct lke_md4_ctx),
    .init = lke_md4_init,
    .update = lke_md4_update,
    .final = lke_md4_final,
    .base   = {
        .cra_name = "lke_md4",
        .cra_driver_name = "lke_md4-generic",
        .cra_blocksize = LKE_MD4_HMAC_BLOCK_SIZE,
        .cra_module = THIS_MODULE,
    }
};

static int __init lke_md4_module_init(void){
    return crypto_register_shash(&lke_md4);
}

static void __exit lke_md4_module_exit(void){
    crypto_unregister_shash(&lke_md4);
}

subsys_initcall(lke_md4_module_init);
module_exit(lke_md4_module_exit);

MODULE_AUTHOR("LiZhang");
MODULE_DESCRIPTION("lke md4 algorithm");
MODULE_LICENSE("GPL");
