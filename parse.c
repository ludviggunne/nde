#include "parse.h"
#include "syntax.h"
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_WORD 64

enum {
	TK_ERR = 1,
	TK_EOF,
	TK_NOT,
	TK_AND,
	TK_OR,
	TK_IMPL,
	TK_CON,
	TK_NAME,
	TK_LPAR,
	TK_RPAR,
};

struct pdata {
	const char *text;
	size_t length;
	size_t cursor;
	char *errbuf;
	size_t errbufsz;
	char word[MAX_WORD];
	int peek;
};

static char currc(struct pdata *p)
{
	if (p->cursor < p->length)
		return p->text[p->cursor];
	return 0;
}

static int wspc(char c)
{
	return c == ' ' || c == '\n' || c == '\t';
}

static void skip_wspc(struct pdata *p)
{
	char c;
	while ((c = currc(p)) && wspc(c)) {
		p->cursor++;
	}
}

static const char *getword(struct pdata *p)
{
	char c;
	size_t wc = 0;
	skip_wspc(p);
	if (!currc(p))
		return NULL;
	while ((c = currc(p)) && !wspc(c)) {
		if (wc == MAX_WORD) {
			snprintf(p->errbuf, p->errbufsz,
				 "too long word in input");
			return NULL;
		}
		p->word[wc++] = c;
		p->cursor++;
	}
	p->word[wc] = 0;
	return p->word;
}

static int getnum(struct pdata *p)
{
	char c;
	int x = 0;

	skip_wspc(p);
	if (!currc(p))
		return -1;
	while ((c = currc(p)) && isdigit(c)) {
		x *= 10;
		x += c - '0';
		p->cursor++;
	}
	return x;
}

static int match(const char *cnst, const char *scan, size_t s)
{
	return s == strlen(cnst) && strncmp(cnst, scan, s) == 0;
}

static int gettok(struct pdata *p)
{
	int tok;
	if (p->peek) {
		tok = p->peek;
		p->peek = 0;
		return tok;
	}

	skip_wspc(p);
	if (!currc(p) || currc(p) == ',')
		return TK_EOF;

	if (currc(p) == '(') {
		p->cursor++;
		return TK_LPAR;
	}

	if (currc(p) == ')') {
		p->cursor++;
		return TK_RPAR;
	}

	if (isalpha(currc(p))) {
		size_t wc = 0;
		while (currc(p) && isalpha(currc(p))) {
			if (wc == MAX_WORD) {
				snprintf(p->errbuf, p->errbufsz,
					 "too long name in formula");
				return TK_ERR;
			}
			p->word[wc++] = currc(p);
			p->cursor++;
		}
		p->word[wc] = 0;
		return TK_NAME;
	}

	const char *tokstr = &p->text[p->cursor];
	size_t len = 0;

	while (currc(p) && !wspc(currc(p))) {
		len++;
		p->cursor++;
		if (match(NOT_STR, tokstr, len))
			return TK_NOT;
		if (match(AND_STR, tokstr, len))
			return TK_AND;
		if (match(OR_STR, tokstr, len))
			return TK_OR;
		if (match(IMPL_STR, tokstr, len))
			return TK_IMPL;
		if (match(CON_STR, tokstr, len))
			return TK_CON;
	}

	return TK_ERR;
}

static int peektok(struct pdata *p)
{
	if (!p->peek)
		p->peek = gettok(p);
	return p->peek;
}

static struct ast *p_cmd(struct pdata *p);
static struct ast *p_rule(struct pdata *p);
static struct ast *p_input(struct pdata *p);
static struct ast *p_form(struct pdata *p);
static struct ast *p_impl(struct pdata *p);
static struct ast *p_andor(struct pdata *p);
static struct ast *p_unit(struct pdata *p);

struct ast *parse(const char *text, size_t length, char *errbuf,
		  size_t errbufsz)
{
	struct ast *root;
	struct pdata p = { 0 };
	p.text = text;
	p.length = length;
	p.errbuf = errbuf;
	p.errbufsz = errbufsz;

	root = p_cmd(&p);
	if (!root)
		return NULL;

	skip_wspc(&p);
	if (currc(&p)) {
		ast_destroy(root);
		root = NULL;
		snprintf(errbuf, errbufsz, "trailing tokens");
	}

	return root;
}

static struct ast *p_cmd(struct pdata *p)
{
	struct ast *cmd = NULL, *lhs = NULL, *rhs = NULL;
	char *text = NULL;
	int type = -1;

	const char *word = getword(p);
	if (!word)
		return NULL;

	if (strcmp(word, "presume") == 0) {
		type = CMD_PRESUME;
		lhs = p_form(p);
		if (!lhs)
			return NULL;
		goto done;
	}

	if (strcmp(word, "assume") == 0) {
		type = CMD_ASSUME;
		lhs = p_form(p);
		if (!lhs)
			return NULL;
		goto done;
	}

	if (strcmp(word, "open") == 0) {
		type = CMD_OPEN;
		goto done;
	}

	if (strcmp(word, "close") == 0) {
		type = CMD_CLOSE;
		goto done;
	}

	if (strcmp(word, "export") == 0) {
		type = CMD_EXPORT;
		text = (char *)getword(p);
		if (!text)
			return NULL;
		text = strdup(text);
		goto done;
	}

	if (strcmp(word, "apply") == 0) {
		type = CMD_APPLY;
		lhs = p_rule(p);
		if (!lhs)
			return NULL;

		skip_wspc(p);
		if (!currc(p))
			goto done;

		rhs = p_input(p);
		if (!rhs) {
			ast_destroy(lhs);
			return NULL;
		}
		goto done;
	}

	snprintf(p->errbuf, p->errbufsz, "unknown command %s", word);
	return NULL;

 done:
	cmd = calloc(1, sizeof(*cmd));
	cmd->type = type;
	cmd->text = text;
	cmd->lhs = lhs;
	cmd->rhs = rhs;
	return cmd;
}

static struct ast *p_rule(struct pdata *p)
{
	struct ast *rule = NULL;
	int type = -1;
	const char *word;

	word = getword(p);
	if (strcmp(word, NOT_INTR_STR) == 0) {
		type = RULE_NOT_INTR;
		goto done;
	}

	if (strcmp(word, NOT_ELIM_STR) == 0) {
		type = RULE_NOT_ELIM;
		goto done;
	}

	if (strcmp(word, AND_INTR_STR) == 0) {
		type = RULE_AND_INTR;
		goto done;
	}

	if (strcmp(word, AND_ELIM_1_STR) == 0) {
		type = RULE_AND_ELIM_1;
		goto done;
	}

	if (strcmp(word, AND_ELIM_2_STR) == 0) {
		type = RULE_AND_ELIM_2;
		goto done;
	}

	if (strcmp(word, OR_INTR_1_STR) == 0) {
		type = RULE_OR_INTR_1;
		goto done;
	}

	if (strcmp(word, OR_INTR_2_STR) == 0) {
		type = RULE_OR_INTR_2;
		goto done;
	}

	if (strcmp(word, OR_ELIM_STR) == 0) {
		type = RULE_OR_ELIM;
		goto done;
	}

	if (strcmp(word, IMPL_INTR_STR) == 0) {
		type = RULE_IMPL_INTR;
		goto done;
	}

	if (strcmp(word, IMPL_ELIM_STR) == 0) {
		type = RULE_IMPL_ELIM;
		goto done;
	}

	if (strcmp(word, CON_ELIM_STR) == 0) {
		type = RULE_CON_ELIM;
		goto done;
	}

	if (strcmp(word, NOT_NOT_INTR_STR) == 0) {
		type = RULE_NOT_NOT_INTR;
		goto done;
	}

	if (strcmp(word, NOT_NOT_ELIM_STR) == 0) {
		type = RULE_NOT_NOT_ELIM;
		goto done;
	}

	if (strcmp(word, MT_STR) == 0) {
		type = RULE_MT;
		goto done;
	}

	if (strcmp(word, PBC_STR) == 0) {
		type = RULE_PBC;
		goto done;
	}

	if (strcmp(word, LEM_STR) == 0) {
		type = RULE_LEM;
		goto done;
	}

	if (strcmp(word, COPY_STR) == 0) {
		type = RULE_COPY;
		goto done;
	}

	snprintf(p->errbuf, p->errbufsz, "unknown rule %s", word);
	return NULL;

 done:
	rule = calloc(1, sizeof(*rule));
	rule->type = type;
	return rule;
}

static struct ast *p_input(struct pdata *p)
{
	int start = 0, end = 0, type;
	struct ast *inp, *lhs = NULL, *rhs = NULL;

	skip_wspc(p);
	if (!isdigit(currc(p))) {
		lhs = p_form(p);
		if (!lhs)
			return NULL;
		type = INPUT_FORM;
	} else {
		type = INPUT_LINE;
		start = getnum(p);
		if (start < 0) {
			snprintf(p->errbuf, p->errbufsz,
				 "invalid rule input syntax");
			return NULL;
		}

		skip_wspc(p);
		if (currc(p) == '-') {
			p->cursor++;
			end = getnum(p);
			if (end < 0) {
				snprintf(p->errbuf, p->errbufsz,
					 "invalid rule input syntax");
				return NULL;
			}
			type = INPUT_BOX;
		}
	}

	skip_wspc(p);
	if (currc(p) == ',') {
		p->cursor++;
		rhs = p_input(p);
		if (!rhs)
			return NULL;
	}

	inp = calloc(1, sizeof(*inp));
	inp->type = type;
	inp->lhs = lhs;
	inp->rhs = rhs;
	inp->start = start - 1;
	inp->end = end - 1;
	return inp;
}

static struct ast *p_form(struct pdata *p)
{
	return p_impl(p);
}

static struct ast *p_impl(struct pdata *p)
{
	struct ast *impl, *lhs, *rhs;
	int tok;

	lhs = p_andor(p);
	if (!lhs)
		return NULL;

	tok = peektok(p);

	if (tok == TK_ERR) {
		snprintf(p->errbuf, p->errbufsz, "syntax error in formula");
		ast_destroy(lhs);
		return NULL;
	}

	if (tok == TK_EOF)
		return lhs;

	if (tok == TK_IMPL) {
		(void)gettok(p);
		rhs = p_impl(p);
		if (!rhs) {
			ast_destroy(lhs);
			return NULL;
		}
		impl = calloc(1, sizeof(*impl));
		impl->type = FORM_IMPL;
		impl->lhs = lhs;
		impl->rhs = rhs;
		return impl;
	}

	return lhs;
}

static struct ast *p_andor(struct pdata *p)
{
	struct ast *andor, *lhs, *rhs;
	int tok;

	lhs = p_unit(p);
	if (!lhs)
		return NULL;

	tok = peektok(p);

	if (tok == TK_ERR) {
		snprintf(p->errbuf, p->errbufsz, "syntax error in formula");
		ast_destroy(lhs);
		return NULL;
	}

	if (tok == TK_EOF)
		return lhs;

	if (tok == TK_AND || tok == TK_OR) {
		(void)gettok(p);
		rhs = p_andor(p);
		if (!rhs) {
			ast_destroy(lhs);
			return NULL;
		}
		andor = calloc(1, sizeof(*andor));
		andor->type = tok == TK_AND ? FORM_AND : FORM_OR;
		andor->lhs = lhs;
		andor->rhs = rhs;
		return andor;
	}

	return lhs;
}

static struct ast *p_unit(struct pdata *p)
{
	struct ast *unit, *child;
	int tok;

	tok = peektok(p);

	if (tok == TK_ERR || tok == TK_EOF) {
		snprintf(p->errbuf, p->errbufsz, "syntax error in formula");
		return NULL;
	}

	if (tok == TK_NAME) {
		unit = calloc(1, sizeof(*child));
		unit->type = FORM_NAME;
		unit->text = strdup(p->word);
		(void)gettok(p);
		return unit;
	}

	if (tok == TK_CON) {
		unit = calloc(1, sizeof(*child));
		unit->type = FORM_CON;
		(void)gettok(p);
		return unit;
	}

	if (tok == TK_LPAR) {
		(void)gettok(p);
		child = p_form(p);
		if (!child)
			return NULL;
		tok = gettok(p);
		if (tok != TK_RPAR) {
			snprintf(p->errbuf, p->errbufsz,
				 "syntax error in formula");
			ast_destroy(child);
			return NULL;
		}
		return child;
	}

	if (tok == TK_NOT) {
		(void)gettok(p);
		child = p_unit(p);
		if (!child)
			return NULL;
		unit = calloc(1, sizeof(*unit));
		unit->type = FORM_NOT;
		unit->lhs = child;
		return unit;
	}

	snprintf(p->errbuf, p->errbufsz, "syntax error in formula");
	return NULL;
}

struct ast *ast_form(struct ast *cmd)
{
	assert(cmd->type & (CMD_PRESUME | CMD_ASSUME));
	return cmd->lhs;
}

int ast_rule(struct ast *cmd)
{
	assert(cmd->type & CMD_APPLY);
	return cmd->lhs->type;
}

struct ast *ast_rule_input(struct ast *cmd, size_t n)
{
	assert(cmd->type == CMD_APPLY);
	struct ast *inp = cmd->rhs;
	while (n && inp) {
		n--;
		inp = inp->rhs;
	}
	return inp;
}

struct ast *ast_copy(struct ast *ast)
{
	struct ast *cpy = malloc(sizeof(*cpy));
	memcpy(cpy, ast, sizeof(*cpy));
	if (ast->text)
		cpy->text = strdup(ast->text);
	if (ast->lhs)
		cpy->lhs = ast_copy(ast->lhs);
	if (ast->rhs)
		cpy->rhs = ast_copy(ast->rhs);
	return cpy;
}

int ast_equal(struct ast *ast1, struct ast *ast2)
{
	if (ast1->type != ast2->type)
		return 0;
	if (ast1->text && ast2->text && strcmp(ast1->text, ast2->text) != 0)
		return 0;
	if (ast1->lhs && ast2->lhs && !ast_equal(ast1->lhs, ast2->lhs))
		return 0;
	if (ast1->rhs && ast2->rhs && !ast_equal(ast1->rhs, ast2->rhs))
		return 0;
	return 1;
}

void ast_destroy(struct ast *ast)
{
	if (!ast)
		return;
	free(ast->text);
	ast_destroy(ast->lhs);
	ast_destroy(ast->rhs);
	free(ast);
}

size_t print_form(struct ast *form, char *buf, size_t s)
{
	assert(is_form(form->type));

	char lbuf[64], rbuf[64];

	switch (form->type) {
	case FORM_NOT:
		print_form(form->lhs, lbuf, sizeof(lbuf));
		return snprintf(buf, s, NOT_STR "%s", lbuf);
	case FORM_AND:
		print_form(form->lhs, lbuf, sizeof(lbuf));
		print_form(form->rhs, rbuf, sizeof(rbuf));
		return snprintf(buf, s, "(%s " AND_STR " %s)", lbuf, rbuf);
	case FORM_OR:
		print_form(form->lhs, lbuf, sizeof(lbuf));
		print_form(form->rhs, rbuf, sizeof(rbuf));
		return snprintf(buf, s, "(%s " OR_STR " %s)", lbuf, rbuf);
		break;
	case FORM_IMPL:
		print_form(form->lhs, lbuf, sizeof(lbuf));
		print_form(form->rhs, rbuf, sizeof(rbuf));
		return snprintf(buf, s, "(%s " IMPL_STR " %s)", lbuf, rbuf);
		break;
	case FORM_CON:
		return snprintf(buf, s, CON_STR);
	case FORM_NAME:
		return snprintf(buf, s, "%s", form->text);
	default:
		assert(0 && "unknown form type");
	}
}

static const char *rulestr(int r)
{
	switch (r) {
	case RULE_NOT_INTR:
		return NOT_INTR_STR;
	case RULE_NOT_ELIM:
		return NOT_ELIM_STR;
	case RULE_AND_INTR:
		return AND_INTR_STR;
	case RULE_AND_ELIM_1:
		return AND_ELIM_1_STR;
	case RULE_AND_ELIM_2:
		return AND_ELIM_2_STR;
	case RULE_OR_INTR_1:
		return OR_INTR_1_STR;
	case RULE_OR_INTR_2:
		return OR_INTR_2_STR;
	case RULE_OR_ELIM:
		return OR_ELIM_STR;
	case RULE_IMPL_INTR:
		return IMPL_INTR_STR;
	case RULE_IMPL_ELIM:
		return IMPL_ELIM_STR;
	case RULE_CON_ELIM:
		return CON_ELIM_STR;
	case RULE_NOT_NOT_INTR:
		return NOT_NOT_INTR_STR;
	case RULE_NOT_NOT_ELIM:
		return NOT_NOT_ELIM_STR;
	case RULE_MT:
		return MT_STR;
	case RULE_PBC:
		return PBC_STR;
	case RULE_LEM:
		return LEM_STR;
	case RULE_COPY:
		return COPY_STR;
	default:
		return NULL;
	}
}

static size_t print_inp(struct ast *inp, char *buf, size_t s)
{
	char ibuf[s];
	char fbuf[s];

	assert(is_input(inp->type));

	if (inp->rhs)
		print_inp(inp->rhs, ibuf, s);

	switch (inp->type) {
	case INPUT_LINE:
		if (inp->rhs) {
			return snprintf(buf, s, "%d, %s", inp->start + 1, ibuf);
		} else {
			return snprintf(buf, s, "%d", inp->start + 1);
		}
		break;
	case INPUT_BOX:
		if (inp->rhs) {
			return snprintf(buf, s, "%d-%d, %s", inp->start + 1,
					inp->end + 1, ibuf);
		} else {
			return snprintf(buf, s, "%d-%d", inp->start + 1,
					inp->end + 1);
		}
		break;
	case INPUT_FORM:
		print_form(inp->lhs, fbuf, sizeof(fbuf));
		if (inp->rhs) {
			return snprintf(buf, s, "%s, %s", fbuf, ibuf);
		} else {
			return snprintf(buf, s, "%s", fbuf);
		}
	default:
		assert(0);
	}
}

size_t print_apply(struct ast *cmd, char *buf, size_t s)
{
	char ibuf[s];

	assert(cmd->type == CMD_APPLY);

	if (cmd->rhs) {
		print_inp(cmd->rhs, ibuf, s);
		return snprintf(buf, s, "%s %s", rulestr(cmd->lhs->type), ibuf);
	} else {
		return snprintf(buf, s, "%s", rulestr(cmd->lhs->type));
	}
}
