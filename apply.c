#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "apply.h"
#include "parse.h"
#include "proof.h"

#define INVINP()\
	snprintf(p->errbuf, sizeof(p->errbuf), "invalid rule inputs");

static int apply_not_intr(struct proof *p, struct ast *cmd)
{
	struct ast *in, *out;
	struct box *box;

	in = cmd->rhs;
	if (!in || in->type != INPUT_BOX) {
		INVINP();
		return 0;
	}

	if (!can_ref_box(p, in->start, in->end))
		return 0;

	box = get_box_with_range(p, in->start, in->end);

	if (!box || p->lns[box->end].form->type != FORM_CON) {
		return INVINP();
		return 0;
	}

	in = p->lns[in->start].form;

	out = calloc(1, sizeof(*out));
	out->type = FORM_NOT;
	out->lhs = ast_copy(in);

	pushln(p, cmd, out);
	return 1;
}

static int apply_not_elim(struct proof *p, struct ast *cmd)
{
	struct ast *in1, *in2, *out;

	in1 = cmd->rhs;

	if (!in1 || in1->type != INPUT_LINE) {
		INVINP();
		return 0;
	}

	in2 = in1->rhs;

	if (!in2 || in2->type != INPUT_LINE) {
		INVINP();
		return 0;
	}

	if (!can_ref_ln(p, in1->start)
	    || !can_ref_ln(p, in2->start)) {
		return 0;
	}

	in1 = p->lns[in1->start].form;
	in2 = p->lns[in2->start].form;

	if (in2->type != FORM_NOT || !ast_equal(in1, in2->lhs)) {
		INVINP();
		return 0;
	}

	out = calloc(1, sizeof(*out));
	out->type = FORM_CON;
	pushln(p, cmd, out);
	return 1;
}

static int apply_and_intr(struct proof *p, struct ast *cmd)
{
	struct ast *lhs, *rhs, *res;

	lhs = cmd->rhs;

	if (!lhs || lhs->type != INPUT_LINE) {
		INVINP();
		return 0;
	}

	rhs = lhs->rhs;

	if (!rhs || rhs->type != INPUT_LINE) {
		INVINP();
		return 0;
	}

	if (!can_ref_ln(p, rhs->start)
	    || !can_ref_ln(p, lhs->start))
		return 0;

	lhs = p->lns[lhs->start].form;
	rhs = p->lns[rhs->start].form;

	lhs = ast_copy(lhs);
	rhs = ast_copy(rhs);

	res = calloc(1, sizeof(*res));
	res->type = FORM_AND;
	res->lhs = lhs;
	res->rhs = rhs;

	pushln(p, cmd, res);

	return 1;
}

static int apply_and_elim_1(struct proof *p, struct ast *cmd)
{
	struct ast *in, *out;

	in = cmd->rhs;
	if (!in || in->type != INPUT_LINE) {
		INVINP();
		return 0;
	}

	if (!can_ref_ln(p, in->start))
		return 0;

	in = p->lns[in->start].form;

	if (in->type != FORM_AND) {
		INVINP();
		return 0;
	}

	out = ast_copy(in->lhs);
	pushln(p, cmd, out);
	return 1;
}

static int apply_and_elim_2(struct proof *p, struct ast *cmd)
{
	struct ast *in, *out;

	in = cmd->rhs;
	if (!in || in->type != INPUT_LINE) {
		INVINP();
		return 0;
	}

	if (!can_ref_ln(p, in->start))
		return 0;

	in = p->lns[in->start].form;

	if (in->type != FORM_AND) {
		INVINP();
		return 0;
	}

	out = ast_copy(in->rhs);
	pushln(p, cmd, out);
	return 1;
}

static int apply_or_intr_1(struct proof *p, struct ast *cmd)
{
	struct ast *lhs, *rhs, *out;

	lhs = cmd->rhs;

	if (!lhs || lhs->type != INPUT_LINE) {
		INVINP();
		return 0;
	}

	rhs = lhs->rhs;

	if (!rhs || rhs->type != INPUT_FORM) {
		INVINP();
		return 0;
	}

	rhs = rhs->lhs;

	if (!can_ref_ln(p, lhs->start)) {
		return 0;
	}

	lhs = p->lns[lhs->start].form;

	out = calloc(1, sizeof(*out));
	out->type = FORM_OR;
	out->lhs = lhs;
	out->rhs = rhs;
	pushln(p, cmd, out);
	return 1;
}

static int apply_or_intr_2(struct proof *p, struct ast *cmd)
{
	struct ast *lhs, *rhs, *out;

	lhs = cmd->rhs;

	if (!lhs || lhs->type != INPUT_FORM) {
		INVINP();
		return 0;
	}

	rhs = lhs->rhs;
	lhs = lhs->lhs;

	if (!rhs || rhs->type != INPUT_LINE) {
		INVINP();
		return 0;
	}

	if (!can_ref_ln(p, rhs->start)) {
		return 0;
	}

	rhs = p->lns[rhs->start].form;

	out = calloc(1, sizeof(*out));
	out->type = FORM_OR;
	out->lhs = lhs;
	out->rhs = rhs;
	pushln(p, cmd, out);
	return 1;
}

static int apply_or_elim(struct proof *p, struct ast *cmd)
{
	struct ast *in1, *in2, *in3, *out;
	struct box *box1, *box2;

	in1 = cmd->rhs;

	if (!in1 || in1->type != INPUT_LINE) {
		INVINP();
		return 0;
	}

	in2 = in1->rhs;

	if (!in2 || in2->type != INPUT_BOX) {
		INVINP();
		return 0;
	}

	in3 = in2->rhs;

	if (!in3 || in3->type != INPUT_BOX) {
		INVINP();
		return 0;
	}

	if (!can_ref_box(p, in2->start, in2->end)
	    || !can_ref_box(p, in3->start, in3->end)) {
		return 0;
	}

	in1 = p->lns[in1->start].form;

	if (in1->type != FORM_OR) {
		INVINP();
		return 0;
	}

	box1 = get_box_with_range(p, in2->start, in2->end);
	box2 = get_box_with_range(p, in3->start, in3->end);

	if (!box1 || !box2) {
		INVINP();
		return 0;
	}

	int match = 1;
	match &= ast_equal(in1->lhs, p->lns[box1->start].form);
	match &= ast_equal(in1->rhs, p->lns[box2->start].form);
	match &= ast_equal(p->lns[box1->end].form, p->lns[box2->end].form);
	if (!match) {
		INVINP();
		return 0;
	}

	out = ast_copy(p->lns[box1->end].form);
	pushln(p, cmd, out);
	return 1;
}

static int apply_impl_intr(struct proof *p, struct ast *cmd)
{
	struct ast *in, *lhs, *rhs, *out;
	struct box *box;

	in = cmd->rhs;

	if (!in || in->type != INPUT_BOX) {
		INVINP();
		return 0;
	}

	if (!can_ref_box(p, in->start, in->end))
		return 0;

	box = get_box_with_range(p, in->start, in->end);
	if (!box) {
		INVINP();
		return 0;
	}

	lhs = p->lns[box->start].form;
	rhs = p->lns[box->end].form;

	out = calloc(1, sizeof(*out));
	out->type = FORM_IMPL;
	out->lhs = ast_copy(lhs);
	out->rhs = ast_copy(rhs);

	pushln(p, cmd, out);
	return 1;
}

static int apply_impl_elim(struct proof *p, struct ast *cmd)
{
	struct ast *in1, *in2, *out;

	in1 = cmd->rhs;

	if (!in1 || in1->type != INPUT_LINE) {
		INVINP();
		return 0;
	}

	in2 = in1->rhs;

	if (!in2 || in2->type != INPUT_LINE) {
		INVINP();
		return 0;
	}

	in1 = p->lns[in1->start].form;
	in2 = p->lns[in2->start].form;

	if (in2->type != FORM_IMPL || !ast_equal(in1, in2->lhs)) {
		INVINP();
		return 0;
	}

	out = ast_copy(in2->rhs);
	pushln(p, cmd, out);
	return 1;
}

static int apply_con_elim(struct proof *p, struct ast *cmd)
{
	struct ast *in1, *in2, *out;

	in1 = cmd->rhs;

	if (!in1 || in1->type != INPUT_LINE) {
		INVINP();
		return 0;
	}

	if (!can_ref_ln(p, in1->start))
		return 0;

	in2 = in1->rhs;

	if (!in2 || in2->type != INPUT_FORM) {
		INVINP();
		return 0;
	}

	in1 = p->lns[in1->start].form;
	in2 = in2->lhs;

	if (in1->type != FORM_CON) {
		INVINP();
		return 0;
	}

	out = ast_copy(in2);
	pushln(p, cmd, out);
	return 1;
}

static int apply_not_not_intr(struct proof *p, struct ast *cmd)
{
	struct ast *in, *not, *out;

	in = cmd->rhs;

	if (!in || in->type != INPUT_LINE) {
		INVINP();
		return 0;
	}

	if (!can_ref_ln(p, in->start))
		return 0;

	in = ast_copy(p->lns[in->start].form);

	not = calloc(1, sizeof(*not));
	not->type = FORM_NOT;
	not->lhs = in;

	out = calloc(1, sizeof(*out));
	out->type = FORM_NOT;
	out->lhs = not;

	pushln(p, cmd, out);
	return 1;
}

static int apply_not_not_elim(struct proof *p, struct ast *cmd)
{
	struct ast *target, *form;

	target = cmd->rhs;

	if (!target || target->type != INPUT_LINE) {
		INVINP();
		return 0;
	}

	if (!can_ref_ln(p, target->start))
		return 0;

	form = p->lns[target->start].form;

	if (form->type != FORM_NOT) {
		INVINP();
		return 0;
	}

	form = form->lhs;

	if (form->type != FORM_NOT) {
		INVINP();
		return 0;
	}

	form = form->lhs;

	pushln(p, cmd, ast_copy(form));
	return 1;
}

static int apply_mt(struct proof *p, struct ast *cmd)
{
	struct ast *in1, *in2, *out;

	in1 = cmd->rhs;

	if (!in1 || in1->type != INPUT_LINE) {
		INVINP();
		return 0;
	}

	in2 = in1->rhs;

	if (!in2 || in2->type != INPUT_LINE) {
		INVINP();
		return 0;
	}

	if (!can_ref_ln(p, in1->start)
	    || !can_ref_ln(p, in2->start)) {
		return 0;
	}

	in1 = p->lns[in1->start].form;
	in2 = p->lns[in2->start].form;

	if (in1->type != FORM_IMPL || in2->type != FORM_NOT) {
		INVINP();
		return 0;
	}

	if (!ast_equal(in2->lhs, in1->rhs)) {
		INVINP();
		return 0;
	}

	out = calloc(1, sizeof(*out));
	out->type = FORM_NOT;
	out->lhs = ast_copy(in1->lhs);

	pushln(p, cmd, out);
	return 1;
}

static int apply_pbc(struct proof *p, struct ast *cmd)
{
	struct ast *in, *not, *out;
	struct box *box;

	in = cmd->rhs;

	if (!in || in->type != INPUT_BOX) {
		INVINP();
		return 0;
	}

	if (!can_ref_box(p, in->start, in->end))
		return 0;

	box = get_box_with_range(p, in->start, in->end);
	if (!box) {
		INVINP();
		return 0;
	}

	if (p->lns[box->end].form->type != FORM_CON) {
		INVINP();
		return 0;
	}

	not = p->lns[box->start].form;
	if (not->type != FORM_NOT) {
		INVINP();
		return 0;
	}

	out = ast_copy(not->lhs);
	pushln(p, cmd, out);
	return 1;
}

static int apply_lem(struct proof *p, struct ast *cmd)
{
	struct ast *in, *not, *out;

	in = cmd->rhs;

	if (!in || in->type != INPUT_FORM) {
		INVINP();
		return 0;
	}

	in = ast_copy(in->lhs);

	not = calloc(1, sizeof(*not));
	not->type = FORM_NOT;
	not->lhs = ast_copy(in);

	out = calloc(1, sizeof(*out));
	out->type = FORM_OR;
	out->lhs = in;
	out->rhs = not;

	pushln(p, cmd, out);
	return 1;
}

static int apply_copy(struct proof *p, struct ast *cmd)
{
	struct ast *in, *out;

	in = cmd->rhs;

	if (!in || in->type != INPUT_LINE) {
		INVINP();
		return 0;
	}

	if (!can_ref_ln(p, in->start))
		return 0;

	in = p->lns[in->start].form;

	out = ast_copy(in);
	pushln(p, cmd, out);
	return 1;
}

int apply_rule(struct proof *p, struct ast *cmd)
{
	assert(cmd->type == CMD_APPLY);

	p->errbuf[0] = 0;

	switch (cmd->lhs->type) {
	case RULE_NOT_INTR:
		return apply_not_intr(p, cmd);
	case RULE_NOT_ELIM:
		return apply_not_elim(p, cmd);
	case RULE_AND_INTR:
		return apply_and_intr(p, cmd);
	case RULE_AND_ELIM_1:
		return apply_and_elim_1(p, cmd);
	case RULE_AND_ELIM_2:
		return apply_and_elim_2(p, cmd);
	case RULE_OR_INTR_1:
		return apply_or_intr_1(p, cmd);
	case RULE_OR_INTR_2:
		return apply_or_intr_2(p, cmd);
	case RULE_OR_ELIM:
		return apply_or_elim(p, cmd);
	case RULE_IMPL_INTR:
		return apply_impl_intr(p, cmd);
	case RULE_IMPL_ELIM:
		return apply_impl_elim(p, cmd);
	case RULE_CON_ELIM:
		return apply_con_elim(p, cmd);
	case RULE_NOT_NOT_INTR:
		return apply_not_not_intr(p, cmd);
	case RULE_NOT_NOT_ELIM:
		return apply_not_not_elim(p, cmd);
	case RULE_MT:
		return apply_mt(p, cmd);
	case RULE_PBC:
		return apply_pbc(p, cmd);
	case RULE_LEM:
		return apply_lem(p, cmd);
	case RULE_COPY:
		return apply_copy(p, cmd);
	default:
		assert(0 && "invalid rule");
	}
}
