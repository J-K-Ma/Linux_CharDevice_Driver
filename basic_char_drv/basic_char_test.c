#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main(){
	int fd = open("/dev/my-test",O_RDWR);
	if(fd == -1){
		printf("fail open\n");
		return -1;
	}
	sleep(3);
	close(fd);
	return 0;
}
