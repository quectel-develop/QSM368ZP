#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <pthread.h>

#define BUF_SIZE (4 * 1024)

// Fixed suffix to append
//#define SUFFIX "#qsm368zp"
//#define SUFFIX_LEN (sizeof(SUFFIX) - 1)  
// exclude null terminator

int baudrate_table[][2] = {
    {50, B50}, {75, B75}, {110, B110}, {134, B134},
    {150, B150}, {200, B200}, {300, B300}, {600, B600},
    {1200, B1200}, {1800, B1800}, {2400, B2400}, {4800, B4800},
    {9600, B9600}, {19200, B19200}, {38400, B38400}, {57600, B57600},
    {115200, B115200}, {230400, B230400}, {460800, B460800}, {500000, B500000},
    {576000, B576000}, {921600, B921600}, {1000000, B1000000}, {1152000, B1152000},
    {1500000, B1500000}, {2000000, B2000000}, {2500000, B2500000}, {3000000, B3000000},
    {3500000, B3500000}, {4000000, B4000000},
};

static int match_baudrate(int baudrate)
{
    int i;
    for (i = 0; i < sizeof(baudrate_table) / sizeof(int) / 2; i++)
        if (baudrate_table[i][0] == baudrate)
            return baudrate_table[i][1];
    return -1;
}

static int open_port(char *node, int baudrate)
{
    int fd, rate;
    struct termios options;

    rate = match_baudrate(baudrate);
    if (rate < 0) {
        printf("match baudrate failed\n");
        return -1;
    }

    fd = open(node, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        printf("Could not open %s\n", node);
        return -1;
    }

    if (tcflush(fd, TCIOFLUSH) < 0) {
        printf("Could not flush uart port\n");
        close(fd);
        return -1;
    }

    if (tcgetattr(fd, &options) < 0) {
        printf("Can't get port settings \n");
        close(fd);
        return -1;
    }

    options.c_cc[VTIME] = 0;
    options.c_cc[VMIN] = 1;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= (CS8 | CLOCAL | CREAD);
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;

    cfsetospeed(&options, rate);
    cfsetispeed(&options, rate);

    if (tcsetattr(fd, TCSANOW, &options) < 0) {
        printf("Can't set port setting \n");
        close(fd);
        return -1;
    }

    fcntl(fd, F_SETFL, 0);
    return fd;
}

void clear_buf(unsigned char *buf)
{
	int i;

	for (i = 0; i < BUF_SIZE; i++)
		buf[i] = 0x00;
}

int main(int argc, char *argv[])
{
    int i, fd, count, baudrate;
    unsigned char buf[BUF_SIZE];

    if (argc != 3) {
        printf("usage: %s /dev/ttySx 115200\n", argv[0]);
        return 1;
    }

    sscanf(argv[2], "%d", &baudrate);
    if (baudrate < 0) {
        printf("get baudrate failed\n");
        return -1;
    }
    printf("baudrate: %d\n", baudrate);

    printf("Attempting to open device: %s\n", argv[1]);
    fd = open_port(argv[1], baudrate);
    if (fd < 0) {
        printf("open %s failed\n", argv[1]);
        return -1;
    }
    printf("Successfully opened serial port.\n");

    while (1) {
		clear_buf(buf);
		count = read(fd, buf, BUF_SIZE);
//        count = read(fd, buf, BUF_SIZE - SUFFIX_LEN - 1);  // leave room for suffix
        if (count < 0) {
            printf("read %s failed, ret: %d\n", argv[1], count);
			close(fd);
            return -1;
        } else if (count > 0) {
            // Echo back original data
            write(fd, buf, count);
            // Append suffix
			for (i = 0; i < count; i++)
                printf("%c", buf[i]);
        }
        // Optional: small delay or yield if needed (not required here)
    }

    printf("Closing port: %s\n", argv[1]);
    close(fd);
    return 0;
}