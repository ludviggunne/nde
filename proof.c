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

int can_ref_ln(struct proof *p, int n)
{
	return 1;
	ndelog("checking if line %d can be referenced... ", n);

	struct box *tbox, *fbox;

	if (n < 0 || n >= p->nlns) {
		ndelog("no!\n");
		snprintf(p->errbuf, sizeof(p->errbuf), "invalid line number");
		return 0;
	}

	tbox = p->lns[n].box;
	if (!tbox) {
		ndelog("yes!\n");
		return 1;
	}

	fbox = p->boxhead;

	while (tbox) {
		if (fbox == tbox) {
			ndelog("yes!\n");
			return 1;
		}
		tbox = tbox->parent;
	}

	ndelog("no!\n");
	snprintf(p->errbuf, sizeof(p->errbuf), "illegal box reference");
	return 0;
}

int can_ref_box(struct proof *p, int start, int end)
{
	return 1;
	struct box *tbox, *fbox;

	ndelog("there are %d lines\n", p->nlns);
	ndelog("checking if box with range %d-%d can be referenced...\n", start,
	       end);

	if (start < 0 || start >= p->nlns || end < 0
	    || end >= p->nlns || end <= start) {
		ndelog("no!\n");
		snprintf(p->errbuf, sizeof(p->errbuf),
			 "invalid line number(s)");
		return 0;
	}

	tbox = p->lns[start].box;

	if (tbox->end < 0) {
		ndelog("no!\n");
		snprintf(p->errbuf, sizeof(p->errbuf), "box not closed");
		return 0;
	}

	while (tbox) {
		ndelog("matching with box %d-%d\n", tbox->start, tbox->end);
		if (tbox->start == start && tbox->end == end)
			break;
		tbox = tbox->parent;
	}

	if (!tbox) {
		ndelog("no!\n");
		snprintf(p->errbuf, sizeof(p->errbuf),
			 "no box matches specified range");
		return 0;
	}

	tbox = tbox->parent;
	if (!tbox) {
		ndelog("yes!\n");
		return 1;
	}

	ndelog("tbox is %d-%d\n", tbox->start, tbox->end);

	fbox = p->boxhead;

	struct box *orig = tbox;

	while (tbox) {
		if (fbox == tbox) {
			ndelog("yes!\n");
			return 1;
		}
		tbox = tbox->parent;
	}

	tbox = orig;

	while (fbox) {
		if (tbox == fbox) {
			ndelog("yes!\n");
			return 1;
		}
		fbox = fbox->parent;
	}

	ndelog("no!\n");
	snprintf(p->errbuf, sizeof(p->errbuf), "illegal box reference");
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
