 /* This is a complex 'sstable' implemention, merge all mtable to on-disk indices 
 * Copyright (c) 2011, BohuTANG <overred.shuttler at gmail dot com>
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

#define SST_MAX (25000)

struct sst *sst_new()
{
	struct sst *s = malloc(sizeof(struct sst));
	s->lsn = 0;

	/* TODO: init all index-meta */
	s->meta = meta_new();

	return s;
}
/*
 * Write node to index file
 * If need is true,means this index-file is new-created.
 * need add IT to meta informations.
 */
void *_write_mmap(struct sst *sst, struct skipnode *x, size_t count, int need)
{
	int result;
	int fd = sst->fd;
	size_t map_size;
	struct sst_footer footer;
	struct skipnode *nodes;
	struct skipnode *last;

	footer.count = count;

	map_size = footer.count * sizeof(struct skipnode);

	/* Write map file */
	result = lseek(fd, map_size-1, SEEK_SET);
	if (result == -1) {
		perror("Error: write mmap");
		close(sst->fd);
		return NULL;
	}

	result = write(fd, "", 1);
	if (result == -1) {
		perror("Error: write mmap");
		close(sst->fd);
		return NULL;
	}

	/* File is ready to mmap */
	nodes = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (nodes == MAP_FAILED) {
		close(fd);
		perror("Error mapping the file");
		return NULL;
	}

	int i;
	for (i = 0 ; i < footer.count; i++) {
		memcpy(&nodes[i], x, sizeof(struct skipnode));
		last = x;
		x = x->forward[0];
	}

	/* Unmap and free*/
	if (munmap(nodes, map_size) == -1) {
		close(fd);
		perror("ERROR: sst_merge unmap merge");
		return NULL;
	}

	result = write(fd, &footer, sizeof(struct sst_footer));
	if (result == -1) {
		perror("Error: write mmap");
		close(sst->fd);
		return NULL;
	}

	close(fd);

	if(need){
		/* Set meta */
		struct meta_node mn;

		memset(mn.end, 0, SKIP_KSIZE);
		memcpy(mn.end, last->key, SKIP_KSIZE);

		memset(mn.index_name, 0, SKIP_KSIZE);
		memcpy(mn.index_name, sst->name, SKIP_KSIZE);

		mn.count = footer.count;
		mn.lsn = sst->meta->size;

		meta_set(sst->meta, &mn);
		__DEBUG("INFO: Last key:<%s>,index name:<%s>,count:<%d>,lsn:<%d>", last->key, sst->name,footer.count,mn.lsn);
	}

	return last;
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

/*
 * If current index-file is not with previous key's index-file
 * need to flush merge cache to disk
 */
struct skipnode *_flush_old_merge(struct sst *sst, struct skiplist *lmerge, struct skipnode *x, int *miss)
{
	char sst_name[SKIP_KSIZE];
	size_t count = lmerge->count;

	if (count <= SST_MAX) {
		skiplist_free(lmerge);
		return x;
	}

	/* 1) flush old merge list */
	int wcount = SST_MAX;
	if(wcount > count){
		wcount = count;
		x = _write_mmap(sst,x, wcount, 0);
		*miss = wcount;
	}else{
		int i = 1;
		/* a)write SST_MAX */
		x = _write_mmap(sst, x ,i*SST_MAX, 0);
		i++;

		/* b): wirte to new index for rest */
		do {
			size_t wcount = SST_MAX;
			memset(sst_name, 0, SKIP_KSIZE);
			snprintf(sst_name, SKIP_KSIZE, "%d.sst", sst->meta->size); 

			memset(sst->name, 0, SKIP_KSIZE);
			memcpy(sst->name, sst_name, SKIP_KSIZE);

			sst->fd = open(sst->name, O_RDWR | O_CREAT, 0644);
			if (i*SST_MAX> count)
				wcount = count-(i-1)*SST_MAX;

			x = _write_mmap(sst, x, wcount, 1);
			*miss = wcount;
			i++;
		}while (i * SST_MAX < count);
	}

	skiplist_free(lmerge);
	return x;
}

/*
 * Flush the list's rest nodes to disk
 */
struct skipnode *_flush_cur_list(struct sst *sst, struct skiplist *list, struct skipnode *x, int *miss)
{
	char sst_name[SKIP_KSIZE];
	size_t count = list->count - *miss;
	int i = 1;

	if (count == 0)
		return x;

	do {
		size_t wcount = SST_MAX;
		memset(sst_name, 0, SKIP_KSIZE);
		snprintf(sst_name, SKIP_KSIZE, "%d.sst", sst->meta->size); 

		memset(sst->name, 0, SKIP_KSIZE);
		memcpy(sst->name, sst_name, SKIP_KSIZE);

		sst->fd = open(sst->name, O_RDWR | O_CREAT, 0644);
		if (i*SST_MAX> count)
			wcount = count-(i-1)*SST_MAX;

		x = _write_mmap(sst, x, wcount, 1);
		i++;

	}while (i * SST_MAX < count);

	return x;
}

void sst_merge(struct sst *sst, struct skiplist *list)
{
	int miss = 0;
	struct skipnode *x= list->hdr->forward[0];

	/* First time,index is NULL,need to be created */
	if (sst->meta->size == 0) {
		x = _flush_cur_list(sst, list, x, &miss);
	} else {
		struct skiplist *lmerge;
		sst->fd = open(sst->name, O_RDWR, 0644);
		lmerge = _read_mmap(sst, list->count);

		for (int i = 0; i < list->count; i++) {
			struct meta_node *mn = meta_get(sst->meta, x->key);
			if (mn == NULL) {
				__DEBUG("Big key over key:<%s>, i is:<%d>",x->key,i);

				/* Flush old merge list */
				x = _flush_old_merge(sst, lmerge, x, &miss);
				x = _flush_cur_list(sst, list, x, &i);

				return;
			}

			int cmp = strcmp(mn->index_name, sst->name);
			if(cmp ==0){
				struct slice sk;
				sk.len = SKIP_KSIZE;
				sk.data = x->key;
				skiplist_insert(lmerge, &sk, x->val, x->opt);
			} else {
				/*
				   __DEBUG("INFO:index changed, key:<%s>,list first:<%s>,index:<%s>,meta file:<%s>,i:<%d>,listcount:<%d>,mergecount:<%d>",
						x->key,
						list->hdr->forward[0]->key,
						sst->name, 
						mn->index_name,
						i,
						list->count,
						lmerge->count);
				*/

				/* 1)flush old merge list */
				x = _flush_old_merge(sst, lmerge, x, &miss);
				i += miss;

				/* 2)open the new index */
				memset(sst->name, 0, SKIP_KSIZE);
				memcpy(sst->name, mn->index_name, SKIP_KSIZE);
				sst->fd = open(sst->name, O_RDWR , 0644);
				lmerge = _read_mmap(sst, list->count);
				
				struct slice sk;
				sk.len = SKIP_KSIZE;
				sk.data = x->key;
				skiplist_insert(lmerge, &sk, x->val, x->opt);
			}

			x = x->forward[0];
		}
		
		/* flush old merge list */
		x = _flush_old_merge(sst, lmerge, x, &miss);
	}
}

void sst_free(struct sst *sst)
{
	if (sst)
		free(sst);
}
