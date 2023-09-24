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
    char readbuf[100];
    unsigned char writebuf[1];

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
        sleep(5);
        cnt++;
        printf("App running %d\r\n",cnt);
        if (cnt>=5)
            break;
    }
    printf("App running finished\r\n");
    ret = close(fd);
    if (ret < 0) {
        printf("close %s failed\r\n", filename);
    }

    return 0;
}

