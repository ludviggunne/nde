#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include "parse.h"
#include "linenoise.h"
#include "proof.h"
#include "apply.h"
#include "log.h"

#define ERROR "\x1b[31merror:\x1b[0m "
#define CLEAR "\x1b[2K\r"

struct termios old, new;
const char *boxlines = "| | | | | | | | | | | | | | | | | | | | | ";
const int annotcol = 80;

void term_restore(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &old);
}

void error(const char *msg)
{
	printf(ERROR "%s", msg);
	if (!isatty(STDIN_FILENO)) {
		printf("\n");
		exit(1);
	}
	(void)fgetc(stdin);
	printf(CLEAR);
	fflush(stdout);
}

char first_non_wspc(const char *str)
{
	while (*str && (*str == ' ' || *str == '\t' || *str == '\n'))
		str++;
	return *str;
}

void println(const char *prompt, const char *formstr, const char *annot)
{
	printf("%s%s\x1b[%dG%s\n", prompt, formstr, annotcol, annot);
	fflush(stdout);
}

int main(int argc, char **argv)
{
	if (argc > 1) {
		if (!ndelog_init(argv[1])) {
			perror("log_init");
			return 1;
		}
	}

	char prompt[32];
	char *line;
	char formbuf[512];
	char cmdbuf[512];
	char errbuf[1024];
	struct ast *cmd;
	struct proof p;

	// disable echoing
	(void)tcgetattr(STDIN_FILENO, &old);
	memcpy(&new, &old, sizeof(struct termios));
	new.c_lflag &= ~ECHO;
	(void)tcsetattr(STDIN_FILENO, TCSANOW, &new);
	atexit(term_restore);

	p = new_proof();

	linenoiseHistorySetMaxLen(100);

	ndelog("starting NDE\n");
	for (;;) {

		snprintf(prompt, sizeof(prompt), "%4d. %.*s",
			 p.nlns + 1, 2 * box_depth(&p), boxlines);

		line = linenoise(prompt);
		if (!line)
			break;

		char first = first_non_wspc(line);
		if (!first || first == '#') {
			printf(CLEAR);
			fflush(stdout);
			linenoiseFree(line);
			continue;
		}

		linenoiseHistoryAdd(line);

		printf(CLEAR);
		fflush(stdout);

		cmd = parse(line, strlen(line), errbuf, sizeof(errbuf));

		if (!cmd) {
			error(errbuf);
			linenoiseFree(line);
			continue;
		}

		switch (cmd->type) {
		case CMD_OPEN:
			memset(prompt, ' ', 5);
			snprintf(prompt, sizeof(prompt), "      %.*s",
				 2 * box_depth(&p), boxlines);
			println(prompt, "", "");
			push_box(&p);
			break;
		case CMD_CLOSE:
			if (!pop_box(&p))
				error("no boxes to close");
			else {
				snprintf(prompt, sizeof(prompt), "      %.*s",
					 2 * box_depth(&p), boxlines);
				println(prompt, "", "");
			}
			break;
		case CMD_PRESUME:
			pushln(&p, cmd, cmd->lhs);
			print_form(cmd->lhs, formbuf, sizeof(formbuf));
			println(prompt, formbuf, "premise");
			break;
		case CMD_ASSUME:
			if (!at_beginning_of_box(&p)) {
				error
				    ("assumption must appear at beginning of box");
				break;
			}
			pushln(&p, cmd, cmd->lhs);
			print_form(cmd->lhs, formbuf, sizeof(formbuf));
			println(prompt, formbuf, "assumption");
			break;
		case CMD_APPLY:
			if (!apply_rule(&p, cmd)) {
				snprintf(errbuf, sizeof(errbuf),
					 "\x1b[33m\"%s\"\x1b[0m, unable to apply rule: %s",
					 line, p.errbuf);
				error(errbuf);
				break;
			}
			print_apply(cmd, cmdbuf, sizeof(cmdbuf));
			print_form(p.lns[p.nlns - 1].form, formbuf,
				   sizeof(formbuf));
			println(prompt, formbuf, cmdbuf);
			break;
		case CMD_UNDO:
			// TODO: pop boxes when needed
			if (p.nlns > 0) {
				p.nlns--;
				printf(CLEAR "\x1b[1A" CLEAR);
				fflush(stdout);
			}
			break;
		}
		linenoiseFree(line);
	}

	if (!isatty(STDIN_FILENO)) {
		printf(CLEAR "\x1b[32mThe proof is correct.\x1b[0m\n");
	}

	printf(CLEAR);
}
