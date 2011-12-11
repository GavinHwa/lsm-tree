#ifndef _LOG_H
#define _LOG_H

#include <stdint.h>
#include "util.h"
#include "platform.h"

struct log{
	int fd;
	int fd_db;
	int islog;
	char name[LOG_NSIZE];
	uint64_t db_alloc;
	struct buffer *buf;
};

struct log *log_new(const char *basedir, const char *name, int islog);
uint64_t log_append(struct log *log, struct slice *sk, struct slice *sv);
void log_trunc(struct log *log);
void log_free(struct log *log);

#endif
