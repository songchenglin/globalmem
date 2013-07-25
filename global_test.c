#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#define DEVICE_NAME "/dev/globalmem0"

int main(void)
{
	int fd;
	int ret;
	int offset =0;
	char write_buf[] = "abcde";
	char read_buf[20] = {0};
	char buf2[20] = {0};
	fd = open(DEVICE_NAME,O_RDWR);
	if(fd < 0) {
		printf("open %s failed!\n fd = %d\n",DEVICE_NAME,fd);
		return -1;
	}
	printf("open %s succeed!fd = %d\n",DEVICE_NAME,fd);
	write(fd,"abcde",sizeof(write_buf));
	close(fd);
	fd = open(DEVICE_NAME,O_RDWR);
	read(fd,buf2,sizeof(write_buf));
	printf("%s",buf2);
/*	ioctl(fd,0x01);
	memset(buf2,0,4*1024);
	read(fd,buf2,4*1024);
	printf("%s",buf2);*/
	close(fd);
	return 0;
}
