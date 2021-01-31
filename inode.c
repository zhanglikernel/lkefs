#include <linux/fs.h>
#include <linux/err.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/pagemap.h>
#include <linux/iversion.h>
#include <linux/sched.h>

#include "common.h"
#include "inode.h"
#include "super.h"
#include "dir.h"
#include "file.h"
#include "namei.h"


static void destroy_inode(struct lke_fs_inode* lke_fs_inode){
    kmem_cache_free(lke_fs_inode_cachep, lke_fs_inode);
}

struct inode *lke_fs_alloc_inode(struct super_block *sb){
    struct lke_fs_inode* lke_inode = NULL;
    struct inode *inode = NULL;
    PDEBUG("lke_fs_alloc_inode:%p", lke_fs_inode_cachep);
    lke_inode = kmem_cache_alloc(lke_fs_inode_cachep, GFP_KERNEL);
    if(!lke_inode){
        inode = NULL;
        goto err_ret;
    }
    inode = LKE_INODE_V_I(lke_inode);
    inode_set_iversion(inode, 1);
    memset(lke_inode -> name, 0, LKE_MAX_PATH_NAME);
    PDEBUG("lke_fs_alloc_inode done.\n");
    return inode;

    kmem_cache_free(lke_fs_inode_cachep, lke_inode);
err_ret:
    return inode;
}

void lke_fs_destroy_inode(struct inode *inode){
    struct lke_fs_inode* lke_fs_inode;
    if(!inode){
        goto out;
    }
    lke_fs_inode = LKE_FS_I(inode);
    if(!lke_fs_inode){
        goto out;
    }
    if(lke_fs_inode -> data){
        kfree(lke_fs_inode -> data);
        lke_fs_inode -> data = NULL;
    }
    destroy_inode(lke_fs_inode);
out:
    return;
}


int lke_mkdir(struct inode * dir, struct dentry * dentry, umode_t mode){
    int result = -ENOMEM;
    struct inode *inode = NULL;
    struct lke_fs_inode *lke_fs_inode = NULL;
    struct lke_dir_entity* lke_parent_dir_entity = NULL;
    
    inode = lke_fs_new_inode(dir -> i_sb, dir, dentry,  mode | S_IFDIR);
    if(IS_ERR(inode)){
        result = PTR_ERR(inode);
        goto out;
    }
    lke_fs_inode = LKE_FS_I(inode);
    PDEBUG("lke_mkdir name:%s", (dentry ->d_name).name);
    strcpy(lke_fs_inode->name, (dentry->d_name).name);
    lke_parent_dir_entity = LKE_FS_I(dir) -> data;
    PDEBUG("lke_dentry_inode:%p", lke_parent_dir_entity);
    lke_parent_dir_entity -> fileNum ++;
    inc_nlink(dir);
    d_instantiate(dentry, inode);
    dget(dentry);
    dir -> i_mtime = dir -> i_ctime = current_time(dir);
    result = 0;
out:
    return result;
}

int lke_rmdir(struct inode * dir, struct dentry * dentry){
    int result = -ENOTEMPTY;
    u64 inode_key;
    struct super_block *sb = dir -> i_sb;
    struct lke_fs_super_block *sbi = VFS_SB_FS_INFO(sb);
    char *pathName;
    int pathLen = LKE_MAX_PATH_NAME;
    struct lke_fs_inode *lke_parent_inode = LKE_FS_I(dir);
    struct inode *inode = d_inode(dentry);
    struct lke_fs_inode *lke_fs_inode = LKE_FS_I(inode);
    struct lke_dir_entity *lke_parent_dir_entity = LKE_FS_INODE_DATA(lke_parent_inode);
    struct lke_dir_entity *lke_dir_entity = LKE_FS_INODE_DATA(lke_fs_inode);

    if(lke_parent_dir_entity -> fileNum != 0){
        pathName = kzalloc(pathLen, ZONE_NORMAL);
        if(!pathName){
            result = -ENOMEM;
            goto out;
        }
        memset(pathName, 0, pathLen);
        strcpy(pathName, lke_fs_inode->name);
        namei(pathName, strlen(pathName), (char *)&inode_key, sizeof(inode_key));
        PDEBUG("rmdir: %s %llx %ld.", pathName, inode_key, inode -> i_ino);
        lke_btree_remove(LKE_FS_DENTRY_INODE_BTREE_HEAD(lke_parent_dir_entity), inode_key);
        lke_parent_dir_entity -> fileNum --;
        kfree(pathName);
        lke_btree_destroy(LKE_FS_DENTRY_INODE_BTREE_HEAD(lke_dir_entity));
        kfree(lke_dir_entity);
        inode_dec_link_count(inode);
        inode_dec_link_count(inode);
        inode_dec_link_count(dir);
        sbi -> sbi_inode_count --;
    }
    result = 0;
out:
    return result;
}

static int lke_create(struct inode* dir, struct dentry * dentry, umode_t mode, bool exel){
    int result = -ENOMEM;
    struct inode* inode = NULL;
    struct lke_fs_inode *lke_fs_parent_inode = NULL;
    struct lke_fs_inode *lke_fs_inode = NULL;
    struct lke_dir_entity* lke_parent_entity = NULL;
    inode = lke_fs_new_inode( dir->i_sb, dir, dentry, mode | S_IFREG );
    if(IS_ERR(inode)){
        result = PTR_ERR(inode);
        goto out;
    }
    lke_fs_parent_inode = LKE_FS_I(dir);
    lke_fs_inode = LKE_FS_I(inode);
    strcpy(lke_fs_inode->name, (dentry->d_name).name);
    lke_parent_entity = LKE_FS_INODE_DATA(lke_fs_parent_inode);
    lke_parent_entity -> fileNum ++;
    inc_nlink(dir);
    d_instantiate(dentry, inode);
    dget(dentry);
    dir -> i_mtime = dir -> i_ctime = current_time(dir);
    result = 0;
out:
    return result;
}

static int lke_unlink(struct inode* dir, struct dentry* dentry){
    int result;
    struct inode* inode = NULL;
    struct lke_fs_inode *lke_fs_parent_inode = NULL;
    struct lke_fs_inode *lke_fs_inode = NULL;
    struct lke_dir_entity* lke_parent_dir_entity = NULL;
    struct lke_file_entity *lke_file_entity = NULL;
    struct super_block *sb = dir -> i_sb;
    struct lke_fs_super_block *sbi = VFS_SB_FS_INFO(sb);
    char *pathName;
    u64 inode_key;
 
    pathName = kzalloc(LKE_MAX_PATH_NAME, ZONE_NORMAL);
    if(!pathName){
        result = -ENOMEM;
        goto out;
    }
    lke_fs_parent_inode = LKE_FS_I(dir);
    lke_parent_dir_entity = LKE_FS_INODE_DATA(lke_fs_parent_inode);
    inode = d_inode(dentry);
    lke_fs_inode = LKE_FS_I(inode);
    lke_file_entity = LKE_FS_INODE_DATA(lke_fs_inode);
    if(lke_parent_dir_entity -> fileNum != 0){
        memset(pathName, 0, LKE_MAX_PATH_NAME);
        strcpy(pathName, lke_fs_inode->name);
        namei(pathName, strlen(pathName), (char *)&inode_key, sizeof(inode_key));
        lke_btree_remove(LKE_FS_DENTRY_INODE_BTREE_HEAD(lke_parent_dir_entity), inode_key);
        lke_parent_dir_entity -> fileNum --;
        kfree(lke_file_entity);
        inode_dec_link_count(inode);
        sbi -> sbi_inode_count --;
    }
    kfree(pathName);
    result = 0;
out:
    return result;
}


static struct dentry* lke_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags){
    struct dentry * result = ERR_PTR(-ENOMEM);
    char *pathName;
    int pathLen = LKE_MAX_PATH_NAME;
    struct lke_fs_inode *lke_fs_inode = NULL;
    u64 inode_key;
    struct inode *target_inode;
    struct lke_dir_entity *lke_parent_entity = NULL;
    lke_fs_inode = LKE_FS_I(dir);
    lke_parent_entity = LKE_FS_INODE_DATA(lke_fs_inode);
    pathName = kzalloc(pathLen, ZONE_NORMAL);
    if(!pathName){
        result = ERR_PTR(-ENOMEM);
        goto err_out;
    }
    if(!lke_fs_inode){
        result = ERR_PTR(-ENOENT);
        goto pathname_free;
    }
    memset(pathName, 0, pathLen);
    strcat(pathName, (dentry->d_name).name);
    namei(pathName, strlen(pathName), (char *)&inode_key, sizeof(inode_key));
    target_inode = lke_btree_lookup(LKE_FS_DENTRY_INODE_BTREE_HEAD(lke_parent_entity), inode_key);
    PDEBUG("lookup pathName:%s %llx" , pathName, inode_key);
    if(!target_inode){
        target_inode = NULL;
        goto out;
    }
    PDEBUG("lookup ino:%ld", target_inode->i_ino);
out:
    kfree(pathName);
    result = d_splice_alias(target_inode, dentry);
    PDEBUG("lke_lookup result:%p %p", result, NULL);
    return result;

pathname_free:
    kfree(pathName);
err_out:
    return result;
}

struct inode_operations lke_fs_dir_inode_ops = {
    .create = lke_create,
    .lookup = lke_lookup,
    .mkdir = lke_mkdir,
    .unlink = lke_unlink,
    // .rename = lke_rename,
    .rmdir = lke_rmdir,
    // .mknod = lke_mknod,
};

struct inode_operations lke_fs_file_inode_ops = {
    .setattr = NULL,
    .getattr = NULL,
};


/* 
 * new inode
 */
struct inode* lke_fs_new_inode(struct super_block *sb, struct inode* dir_inode, struct dentry *dentry, umode_t mode){
    struct inode* inode = NULL; 
    struct lke_fs_super_block *sbi = NULL;
    char *pathName;
    int pathLen = LKE_MAX_PATH_NAME;
    u64 inode_key;
    struct lke_fs_inode *parent_dir_lke_inode = NULL;
    struct lke_dir_entity *parent_dir_lke_entity = NULL;
    struct lke_fs_inode *lke_file_inode = NULL;
    struct lke_dir_entity *lke_dir_entity = NULL; 
    struct lke_file_entity *lke_file_entity = NULL;
    void *data = NULL;
    int result;
    inode = new_inode(sb);
    if( !inode ){
        goto err_out;
    }
    pathName = kzalloc(pathLen, ZONE_NORMAL);
    if( !pathName ){
        drop_nlink(inode);
        inode = ERR_PTR(-ENOMEM);
        goto err_out;
    }
    if( !dir_inode ){
        strcpy(pathName, "/");
    } else {
        strcpy(pathName, (dentry->d_name).name);
    }
    sbi = VFS_SB_FS_INFO(sb);
    namei(pathName, strlen(pathName), (char *)&inode_key, sizeof(inode_key));
    if(NULL != dir_inode){
        parent_dir_lke_inode = LKE_FS_I(dir_inode);
        parent_dir_lke_entity = LKE_FS_INODE_DATA(parent_dir_lke_inode);
        if(lke_btree_lookup(LKE_FS_DENTRY_INODE_BTREE_HEAD(parent_dir_lke_entity), inode_key)){
            drop_nlink(inode);
            inode = ERR_PTR(-EEXIST);
            goto pathName_free;
        }
    }
    lke_file_inode = LKE_FS_I(inode);
    inode -> i_ino = get_next_ino();
    PDEBUG("lke_fs_new_inode %s %ld", pathName, inode -> i_ino);
    inode -> i_atime = inode -> i_mtime = inode -> i_ctime = current_time(inode);
    inode_init_owner(inode, dir_inode, mode);
    switch(mode & S_IFMT){
    case S_IFDIR:
        data = kzalloc(sizeof(struct lke_dir_entity), ZONE_NORMAL);
        if(!data){
            drop_nlink(inode);
            goto pathName_free;
        }
        lke_dir_entity = data;
        LKE_FS_INODE_DATA(lke_file_inode) = lke_dir_entity;
        LKE_FS_DENTRY_INODE_BTREE_HEAD(lke_dir_entity) = lke_btree_head_alloc();
        if(NULL == LKE_FS_DENTRY_INODE_BTREE_HEAD(lke_dir_entity)){
            goto data_release;
        }
        if(0 != (result = (lke_btree_init(LKE_FS_DENTRY_INODE_BTREE_HEAD(lke_dir_entity))))){
            inode = ERR_PTR(result);
            lke_btree_destroy(LKE_FS_DENTRY_INODE_BTREE_HEAD(lke_dir_entity));
            drop_nlink(inode);
            goto data_release;
        }
        
        inode -> i_op = &lke_fs_dir_inode_ops;
        inode -> i_fop = &lke_fs_dir_op;
        mutex_init(&lke_dir_entity->lock);
        inc_nlink(inode);
        break;
    case S_IFREG:
        data = kzalloc(sizeof(struct lke_file_entity), ZONE_NORMAL);
        if(!data){
            drop_nlink(inode);
            goto pathName_free;
        }
        LKE_FS_INODE_DATA(lke_file_inode) = data;
        lke_file_entity = LKE_FS_FILE_DATA(lke_file_inode);
        mutex_init(&lke_file_entity->lock);
        init_waitqueue_head(&(lke_file_entity->outq));
        lke_file_entity -> ready = 0;
        inode -> i_op = &lke_fs_file_inode_ops;
        inode -> i_fop = &lke_fs_file_ops;
        break;
    default:
        break;
    }
    if(NULL != dir_inode){
        PDEBUG("lke_btree_insert:%lld %s.", inode_key, pathName);
        lke_btree_insert(LKE_FS_DENTRY_INODE_BTREE_HEAD(parent_dir_lke_entity), inode_key, inode);
    }
    sbi -> sbi_inode_count ++;
    kfree(pathName);
    return inode;

data_release:
    kfree(data);
pathName_free:
    kfree(pathName);
err_out:
    return inode;
}

struct inode* lke_fs_get_inode(struct super_block *sb, struct inode* dir_inode, char filename[256]){
        return NULL;
}

void lke_fs_remove_inode(struct super_block *sb, struct inode* dir_inode, char filename[256]){
    return;
}
