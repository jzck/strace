#include <unistd.h>
#include <fcntl.h>

int 	main(void)
{
	write(1, "write in tracee\n", 16);
	int fd = open("test.c", 0);
	close(fd);
	dup2(3, fd);
	return 0;
}
