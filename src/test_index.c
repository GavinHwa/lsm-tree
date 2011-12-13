/*
 * LSM-Tree storage engine
 * Copyright (c) 2011, BohuTANG <overred.shuttler at gmail dot com>
 * All rights reserved.
 * Code is licensed with BSD. See COPYING.BSD file.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "index.h"
#include "debug.h"
#include "util.h"

#define KSIZE (16)
#define VSIZE (20)

#define WRITE_COUNT (3000001)
#define READ_COUNT (100000)
#define MAX_MTBL_SIZE (1000000)

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

void _write_test()
{
	int i;
	long long start,end;
	struct slice sk, sv;

	char key[KSIZE];
	char val[VSIZE];

	start = ustime();
	struct index *idx = index_new(getcwd(NULL, 0), "test_idx", MAX_MTBL_SIZE);
	for (i = 0; i < WRITE_COUNT; i++) {
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
	index_free(idx);
	__DEBUG("%s:count<%d>,cost:<%llu>,<%llu>opts/sec", 
			"INFO: Finish index build test",
			WRITE_COUNT, 
			(end - start),
			WRITE_COUNT / (end -start)
			); 
}

void _read_test()
{
	int i;
	long long start,end;
	struct slice sk;

	char key[KSIZE];

	start = ustime();
	struct index *idx = index_new(getcwd(NULL, 0), "test_idx", MAX_MTBL_SIZE);
	for (i = 0; i < READ_COUNT; i++) {
		random_key(key, KSIZE);
		sk.data = key;
		sk.len = KSIZE;

		char *data = index_get(idx, &sk);
		if (data) 
			free(data);

		if ((i % 10000) == 0) {
			fprintf(stderr,"random read finished %d ops%30s\r", i, "");
			fflush(stderr);
		}
	}

	end = ustime();
	index_free(idx);
	__DEBUG("%s:count<%d>,cost:<%llu>,<%llu>opts/sec", 
			"INFO: Finish get test",
			READ_COUNT, 
			(end - start),
			READ_COUNT / (end -start)
			); 
}

int main()
{
	_write_test();
	_read_test();
	return 0;
}
