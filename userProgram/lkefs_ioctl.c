#include <stdio.h>
#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lke_ioctl.h"

int main(int argc, char **argv){
    char *devname;
    int fd;
    devname = argv[1];
    if((fd = open(devname, O_RDWR)) < 0){
        printf("open failed.\n");
        return -1;
    }
    ioctl(fd, LKE_FS_FILE_IOCREADY);
    return 0;
}
