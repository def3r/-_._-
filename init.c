// init.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

// envz.h -> GNU Extension
// environ.h -> POSIX
// #include <envz.h> // we have environ.h .. ?
// tbh envz.h is kinda cool, see `man envz`

extern char **environ;

#include <editline.h>

static char *sh_prompt = " > ";

// Lets have static path rn
static char *paths[] = { "/bin/" };
static int paths_len = 1;

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
	printf("\n--- Welcome to the Kernel ---\n");
	// for (char **current = environ; *current; current++) {
	//   puts(*current);
	// }

	init_setenv();

	while (1) {
		char *sh_in = readline(sh_prompt);
		printf("%s\n", sh_in);

		pid_t sh_child = fork();
		if (sh_child == 0) {
			execlp(sh_in, sh_in, NULL);
			return 1;
		}
		waitpid(sh_child, NULL, 0);

		free(sh_in);
	}
	return 0;
}
