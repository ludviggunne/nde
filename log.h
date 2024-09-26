#ifndef LOG_H
#define LOG_H

int ndelog_init(const char *fifo_path);
void ndelog(const char *fmt, ...);

#endif
