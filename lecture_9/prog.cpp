#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <iostream>

using namespace std;

#define BUF_LEN 256

void read_all(int fd) {
	int len;
	char buf[BUF_LEN];
	while((len = read(fd, buf, BUF_LEN)) <= BUF_LEN && len > 0) {
			printf("read: %d >", len);
			for(int x = 0; x < len; x++)
				putc(buf[x], stdout);
			printf("<\n");
	}
}

void write_times(int fd, const char* msg, int times) {
	int len;
	char buf[BUF_LEN];
	strcpy(buf, msg);
	len = strlen(msg);

	int written;
	for(int x = 0; x < times; x++) {
		written = 0;
		while((written += write(fd, buf + written, len - written)) < len);
	}
}

void io_control(int fd) {
	ioctl(fd, 200, 5);
	ioctl(fd, 201);
}

int main(int argc, char* argv[]) {

	int fd = open("/dev/mpp", O_RDWR);
	printf("fd: %d\n", fd);

	read_all(fd);
	write_times(fd, "Hello from user!", 10);
	io_control(fd);
	read_all(fd);

	close(fd);
	return 0;
}