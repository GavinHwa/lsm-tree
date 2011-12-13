 /* This is a binary search/insert meta
 * +--------+--------+--------+--------+
 * |             end key               |
 * +--------+--------+--------+--------+
 * |          index-file name          |
 * +--------+--------+--------+--------+
 *
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "meta.h"
#include "debug.h"

struct meta *meta_new()
{
	struct meta *m = malloc(sizeof(struct meta));
	m->sn = 0;
	m->size = 0;

	return m;
}

struct meta_node *meta_get(struct meta *meta, char *key)
{
	size_t left = 0, right = meta->size, i;
	while (left < right) {
		i = (right -left) / 2 +left;
		int cmp = strcmp(key, meta->nodes[i].end);
		if (cmp == 0)
			break ;

		if (cmp < 0)
			right = i;
		else
			left = i + 1;
	}
	i = left;
	if ( i == meta->size)
		return NULL;

	return &meta->nodes[i];
}

void meta_set(struct meta *meta, struct meta_node *node)
{
	size_t left = 0, right = meta->size;
	while (left < right) {
		size_t i = (right -left) / 2 +left;
		int cmp = strcmp(node->end, meta->nodes[i].end);
		if (cmp == 0) {
			memcpy(meta->nodes[i].end, node->end, SKIP_KSIZE);
			return ;
		}

		if (cmp < 0)
			right = i;
		else
			left = i + 1;
	}

	size_t i = left;
	meta->size++;
	node->lsn = meta->size;
	memmove(&meta->nodes[i + 1], &meta->nodes[i], (meta->size - i) * META_NODE_SIZE);
	memcpy(&meta->nodes[i], node, META_NODE_SIZE);
}

void meta_set_byname(struct meta *meta, struct meta_node *node)
{
	size_t left = 0, right = meta->size;
	while (left < right) {
		size_t i = (right -left) / 2 +left;
		int cmp = strcmp(node->index_name, meta->nodes[i].index_name);
		if (cmp == 0) {
			memcpy(meta->nodes[i].end, node->end, SKIP_KSIZE);
			meta->nodes[i].count = node->count;
			return ;
		}

		if (cmp < 0)
			right = i;
		else
			left = i + 1;
	}

}

void meta_dump(struct meta *meta)
{
	int i;
	printf("--Meta dump:count<%d>\n", meta->size);
	for (i = 0; i< meta->size; i++) {
		struct meta_node n = meta->nodes[i];
		printf("	(%d) end:<%s>,indexname:<%s>,hascount:<%d>,lsn:<%d>\n",
				i,
				n.end,
				n.index_name,
				n.count,
				n.lsn);
	}
}

void meta_free(struct meta *meta)
{
	if (meta)
		free(meta);
}
