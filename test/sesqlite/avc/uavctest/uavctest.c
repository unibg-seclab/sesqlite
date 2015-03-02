#include <selinux/selinux.h>
#include <selinux/avc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "timer.h"
#include "ft_hash.h"

void cleanavc() {
	avc_reset();
	avc_cleanup();
	avc_reset();
}

int main(int argc, char **argv) {
	int i;
	long long diff;
	int *res = NULL;
	int times = 10;
	int clean_avc = 0;
	struct timespec start, end;

	if (argc > 1)
		times = atoi(argv[1]);
	if (argc > 2)
		clean_avc = atoi(argv[2]);

	Hash hmap;
	HashInit(&hmap, HASH_STRING, 0);

	char *scon = "unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023";
	char *tcon = "unconfined_u:object_r:sesqlite_public:s0";
	char *clas = "db_column";
	char *perm = "select";
	char *key = "unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023 unconfined_u:object_r:sesqlite_public:s0 db_column select";

	res = malloc(sizeof(int));
	*res = selinux_check_access(scon, tcon, clas, perm, NULL);
	HashInsert(&hmap, strdup(key), strlen(key), res);

	for (i = 0; i < times; ++i) {

		if ( clean_avc != 0 ){
			cleanavc();
			HashClear(&hmap);
		}

		GETTIME(start)

		res = HashFind(&hmap, key, strlen(key));
		if (res == NULL) {
			res = malloc(sizeof(int));
			*res = selinux_check_access(scon, tcon, clas, perm, NULL);
			HashInsert(&hmap, strdup(key), strlen(key), res);
		}

		GETTIME(end)
		diff = TIMESPEC_DIFF(start, end);
		printf("%lld\n", diff);
	}

	return 0;
}
