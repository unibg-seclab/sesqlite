#include <selinux/selinux.h>
#include <selinux/avc.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "timer.h"

void cleanavc() {
	avc_reset();
	avc_cleanup();
	avc_reset();
}

int main(int argc, char **argv) {
	int i, rc;
	long long diff;
	struct timespec start, end;
	int times = 10;
	int clean_avc = 0;

	if (argc > 1)
		times = atoi(argv[1]);
	if (argc > 2)
		clean_avc = atoi(argv[2]);

	char *scon = "unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023";
	char *tcon = "unconfined_u:object_r:sesqlite_public:s0";
	char *clas = "db_column";
	char *perm = "select";

	selinux_check_access(scon, tcon, clas, perm, NULL);

	if ( clean_avc != 0 )
		cleanavc();

	for (i = 0; i < times; ++i) {

		if ( clean_avc != 0)
			cleanavc();

		GETTIME(start)
		rc = selinux_check_access(scon, tcon, clas, perm, NULL);
		GETTIME(end)

		diff = TIMESPEC_DIFF(start, end);
		printf("%lld\n", diff);
	}

	return 0;
}
