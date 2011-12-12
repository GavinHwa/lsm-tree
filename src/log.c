 /* Copyright (c) 2011, BohuTANG <overred.shuttler at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of lsmtree nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __USE_FILE_OFFSET64
	#define __USE_FILE_OFFSET64
#endif

#ifndef __USE_LARGEFILE64
	#define __USE_LARGEFILE64
#endif

#ifndef _LARGEFILE64_SOURCE
	#define _LARGEFILE64_SOURCE
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "buffer.h"
#include "log.h"
#include "debug.h"


int _file_exists(const char *path)
{
	int fd = open(path, O_RDWR);
	if (fd > -1) {
		close(fd);
		return 1;
	}
	return 0;
}

struct log *log_new(const char *basedir, const char *name, int islog)
{
	struct log *l;
	char log_name[LOG_NSIZE];
	char db_name[LOG_NSIZE];

	l = malloc(sizeof(struct log));
	l->islog = islog;

	memset(log_name, 0 ,LOG_NSIZE);
	snprintf(log_name, LOG_NSIZE, "%s/%s.log", basedir, name);
	memcpy(l->name, log_name, LOG_NSIZE);

	memset(db_name, 0, LOG_NSIZE);
	snprintf(db_name, LOG_NSIZE, "%s/%s.db", basedir, name);

	if (_file_exists(log_name)) {
		l->fd = open(log_name, LSM_OPEN_FLAGS, 0644);
		__DEBUG("%s", "WARNING: Find log file,need to recover");
		/*TODO: log recover*/
	} else
		l->fd = open(log_name, LSM_CREAT_FLAGS, 0644);

	if (_file_exists(db_name)) {
		l->fd_db = open(db_name, LSM_OPEN_FLAGS, 0644);
		l->db_alloc = lseek(l->fd_db, 0, SEEK_END);
	} else {
		l->fd_db = open(db_name, LSM_CREAT_FLAGS, 0644);
		l->db_alloc = 0;
	}

	l->buf = buffer_new(256);
	l->db_buf = buffer_new(1024);


	return l;
}

uint64_t log_append(struct log *l, struct slice *sk, struct slice *sv)
{
	char *line;
	char *db_line;
	int len;
	int db_len;
	struct buffer *buf = l->buf;
	struct buffer *db_buf = l->db_buf;
	uint64_t db_offset = l->db_alloc;

	/* DB write */
	buffer_putint(db_buf, sv->len);
	buffer_putnstr(db_buf, sv->data, sv->len);
	db_len = db_buf->NUL;
	db_line = buffer_detach(db_buf);

	lseek(l->fd_db, l->db_alloc, SEEK_SET);
	if (write(l->fd_db, db_line, db_len) != db_len) {
		__DEBUG("%s:length:<%d>", "ERROR: Data AOF **ERROR**", db_len);
		return db_offset;
	}
	l->db_alloc += db_len;

	/* LOG write */
	if (l->islog) {
		buffer_putint(buf, sk->len);
		buffer_putnstr(buf, sk->data, sk->len);
		buffer_putint(buf, db_offset);

		len = buf->NUL;
		line = buffer_detach(buf);

		if (write(l->fd, line, len) != len)
			__DEBUG("%s,buffer is:%s,buffer length:<%d>", "ERROR: Log AOF **ERROR**", line, len);
	}


	return db_offset;
}


void log_trunc(struct log *l)
{
	buffer_clear(l->buf);
	remove(l->name);
	l->fd = open(l->name, LSM_CREAT_FLAGS, 0644);
}

void log_free(struct log *l)
{
	if (l) {
		buffer_free(l->buf);
		remove(l->name);
		close(l->fd);
		free(l);
	}
}
