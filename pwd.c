#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define L_M_CWD 100

int main()
{
	char buf[L_M_CWD];
	memset(buf, '\0', L_M_CWD);

	if (!getcwd(buf, L_M_CWD)) {
		fprintf(stderr, "pwd failed: %d\n%s", errno, strerror(errno));
	}

	printf("%s\n", buf);

	return 0;
}
