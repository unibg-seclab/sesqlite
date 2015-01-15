#include <selinux/selinux.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define TIMER_GET		(double) clock() / CLOCKS_PER_SEC
#define TIMER_START		_tstart = TIMER_GET;
#define TIMER_INIT		double _tstart; TIMER_START;
#define TIMER_DIFF		(TIMER_GET) - _tstart

int main(int argc, char **argv) {
	int times = 10;
	if (argc > 1)
		times = atoi(argv[1]);

	char *scon = "unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023";
	char *tcon = "unconfined_u:object_r:sesqlite_public:s0";
	char *clas = "db_column";
	char *perm = "select";

	TIMER_INIT

	int i, rc;
	double diff;
	for (i = 0; i < times; ++i) {
		TIMER_START
		rc = selinux_check_access(scon, tcon, clas, perm, NULL);
		diff = TIMER_DIFF;
		printf("%2d) res: %d  time: %f\n", i, rc, TIMER_DIFF);
	}

	return 0;
}
