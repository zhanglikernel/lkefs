#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/fcntl.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>

#include "common.h"
#include "inode.h"
#include "file.h"

loff_t lke_fs_file_llseek(struct file *filp, loff_t off, int whence){
    struct lke_file_entity *lke_file_entity = filp->private_data;
    loff_t newpos;
    switch(whence){
        case 0:
            newpos = off;
            break;
        case 1:
            newpos = filp->f_pos + off;
            break;
        case 2:
            newpos = lke_file_entity -> size + off;
            break;
        default:
            return -EINVAL;
    }
    if(newpos < 0){
        return -EINVAL;
    }
    filp->f_pos = newpos;
    return newpos;
}

static void lke_fs_file_trim(struct lke_file_entity* lke_file_entity){
    struct lke_file_qset *dptr, *next;
    int qset = LKE_FILE_QSET;
    int i;
    for( dptr = lke_file_entity -> data; dptr; dptr = next){
        if( dptr -> data ){
            for( i = 0; i < qset; i ++){
                kfree(dptr->data[i]);
            }
            kfree(dptr->data);
            dptr->data = NULL;
        }
        next = dptr->next;
        kfree(dptr);
    }
    lke_file_entity -> size = 0;
    lke_file_entity -> data = NULL;
}

long lke_fs_file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    int err = 0;
    int retval = 0;
    struct lke_file_entity *lke_file_entity = filp->private_data;
    if(_IOC_TYPE(cmd) != LKE_FS_FILE_IOC_MAGIC){
        return -ENOTTY;
    }
    if( _IOC_NR(cmd) > LKE_FS_FILE_IO_MAXNR ){
        return -ENOTTY;
    }
    err = !access_ok((void __user*)arg, _IOC_SIZE(cmd));
    if(err){
        return -ENOTTY;
    }
    switch(cmd){
        case LKE_FS_FILE_IOCREADY:
            lke_file_entity->ready = 1;
            wake_up_interruptible(&lke_file_entity->outq);
            break;
        default:
            return -ENOTTY;
    }
    return retval;
}

int lke_fs_file_open(struct inode *inode, struct file *filp){
    struct lke_fs_inode *lke_fs_inode = LKE_FS_I(inode);
    struct lke_file_entity *lke_file_entity = LKE_FS_INODE_DATA(lke_fs_inode);
    filp -> private_data = lke_file_entity;
    if((filp -> f_flags & O_ACCMODE) == O_WRONLY ){
        if(mutex_lock_interruptible(&lke_file_entity->lock)){
            return -ERESTARTSYS;
        }
        lke_fs_file_trim(lke_file_entity);
        mutex_unlock(&lke_file_entity->lock);
    }
    PDEBUG("lke_fs_file_open.");
    return 0;
}

struct lke_file_qset* lke_fs_file_follow(struct lke_file_entity* lke_file_entity, int itemindex){
    struct lke_file_qset *dptr;
    int i;
    dptr = lke_file_entity -> data;
    if(!dptr){
        dptr = kmalloc(sizeof(struct lke_file_qset), GFP_KERNEL);
        if(dptr == NULL){
            return NULL;
        }
        memset(dptr, 0, sizeof(struct lke_file_qset));
        lke_file_entity -> data = dptr;
    }
    
    for( dptr = lke_file_entity->data, i = 0; i < itemindex; i++ ){
        if(!dptr->next){
            dptr->next = kmalloc(sizeof(struct lke_file_qset), GFP_KERNEL);
            if(NULL == dptr->next){
                return NULL;
            }
            memset(dptr->next, 0, sizeof(struct lke_file_qset));
        }
        dptr = dptr -> next;
    }
    return dptr;
}

ssize_t lke_fs_file_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos){
    struct lke_file_entity *lke_file_entity = filp -> private_data;
    struct lke_file_qset * dptr;
    int quantum = LKE_FILE_QUANTUM;
    int qset = LKE_FILE_QSET;
    int itemsize = qset * quantum;
    int item, s_pos, q_pos, rest;
    ssize_t retval = 0;
    PDEBUG("lke_fs_file_write.");
    if(mutex_lock_interruptible(&lke_file_entity->lock)){
        return -ERESTARTSYS;
    }
    item = (long)*f_pos / itemsize;
    rest = (long)*f_pos % itemsize;
    s_pos = rest / quantum;
    q_pos = rest % quantum;
    dptr = lke_fs_file_follow(lke_file_entity, item);
    if(NULL == dptr){
        goto out;
    }
    if(!dptr->data){
        dptr -> data = kmalloc(sizeof(char *)*qset, GFP_KERNEL);
        if(!dptr -> data){
            goto out;
        }
        memset(dptr -> data, 0, sizeof(char *) * qset);
    }
    if(!dptr->data[s_pos]){
        dptr->data[s_pos] = kmalloc( sizeof(char) * quantum, GFP_KERNEL);
        if(!dptr->data[s_pos]){
            goto out;
        }
    }
    if(count > quantum - q_pos){
        count = quantum - q_pos;
    }
    if(copy_from_user(dptr->data[s_pos] + q_pos, buf, count)){
        retval = -EFAULT;
        goto out;
    }
    *f_pos += count;
    if(lke_file_entity -> size < *f_pos){
        lke_file_entity -> size = *f_pos;
    }
    retval = count;
    lke_file_entity -> ready = 0;
    PDEBUG("write something:%ld %s.\n", count, buf);
out:
    mutex_unlock(&lke_file_entity->lock);
    return retval;
}

ssize_t lke_fs_file_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos){
    struct lke_file_entity *lke_file_entity = filp -> private_data;
    struct lke_file_qset *dptr;
    int quantum = LKE_FILE_QUANTUM;
    int qset = LKE_FILE_QSET;
    int itemsize = qset * quantum;
    int item, s_pos, q_pos, rest;
    ssize_t retval = 0;
    if(mutex_lock_interruptible(&lke_file_entity->lock)){
        return -ERESTARTSYS;
    }
    if(*f_pos > lke_file_entity -> size){
        goto out;
    }
    if(count > lke_file_entity -> size - *f_pos){
        count = lke_file_entity-> size - *f_pos;
    }
    item = (long)*f_pos / itemsize;
    rest = (long)*f_pos % itemsize;
    s_pos = rest / quantum;
    q_pos = rest % quantum;
    dptr = lke_fs_file_follow(lke_file_entity, item);
    if(NULL == dptr || !dptr -> data || !dptr->data[s_pos]){
        goto out;
    }
    if(count > quantum - q_pos){
        count = quantum - q_pos;
    }
    if(copy_to_user(buf, dptr->data[s_pos]+q_pos, count)){
        retval = -EFAULT;
        goto out;
    }
    *f_pos += count;
    retval = count;
    PDEBUG("read something:%ld %s", count, buf);
out:
    mutex_unlock(&lke_file_entity->lock);
    return retval;
}

int lke_fs_file_release(struct inode *inode, struct file *filp){
    return 0;
}

static unsigned int lke_fs_file_poll(struct file *filp, poll_table *wait){
    struct lke_file_entity *lke_file_entity = filp->private_data;
    unsigned long mask = 0;
    mutex_lock(&lke_file_entity -> lock);
    poll_wait(filp, &lke_file_entity->outq, wait);
    if(1 == lke_file_entity->ready){
        mask |= POLLIN | POLLRDNORM;
        lke_file_entity->ready = 0;
    }
    mutex_unlock(&lke_file_entity -> lock);
    return mask;
}

struct file_operations lke_fs_file_ops = {
    .open = lke_fs_file_open,
    .release = lke_fs_file_release,
    .llseek = lke_fs_file_llseek,
    .read = lke_fs_file_read,
    .write = lke_fs_file_write,
    .poll = lke_fs_file_poll,
    .iopoll = NULL,
    .unlocked_ioctl = lke_fs_file_ioctl,
    .compat_ioctl = NULL,
};
