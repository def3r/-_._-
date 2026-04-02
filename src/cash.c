// B lang : C lang
// ? : bash
// bash but worst
// INFO: maybe "crash" in future, futuristic name

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include <editline.h>

// vector impl {{{
struct DbVoidPtrVector {
	int size;
	int capacity;
	void **arr;
};

typedef struct DbVoidPtrVector char_v;

#define v_new(vType, v)                              \
	do {                                         \
		v = malloc(sizeof(vType));           \
		if (v != NULL) {                     \
			memset(v, 0, sizeof(vType)); \
		}                                    \
	} while (0)

#define v_free(v)                     \
	do {                          \
		if (v != NULL) {      \
			free(v->arr); \
			free(v);      \
			v = NULL;     \
		}                     \
	} while (0)

#define v_free_arr(v)                                          \
	do {                                                   \
		if (v != NULL) {                               \
			while (v->size) {                      \
				if (v->arr[v->size] != NULL)   \
					free(v->arr[v->size]); \
				v->size--;                     \
			}                                      \
			free(v->arr);                          \
			free(v);                               \
			v = NULL;                              \
		}                                              \
	} while (0)

#define v_push(v, c)                                                          \
	do {                                                                  \
		if (v != NULL) {                                              \
			if (v->size == v->capacity) {                         \
				v->capacity = v->capacity == 0 ? 1 :          \
								 v->capacity; \
				void *newArr = malloc(sizeof(*v->arr) * 2 *   \
						      v->capacity);           \
				if (v->arr) {                                 \
					memcpy(newArr, v->arr,                \
					       sizeof(*v->arr) * v->size);    \
					free(v->arr);                         \
				}                                             \
				v->arr = newArr;                              \
				v->capacity *= 2;                             \
			}                                                     \
			v->arr[v->size++] = c;                                \
		}                                                             \
	} while (0)

#define v_push_str(v, c)                          \
	do {                                      \
		if (v != NULL) {                  \
			char *c_copy = strdup(c); \
			v_push(v, c_copy);        \
		}                                 \
	} while (0)
// }}}

static char *sh_prompt = "cash > ";

static pid_t sh_exec(char *argv[])
{
	pid_t child = fork();
	if (child == -1) {
		fprintf(stderr, "(%d): %s\n", errno, strerror(errno));
	} else if (child == 0) {
		execvp(argv[0], argv);
		fprintf(stderr, "(%d): %s\n", errno, strerror(errno));
		return errno;
	}
	return child;
}

static void tokens_wspc(char_v *tok_v, char *in)
{
	char *tok_r = NULL;
	char *tok_wspc = strtok_r(in, " ", &tok_r);
	while (tok_wspc != NULL) {
		v_push_str(tok_v, tok_wspc);
		tok_wspc = strtok_r(NULL, " ", &tok_r);
	}
}

// First tokenize for "", then for whitespace
static char_v *tokens_line(char *in)
{
	char_v *tok_v;
	v_new(char_v, tok_v);

	if (strchr(in, '"') == NULL) {
		tokens_wspc(tok_v, in);
		return tok_v;
	}

	// TODO:
	// 1. Issue with strtok is that it doesn't care about the length of the
	// delim substr found, ex: " and """ and """"" all just become " so
	// consecutive unmatched delims can be parsed incorrectly.
	// 2. thus is the case for start and end of the line: ""abc is error, "this is also error"
	char *tok_r1 = NULL;
	char *tok_str = strtok_r(in, "\"", &tok_r1);
	bool tok_open = true;
	while (tok_str != NULL) {
		tokens_wspc(tok_v, tok_str);

		if (tok_open) {
			tok_str = strtok_r(NULL, "\"", &tok_r1);
			if (*tok_r1 == '\0' && *(tok_r1 - 1) != '\0') {
				fprintf(stderr, "Missing \"\n");
				v_free_arr(tok_v);
				return NULL;
			}
			tok_open = false;
		}

		tok_str = strtok_r(NULL, "\"", &tok_r1);
		tok_open = *tok_r1 != '\0';
	}

	return tok_v;
}

int main()
{
	while (1) {
		char *in = readline(sh_prompt);

		char_v *tok_v = tokens_line(in);
		if (tok_v == NULL) {
			goto cleanup;
		}
		v_push(tok_v, NULL);

		pid_t child = sh_exec((char **)tok_v->arr);
		if (child != -1) {
			waitpid(child, NULL, 0);
		}

cleanup:
		free(in);
		v_free_arr(tok_v);
	}
}

// vim foldmethod=marker
