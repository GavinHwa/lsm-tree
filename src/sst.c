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

#include "skiplist.h"
#include "sst.h"
#include "debug.h"

struct sst *sst_new()
{
	struct sst *s = malloc(sizeof(struct sst));
	s->lsn = 0;

	return s;
}


void sst_merge(struct sst *sst, void *list)
{
	struct sst_footer footer;
	struct skiplist *l = (struct skiplist*)list;
	struct skiplist *lmerge;
	struct skipnode *nodes, *x;
	size_t map_size;
	int result;
	
	char *sst_name = "0.sst";
	sst->fd = open(sst_name, O_RDWR, 0644);
	if(sst->fd > -1) {
		size_t file_end = lseek(sst->fd, 0, SEEK_END);
		result = lseek(sst->fd, file_end - sizeof(struct sst_footer), SEEK_SET);
		if (result == -1) {
			perror("Error: sst_merge,read footer");
		}

		read(sst->fd, &footer, sizeof(struct sst_footer));
		__DEBUG("INFO: Index is exists! Dump sst_footer:count:<%d>", footer.count);

		/* 1) Mmap */
		map_size = footer.count * sizeof(struct skipnode);
		nodes =  mmap(0, map_size, PROT_READ, MAP_SHARED, sst->fd, 0);

		/* 2) Insert into merge list */
		lmerge = skiplist_new(footer.count + l->count +1);
		for (int i = 0; i < footer.count; i++) {
			struct slice sk;
			sk.len = SKIP_KSIZE;
			sk.data = nodes[i].key;
			skiplist_insert(lmerge, &sk, nodes[i].val, nodes[i].opt);
		}
		/* Unmap and Free */
		if (munmap(nodes, map_size) == -1)
			perror("Error: sst_merge,munmap");

		__DEBUG("INFO: Merge mtable, need merge count:<%d>,lmerge count:<%d>", l->count,lmerge->count);

		/* 3) Do merge */
		x= l->hdr->forward[0];
		for (int i = 0; i < l->count; i++) {
			struct slice sk;
			sk.len = SKIP_KSIZE;
			sk.data = x->key;
			skiplist_insert(lmerge, &sk, x->val, x->opt);
			x = x->forward[0];
		}

		__DEBUG("INFO: Merge finished, count:<%d>", lmerge->count);

		footer.count = lmerge->count;
		result = lseek(sst->fd, 0, SEEK_SET);

		map_size = footer.count * sizeof(struct skipnode);

		/* Write map file */
		result = lseek(sst->fd, map_size-1, SEEK_CUR);
		result = write(sst->fd, "", 1);

		/* File is ready to mmap */
		nodes = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, sst->fd, 0);
		if (nodes == MAP_FAILED) {
			close(sst->fd);
			perror("Error mapping the file");
		}

		x = lmerge->hdr->forward[0];
		for (int i = 0; i < footer.count; i++) {
			memcpy(&nodes[i], x, sizeof(struct skipnode));
			x = x->forward[0];
		}

		/* Unmap and free*/
		if (munmap(nodes, map_size) == -1)
			perror("ERROR: sst_merge unmap merge");

		result = write(sst->fd, &footer, sizeof(struct sst_footer));
		close(sst->fd);

		__DEBUG("INFO: Write back  index,count:<%d>", footer.count);

		skiplist_free(lmerge);

	} else {
		sst->fd = open(sst_name, O_RDWR | O_CREAT, 0644);
		sst->lsn = 0;

		footer.count = l->count;

		/* Write map file */
		map_size = footer.count * sizeof(struct skipnode);
		result = lseek(sst->fd, map_size-1, SEEK_SET);
		result = write(sst->fd, "", 1);

		/* File is ready to mmap */
		nodes = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, sst->fd, 0);
		if (nodes == MAP_FAILED) {
			close(sst->fd);
			perror("Error mapping the file...");
		}

		x = l->hdr->forward[0];
		for (int i = 0; i < footer.count; i++) {
			memcpy(&nodes[i], x, sizeof(struct skipnode));
			x = x->forward[0];
		}
		
		/* Write footer */
		write(sst->fd, &footer, sizeof(struct sst_footer));

		/* Unmap and free*/
		if (munmap(nodes, map_size) == -1)
			perror("ERROR: sst_merge unmap merge");
		close(sst->fd);
	}

}
