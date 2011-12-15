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
#include <time.h>
#include <sys/time.h>
#include "lsmtree/index.h"
#include "lsmtree/debug.h"
#include "lsmtree/util.h"

#define KSIZE (16)
#define VSIZE (20)
#define MAX_MTBL_SIZE (1000000)
#define V "0.1"
#define LINE 		"+-----------------------------+----------------+------------------------------+-------------------+\n"
#define LINE1		"---------------------------------------------------------------------------------------------------\n"

long long _ustime(void)
{
	struct timeval tv;
	long long ust;

	gettimeofday(&tv, NULL);
	ust = ((long long)tv.tv_sec)*1000000;
	ust += tv.tv_usec;
	return ust / 1000000;
}

void _random_key(char *key,int length) {
	char salt[36]= "abcdefghijklmnopqrstuvwxyz0123456789";
	memset(key, 0, length);
	for (int i = 0; i < length; i++)
		key[i] = salt[rand() % length];
}



void _print_header(int count)
{
	double index_size = (double)((double)(KSIZE + 8 + 1) * count) / 1048576.0;
	double data_size = (double)((double)(VSIZE + 4) * count) / 1048576.0;

	printf("Keys:		%d bytes each\n", KSIZE);
	printf("Values:		%d bytes each\n", VSIZE);
	printf("Entries:	%d\n", count);
	printf("IndexSize:	%.1f MB (estimated)\n", index_size);
	printf("DataSize:	%.1f MB (estimated)\n", data_size);
	printf(LINE1);
}

void _print_environment()
{
	time_t now = time(NULL);
	printf("LSM-Tree:	version %s(LSM-Tree storage engine)\n", V);
	printf("Date:		%s", (char*)ctime(&now));

	int num_cpus = 0;
	char cpu_type[256] = {0};
	char cache_size[256] = {0};

	FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
	if (cpuinfo) {
		char line[1024] = {0};
		while (fgets(line, sizeof(line), cpuinfo) != NULL) {
			const char* sep = strchr(line, ':');
			if (sep == NULL || strlen(sep) < 10)
				continue;

			char key[1024] = {0};
			char val[1024] = {0};
			strncpy(key, line, sep-1-line);
			strncpy(val, sep+1, strlen(sep)-1);
			if (strcmp("model name", key) == 0) {
				num_cpus++;
				strcpy(cpu_type, val);
			}
			else if (strcmp("cache size", key) == 0)
				strncpy(cache_size, val + 1, strlen(val) - 1);	
		}

		fclose(cpuinfo);
		printf("CPU:		%d * %s", num_cpus, cpu_type);
		printf("CPUCache:	%s\n", cache_size);
	}
}

void _write_test(long int count)
{
	int i;
	double cost;
	long long start,end;
	struct slice sk, sv;

	char key[KSIZE];
	char val[VSIZE];

	start = _ustime();
	char *path = getcwd(NULL, 0);
	struct index *idx = index_new(path, "test_idx", MAX_MTBL_SIZE);
	for (i = 0; i < count; i++) {
		_random_key(key, KSIZE);
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
	end = _ustime();
	cost = end -start;

	index_free(idx);

	printf(LINE);
	printf("|Random-Write	(done:%ld): %.6f sec/op; %.1f writes/sec(estimated); cost:%.3f(sec)\n"
		,count, (double)(cost / count)
		,(double)(count / cost)
		,cost);	
}

void _read_test(long int count)
{
	int i;
	double cost;
	long long start,end;
	struct slice sk;

	char key[KSIZE];

	start = _ustime();
	struct index *idx = index_new(getcwd(NULL, 0), "test_idx", MAX_MTBL_SIZE);
	for (i = 0; i < count; i++) {
		_random_key(key, KSIZE);
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

	end = _ustime();
	cost = end - start;
	index_free(idx);

 	cost = end - start;
	printf(LINE);
	printf("|Random-Read	(done:%ld): %.6f sec/op; %.1f reads /sec(estimated); cost:%.3f(sec)\n"
		,count
		,(double)(cost / count)
		,(double)(count / cost)
		,cost);
}

void _readone_test(char *key)
{
	struct slice sk;

	struct index *idx = index_new(getcwd(NULL, 0), "test_idx", MAX_MTBL_SIZE);
	sk.data = key;
	sk.len = KSIZE;

	char *data = index_get(idx, &sk);
	if (data){ 
		__DEBUG("Get Key:<%s>--->value is :<%s>", key, data);
		free(data);
	} else
		__DEBUG("Get Key:<%s>,but value is NULL", key);

	index_free(idx);
}



int main(int argc, char **argv)
{
	long int count;

	if (argc != 3) {
		fprintf(stderr,"Usage: bench <op: write | read> <count>\n");
		exit(1);
	}
	
	if (strcmp(argv[1], "write") == 0) {
		count = atoi(argv[2]);
		_print_header(count);
		_print_environment();
		_write_test(count);
	} else if (strcmp(argv[1], "read") == 0) {
		count = atoi(argv[2]);
		_print_header(count);
		_print_environment();
		
		_read_test(count);
	} else if (strcmp(argv[1], "readone") == 0) {
		_readone_test(argv[2]);
	} else {
		fprintf(stderr,"Usage: bench <op: write | read> <count>\n");
		exit(1);
	}
	return 0;
}
