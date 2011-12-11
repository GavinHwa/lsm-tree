#ifndef _UTIL_H
#define _UTIL_H

#define SST_NSIZE (1024)
#define INDEX_NSIZE (1024)
#define LOG_NSIZE (1024)

struct slice{
	char *data;
	int len;
};

void _ensure_dir_exists(const char *path);
#endif
