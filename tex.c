#include "tex.h"
#include "parse.h"
#include <assert.h>

// *INDENT-OFF*
const char *preamble = 
"\\documentclass{article}\n"
"\\usepackage{logicproof}\n"
"\\usepackage{amssymb}\n"
"\\begin{document}\n"
"\\begin{logicproof}{%d}\n";

const char *postamble =
"\\end{logicproof}\n"
"\\end{document}\n";
// *INDENT-ON*

static int println(FILE * f, struct ln *ln, int last_was_ln);
static int printform(FILE * f, struct ast *form, int);
static int printcmd(FILE * f, struct ast *cmd);
static int printinps(FILE * f, struct ast *inps);
static int prec(int type);
static int get_max_depth(struct proof *p);

int export_tex(FILE *f, struct proof *p)
{
	struct ln *ln;
	struct ast *cmd;
	int last_was_ln = 0;

	fprintf(f, preamble, get_max_depth(p) + 1);

	ln = &p->lns[0];

	for (int i = 0; i < p->ncmds; ++i) {
		cmd = p->allcmds[i];

		switch (cmd->type) {
		case CMD_OPEN:
			if (last_was_ln)
				fprintf(f, "\\\\\n");
			else
				fprintf(f, "\n");
			fprintf(f, "\\begin{subproof}");
			last_was_ln = 0;
			break;
		case CMD_CLOSE:
			fprintf(f, "\n");
			fprintf(f, "\\end{subproof}");
			last_was_ln = 0;
			break;
		default:
			(void)println(f, ln, last_was_ln);
			last_was_ln = 1;
			ln++;
			break;
		}
	}

	fprintf(f, "%s", postamble);

	return 1;
}

static int println(FILE *f, struct ln *ln, int last_was_ln)
{
	if (last_was_ln)
		fprintf(f, "\\\\\n");
	else
		fprintf(f, "\n");
	printform(f, ln->form, -100);
	fprintf(f, " & ");
	printcmd(f, ln->cmd);
	return 1;
}

static int printform(FILE *f, struct ast *form, int parentprec)
{
	int p = prec(form->type);
	if (p <= parentprec)
		fprintf(f, "(");

	switch (form->type) {
	case FORM_NOT:
		fprintf(f, "\\neg ");
		(void)printform(f, form->lhs, p);
		break;
	case FORM_AND:
		(void)printform(f, form->lhs, p);
		fprintf(f, " \\land ");
		(void)printform(f, form->rhs, p);
		break;
	case FORM_OR:
		(void)printform(f, form->lhs, p);
		fprintf(f, " \\lor ");
		(void)printform(f, form->rhs, p);
		break;
	case FORM_IMPL:
		(void)printform(f, form->lhs, p);
		fprintf(f, " \\to ");
		(void)printform(f, form->rhs, p);
		break;
	case FORM_CON:
		fprintf(f, "\\perp");
		break;
	case FORM_NAME:
		fprintf(f, "%s", form->text);
		break;
	default:
		assert(0 && "invalid form type");
	}

	if (p <= parentprec)
		fprintf(f, ")");

	return 1;
}

static int printcmd(FILE *f, struct ast *cmd)
{
	struct ast *rule, *inps;

	switch (cmd->type) {
	case CMD_PRESUME:
		fprintf(f, "premise");
		return 1;
	case CMD_ASSUME:
		fprintf(f, "assumption");
		return 1;
	case CMD_APPLY:
		break;
	default:
		assert(0 && "invalid cmd");
	}

	rule = cmd->lhs;
	inps = cmd->rhs;

	switch (rule->type) {
	case RULE_NOT_INTR:
		fprintf(f, "\\(\\neg i\\) ");
		break;
	case RULE_NOT_ELIM:
		fprintf(f, "\\(\\neg e\\) ");
		break;
	case RULE_AND_INTR:
		fprintf(f, "\\(\\land i\\) ");
		break;
	case RULE_AND_ELIM_1:
		fprintf(f, "\\(\\land e_1\\) ");
		break;
	case RULE_AND_ELIM_2:
		fprintf(f, "\\(\\land e_2\\) ");
		break;
	case RULE_OR_INTR_1:
		fprintf(f, "\\(\\lor i_1\\) ");
		break;
	case RULE_OR_INTR_2:
		fprintf(f, "\\(\\lor i_2\\) ");
		break;
	case RULE_OR_ELIM:
		fprintf(f, "\\(\\lor e_2\\) ");
		break;
	case RULE_IMPL_INTR:
		fprintf(f, "\\(\\to i\\) ");
		break;
	case RULE_IMPL_ELIM:
		fprintf(f, "\\(\\to e\\) ");
		break;
	case RULE_CON_ELIM:
		fprintf(f, "\\(\\perp e\\) ");
		break;
	case RULE_NOT_NOT_INTR:
		fprintf(f, "\\(\\neg\\neg i\\) ");
		break;
	case RULE_NOT_NOT_ELIM:
		fprintf(f, "\\(\\neg\\neg e\\) ");
		break;
	case RULE_MT:
		fprintf(f, "MT ");
		break;
	case RULE_PBC:
		fprintf(f, "PBC ");
		break;
	case RULE_LEM:
		fprintf(f, "LEM ");
		break;
	case RULE_COPY:
		fprintf(f, "copy ");
		break;
	default:
		assert(0 && "invalid rule");
	}

	printinps(f, inps);

	return 1;
}

static int prec(int type)
{
	switch (type) {
	case FORM_NOT:
	case FORM_CON:
	case FORM_NAME:
		return 0;
	case FORM_AND:
	case FORM_OR:
		return -1;
	case FORM_IMPL:
		return -2;
	default:
		assert(0 && "invalid form type");
	}
}

static int printinps(FILE *f, struct ast *inps)
{
	for (;;) {
		if (inps->type == INPUT_FORM) {
			inps = inps->rhs;
			continue;
		}
		if (inps->type == INPUT_LINE) {
			fprintf(f, "%d", inps->start + 1);
		}
		if (inps->type == INPUT_BOX) {
			fprintf(f, "%d-%d", inps->start + 1, inps->end + 1);
		}
		inps = inps->rhs;
		if (inps)
			fprintf(f, ", ");
		else
			break;
	}
	return 1;
}

static int get_max_depth(struct proof *p)
{
	int max = 0, d;
	for (int i = 0; i < p->nlns; i++) {
		d = box_depth(p->lns[i].box);
		if (d > max)
			max = d;
	}
	return d;
}
