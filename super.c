/*
 *  filesystem test for select, poll, epoll, btree data-struct, rcu, etc
 * 
 */

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/dcache.h>

#include "common.h"

#include "lke_fs.h"
#include "super.h"
#include "inode.h"
#include "lke_btree.h"
#include "dir.h"


struct kmem_cache *lke_fs_inode_cachep;

static void init_once(void* foo){
    struct lke_fs_inode * lke_inode = foo;
    inode_init_once(LKE_INODE_V_I(lke_inode));
}

static int lke_object_kmem_cache_create(void){
    int result;
    lke_fs_inode_cachep = NULL;
    lke_fs_inode_cachep = kmem_cache_create("lke_fs_inode", sizeof(struct lke_fs_inode), 0, SLAB_HWCACHE_ALIGN, init_once);
    PDEBUG("lke_object_kmem_cache_create:%p.", lke_fs_inode_cachep);
    if(!lke_fs_inode_cachep){
        result = -ENOMEM;
        goto errout;
    }
    result = 0;

    return result;
errout:
    return result;
    
}

static void lke_object_kmem_cache_destroy(void){
    if(NULL != lke_fs_inode_cachep){
        kmem_cache_destroy(lke_fs_inode_cachep);
    }
}


static void lke_fs_put_super(struct super_block* sb){
    struct lke_fs_super_block *sbi = NULL;
    PDEBUG("lke_put_super.");
    if(NULL == sb){
        goto out;
    }
    sbi = VFS_SB_FS_INFO(sb);
    if(NULL != sbi){
        kfree(sbi);
    }
out:
    return;
}

int lke_fs_statfs(struct dentry* dentry, struct kstatfs *buf){
    PDEBUG("lke_fs_statfs");
    return simple_statfs(dentry, buf);
}


static const struct super_operations lke_fs_super_operations = {
    .statfs = lke_fs_statfs,
    .alloc_inode = lke_fs_alloc_inode,
    .free_inode = lke_fs_destroy_inode,
    .put_super = lke_fs_put_super,
};

static int lke_fs_fill_super(struct super_block* sb, void *data, int silent){
    int result = 0;
    struct lke_fs_super_block *sbi = NULL;
    struct inode* root_inode;
    sbi = kzalloc(sizeof(struct lke_fs_super_block), ZONE_NORMAL);
    if(unlikely(sbi == NULL)){
        result = -ENOMEM;
        goto out;
    }
    sb -> s_magic = LKE_FS_MAGIC;
    VFS_SB_FS_INFO(sb) = sbi;
    LKE_FS_SB_VFS_SB(sbi) = sb;

    strcpy(sbi -> sbi_filesystem_version, LKE_FS_MAGIC_VERSOIN);
    sbi -> sbi_inode_count = 0;
    sb -> s_op = &lke_fs_super_operations;
    sb -> s_magic = LKE_FS_MAGIC;
    root_inode = lke_fs_new_inode(sb, NULL, NULL, S_IFDIR | DEFAULT_DIR_MODE);
    if(IS_ERR(root_inode)){
        PDEBUG("vfs_node is null.");
        result = -ENOMEM;
        goto lke_sbi_release;
    } else {
        PDEBUG("vfs_node address: %p.\n", root_inode);
    }
    sbi -> sbi_inode_count ++;
    sb -> s_root = d_make_root(root_inode);
    if(!sb->s_root){
        result = -ENOMEM;
        goto lke_sbi_release;
    }
    result = 0;
out:
    return result;
lke_sbi_release:
    kfree(sbi);
    return result;
}


static struct dentry* lke_fs_mount(struct file_system_type *fs_type, int flags, const char* dev_name, void *data){
    return mount_nodev(fs_type, flags, data, lke_fs_fill_super);
}


static void lke_kill_super(struct super_block *sb){
    kill_litter_super(sb);
}

static struct file_system_type lke_fs_type = {
    .owner = THIS_MODULE,
    .name = "lkefs",
    .mount = lke_fs_mount,
    .kill_sb = lke_kill_super,
};

static int __init lke_fs_init(void){
    int rtval;
    int result;
    PDEBUG("lke_fs_init\n");
    rtval = register_filesystem(&lke_fs_type);
    if( rtval < 0 ){
        PERROR("lke_fs init failed\n");
        PERROR("register_filesystem return:%d\n", rtval);
        result = rtval;
        goto err_ret;
    }
    rtval = lke_object_kmem_cache_create();
    if(rtval < 0){
        result = rtval;
        goto lke_object_kmem_release;
    }
    result = 0;

    return result;

lke_object_kmem_release:
    lke_object_kmem_cache_destroy();
    unregister_filesystem(&lke_fs_type);
err_ret:
    return result;
}

static void __exit lke_fs_exit(void){
    int rtval;
    PDEBUG("lke_fs_exit\n");
    rtval = unregister_filesystem(&lke_fs_type);
    if(likely( 0 == rtval )){
        PDEBUG("unregister lke_fs_type succeed.\n");
    }else{
        PERROR("unregister lke_fs_type fail with %d.\n", rtval );
        goto out;
    }
    lke_object_kmem_cache_destroy();
out:
    return;
}

module_init(lke_fs_init);
module_exit(lke_fs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LiZhang");
