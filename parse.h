#ifndef PARSE_H
#define PARSE_H

#include <stddef.h>

// TODO: dont use flags
#define FORM_NOT          (1ul << 0)
#define FORM_AND          (1ul << 1)
#define FORM_OR           (1ul << 2)
#define FORM_IMPL         (1ul << 3)
#define FORM_CON          (1ul << 4)
#define FORM_NAME         (1ul << 5)
#define CMD_PRESUME       (1ul << 6)
#define CMD_ASSUME        (1ul << 7)
#define CMD_OPEN          (1ul << 8)
#define CMD_CLOSE         (1ul << 9)
#define CMD_APPLY         (1ul << 10)
#define CMD_UNDO          (1ul << 11)
#define INPUT_LINE        (1ul << 12)
#define INPUT_BOX         (1ul << 13)
#define INPUT_FORM        (1ul << 14)
#define RULE_NOT_INTR     (1ul << 15)
#define RULE_NOT_ELIM     (1ul << 16)
#define RULE_AND_INTR     (1ul << 17)
#define RULE_AND_ELIM_1   (1ul << 18)
#define RULE_AND_ELIM_2   (1ul << 19)
#define RULE_OR_INTR_1    (1ul << 20)
#define RULE_OR_INTR_2    (1ul << 21)
#define RULE_OR_ELIM      (1ul << 22)
#define RULE_IMPL_INTR    (1ul << 23)
#define RULE_IMPL_ELIM    (1ul << 24)
#define RULE_CON_ELIM     (1ul << 25)
#define RULE_NOT_NOT_INTR (1ul << 26)
#define RULE_NOT_NOT_ELIM (1ul << 27)
#define RULE_MT           (1ul << 28)
#define RULE_PBC          (1ul << 29)
#define RULE_LEM          (1ul << 30)
#define RULE_COPY         (1ul << 31)

#define FORM  (FORM_NOT | FORM_AND | FORM_OR | FORM_IMPL | FORM_CON | FORM_NAME)
#define CMD (CMD_PRESUME | CMD_ASSUME | CMD_OPEN | CMD_CLOSE | CMD_APPLY | CMD_UNDO)
#define INPUT (INPUT_LINE | INPUT_BOX | INPUT_FORM)
#define RULE (RULE_NOT_INTR | RULE_NOT_ELIM | RULE_AND_INTR | RULE_AND_ELIM_1 | \
	RULE_AND_ELIM_2 | RULE_OR_INTR_1 | RULE_OR_INTR_2 | RULE_OR_ELIM | \
	RULE_IMPL_INTR | RULE_IMPL_ELIM | RULE_CON_ELIM | RULE_NOT_NOT_INTR | \
	RULE_NOT_NOT_ELIM | RULE_MT | RULE_PBC | RULE_LEM)

struct ast {
	long int type;
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
