#ifndef PROOF_H
#define PROOF_H

#include <stddef.h>

struct box {
	int start;
	int end;
	struct box *parent;
};

struct ln {
	struct ast *cmd;
	struct ast *form;
	struct box *box;
};

struct proof {
	struct ln *lns;
	int nlns;
	int lncap;
	struct box *boxhead;
	char errbuf[512];
};

struct proof new_proof(void);
void pushln(struct proof *p, struct ast *cmd, struct ast *form);
int box_depth(struct proof *p);
void push_box(struct proof *p);
int pop_box(struct proof *p);
int can_ref_ln(struct proof *p, int n);
int can_ref_box(struct proof *p, int start, int end);
struct box *get_box_with_range(struct proof *p, int start, int end);
int at_beginning_of_box(struct proof *p);

#endif
