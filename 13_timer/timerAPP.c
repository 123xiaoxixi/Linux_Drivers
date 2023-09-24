#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>


#define LEDOFF  0
#define LEDON   1

#define CLOSE_CMD   _IO(0xEF, 1)
#define OPEN_CMD    _IO(0xEF, 2)
#define SETPERIOD_CMD   _IOW(0xEF, 3, int)

int main(int argc, char *argv[]) {
    int ret = 0;
    int fd = 0;
    ssize_t count;
    int cnt = 0;
    char readbuf[100];
    unsigned int cmd;
    unsigned int arg;
    unsigned char writebuf[1];
    unsigned char str[100];

    if (argc != 3) {
        printf("error usage\r\n");
        return -1;
    }

    char *filename = argv[1];
    fd = open(filename, O_RDWR);
    if (fd < 0 ) {
        printf("open %s failed\r\n", filename);
    }

    writebuf[0] = atoi(argv[2]);
    if (write(fd, writebuf, 1) < 0) {
        printf("write() failed\r\n");
        close(fd);
        return -1;
    }

    while(1) {
        printf("input cmd:");
        ret = scanf("%d",&cmd);
        if (ret != 1) {
            gets(str);
        }
        if (cmd  == 1) {        //關閉
            ioctl(fd, CLOSE_CMD, &arg);
        } else if (cmd == 2) {  //打開
            ioctl(fd, OPEN_CMD, &arg);
        } else if (cmd == 3) {  //設置
            printf("\r\ninput period:");
            ret = scanf("%d", &arg);
            if (ret != 1) {
                gets(str);
            }
            ioctl(fd, SETPERIOD_CMD, &arg);
        }
    }
    printf("App running finished\r\n");
    ret = close(fd);
    if (ret < 0) {
        printf("close %s failed\r\n", filename);
    }

    return 0;
}

