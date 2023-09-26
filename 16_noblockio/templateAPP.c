#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>

#include <poll.h>

#define LEDOFF  0
#define LEDON   1

int main(int argc, char *argv[]) {
    int ret = 0;
    int fd = 0;
#ifdef M_SELECT
    fd_set readfds;
    struct timeval timeout;
#else
    struct pollfd fds;
#endif
    ssize_t count;
    int cnt = 0;
    char readbuf[100] = {0};
    unsigned char writebuf[1];

    if (argc != 2) {
        printf("error usage\r\n");
        return -1;
    }

//int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

    char *filename = argv[1];
    fd = open(filename, O_RDWR | O_NONBLOCK);   //非阻塞打开
    if (fd < 0 ) {
        printf("open %s failed\r\n", filename);
    }
#ifdef M_SELECT
    while(1) {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        ret = 0;

        ret = select(fd + 1, &readfds, NULL, NULL, &timeout); 
        // printf("ret=%d  ",ret);
        switch (ret)
        {          
            case 0: {       //超时
                printf("templateAPP: timeout\r\n");
                break;
            }
            case -1:{
                printf("templateAPP: select error\r\n");
                break;  
            } 
            default:
            {
                if (FD_ISSET(fd, &readfds)) {
                    // printf("templateAPP: can read()\r\n");
                    // printf("templateAPP: start read()\r\n");
                    count = read(fd, readbuf, 1);
                    // printf("templateAPP: read() finished\r\n");
                    if (count < 0) {
                        
                    } else if (readbuf[0] > 0){
                        printf("ret = %d\r\n",ret);
                        printf("\r\n****keyvalue:%#x\r\n", readbuf[0]);
                    }
                }
                break;
            }
        }
        
    }
#else
    while(1) {
            ret = 0;
            fds.fd = fd;
            fds.events = POLLIN;    //关心的事件类型
            // ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
            ret = poll(&fds, 1, 500);
            if (ret == 0) {
                printf("timeout\r\n");
            } else if (ret < 0) {
                printf("error\r\n");
            } else {
                if (fds.revents | POLLIN) { //fds.revents表示实际发生的事件类型
                    count = read(fd, readbuf, 1);
                    // printf("templateAPP: read() finished\r\n");
                    if (count < 0) {
                        
                    } else if (readbuf[0] > 0){
                        printf("ret = %d\r\n",ret);
                        printf("****keyvalue:%#x\r\n", readbuf[0]);
                    }
                }
            }
        }
#endif

    printf("App running finished\r\n");
    ret = close(fd);
    if (ret < 0) {
        printf("close %s failed\r\n", filename);
    }

    return 0;
}

