#ifndef PARSE_H
#define PARSE_H

#include <stddef.h>

enum {
	FORM_NOT,
	FORM_AND,
	FORM_OR,
	FORM_IMPL,
	FORM_CON,
	FORM_NAME,
	CMD_PRESUME,
	CMD_ASSUME,
	CMD_OPEN,
	CMD_CLOSE,
	CMD_APPLY,
	CMD_EXPORT,
	INPUT_LINE,
	INPUT_BOX,
	INPUT_FORM,
	RULE_NOT_INTR,
	RULE_NOT_ELIM,
	RULE_AND_INTR,
	RULE_AND_ELIM_1,
	RULE_AND_ELIM_2,
	RULE_OR_INTR_1,
	RULE_OR_INTR_2,
	RULE_OR_ELIM,
	RULE_IMPL_INTR,
	RULE_IMPL_ELIM,
	RULE_CON_ELIM,
	RULE_NOT_NOT_INTR,
	RULE_NOT_NOT_ELIM,
	RULE_MT,
	RULE_PBC,
	RULE_LEM,
	RULE_COPY,
};

static inline int is_form(int type)
{
	switch (type) {
	case FORM_NOT:
	case FORM_AND:
	case FORM_OR:
	case FORM_IMPL:
	case FORM_CON:
	case FORM_NAME:
		return 1;
	default:
		return 0;
	}
}

static inline int is_cmd(int type)
{
	switch (type) {
	case CMD_PRESUME:
	case CMD_ASSUME:
	case CMD_OPEN:
	case CMD_CLOSE:
	case CMD_APPLY:
	case CMD_EXPORT:
		return 1;
	default:
		return 0;
	}
}

static inline int is_input(int type)
{
	switch (type) {
	case INPUT_LINE:
	case INPUT_BOX:
	case INPUT_FORM:
		return 1;
	default:
		return 0;
	}
}

static inline int is_rule(int type)
{
	switch (type) {
	case RULE_NOT_INTR:
	case RULE_NOT_ELIM:
	case RULE_AND_INTR:
	case RULE_AND_ELIM_1:
	case RULE_AND_ELIM_2:
	case RULE_OR_INTR_1:
	case RULE_OR_INTR_2:
	case RULE_OR_ELIM:
	case RULE_IMPL_INTR:
	case RULE_IMPL_ELIM:
	case RULE_CON_ELIM:
	case RULE_NOT_NOT_INTR:
	case RULE_NOT_NOT_ELIM:
	case RULE_MT:
	case RULE_PBC:
	case RULE_LEM:
		return 1;
	default:
		return 0;
	}
}

struct ast {
	int type;
	char *text;
	struct ast *lhs;
	struct ast *rhs;
	int start;
	int end;
};

struct ast *parse(const char *text, size_t length, char *errbuf,
		  size_t errbuf_length);
struct ast *ast_form(struct ast *cmd);
int ast_rule(struct ast *cmd);
struct ast *ast_rule_input(struct ast *cmd, size_t n);
struct ast *ast_copy(struct ast *ast);
int ast_equal(struct ast *ast1, struct ast *ast2);
void ast_destroy(struct ast *ast);
size_t print_form(struct ast *form, char *buf, size_t s);
size_t print_apply(struct ast *cmd, char *buf, size_t s);

#endif
