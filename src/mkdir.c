#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	if (argc == 1)
		return 0;

	// Keep it simpl (for now)
	for (int i = 1; i < argc; i++) {
		if (mkdir(argv[i], 0677) == -1) {
			fprintf(stderr, "mkdir %d: %s\n", errno,
				strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
}
