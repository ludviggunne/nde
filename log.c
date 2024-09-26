#include "log.h"
#include <stdio.h>
#include <stdarg.h>

static FILE *f = NULL;

int ndelog_init(const char *fifo_path)
{
	f = fopen(fifo_path, "w");
	return !!f;
}

void ndelog(const char *fmt, ...)
{
	if (!f)
		return;
	va_list vl;
	va_start(vl, fmt);
	vfprintf(f, fmt, vl);
	fflush(f);
	va_end(vl);
}
