#ifndef _SST_H
#define _SST_H

#include <stdint.h>
#include "skiplist.h"
#include "meta.h"
#include "util.h"


/*
* +--------+--------+--------+--------+
* |             sst block 1           |
* +--------+--------+--------+--------+
* |             sst block 2           |
* +--------+--------+--------+--------+
* |      ... all the other blocks ..  |
* +--------+--------+--------+--------+
* |             sst block N           |
* +--------+--------+--------+--------+
* |  start   |   end   | block counts |
* +--------+--------+--------+--------+
*/

struct sst_footer{
	uint32_t count;
};

struct sst{
	char name[SKIP_KSIZE];
	uint32_t lsn;
	uint32_t cur_lsn;
	struct meta *meta;
};

struct sst *sst_new();
void sst_merge(struct sst *sst, struct skiplist *list);
void sst_free(struct sst *sst);

#endif
