// B lang : C lang
// ? : bash
// bash but worst
// INFO: maybe "crash" in future, futuristic name

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
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
typedef struct DbVoidPtrVector cmd_v;

#define V_INIT { .size = 0, .arr = NULL, .capacity = 0 }
#define CHAR_V_INIT (char_v) V_INIT
#define CMD_V_INIT (cmd_v) V_INIT

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
				if (v->arr[--v->size] != NULL) \
					free(v->arr[v->size]); \
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

#define L_M_PROMPT 256

struct cmd {
	char_v argv;
	int infd, outfd, errfd;
};

static struct cmd *alloc_cmd()
{
	struct cmd *node = malloc(sizeof(struct cmd));
	node->argv = CHAR_V_INIT;
	node->infd = STDIN_FILENO;
	node->outfd = STDOUT_FILENO;
	node->errfd = STDERR_FILENO;
	return node;
}

static void free_cmd_v(cmd_v *cmds)
{
	if (!cmds)
		return;
	for (int i = 0; i < cmds->size; i++) {
		char_v *argv = &(((struct cmd *)cmds->arr[i])->argv);
		v_free_arr(argv);
	}
	free(cmds);
}

static pid_t cash_exec(char_v *tok_v)
{
	char **argv = (char **)tok_v->arr;

	pid_t child = fork();
	if (child == -1) {
		fprintf(stderr, "(%d): %s\n", errno, strerror(errno));
	} else if (child == 0) {
		execvp(argv[0], argv);
		fprintf(stderr, "(%d): %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	return child;
}

static void init_pipes(cmd_v *cmds)
{
	if (!cmds)
		return;
	for (int i = 0; i < cmds->size - 1; i++) {
		int fd[2];
		pipe(fd);

		struct cmd *cmd_left = ((struct cmd *)(cmds->arr[i]));
		struct cmd *cmd_right = ((struct cmd *)(cmds->arr[i + 1]));
		cmd_left->outfd = fd[1];
		cmd_right->infd = fd[0];
	}
}

static void cash_cmd_exec(cmd_v *cmds, int i)
{
	struct cmd *cmd = cmds->arr[i];
	if (fork() == 0) {
		if (cmd->infd != STDIN_FILENO)
			dup2(cmd->infd, STDIN_FILENO);
		if (cmd->outfd != STDOUT_FILENO)
			dup2(cmd->outfd, STDOUT_FILENO);
		if (cmd->errfd != STDERR_FILENO)
			dup2(cmd->errfd, STDERR_FILENO);

		for (int j = 0; j < cmds->size; j++) {
			struct cmd *c = cmds->arr[j];
			if (c->infd != STDIN_FILENO)
				close(c->infd);
			if (c->outfd != STDOUT_FILENO)
				close(c->outfd);
		}

		execvp(cmd->argv.arr[0], (char **)cmd->argv.arr);
		perror("execvp");
		exit(1);
	}
}

static void cash_exec_pipeline(cmd_v *cmds)
{
	init_pipes(cmds);

	for (int i = 0; i < cmds->size; i++) {
		cash_cmd_exec(cmds, i);
	}

	for (int i = 0; i < cmds->size; i++) {
		struct cmd *cmd = ((struct cmd *)(cmds->arr[i]));
		if (cmd->infd != STDIN_FILENO)
			close(cmd->infd);
		if (cmd->outfd != STDOUT_FILENO)
			close(cmd->outfd);
		if (cmd->errfd != STDERR_FILENO)
			close(cmd->errfd);
	}

	for (int i = 0; i < cmds->size; i++) {
		wait(NULL);
	}
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
			v_push_str(tok_v, tok_str);
			tok_open = false;
		}

		tok_str = strtok_r(NULL, "\"", &tok_r1);
		tok_open = *tok_r1 != '\0';
	}

	return tok_v;
}

// Tokenize for pipes
static cmd_v *tokens_pipe(char *in)
{
	cmd_v *cmds;
	v_new(cmd_v, cmds);

	char *tok_p = NULL;
	char *tok_str = strtok_r(in, "|", &tok_p);
	do {
		struct cmd *new_cmd = alloc_cmd();
		char_v *argv = tokens_line(tok_str);
		if (argv == NULL) {
			free_cmd_v(cmds);
			return NULL;
		}
		v_push(argv, NULL); // For execv
		new_cmd->argv = *argv;
		v_push(cmds, new_cmd);
		tok_str = strtok_r(NULL, "|", &tok_p);
	} while (tok_str != NULL);

	return cmds;
}

static bool cash_cd(char_v *tok_v)
{
	int eval = chdir(tok_v->arr[1]);
	if (eval == -1)
		fprintf(stderr, "(%d): %s\n", errno, strerror(errno));
	return eval != -1;
}

static void get_prompt(char prompt[L_M_PROMPT])
{
	static char host_name[128] = "\0";
	if (host_name[0] == '\0')
		gethostname(host_name, 128);

	memset(prompt, '\0', L_M_PROMPT);
	strcat(prompt, host_name);
	strcat(prompt, "'s cash@");

	size_t l_prompt = strlen(prompt);
	getcwd(prompt + l_prompt, L_M_PROMPT - l_prompt);
	if (strlen(prompt) < L_M_PROMPT - 3) {
		strcat(prompt, " $ ");
	}
}

int main()
{
	int8_t exitcode = 0;
	bool cash_exit = false;

	char prompt[L_M_PROMPT];
	bool upd_prompt = true;

	while (!cash_exit) {
		if (upd_prompt) {
			get_prompt(prompt);
			upd_prompt = false;
		}
		char *in = readline(prompt);

		cmd_v *cmds = tokens_pipe(in);
		if (cmds == NULL)
			goto cleanup;

		// printf("cmds.size: %d\n", cmds->size);
		for (int i = 0; i < cmds->size; i++) {
			// printf("Instruction %d: ",
			//        ((char_v *)cmds->arr[i])->size);
			fflush(stdout);
			for (int j = 0; j < ((char_v *)cmds->arr[i])->size;
			     j++) {
				// printf("%s+",
				//        (char *)((char_v *)cmds->arr[i])->arr[j]);
			}
			// printf("\n");
		}

		char_v *tok_v = &((struct cmd *)cmds->arr[0])->argv;
		if (tok_v == NULL)
			goto cleanup;

		// TODO: These aren't piped... Also size now includes NULL
		if (strcmp(tok_v->arr[0], "cd") == 0 &&
		    tok_v->size == 2 + /*NULL*/ 1) {
			if (cash_cd(tok_v))
				upd_prompt = true;
			goto cleanup;

		} else if (strcmp(tok_v->arr[0], "exit") == 0 &&
			   tok_v->size <= 2 + 1) {
			errno = 0;
			long eval = tok_v->size == 1 ?
					    0 :
					    strtol(tok_v->arr[1], NULL, 0);
			if (errno == ERANGE) {
				fprintf(stderr, "(%d): %s\n", errno,
					strerror(errno));
				goto cleanup;
			}
			if (eval < INT8_MIN || eval > INT8_MAX) {
				fprintf(stderr, "Out of range: %ld\n", eval);
				goto cleanup;
			}

			exitcode = eval;
			cash_exit = true;
			goto cleanup;
		}

		// pid_t child = cash_exec(tok_v);
		// if (child != -1)
		// 	waitpid(child, NULL, 0);

		cash_exec_pipeline(cmds);

cleanup:
		free(in);
		free_cmd_v(cmds);
	}

	return exitcode;
}

// vim: foldmethod=marker
