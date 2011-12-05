#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "meta.h"
#include "debug.h"

struct meta *meta_new()
{
	struct meta *m = malloc(sizeof(struct meta));
	m->sn = 0;

	return m;
}

struct meta_node *meta_get(struct meta *meta, char *key)
{
	size_t left = 0, right = meta->size, i;
	while (left < right) {
		i = (right -left) / 2 +left;
		int cmp = strcmp(key, meta->nodes[i].begin);
		if (cmp == 0)
			break ;

		if (cmp < 0)
			right = i;
		else
			left = i + 1;
	}

	return &meta->nodes[i];
}

void meta_set(struct meta *meta, struct meta_node *node)
{
	size_t left = 0, right = meta->size;
	while (left < right) {
		size_t i = (right -left) / 2 +left;
		int cmp = strcmp(node->begin, meta->nodes[i].begin);
		if (cmp == 0)
			return ;

		if (cmp < 0)
			right = i;
		else
			left = i + 1;
	}

	size_t i = left;
	meta->size++;
	memmove(&meta->nodes[i + 1], &meta->nodes[i], (meta->size - i) * META_NODE_SIZE);
	memcpy(&meta->nodes[i], node, META_NODE_SIZE);
}

void meta_dump(struct meta *meta)
{
	int i;
	printf("--Meta dump:count<%d>\n", meta->size);
	for (i = 0; i< meta->size; i++) {
		struct meta_node n = meta->nodes[i];
		printf("	(%d) begin:<%s>,indexname:<%s>,hascount:<%d>\n",
				i,
				n.begin,
				n.index_name,
				n.count);
	}
}

void meta_free(struct meta *meta)
{
	if (meta)
		free(meta);
}
