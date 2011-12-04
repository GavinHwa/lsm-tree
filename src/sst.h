#ifndef _SST_H
#define _SST_H

#include <stdint.h>
#include "util.h"

#define SST_NSIZE (64)

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
	char name[SST_NSIZE];
	size_t lsn;
	int fd;
};

struct sst *sst_new();
void sst_merge(struct sst *sst, void *list);

#endif
