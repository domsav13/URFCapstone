// motor_daemon.c
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <termios.h>
#include <errno.h>
#include <time.h>

#define SERIAL_PORT "/dev/ttyACM0"
#define FIFO_PATH   "/tmp/arduino_cmd"
typedef speed_t Baud;

static int openSerialPort(const char* port) {
    int fd = open(port, O_RDWR|O_NOCTTY|O_SYNC);
    if (fd<0) perror("open serial");
    return fd;
}
static int configureSerialPort(int fd, Baud baud) {
    struct termios t; memset(&t,0,sizeof t);
    if(tcgetattr(fd,&t)) { perror("tcgetattr"); return -1; }
    cfsetospeed(&t,baud); cfsetispeed(&t,baud);
    t.c_cflag=(t.c_cflag&~CSIZE)|CS8;
    t.c_iflag&=~IGNBRK; t.c_lflag=0; t.c_oflag=0;
    t.c_cc[VMIN]=0; t.c_cc[VTIME]=5;
    t.c_iflag&=~(IXON|IXOFF|IXANY);
    t.c_cflag|=(CLOCAL|CREAD);
    t.c_cflag&=~(PARENB|PARODD); t.c_cflag&=~CSTOPB;
#ifdef CRTSCTS
    t.c_cflag&=~CRTSCTS;
#endif
    if(tcsetattr(fd,TCSANOW,&t)) { perror("tcsetattr"); return -1; }
    return 0;
}
static int sendCmd(int fd,const char* s){
    char buf[80]; int len=snprintf(buf,sizeof buf,"%s\n",s);
    if(len<0||len>=(int)sizeof buf) return -1;
    return write(fd,buf,len)<0?-1:0;
}

int main(void){
    int sd=openSerialPort(SERIAL_PORT);
    if(sd<0) return 1;
    if(configureSerialPort(sd,B115200)<0){ close(sd); return 1; }
    if(access(FIFO_PATH,F_OK)==-1) mkfifo(FIFO_PATH,0666);

    int fr=open(FIFO_PATH,O_RDONLY);
    int fw=open(FIFO_PATH,O_WRONLY|O_NONBLOCK);
    if(fr<0){ perror("fifo"); close(sd);return 1; }

    char cmd[64]; struct timespec p={0,50000000L};
    while(1){
        ssize_t n=read(fr,cmd,sizeof(cmd)-1);
        if(n>0){
            cmd[n]=0;
            if(cmd[n-1]=='\n') cmd[n-1]=0;
            sendCmd(sd,cmd);
        }
        nanosleep(&p,NULL);
    }
    //never reached
    close(fr); close(fw); close(sd);
    return 0;
}
