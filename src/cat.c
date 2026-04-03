#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

static void cat(FILE *in)
{
	char *buf = NULL;
	size_t l_buf = 0;
	ssize_t l_line;
	while ((l_line = getline(&buf, &l_buf, in)) != -1) {
		fwrite(buf, l_line, 1, stdout);
	}
	free(buf);
}

int main(int argc, char *argv[])
{
	int i = 1;
	do {
		FILE *in = argc == 1 ? stdin : fopen(argv[i], "r");
		if (!in) {
			fprintf(stderr, "cat: %s, %s\n", argv[i],
				strerror(errno));
			return 1;
		}

		cat(in);
		fclose(in);
	} while (++i < argc);

	return 0;
}
