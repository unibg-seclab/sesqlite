#include <time.h>

#define GETTIME(t) clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
#define TIMESPEC_DIFF(start, end) ((long long) ((end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec)))

