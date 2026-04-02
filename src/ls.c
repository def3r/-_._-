#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int main()
{
	DIR *dir = opendir(".");
	if (dir == NULL) {
		fprintf(stderr, "(%d): %s", errno, strerror(errno));
		return errno;
	}

	struct dirent *entry = NULL;
	while ((entry = readdir(dir)) != NULL) {
		printf("%s\t", entry->d_name);
	}
	printf("\n");

	closedir(dir);
	return 0;
}
