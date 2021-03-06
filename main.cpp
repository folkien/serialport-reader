#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>

#define USE_CTSRTS 1
#if USE_CTSRTS == 1
    const char flowVersion[] = "Flow version: CTSRTS.";
#else
    const char flowVersion[] = "Flow version: none.";
#endif

bool isRunning = true;

static struct termios oldterminfo;

void info() {
    printf("Serial detonator v.0.1.\n"\
            "%s\n",
            flowVersion
            );
}

char * bombardData = NULL;
void bombardDataCreate() {
    unsigned short i = 0;
    bombardData = (char*)malloc(256*sizeof(char));
    for (i=0;i<256; i++) {
        bombardData[i] = i;
    }
}
void bombardDataDestroy() {
    free(bombardData);
}

void closeserial(int fd)
{
    tcsetattr(fd, TCSANOW, &oldterminfo);
    if (close(fd) < 0)
        perror("closeserial()");
}


int openserial(char *devicename)
{
    int fd;
    struct termios attr;

    if ((fd = open(devicename, O_RDWR | O_NOCTTY )) == -1) {
        perror("openserial(): open()");
        return 0;
    }
    if (tcgetattr(fd, &oldterminfo) == -1) {
        perror("openserial(): tcgetattr()");
        return 0;
    }
    attr = oldterminfo;
    attr.c_cflag = B19200 | CS8 | 0x00 |  CLOCAL | CREAD;
    attr.c_oflag = 0;
    attr.c_iflag = IGNPAR | ICRNL | CR0 | TAB0 | BS0 | VT0 | FF0 ;
    attr.c_lflag = ICANON;
    attr.c_cc[VTIME] = 1;
    attr.c_cc[VMIN] = 0;
#if USE_CTSRTS == 1
    attr.c_cflag |= CRTSCTS;
#endif

    if (cfsetispeed(&attr, B19200) < 0) {
        perror("set Input speed(): error");
        return 0;
    };
    if (cfsetospeed(&attr, B19200) < 0) {
        perror("set output speed(): error");
        return 0;
    };

    if (tcflush(fd, TCIOFLUSH) == -1) {
        perror("openserial(): tcflush()");
        return 0;
    }
    if (tcsetattr(fd, TCSANOW, &attr) == -1) {
        perror("initserial(): tcsetattr()");
        return 0;
    }
    return fd;
}


int setRTS(int fd, int level)
{
    int status;

    if (ioctl(fd, TIOCMGET, &status) == -1) {
        perror("setRTS(): TIOCMGET");
        return 0;
    }
    if (level)
        status |= TIOCM_RTS;
    else
        status &= ~TIOCM_RTS;
    if (ioctl(fd, TIOCMSET, &status) == -1) {
        perror("setRTS(): TIOCMSET");
        return 0;
    }
    return 1;
}

int setCTS(int fd, int level)
{
    int status;

    if (ioctl(fd, TIOCMGET, &status) == -1) {
        perror("setRTS(): TIOCMGET");
        return 0;
    }
    if (level)
        status |= TIOCM_CTS;
    else
        status &= ~TIOCM_CTS;

    if (ioctl(fd, TIOCMSET, &status) == -1) {
        perror("setRTS(): TIOCMSET");
        return 0;
    }
    return 1;
}

int RTSvalue(int fd, int value) {
   int RTS_flag = TIOCM_RTS;
   if (value == 1) {
        if (ioctl(fd,TIOCMBIS,&RTS_flag) != -1) {
            printf("RTS set 1.\n");
        };//Set RTS pin
   }
   else {
        if (ioctl(fd,TIOCMBIC,&RTS_flag) != -1) {
            printf("RTS set 0.\n");
        };
   }
}

void waitOnKeyPress() {
    char character;
    printf("Waiting for pressing enter key.\n");
    scanf("%c",&character);
}

/* this function is run by the second thread */
void *keyboardReader(void *x_void_ptr)
{
    waitOnKeyPress();
    isRunning = false;
    return NULL;
}

int main()
{
    int fd;
    char *serialdev = "/dev/ttyUSB0";
    pthread_t keyboardThread;
    /* create a second thread which executes inc_x(&x) */

    info();

  /*  if(pthread_create(&keyboardThread, NULL, keyboardReader, &fd)) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }*/

    fd = openserial(serialdev);
    if (!fd) {
        fprintf(stderr, "Error while initializing %s. Maybe you should use root privileges?\n", serialdev);
        return 1;
    }

    RTSvalue(fd,0);
    waitOnKeyPress();
    RTSvalue(fd,1);
    waitOnKeyPress();

    bombardDataCreate();
    printf("Start of data transmission on %s.\n",serialdev);
    while (isRunning) {
        write(fd, bombardData, sizeof(bombardData));
    }
    printf("Stop of data transmission.\n");
    bombardDataDestroy();

    /*
    for (int i = 0; i<10; ++i) {
        RTSvalue(fd,1);
        sleep(1);
        RTSvalue(fd,0);
        sleep(1);
    }
    */

    closeserial(fd);

    /* wait for the second thread to finish */
/*    if(pthread_join(keyboardThread, NULL)) {
        fprintf(stderr, "Error joining thread\n");
        return 2;
    }*/
    return 0;
}
