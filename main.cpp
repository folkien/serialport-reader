#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>

bool isRunning = true;

static struct termios oldterminfo;


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
    attr.c_cflag = CS8 | CRTSCTS |  CLOCAL | CREAD;
    attr.c_oflag = 0;
    attr.c_iflag = 0;
    attr.c_lflag = 0;
    attr.c_cc[VTIME] = 10;
    attr.c_cc[VMIN] = 0;

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
    ioctl(fd,TIOCMBIS,&RTS_flag);//Set RTS pin
   }
   else {
    ioctl(fd,TIOCMBIC,&RTS_flag);//Clear RTS printf
   }
}

/* this function is run by the second thread */
void *keyboardReader(void *x_void_ptr)
{
    char character;
    printf("Waiting for pressing enter key.\n");
    scanf("%c",&character);
    isRunning = false;
    return NULL;
}

int main()
{
    int fd;
    char *serialdev = "/dev/ttyUSB0";
    char *testData  = "<12234567890ABCDEFQWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm>";
    pthread_t keyboardThread;
        /* create a second thread which executes inc_x(&x) */
  /*  if(pthread_create(&keyboardThread, NULL, keyboardReader, &fd)) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }*/

    fd = openserial(serialdev);
    if (!fd) {
        fprintf(stderr, "Error while initializing %s.\n", serialdev);
        return 1;
    }

    while (isRunning) {
        write(fd, testData, sizeof(testData));
    }

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
