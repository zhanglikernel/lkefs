#include <linux/string.h>
#include <crypto/internal/hash.h>

struct sdesc{
    struct shash_desc shash;
    char ctx[];
};

static struct sdesc *init_desc(struct crypto_shash *alg){
    struct sdesc *sdesc;
    int size;

    size = sizeof(struct sdesc) + crypto_shash_descsize(alg);
    sdesc = kmalloc(size, GFP_KERNEL);
    if(!sdesc){
        return ERR_PTR(-ENOMEM);
    }
    sdesc -> shash.tfm = alg;
    return sdesc;
}

static int calc_hash(struct crypto_shash *alg, const unsigned char *data, unsigned int datalen, unsigned char* digest){
    struct sdesc *sdesc;
    int ret;

    sdesc = init_desc(alg);
    if(IS_ERR(sdesc)){
        return PTR_ERR(sdesc);
    }
    ret = crypto_shash_digest(&sdesc->shash, data, datalen, digest);
    kfree(sdesc);
    return ret;
}

int namei(char *pathName, int pathLen, char *outBuff, int outBuffLen){
    int result = -1;
    struct crypto_shash *alg = NULL;
    char *hash_alg_md4 = "lke_md4";
    char *digest;
    int digestLen = 128;
    alg = crypto_alloc_shash(hash_alg_md4, 0, 0);
    if(IS_ERR(alg)){
        result = PTR_ERR(alg);
        goto err;
    }
    digest = kmalloc(digestLen, GFP_KERNEL);
    if(!digest){
        result = PTR_ERR(digest);
        goto digest_release;
    }
    result = calc_hash(alg, pathName, pathLen, digest);
    memset(outBuff, 0, outBuffLen);
    memcpy(outBuff, digest, outBuffLen);
    kfree(digest);
    result = 0;
    return result;
digest_release:
    crypto_free_shash(alg);
err:
    return result;
}
