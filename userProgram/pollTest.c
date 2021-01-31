#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <poll.h>

#define POLL_NR 10

char buffer[4096];

int main(int argc, char **argv){
    int i;
    int fd;
    struct pollfd stuPoll[POLL_NR];
    int time_out_ms = 3000;
    int num;
    memset(stuPoll, 0, sizeof(stuPoll));
    for( i = 0 ; i < POLL_NR; i ++ ){
        stuPoll[i].fd = -1;
    }
    for( i = 1; i < argc; i ++){
        fd = open(argv[i], O_RDONLY | O_NONBLOCK);
        printf("%s %d.\n", argv[i], fd);
        stuPoll[i].fd = fd;
        stuPoll[i].events = 0;
        stuPoll[i].events |= POLLIN;
    }
    while(1){
        num = poll(stuPoll, POLL_NR, time_out_ms);
        if(num > 0){
            printf("num > 0.\n");
            for( i = 0 ; i < POLL_NR; i ++ ){
                if((stuPoll[i].fd != -1) && (POLLIN & stuPoll[i].revents)){
                    read(stuPoll[i].fd, buffer, sizeof(buffer));
                    printf("fd:%d read somthing.\n%s.\n", stuPoll[i].fd, buffer);
                }
            }
        }
    }
    return 0;
}
