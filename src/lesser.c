#include <alloca.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#define CSI(seq) "\033[" seq
#define MIN(a, b) a < b ? a : b;
#define TAB_SIZE 8

struct sv {
	char *buf;
	int bytes;

	// this contains expanded tabs buf and len
	char *t_buf;
	int t_bytes;
};

struct termios orig_termios;

static void get_termsize(uint16_t *row, uint16_t *col)
{
	struct winsize ws;
	if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1) {
		fprintf(stderr, "lesser: cant get terminal size\n\t(%d): %s\n",
			errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (row)
		*row = ws.ws_row;
	if (col)
		*col = ws.ws_col;
}

static void mapfile(char *path, char **buf)
{
	int fd = open(path, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "(%d): %s", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	struct stat st;
	fstat(fd, &st);

	// Virtual fs (like proc) has 0 size
	if (st.st_size == 0) {
		size_t cap = 4096;
		*buf = malloc(cap);
		size_t size = 0;
		ssize_t n;
		while ((n = read(fd, *buf + size, cap - size)) > 0) {
			size += n;
			if (size == cap) {
				cap *= 2;
				*buf = realloc(*buf, cap);
			}
		}
		(*buf)[size - 1] = '\0';
	} else {
		*buf = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	}
	close(fd);
}

static void exit_rawmode()
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static void enable_rawmode()
{
	tcgetattr(STDIN_FILENO, &orig_termios);
	struct termios raw = orig_termios;
	tcgetattr(STDIN_FILENO, &raw);
	raw.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void enter_alt_screen()
{
	static char *seq = CSI("?1049h");
	write(STDOUT_FILENO, seq, strlen(seq));
}

static void exit_alt_screen()
{
	static char *seq = CSI("?1049l");
	write(STDOUT_FILENO, seq, strlen(seq));
}

static void show_cursor()
{
	static char *seq = CSI("?25h");
	write(STDOUT_FILENO, seq, strlen(seq));
}

static void hide_cursor()
{
	static char *seq = CSI("?25l");
	write(STDOUT_FILENO, seq, strlen(seq));
}

static void exit_lesser()
{
	show_cursor();
	exit_rawmode();
	exit_alt_screen();
}

static size_t str_count(char *buf, char c)
{
	size_t count = 0;
	while (buf) {
		buf = strchr(buf, c);
		buf = buf ? buf + 1 : NULL;
		count++;
	}
	return count;
}

static void expand_tab(struct sv *view)
{
	char line[view->bytes + 1];
	memcpy(line, view->buf, view->bytes);
	line[view->bytes] = '\0';

	size_t total_tabs = str_count(line, '\t') - 1;
	if (total_tabs == 0) {
		return;
	}

	// worst case: every tab expands to TAB_SIZE spaces
	size_t expanded_size = view->bytes + total_tabs * (TAB_SIZE - 1) + 1;
	char *expanded = alloca(expanded_size);
	int src = 0, col = 0;

	while (src < view->bytes) {
		if (line[src] == '\t') {
			int spaces = TAB_SIZE - (col % TAB_SIZE);
			memset(expanded + col, ' ', spaces);
			col += spaces;
		} else {
			expanded[col++] = line[src];
		}
		src++;
	}
	expanded[col] = '\0';

	view->t_buf = malloc(col + 1);
	memcpy(view->t_buf, expanded, col + 1);
	view->t_bytes = col;
}

int main(int argc, char *argv[])
{
	if (argc == 1) {
		printf("TODO: argc == 1\n");
		return 1;
	}
	uint16_t term_row, term_col;
	get_termsize(&term_row, &term_col);

	// NOTE: only less 1 file for now
	char *buf = NULL;
	mapfile(argv[1], &buf);

	size_t line_count = str_count(buf, '\n');
	struct sv lines[line_count];
	char *temp_buf = buf;
	for (int i = 0; i < line_count; i++) {
		char *newline = strchrnul(temp_buf, '\n');
		lines[i] = (struct sv){ .buf = temp_buf,
					.bytes = newline - temp_buf,
					.t_buf = NULL,
					.t_bytes = 0 };

		expand_tab(&lines[i]);

		temp_buf = newline + 1;
	}

	// for (int i = 0; i < line_count; i++) {
	// 	char *buf_t = alloca(lines[i].bytes);
	// 	memcpy(buf_t, lines[i].buf, lines[i].bytes);
	// 	printf("%d: %.*s\n", str_count(buf_t, '\t') - 1, lines[i].bytes,
	// 	       lines[i].buf);
	// }

	// exit(EXIT_SUCCESS);

	atexit(exit_lesser);

	enter_alt_screen();
	enable_rawmode();
	hide_cursor();

	char c;
	size_t offset = 0;
	size_t topline = 0;
	bool upd_screen = true;
	bool upd_stl = true;
	uint8_t g_count = 0;
	do {
		if (upd_screen) {
			printf("%s", CSI("2J")); // Clear screen
			printf("%s", CSI("1;1H")); // Mov cursor to 1,1
			fflush(stdout);

			size_t curline = topline;
			for (int r = 0; r < term_row && curline < line_count;
			     r++) {
				if (lines[curline].t_bytes > term_col ||
				    lines[curline].bytes > term_col) {
					int len;
					char *temp_buf;
					if (lines[curline].t_bytes != 0) {
						len = lines[curline].t_bytes;
						temp_buf = lines[curline].t_buf;
					} else {
						len = lines[curline].bytes;
						temp_buf = lines[curline].buf;
					}

					size_t offset = 0;
					while (len > term_col &&
					       r < term_row - 1) {
						printf("%.*s\n", term_col,
						       temp_buf + offset);
						len -= term_col;
						offset += term_col;
						r++;
					}
					printf("%.*s\n",
					       (len > term_col) ? term_col :
								  len,
					       temp_buf + offset);
				} else {
					printf("%.*s\n", lines[curline].bytes,
					       lines[curline].buf);
				}
				curline++;
			}

			fflush(stdout);
			upd_screen = false;
			upd_stl = true; // Update stl
		}

		if (upd_stl) {
			printf(CSI("%d;1H"),
			       term_row); // Mov cursor to term_row,1
			printf("%s", CSI("7m")); // Invert
			printf(" %s ", (topline + term_row < line_count) ?
					       "MORE" :
					       "END");
			printf("%s", CSI("0m")); // Reset
			upd_stl = false;
		}

		c = getchar();

		char *buf_n = buf + offset;
		switch (c) {
		case 'j': {
			if (topline == line_count - 1)
				break;
			topline++;
			upd_screen = true;
		} break;

		case 'k': {
			if (topline == 0)
				break;
			topline--;
			upd_screen = true;
		} break;

		case 'd': {
			if (topline == line_count - 1)
				break;
			topline += term_row / 2;
			topline = MIN(line_count - 1, topline);
			upd_screen = true;
		} break;

		case 'u': {
			if (topline == 0)
				break;
			topline = topline <= term_row / 2 ?
					  0 :
					  topline - term_row / 2;
			upd_screen = true;
		} break;

		case 'g': {
			if (++g_count == 1)
				continue;
			topline = 0;
			upd_screen = true;
		} break;

		case 'G': {
			topline = line_count > term_row ?
					  line_count - term_row :
					  0;
			upd_screen = true;
		} break;
		}

		g_count = 0;
	} while (tolower(c) != 'q');
}
