#ifndef _SUPER_H_
#define _SUPER_H_

#include <linux/fs.h>

#include "lke_fs.h"

struct lke_fs_super_block {
    unsigned long sbi_magic;
    char sbi_filesystem_version[20];
    struct super_block *vfs_sb;
    unsigned long sbi_inode_count;
};


extern struct kmem_cache *lke_fs_inode_cachep;

#define VFS_SB_FS_INFO(sb) ( sb -> s_fs_info )
#define LKE_FS_SB_VFS_SB(sb)  ( sb -> vfs_sb )

#endif
