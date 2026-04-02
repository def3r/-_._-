// init.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

// Lets have static paths rn
static char *paths[] = { "/bin/", "/sbin/" };
static int paths_len = 2;

// Term-related
#define CSI(seq) "\033[" seq

void init_setenv()
{
	size_t path_str_size = 0;
	for (int i = 0; i < paths_len; i++) {
		path_str_size += strlen(paths[i]) + 1;
	}
	char *path_str = malloc(path_str_size);
	size_t path_str_offset = 0;
	for (int i = 0; i < paths_len; i++) {
		size_t len = strlen(paths[i]);
		memcpy(path_str + path_str_offset, paths[i], len);
		path_str_offset += len;
		path_str[path_str_offset++] = ':';
	}
	path_str[path_str_offset - (path_str_offset > 0)] = '\0';

	setenv("PATH", path_str, 1);
}

int main(int argc, char *argv[])
{
	printf("%s", CSI("2J")); // Clear screen
	printf("%s", CSI("1;1H")); // Mov cursor to 1,1

	printf("--- Welcome to the Kernel ---\n");
	// for (char **current = environ; *current; current++) {
	//   puts(*current);
	// }

	init_setenv();

	pid_t p_cash = fork();
	if (p_cash == 0) {
		execlp("cash", "cash", NULL);
		return 1;
	}
	waitpid(p_cash, NULL, 0);
	printf("Time to sleep WEEEEE!\n");

	while (1) {
		sleep(100);
	}
	return 0;
}
