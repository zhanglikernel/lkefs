#ifndef _FILE_H_
#define _FILE_H_

#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/ioctl.h>

#ifndef LKE_FILE_QUANTUM
#define LKE_FILE_QUANTUM 1024
#endif

#ifndef LKE_FILE_QSET
#define LKE_FILE_QSET   4096
#endif

struct lke_file_qset{
    void **data;
    struct lke_file_qset *next;
};

struct lke_file_entity{
    struct lke_file_qset *data;
    unsigned long size;
    struct mutex lock;
    wait_queue_head_t outq;
    int ready;
};

extern struct file_operations lke_fs_file_ops;
#define LKE_FS_FILE_DATA(file)  (file->data)

#define LKE_FS_FILE_IOC_MAGIC    'K'

#define LKE_FS_FILE_IOCREADY     _IO(LKE_FS_FILE_IOC_MAGIC, 0)

#define LKE_FS_FILE_IO_MAXNR    0

#endif
