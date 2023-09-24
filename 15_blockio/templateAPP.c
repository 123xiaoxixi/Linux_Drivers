#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


#define LEDOFF  0
#define LEDON   1

int main(int argc, char *argv[]) {
    int ret = 0;
    int fd = 0;
    ssize_t count;
    int cnt = 0;
    char readbuf[100] = {0};
    unsigned char writebuf[1];

    if (argc != 2) {
        printf("error usage\r\n");
        return -1;
    }

    char *filename = argv[1];
    fd = open(filename, O_RDWR);
    if (fd < 0 ) {
        printf("open %s failed\r\n", filename);
    }

    while(1) {
        printf("templateAPP: start read()\r\n");
        count = read(fd, readbuf, 1);
        printf("templateAPP: read() finished\r\n");
        if (count < 0) {
            
        } else if (readbuf[0])
            printf("keyvalue:%#x\r\n", readbuf[0]);
    }

    printf("App running finished\r\n");
    ret = close(fd);
    if (ret < 0) {
        printf("close %s failed\r\n", filename);
    }

    return 0;
}

