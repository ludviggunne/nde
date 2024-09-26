#ifndef APPLY_H
#define APPLY_H

#include <stddef.h>
#include "proof.h"

int apply_rule(struct proof *p, struct ast *cmd);

#endif
