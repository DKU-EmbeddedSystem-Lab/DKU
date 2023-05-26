#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
	int i, fd, ret;
	char *msg = "programmersdkufs";
	char buf[4096], *path = argv[1];
	int size = atoi(argv[2]) * 1024;

	for (i = 0; i < 4096; i += 16) {
		memcpy(buf + i, msg, 16);
	}

	fd = open(path, O_RDWR|O_SYNC|O_CREAT, 0666);
	if (fd < 0) {
		printf("Failed to open %s\n", path);
		return fd;
	}

	for (; size > 0; size -= 4096) {
		if (size > 4096)
			ret = write(fd, buf, 4096);
		else
			ret = write(fd, buf, size);
		if (ret < 0) {
			printf("Write failed\n");
			return ret;
		}
	}

	return 0;
}
