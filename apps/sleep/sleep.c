#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

extern void sleep(unsigned int sec);

int main(int argc, char **argv) {
	
	struct timeval start, stop, res;
	
	gettimeofday(&start, NULL);
	sleep(2);
	gettimeofday(&stop, NULL);

	timersub(&stop, &start, &res);
	printf("%ld.%06ld\n", res.tv_sec, res.tv_usec);

	return 0;
}
