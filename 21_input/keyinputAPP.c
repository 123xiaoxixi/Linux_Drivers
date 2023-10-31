#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/input.h>


#define LEDOFF  0
#define LEDON   1

// ./keyinputAPP /dev/input/event1

static struct input_event inputevent;

int main(int argc, char *argv[]) {
    int ret = 0;
    int fd = 0;
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
        ret = read(fd, &inputevent, sizeof(inputevent));
        if (ret > 0) {  //读取成功
            switch(inputevent.type) {
                case EV_KEY:
                    printf("EV_KEY事件\r\n");
                    if (inputevent.code < BTN_MISC) { //是按键而非button
                        printf("key %d %s\r\n", inputevent.code, inputevent.value ? "Press" : "Release");
                    } else {    //button
                        printf("button\r\n");
                    }
                    break;
                case EV_SYN:
                    printf("EV_SYN事件\r\n");
                    break;
                case EV_REL:
                    printf("EV_REL事件\r\n");
                    break;
                case EV_ABS:
                    printf("EV_ABS事件\r\n");
                    break;
                case EV_REP:
                    printf("EV_REP事件\r\n");
                    break;
            }
        } else {
            printf("读取失败\r\n");
        }
    }
    
    close(fd);
    return 0;
}

