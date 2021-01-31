#ifndef _DIR_H_
#define _DIR_H_

extern struct file_operations lke_fs_dir_op;

#define LKE_FILE_NAME_LEN   16
#define LKE_MAX_PATH_NAME   256

#define DEFAULT_DIR_MODE    0755

#include "lke_btree.h"

struct lke_dir_entity {
    struct lke_btree_head* btree_head;
    int fileNum;
    struct mutex lock;
};

#define LKE_FS_DENTRY_INODE_BTREE_HEAD(inode) (inode -> btree_head)

#endif
