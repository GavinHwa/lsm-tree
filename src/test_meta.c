#include <stdio.h>
#include <stdlib.h>

#include "skiplist.h"
#include "meta.h"
#include "debug.h"

int main(int argc, char **argv)
{
	struct meta *m = meta_new();
	struct meta_node node;
	char *k1 = "key:123456";

	for(int i=0;i<10;i++){
		snprintf(node.end, SKIP_KSIZE, "key:%d", rand());
		snprintf(node.index_name, SKIP_KSIZE, "index:%d", i);
		node.count = i*i;
		meta_set(m, &node);
	}
	meta_dump(m);

	struct meta_node *n1 = meta_get(m, k1);
	if (n1) {
		__DEBUG("Get <%s> metanode: end:<%s>,index:<%s>,count<%d>\n", k1,
			n1->end,
			n1->index_name, 
			n1->count);
	} else {
		__DEBUG("Need create a new one meta index,Get <%s>", k1);
	}


	k1 = "Key:01234";
	n1 = meta_get(m, k1);
	if (n1) {
		__DEBUG("Get <%s> metanode: end:<%s>,index:<%s>,count<%d>\n", k1,
			n1->end,
			n1->index_name, 
			n1->count);
	} else {
		__DEBUG("Need create a new one meta index,Get <%s>", k1);
	}

	k1 = "key:91243";
	n1 = meta_get(m, k1);
	if (n1) {
		__DEBUG("Get <%s> metanode: end:<%s>,index:<%s>,count<%d>\n", k1,
			n1->end,
			n1->index_name, 
			n1->count);
	} else {
		__DEBUG("Need create a new one meta index,Get <%s>", k1);
	}


	meta_free(m);

	return 0;
}