#ifndef _META_H
#define _META_H

#include <stdint.h>
#include "skiplist.h"

#define META_SIZE (10000)
#define META_NODE_SIZE sizeof(struct meta_node)

struct meta_node{
	char begin[SKIP_KSIZE];
	char index_name[SKIP_KSIZE];
	uint32_t count;
	uint32_t lsn;
};

struct meta{
	int32_t sn;
	int32_t size;
	struct meta_node nodes[META_SIZE];
};

struct meta *meta_new();
struct meta_node *meta_get(struct meta *meta, char *key);
void meta_set(struct meta *meta, struct meta_node *node);
void meta_dump(struct meta *meta);
void meta_free(struct meta *meta);

#endif
