#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define KEY0VALUE    0xF0
#define INVAKEY      0x0

int main(int argc, char *argv[]) {
    int ret = 0;
    int fd = 0;
    ssize_t count;
    int value;
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
        read(fd, &value, sizeof(value));
        if (value == KEY0VALUE) {
            printf("key0 has been pressed,value = %d\r\n", value);
        }
    }

    ret = close(fd);
    if (ret < 0) {
        printf("close %s failed\r\n", filename);
    }

    return 0;
}

