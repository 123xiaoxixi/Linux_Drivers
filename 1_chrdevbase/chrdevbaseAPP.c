#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int ret = 0;
    int fd = 0;
    ssize_t count;
    char readbuf[100];
    char writebuf[] = "user data";

    if (argc != 3) {
        printf("error usage\r\n");
        return -1;
    }

    char *filename = argv[1];
    fd = open(filename, O_RDWR);
    if (fd < 0 ) {
        printf("open %s failed\r\n", filename);
    }

    if (atoi(argv[2]) == 1) {

        count = read(fd, readbuf, 50);
        printf("read count = %d\r\n", count);
        if (count < 0) {
            printf("read %s failed\r\n",filename);
        } else
            printf("readbuf:%s\r\n", readbuf);
    } else if (atoi(argv[2]) == 2) {
        count = write(fd, writebuf, sizeof(writebuf));
        if (count < 0) {
            printf("write %s failed\r\n",filename);
        }
    }

    ret = close(fd);
    if (ret < 0) {
        printf("close %s failed\r\n", filename);
    }

    return 0;
}

