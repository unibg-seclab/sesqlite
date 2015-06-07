#include "sqlite3.h"
#include "ts_util.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECKRC(RC, MSG) if( RC!=SQLITE_OK ){ printf(MSG); exit(1); }
#define TIMESTART struct timespec iStart = tsTOD()
#define TIMEDIFF (tsFloat(tsSubtract(tsTOD(), iStart)))

int callback(void* udp, int num, char** vals, char** names){
	return 0;
}

void test_open(sqlite3** db, char *file){
	int rc = sqlite3_open(file, db);
	CHECKRC(rc, "ERROR in OPEN\n");
	sqlite3_exec(*db, "SELECT 1;", 0, 0, 0);  // force loading the objects
	rc = sqlite3_exec(*db, "CREATE TABLE TEST(A INTEGER, B INTEGER, C TEXT);", 0, 0, 0);
	CHECKRC(rc, "ERROR in CREATE\n");
}

float test_insert(sqlite3* db, int n){
	int i, rc;
	sqlite3_stmt *pStmt;
	TIMESTART;
	rc = sqlite3_prepare_v2(db, "INSERT INTO TEST VALUES (?, ?, ?);", -1, &pStmt, NULL);
	CHECKRC(rc, "ERROR in INSERT\n");
	for( i=1; i<=n; ++i ){
		sqlite3_bind_int(pStmt, 1, n);
		sqlite3_bind_int(pStmt, 2, n*10);
		sqlite3_bind_text(pStmt, 3, "text", -1, SQLITE_STATIC);
		sqlite3_step(pStmt);
		sqlite3_reset(pStmt);
	}
	sqlite3_finalize(pStmt);
	return TIMEDIFF;
}

float test_update(sqlite3* db){
	TIMESTART;
	int rc = sqlite3_exec(db, "UPDATE TEST SET B=2*B WHERE 1=1;", callback, 0, 0);
	CHECKRC(rc, "ERROR in UPDATE\n");
	return TIMEDIFF;
}

float test_delete(sqlite3* db){
	TIMESTART;
	int rc = sqlite3_exec(db, "DELETE FROM TEST WHERE 1=1;", callback, 0, 0);
	CHECKRC(rc, "ERROR in DELETE\n");
	return TIMEDIFF;
}

float test_execute(sqlite3* db, char* sql){
	TIMESTART;
	int rc = sqlite3_exec(db, sql, callback, 0, 0);
	CHECKRC(rc, "ERROR in EXECUTE\n");
	return TIMEDIFF;
}


float test_select(sqlite3* db){
	TIMESTART;
	int rc = sqlite3_exec(db, "SELECT B FROM TEST WHERE 1=1;", callback, 0, 0);
	return TIMEDIFF;
}

float* test(int n, int inload){
	sqlite3 *db;
	float *res = malloc(sizeof(float) * 4);
	test_open(&db, ":memory:");
	res[0] = test_insert(db, n);
	res[1] = test_select(db);
	res[2] = test_update(db);
	res[3] = test_delete(db);
	printf("[%f, %f, %f, %f]", res[0], res[1], res[2], res[3]);
	return res;
}

int main(int argc, char** argv){
	test(atoi(argv[1]), atoi(argv[2]));
	return 0;
}

