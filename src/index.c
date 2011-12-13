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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "sst.h"
#include "index.h"
#include "skiplist.h"
#include "log.h"
#include "debug.h"

#define TOLOG (1)
#define DB_DIR "dbs"

struct index *index_new(const char *basedir, const char *name, int max_mtbl_size)
{
	char dir[INDEX_NSIZE];
	struct index *idx = malloc(sizeof(struct index));

	memset(dir, 0, INDEX_NSIZE);
	snprintf(dir, INDEX_NSIZE, "%s/%s", basedir, DB_DIR);
	_ensure_dir_exists(dir);
	
	idx->lsn = 0;
	idx->max_mtbl = 1;
	idx->max_mtbl_size = max_mtbl_size;
	memset(idx->basedir, 0, INDEX_NSIZE);
	memcpy(idx->basedir, dir, INDEX_NSIZE);

	memset(idx->name, 0, INDEX_NSIZE);
	memcpy(idx->name, name, INDEX_NSIZE);

	/* mtable */
	idx->mtbls = calloc(1, sizeof(struct skiplist*));
	idx->mtbls[0] = skiplist_new(idx->max_mtbl_size);

	/* sst */
	idx->sst = sst_new(idx->basedir);

	/* log */
	idx->log = log_new(idx->basedir, idx->name, TOLOG);

	return idx;
}


int index_add(struct index *idx, struct slice *sk, struct slice *sv)
{
	uint64_t db_offset;
	struct skiplist *list;

	db_offset = log_append(idx->log, sk, sv);
	list = idx->mtbls[idx->lsn];

	if (!list) {
		__DEBUG("ERROR: List<%d> is NULL", idx->lsn);
		return 0;
	}

	if (!skiplist_notfull(list)) {
		__DEBUG("%s", "INFO:Merge start...");
		sst_merge(idx->sst, idx->mtbls[0]);
		skiplist_free(idx->mtbls[0]);
		__DEBUG("%s", "INFO:Merge end...");

		log_trunc(idx->log);

		list = skiplist_new(idx->max_mtbl_size);
		idx->mtbls[idx->lsn] = list;
	}
	skiplist_insert(list, sk->data, db_offset, ADD);

	return 1;
}

void *index_get(struct index *idx, struct slice *sk)
{
	struct skiplist *list = idx->mtbls[idx->lsn];
	struct skipnode *node = skiplist_lookup(list, sk->data);
	if (node) {
		/* TODO:read data */

	} else {
		uint64_t off;
		off = sst_getoff(idx->sst, sk);
		if (off != 0) {
			/* TODO:read data from db file */
		}
	}

	return NULL;
}

void index_free(struct index *idx)
{
	log_free(idx->log);
	free(idx);
}
