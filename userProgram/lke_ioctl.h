#ifndef _LKE_IOCTL_H
#define _LKE_IOCTL_H

#include <sys/ioctl.h>

#define LKE_FS_FILE_IOC_MAGIC    'K'

#define LKE_FS_FILE_IOCREADY     _IO(LKE_FS_FILE_IOC_MAGIC, 0)

#define LKE_FS_FILE_IO_MAXNR    0

#endif
