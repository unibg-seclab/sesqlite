#include <selinux/selinux.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "ft_hash.h"

#define TIMER_GET		(double) clock() / CLOCKS_PER_SEC
#define TIMER_START		_tstart = TIMER_GET;
#define TIMER_INIT		double _tstart; TIMER_START;
#define TIMER_DIFF		(TIMER_GET) - _tstart

int main(int argc, char **argv) {
	int times = 10;
	if (argc > 1)
		times = atoi(argv[1]);

	Hash hmap;
	HashInit(&hmap, HASH_STRING, 0);

	int i;
	int *res = NULL;

	char *scon = "unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023";
	char *tcon = "unconfined_u:object_r:sesqlite_public:s0";
	char *clas = "db_column";
	char *perm = "select";
	char *key = "unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023 unconfined_u:object_r:sesqlite_public:s0 db_column select";

	TIMER_INIT

	for (i = 0; i < times; ++i) {
		TIMER_START
		res = HashFind(&hmap, key, strlen(key));
		if (res == NULL) {
			res = malloc(sizeof(int));
			*res = selinux_check_access(scon, tcon, clas, perm, NULL);
			HashInsert(&hmap, strdup(key), strlen(key), res);
		}
		printf("%2d) res: %d  time: %f\n", i, *res, TIMER_DIFF);
	}

	return 0;
}
