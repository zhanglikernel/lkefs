#include <linux/module.h>
#include <crypto/internal/hash.h>
#include <linux/string.h>

struct sdesc{
    struct shash_desc shash;
    char ctx[];
};

static struct sdesc *init_sdesc(struct crypto_shash *alg){
    struct sdesc *sdesc;
    int size;

    size = sizeof(struct shash_desc) + crypto_shash_descsize(alg);
    sdesc = kmalloc(size, GFP_KERNEL);
    if(!sdesc){
        return ERR_PTR(-ENOMEM);
    }
    sdesc->shash.tfm = alg;
    return sdesc;
}

static int calc_hash(struct crypto_shash *alg, const unsigned char *data, unsigned int datalen, unsigned char* digest){
    struct sdesc *sdesc;
    int ret;

    sdesc = init_sdesc(alg);
    if(IS_ERR(sdesc)){
        return PTR_ERR(sdesc);
    }
    ret = crypto_shash_digest(&sdesc->shash, data, datalen, digest);
    kfree(sdesc);
    return ret;
}

static int __init lke_hash_test_init(void){
    struct crypto_shash *alg = NULL;
    char *hash_alg_name = "md4";
    int ret;
    int result = 0;
    char *data = "fuck you";
    unsigned int datalen = strlen(data);
    char digest[128];
    int i;
    char outBuff[1024];

    alg = crypto_alloc_shash(hash_alg_name, 0, 0);
    if(IS_ERR(alg)){
        printk("crypto_alloc_shash error.\n");
        result = -1;
        goto out;
    }
    memset(digest, 0, sizeof(digest));
    memset(outBuff, 0, sizeof(outBuff));
    ret = calc_hash(alg, data, datalen, digest);
    printk("digest.\n");
    printk("%s.\n", digest);
    sprintf(outBuff + strlen(outBuff), "0x");
    for(i = 0; i < strlen(digest); i ++){
        sprintf(outBuff + strlen(outBuff), "%x", digest[i] & 0xff);
    }
    printk("%s.", outBuff);
    printk("end.");
    crypto_free_shash(alg);
    result = 0;
out:
    return result;
}

static void __exit lke_hash_test_exit(void){

}

module_init(lke_hash_test_init);
module_exit(lke_hash_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LiZhang");
