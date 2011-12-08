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
	int fd;
	size_t map_size;
	struct sst_footer footer;
	struct skipnode *nodes;
	struct skipnode *last, *first;

	first = x;

	footer.count = count;

	map_size = footer.count * sizeof(struct skipnode);
	if (need)
		fd = open(sst->name, O_RDWR | O_CREAT, 0644);
	else
		fd = open(sst->name, O_RDWR, 0644);

	/* Write map file */
	result = lseek(fd, map_size-1, SEEK_SET);
	if (result == -1) {
		perror("Error: write mmap");
		close(fd);
		return NULL;
	}

	result = write(fd, "", 1);
	if (result == -1) {
		perror("Error: write mmap");
		close(fd);
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
		close(fd);
		return NULL;
	}

	if(need){
		/* Set meta */
		struct meta_node mn;

		memset(mn.end, 0, SKIP_KSIZE);
		memcpy(mn.end, last->key, SKIP_KSIZE);

		memset(mn.index_name, 0, SKIP_KSIZE);
		memcpy(mn.index_name, sst->name, SKIP_KSIZE);

		mn.count = footer.count;

		meta_set(sst->meta, &mn);
	}

	close(fd);

	__DEBUG("INFO:(Write) key:<%s>--<%s>,cur:<%s>,count:<%d>", first->key, last->key, sst->name, footer.count);
	return x;
}

struct skiplist *_read_mmap(struct sst *sst, size_t count)
{
	int result;
	size_t map_size;
	struct sst_footer footer;
	struct skipnode *nodes;
	struct skiplist *lmerge;
	int fd = open(sst->name, O_RDWR, 0644);

	/* Read index footer */
	size_t file_end = lseek(fd, 0, SEEK_END);
	result = lseek(fd, file_end - sizeof(struct sst_footer), SEEK_SET);
	if (result == -1) {
		perror("Error: sst_merge,read footer");
	}
	read(fd, &footer, sizeof(struct sst_footer));

	/* 1) Mmap */
	map_size = footer.count * sizeof(struct skipnode);
	nodes =  mmap(0, map_size, PROT_READ, MAP_SHARED, fd, 0);

	/* 2) Load into merge list */
	lmerge = skiplist_new(footer.count + count +1);
	for (int i = 0; i < footer.count; i++) {
		skiplist_insert_node(lmerge, &nodes[i]);
	}

	/* Unmap and Free */
	if (munmap(nodes, map_size) == -1)
		perror("Error: sst_merge,munmap");

	close(fd);

	return lmerge;
}

/*
 * If current index-file is not with previous key's index-file
 * need to flush merge cache to disk
 */
void _flush_merge_list(struct sst *sst, struct skipnode *x, size_t count)
{
	if(count < SST_MAX){
		_write_mmap(sst, x, count, 0);
	} else {
		int mod = (count - SST_MAX) / SST_MAX;
		int rest = count % SST_MAX;

		x = _write_mmap(sst, x, SST_MAX, 0);

		for (int i = 0; i < mod -1; i++) {
			memset(sst->name, 0, SKIP_KSIZE);
			snprintf(sst->name, SKIP_KSIZE, "%d.sst", sst->meta->size); 
			x = _write_mmap(sst, x, SST_MAX, 1);
		}

		/*merge the last one*/
		memset(sst->name, 0, SKIP_KSIZE);
		snprintf(sst->name, SKIP_KSIZE, "%d.sst", sst->meta->size); 
		x = _write_mmap(sst, x, SST_MAX + rest, 1);
	}	
}

/*
 * Flush the list's rest nodes to disk
 */
void _flush_new_list(struct sst *sst, struct skipnode *x, size_t count)
{
	if(count < SST_MAX){
		memset(sst->name, 0, SKIP_KSIZE);
		snprintf(sst->name, SKIP_KSIZE, "%d.sst", sst->meta->size); 
		_write_mmap(sst, x, count, 1);
	} else {
		int mod = (count - SST_MAX) / SST_MAX;
		int rest = count % SST_MAX;

		for (int i = 0; i < mod - 1; i++) {
			memset(sst->name, 0, SKIP_KSIZE);
			snprintf(sst->name, SKIP_KSIZE, "%d.sst", sst->meta->size); 
			x = _write_mmap(sst, x, SST_MAX, 1);
		}

		memset(sst->name, 0, SKIP_KSIZE);
		snprintf(sst->name, SKIP_KSIZE, "%d.sst", sst->meta->size); 
		x = _write_mmap(sst, x, SST_MAX + rest, 1);
	}
}

void _flush_list(struct sst *sst, struct skipnode *x,struct skipnode *hdr,int flush_count)
{
	int pos = 0;
	int count = flush_count;
	struct skipnode *cur = x;
	struct skipnode *first = hdr;
	struct skiplist *merge = NULL;

	while(cur != first) {
		struct meta_node *meta_info = meta_get(sst->meta, cur->key);

		/* If m is NULL, cur->key more larger than meta's largest area
		 * need to create new index-file
		 */
		if(meta_info == NULL){

			/* If merge is NULL,it has no merge*/
			if(merge != NULL) {
				struct skipnode *h = merge->hdr->forward[0];
				_flush_merge_list(sst, h, merge->count);
				skiplist_free(merge);
				merge = NULL;
			}

			/* Flush the last nodes to disk */
			_flush_new_list(sst, x, count - pos);

			return;
		} else {

			/* If m is not NULL,means found the index of the cur
			 * We need:
			 * 1) compare the sst->name with meta index name
			 *		a)If 0: add the cur to merge,and continue
			 *		b)others:
			 *			b1)Flush the merge list to disk
			 *			b2)Open the meta's mmap,and load all blocks to new merge,add cur to merge
			 */
			int cmp = strcmp(sst->name, meta_info->index_name);
			if(cmp == 0) {
				if (merge == NULL)
					merge = _read_mmap(sst,count);	

				skiplist_insert_node(merge, cur);
			} else {
				if(merge){
					struct skipnode *h = merge->hdr->forward[0];
					_flush_merge_list(sst, h, merge->count);
					skiplist_free(merge);
					merge = NULL;
				}

				memset(sst->name, 0, SKIP_KSIZE);
				memcpy(sst->name, meta_info->index_name, SKIP_KSIZE);
				merge = _read_mmap(sst, count);

				/* Add to merge list */
				skiplist_insert_node(merge, cur);
			}

		}

		pos++;
		cur = cur->forward[0];
	}

	if (merge) {
	//	_flush_merge_list(sst, merge->hdr->forward[0], merge->count);
		skiplist_free(merge);
	}
}

void sst_merge(struct sst *sst, struct skiplist *list)
{
	struct skipnode *x= list->hdr->forward[0];

	/* First time,index is NULL,need to be created */
	if (sst->meta->size == 0)
		 _flush_new_list(sst, x, list->count);
	else
		_flush_list(sst, x, list->hdr, list->count);

}

void sst_free(struct sst *sst)
{
	if (sst)
		free(sst);
}
