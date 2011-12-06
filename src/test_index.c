#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "index.h"
#include "debug.h"
#include "util.h"

#define KSIZE (16)
#define VSIZE (20)

#define LOOP (1000001)
#define MAX_MTBL (3)
#define MAX_MTBL_SIZE (250000)

long long ustime(void)
{
	struct timeval tv;
	long long ust;

	gettimeofday(&tv, NULL);
	ust = ((long long)tv.tv_sec)*1000000;
	ust += tv.tv_usec;
	return ust / 1000000;
}

int main()
{
	int i,ret;
	char key[KSIZE];
	char val[VSIZE];

	long long start,end;
	start = ustime();

	struct slice sk, sv;

	struct index *idx = index_new("test_idx", MAX_MTBL, MAX_MTBL_SIZE);
	for (i = 0; i < LOOP; i++) {
		snprintf(key, KSIZE, "key:%d", i);
		snprintf(val, VSIZE, "val:%d", i);

		sk.len = KSIZE;
		sk.data = key;
		sv.len = VSIZE;
		sv.data = val;

		ret = index_add(idx, &sk, &sv);
		if (!ret)
			__DEBUG("%s,key:<%s>,value:<%s>", "ERROR: Write failed....", key, val);
	}

	end = ustime();

	__DEBUG("%s:count<%d>,cost:<%llu>,<%llu>opts/sec", 
			"INFO: Finish index build test",
			LOOP, 
			(end - start),
			LOOP / (end -start)
			); 

	index_free(idx);
	return 0;

}
