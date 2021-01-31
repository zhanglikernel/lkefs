#ifndef _INODE_H_
#define _INODE_H_

#include <linux/types.h>
#include <linux/fs.h>

#include "lke_fs.h"
#include "dir.h"

#define LKE_INODE_TYPE_REG  (0x01 << 0)
#define LKE_INODE_TYPE_FILE (0x01 << 1)

struct lke_fs_inode {
    struct inode vfs_inode;
    void* data;
    char name[LKE_MAX_PATH_NAME];
};

#define LKE_INODE_V_I(inode)    ( &(inode -> vfs_inode) )
#define LKE_FS_INODE_DATA(lke_inode) ( lke_inode -> data)

static inline struct lke_fs_inode* LKE_FS_I(struct inode* inode){
    return container_of(inode, struct lke_fs_inode, vfs_inode);
}

extern struct inode_operations lke_fs_dir_inode_ops;
extern struct inode_operations lke_fs_file_inode_ops;


struct inode *lke_fs_alloc_inode(struct super_block* sb);
void lke_fs_destroy_inode(struct inode *inode);
struct inode* lke_fs_new_inode(struct super_block *sb, struct inode* dir_inode, struct dentry *dentry, umode_t mode);

#endif
