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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "sst.h"
#include "debug.h"

#define SST_MAX (200000)

struct sst *sst_new()
{
	struct sst *s = malloc(sizeof(struct sst));
	s->lsn = 0;
	/* TODO: init all index-meta */
	s->meta = meta_new();

	return s;
}


void *_write_mmap(struct sst *sst, struct skipnode *n,size_t from, size_t count)
{
	int result;
	size_t map_size;
	struct sst_footer footer;
	struct skipnode *nodes;

	struct skipnode *first = n;
	struct skipnode *x = n;

	footer.count = count;
	map_size = footer.count * sizeof(struct skipnode);

	/* Write map file */
	result = lseek(sst->fd, map_size-1, SEEK_SET);
	if (result == -1) {
		perror("Error: write mmap");
		close(sst->fd);
		return NULL;
	}

	result = write(sst->fd, "", 1);
	if (result == -1) {
		perror("Error: write mmap");
		close(sst->fd);
		return NULL;
	}

	/* File is ready to mmap */
	nodes = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, sst->fd, 0);
	if (nodes == MAP_FAILED) {
		close(sst->fd);
		perror("Error mapping the file");
		return NULL;
	}

	int i;
	for (i = 0 ; i < footer.count; i++) {
		memcpy(&nodes[i], x, sizeof(struct skipnode));
		x = x->forward[0];
	}
	

	/* Unmap and free*/
	if (munmap(nodes, map_size) == -1) {
		close(sst->fd);
		perror("ERROR: sst_merge unmap merge");
		return NULL;
	}

	result = write(sst->fd, &footer, sizeof(struct sst_footer));
	if (result == -1) {
		perror("Error: write mmap");
		close(sst->fd);
		return NULL;
	}

	close(sst->fd);

	/* Set meta */
	struct meta_node mn;

	memset(mn.begin, 0, SKIP_KSIZE);
	memcpy(mn.begin, first->key, SKIP_KSIZE);

	memset(mn.index_name, 0, SKIP_KSIZE);
	memcpy(mn.index_name, sst->name, SKIP_KSIZE);

	mn.count = footer.count;
	mn.lsn = sst->meta->size;

	meta_set(sst->meta, &mn);
	__DEBUG(" firsrt key:<%s>,index name:<%s>,lsn:<%d>", first->key, sst->name,mn.lsn);

	return x;

}

struct skiplist *_read_mmap(struct sst *sst, size_t count)
{
	int result;
	size_t map_size;
	struct sst_footer footer;
	struct skipnode *nodes;
	struct skiplist *lmerge;

	/* Read index footer */
	size_t file_end = lseek(sst->fd, 0, SEEK_END);
	result = lseek(sst->fd, file_end - sizeof(struct sst_footer), SEEK_SET);
	if (result == -1) {
		perror("Error: sst_merge,read footer");
	}
	read(sst->fd, &footer, sizeof(struct sst_footer));

	/* 1) Mmap */
	map_size = footer.count * sizeof(struct skipnode);
	nodes =  mmap(0, map_size, PROT_READ, MAP_SHARED, sst->fd, 0);

	/* 2) Load into merge list */
	lmerge = skiplist_new(footer.count + count +1);
	for (int i = 0; i < footer.count; i++) {
		struct slice sk;
		sk.len = SKIP_KSIZE;
		sk.data = nodes[i].key;
		skiplist_insert(lmerge, &sk, nodes[i].val, nodes[i].opt);
	}

	/* Unmap and Free */
	if (munmap(nodes, map_size) == -1)
		perror("Error: sst_merge,munmap");

	return lmerge;
}


void sst_merge(struct sst *sst, struct skiplist *list)
{
	char sst_name[SKIP_KSIZE];
	struct skipnode *x= list->hdr->forward[0];
	/* First time,index is NULL,need to be created */
	if (sst->meta->size == 0) {
		memset(sst_name, 0, SKIP_KSIZE);
		snprintf(sst_name, SKIP_KSIZE, "%d.sst", sst->lsn); 

		memset(sst->name, 0, SKIP_KSIZE);
		memcpy(sst->name, sst_name, SKIP_KSIZE);
		
		sst->fd = open(sst->name, O_RDWR | O_CREAT, 0644);
		sst->lsn = 0;

		/* write to index file */
		_write_mmap(sst, x, 0, list->count);
		__DEBUG("INFO: First write,index name:<%s> ...", sst->name);
	
	} else {
		size_t clsn = 0;
		struct skiplist *lmerge;
		struct skipnode *mx;
		struct meta_node *mnode = meta_get(sst->meta, x->key);
		size_t count = list->count;


		clsn = mnode->lsn;
		memset(sst->name, 0, SKIP_KSIZE);
		memcpy(sst->name, mnode->index_name, SKIP_KSIZE);

		sst->fd = open(sst->name, O_RDWR, 0644);
		if(sst->fd == -1) {
			perror("Error: open index name");
			return;
		}

		/* Get merge list */
		lmerge = _read_mmap(sst, list->count);
		mx = lmerge->hdr->forward[0];

		/* Ready to merge list */
		for (int i = 0; i < count; i++) {
			mnode = meta_get(sst->meta, x->key);
			if (mnode->lsn != clsn) {
				
				/* First to write back old merge list
				 * And free merge list
				 */
				/* TODO:split  */
				size_t m_count = lmerge->count;
				_write_mmap(sst, mx, 0, m_count);
				m_count -= SST_MAX;
				free(lmerge);

				/* Read new mmap from different index name 
				 * Creat a new merge list
				 */
				memset(sst->name, 0, SKIP_KSIZE);
				memcpy(sst->name, mnode->index_name, SKIP_KSIZE);
				sst->fd = open(sst->name, O_RDWR, 0644);
				lmerge = _read_mmap(sst, list->count);

				mx = lmerge->hdr->forward[0];

				clsn = mnode->lsn;
			}

			/* Insert new record into merge list */
			struct slice sk;
			sk.len = SKIP_KSIZE;
			sk.data = x->key;
			skiplist_insert(lmerge, &sk, x->val, x->opt);

			x = x->forward[0];
		}

		/* Write back index file */
		size_t m_count = lmerge->count;

		/* TODO:split index if over MAX */
		_write_mmap(sst, mx, 0, m_count);
		free(lmerge);
	}
}

void sst_free(struct sst *sst)
{
	if (sst)
		free(sst);
}
