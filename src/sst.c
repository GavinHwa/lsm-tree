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

#define SST_MAX (15000)

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
 * If 'need_new' is true,means that needing to creat a new index-file.
 * And add the meta(end key,and index-file name)  into 'meta'.
 */
void *_write_mmap(struct sst *sst, struct skipnode *x, size_t count, int need_new)
{
	int result;
	int fd;
	size_t map_size;
	struct sst_footer footer;
	struct skipnode *nodes;
	struct skipnode *last;

	footer.count = count;

	map_size = footer.count * sizeof(struct skipnode);
	if (need_new)
		fd = open(sst->name, O_RDWR | O_CREAT, 0644);
	else
		fd = open(sst->name, O_RDWR, 0644);

	/* Write map file */
	result = lseek(fd, map_size -1, SEEK_SET);
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

	lseek(fd, map_size, SEEK_CUR);
	result = write(fd, &footer, sizeof(struct sst_footer));
	if (result == -1) {
		perror("Error: write mmap");
		close(fd);
		return NULL;
	}
	
	/* Set meta */
	struct meta_node mn;

	mn.count = footer.count;
	memset(mn.end, 0, SKIP_KSIZE);
	memcpy(mn.end, last->key, SKIP_KSIZE);

	memset(mn.index_name, 0, SKIP_KSIZE);
	memcpy(mn.index_name, sst->name, SKIP_KSIZE);

	
	if (need_new) 
		meta_set(sst->meta, &mn);
	else 
		meta_set_byname(sst->meta, &mn);

	close(fd);

	return x;
}

struct skiplist *_read_mmap(struct sst *sst, size_t count)
{
	int result;
	size_t map_size;
	struct sst_footer footer;
	struct skipnode *nodes;
	struct skiplist *merge;
	int fd = open(sst->name, O_RDWR, 0644);

	/* Read index footer */
	result = lseek(fd, -sizeof(struct sst_footer), SEEK_END);
	if (result == -1) {
		perror("Error: sst_merge,read footer");
	}
	read(fd, &footer, sizeof(struct sst_footer));

	lseek(fd, 0, SEEK_SET);

	/* 1) Mmap */
	map_size = footer.count * sizeof(struct skipnode);
	nodes =  mmap(0, map_size, PROT_READ, MAP_SHARED, fd, 0);

	/* 2) Load into merge list */
	merge = skiplist_new(footer.count + count + 1);
	for (int i = 0; i < footer.count; i++) {
		skiplist_insert_node(merge, &nodes[i]);
	}

	/* Unmap and Free */
	if (munmap(nodes, map_size) == -1)
		perror("Error: sst_merge,munmap");

	close(fd);

	return merge;
}

void _flush_merge_list(struct sst *sst, struct skipnode *x, size_t count)
{
	int mul;
	int rem;

	/* Less than 2x SST_MAX,compact one index file */
	if (count < (SST_MAX * 2)) {
		x = _write_mmap(sst, x, count, 0);
	} else {
		x = _write_mmap(sst, x, SST_MAX, 0);

		/* first+last */
		mul = (count - SST_MAX * 2) / SST_MAX;
		rem = count % SST_MAX;

		for (int i = 0; i < mul; i++) {
			memset(sst->name, 0, SKIP_KSIZE);
			snprintf(sst->name, SKIP_KSIZE, "%d.sst", sst->meta->size); 
			x = _write_mmap(sst, x, SST_MAX, 1);
		}

		/* The remain part,will be larger than SST_MAX */
		memset(sst->name, 0, SKIP_KSIZE);
		snprintf(sst->name, SKIP_KSIZE, "%d.sst", sst->meta->size); 

		x = _write_mmap(sst, x, SST_MAX + rem, 1);
	}	
}

void _flush_new_list(struct sst *sst, struct skipnode *x, size_t count)
{
	int mul ;
	int rem;

	if (count < (SST_MAX * 2)) {
		memset(sst->name, 0, SKIP_KSIZE);
		snprintf(sst->name, SKIP_KSIZE, "%d.sst", sst->meta->size); 
		x = _write_mmap(sst, x, count, 1);
	} else {
		mul = count / SST_MAX;
		rem = count % SST_MAX;

		for (int i = 0; i < (mul - 1); i++) {
			memset(sst->name, 0, SKIP_KSIZE);
			snprintf(sst->name, SKIP_KSIZE, "%d.sst", sst->meta->size); 
			x = _write_mmap(sst, x, SST_MAX, 1);
		}

		memset(sst->name, 0, SKIP_KSIZE);
		snprintf(sst->name, SKIP_KSIZE, "%d.sst", sst->meta->size); 
		x = _write_mmap(sst, x, SST_MAX + rem, 1);
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
				if (merge) {
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
		struct skipnode *h = merge->hdr->forward[0];
		_flush_merge_list(sst, h, merge->count);
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
