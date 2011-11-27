#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "skiplist.h"
#include "debug.h"
#include "util.h"
#include "platform.h"

#define MAXNUM (29)
int main()
{
	int k;
	char key[SKIP_KSIZE];

	struct slice sk;
	struct skiplist *list;
	struct skipnode *node;

	list = skiplist_new(MAXNUM-1);

	for (k=0; k < MAXNUM; k++) {
		snprintf(key, SKIP_KSIZE, "key:%d", k);
		
		sk.data = key;
		sk.len = SKIP_KSIZE;

		if (skiplist_notfull(list))
			skiplist_insert(list, &sk, k, ADD);
		else
			__DEBUG("%s", "WARNING: You need more skiplists...");
	}

	if (MAXNUM < 1000)
		skiplist_dump(list);

	snprintf(key, SKIP_KSIZE, "key:%d", MAXNUM / 2);
	node = skiplist_lookup(list, key);
	__DEBUG("INFO: Lookup key is:<%s>, result: key:<%s>,val:<%llu>", key, node->key, node->val);

	skiplist_free(list);
	return 0;
}
