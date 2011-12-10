#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "index.h"
#include "debug.h"
#include "util.h"

#define KSIZE (16)
#define VSIZE (20)

#define LOOP (5000000)
#define MAX_MTBL (1)
#define MAX_MTBL_SIZE (500000)

long long ustime(void)
{
	struct timeval tv;
	long long ust;

	gettimeofday(&tv, NULL);
	ust = ((long long)tv.tv_sec)*1000000;
	ust += tv.tv_usec;
	return ust / 1000000;
}

void random_key(char *key,int length) {
	char salt[36]= "abcdefghijklmnopqrstuvwxyz0123456789";
	memset(key, 0, length);
	for (int i = 0; i < length; i++)
		key[i] = salt[rand() % length];
}

int main()
{
	int i;
	long long start,end;
	struct slice sk, sv;

	char key[KSIZE];
	char val[VSIZE];

	start = ustime();
	struct index *idx = index_new("test_idx", MAX_MTBL, MAX_MTBL_SIZE);
	for (i = 0; i < LOOP; i++) {
		random_key(key, KSIZE);
		snprintf(val, VSIZE, "val:%d", i);

		sk.len = KSIZE;
		sk.data = key;
		sv.len = VSIZE;
		sv.data = val;

		index_add(idx, &sk, &sv);
		if ((i % 10000) == 0) {
			fprintf(stderr,"random write finished %d ops%30s\r", i, "");
			fflush(stderr);
		}
	}

	end = ustime();

	__DEBUG("%s:count<%d>,cost:<%llu>,<%llu>opts/sec", 
			"INFO: Finish index build test",
			LOOP, 
			(end - start),
			LOOP / (end -start)
			); 

	return 0;

}
