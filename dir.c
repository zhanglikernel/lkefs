#include <linux/fs.h>
#include <linux/string.h>

#include "common.h"
#include "lke_fs.h"
#include "inode.h"
#include "super.h"
#include "dir.h"
#include "namei.h"

static void travesal_readdir(void *value, void *dataStruct){
    struct dir_context *ctx = dataStruct;
    struct inode *inode = value;
    struct lke_fs_inode *lke_fs_inode = LKE_FS_I(inode);
    PDEBUG("travesal:%s", lke_fs_inode->name);
    if(!dir_emit(ctx, lke_fs_inode->name, strlen(lke_fs_inode->name), inode -> i_ino, (inode->i_mode >> 12) & 15)){
        goto out;
    }
    ctx->pos++;
out:
    return;
}

static int lke_fs_readdir(struct file *file, struct dir_context *ctx){
    int result = 0;
    struct inode *inode = file_inode(file);
    struct lke_fs_inode *lke_fs_inode = LKE_FS_I(inode);
    struct lke_dir_entity *lke_dir_entity = LKE_FS_INODE_DATA(lke_fs_inode);
    PDEBUG("lke_fs_readdir:%s %ld" , lke_fs_inode -> name, inode->i_ino);
    if(ctx -> pos > 0){
        goto out;
    }
    if(!dir_emit_dots(file, ctx)){
        goto out;
    }
    if(LKE_FS_DENTRY_INODE_BTREE_HEAD(lke_dir_entity) == NULL){
        PDEBUG("zhanglitest lke_dir_entity.");
    }
    lke_btree_value_traversal(LKE_FS_DENTRY_INODE_BTREE_HEAD(lke_dir_entity), travesal_readdir, ctx);
out:
    return result;
}

static int lke_dir_open(struct inode* dir, struct file *file){
    PDEBUG("lke_dir_open");
    return 0;
}

static int lke_dir_release(struct inode *inode, struct file* file){
    return 0;
}


struct file_operations lke_fs_dir_op = {
    .open		= lke_dir_open,
	.release	= lke_dir_release,
    .llseek = generic_file_llseek,
    .read = generic_read_dir,
    .iterate_shared = lke_fs_readdir,
    .fsync = noop_fsync,
};
