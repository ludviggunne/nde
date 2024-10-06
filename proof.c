#include "proof.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "log.h"

struct proof new_proof(void)
{
	struct proof p = { 0 };
	p.lncap = 32;
	p.lns = malloc(p.lncap * sizeof(*p.lns));
	p.cmdcap = 32;
	p.allcmds = malloc(p.cmdcap * sizeof(*p.allcmds));
	return p;
}

void pushln(struct proof *p, struct ast *cmd, struct ast *form)
{
	struct ln ln;
	ln.cmd = cmd;
	ln.form = form;
	ln.box = p->boxhead;

	if (p->nlns == p->lncap) {
		p->lncap *= 2;
		p->lns = realloc(p->lns, p->lncap * sizeof(*p->lns));
	}
	p->lns[p->nlns++] = ln;
}

void pushcmd(struct proof *p, struct ast *cmd)
{
	if (p->ncmds == p->cmdcap) {
		p->cmdcap *= 2;
		p->allcmds =
		    realloc(p->allcmds, p->cmdcap * sizeof(*p->allcmds));
	}
	p->allcmds[p->ncmds++] = cmd;
}

int can_ref_ln(struct proof *p, int n)
{
	struct box *inner, *outer;
	if (n < 0 || n >= p->nlns)
		return 0;

	outer = p->lns[n].box;
	if (!outer)
		return 1;

	inner = p->boxhead;

	while (inner) {
		if (inner == outer)
			return 1;
		inner = inner->parent;
	}

	return 0;
}

int can_ref_box(struct proof *p, int start, int end)
{
	struct box *box, *inner, *outer;
	if (start < 0 || end >= p->nlns || end <= start)
		return 0;

	box = get_box_with_range(p, start, end);
	if (!box)
		return 0;

	outer = box->parent;
	if (!outer)
		return 1;

	inner = p->boxhead;

	while (inner) {
		if (inner == outer)
			return 1;
		inner = inner->parent;
	}

	return 0;
}

int box_depth(struct proof *p)
{
	int c;
	struct box *b = p->boxhead;
	for (c = 0; b; b = b->parent, c++) ;
	return c;
}

void push_box(struct proof *p)
{
	struct box *b = calloc(1, sizeof(*b));
	b->start = p->nlns;
	b->parent = p->boxhead;
	p->boxhead = b;
}

int pop_box(struct proof *p)
{
	struct box *b = p->boxhead;
	if (!b)
		return 0;
	p->boxhead->end = p->nlns - 1;
	p->boxhead = b->parent;
	return 1;
}

int at_beginning_of_box(struct proof *p)
{
	if (p->boxhead)
		return p->boxhead->start == p->nlns;
	return p->nlns == 0;
}

struct box *get_box_with_range(struct proof *p, int start, int end)
{
	struct box *box = p->lns[start].box;
	while (box) {
		if (box->start == start && box->end == end)
			return box;
		box = box->parent;
	}
	return NULL;
}
