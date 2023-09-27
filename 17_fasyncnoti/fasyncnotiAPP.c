#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>


#define LEDOFF  0
#define LEDON   1

int fd = 0;
char readbuf[100] = {0};

void sigio_handler(int arg) {
    if (read(fd, readbuf, 1) < 0) {

    } else if (readbuf[0]) {
        printf("sigio signal, keyvalue:%#x\r\n", readbuf[0]);
    }
}

int main(int argc, char *argv[]) {
    int ret = 0;
    int flags = 0;
    
    ssize_t count;
    int cnt = 0;
    
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

    signal(SIGIO, sigio_handler);       //设置SIGIO信号的信号处理函数
    //设置当前进程接收SIGIO信号,设定为fd指向设备的事件的接收目标
    //当驱动通过kill_fasync()发送信号时当前进程才能收到信号
    fcntl(fd, F_SETOWN, getpid());  

    //将进程添加到设备驱动fasync事件等待队列    
    flags = fcntl(fd, F_GETFL);         //获取fd文件状态标志
    fcntl(fd, F_SETFL, flags | FASYNC);



    while(1);
    printf("App running finished\r\n");
    ret = close(fd);
    if (ret < 0) {
        printf("close %s failed\r\n", filename);
    }

    return 0;
}

