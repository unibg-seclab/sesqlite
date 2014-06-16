/*
 * createtabletest.c
 *
 *  Created on: Jun 13, 2014
 *      Author: mutti
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sqlite3.h>
#include <assert.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <CUnit/Console.h>

#define CU_ADD_TEST(suite, test) (CU_add_test(suite, #test, (CU_TestFunc)test))

static sqlite3 *db;

/* The suite initialization function.
 * Returns zero on success, non-zero otherwise.
 */
int init_suite1(void) {

	int rc = -1;

	rc = sqlite3_open(":memory:", &db);
	if (rc == -1) {
		fprintf(stderr, "Cannot initialize database.\n");
		sqlite3_close(db);
		return rc;
	}

	return rc;
}

/* The suite cleanup function.
 * Returns zero on success, non-zero otherwise.
 */
int clean_suite1(void) {

	sqlite3_close(db);
	return 0;
}

int insert_sesqlite_master(char *tableName, char *colName,
		char *securityContext) {
	int rc = SQLITE_OK;
	char * sql = NULL;
	sql = (char*) malloc(255 * sizeof(char));

	strcpy(sql, "INSERT INTO sesqlite_master(name, ");

	if (colName != NULL)
		strcat(sql, "column, security_context) values('");
	else
		strcat(sql, "security_context) values('");

	strcat(sql, tableName);
	strcat(sql, "', '");

	if (colName != NULL) {
		strcat(sql, colName);
		strcat(sql, "', '");
	}

	strcat(sql, securityContext);
	strcat(sql, "');");

	rc = sqlite3_exec(db, sql, 0, 0, 0);

	return rc;
}

int insert_table(char *tableName, char *fCol, char *sCol, char *securityContext) {
	int rc = -1;
	char * sql = NULL;
	sql = (char*) malloc(255 * sizeof(char));

	strcpy(sql, "INSERT INTO sesqlite_master(name, security_context) values('");
	strcat(sql, tableName);
	strcat(sql, "', '");
	strcat(sql, securityContext);
	strcat(sql, "');");

	rc = sqlite3_exec(db, sql, 0, 0, 0);

	return rc;
}

/*
 * Table structure:
 * 	t1: a (TEXT)|b (TEXT)
 *	t2: d (TEXT)|e (TEXT)
 *	t3: f (TEXT)|g (TEXT)
 */

void test_insert_sesqlite_master(void) {

	int rc = -1;
	char *pzErrMsg;

	rc =
			sqlite3_exec(db,
					"INSERT INTO sesqlite_master(security_context, name) values('unconfined_u:object_r:app_data_db_t:s0', 't1');",
					0, 0, &pzErrMsg);
	CU_ASSERT(rc == SQLITE_OK);
	if (rc != SQLITE_OK)
		fprintf(stderr, "Error! %s\n", pzErrMsg);

	sqlite3_free(pzErrMsg);
	rc =
			sqlite3_exec(db,
					"INSERT INTO sesqlite_master(security_context, name) values('unconfined_u:object_r:app_data_db_t:s0', 't2');",
					0, 0, &pzErrMsg);
	CU_ASSERT(rc == SQLITE_OK);
	if (rc != SQLITE_OK)
		fprintf(stderr, "Error! %s\n", pzErrMsg);

	sqlite3_free(pzErrMsg);
	rc = sqlite3_exec(db,
			"INSERT INTO sesqlite_master values('unconfined_u:object_r:app_data_db_c:s0', 't2', 'd');",
			0, 0, &pzErrMsg);
	CU_ASSERT(rc == SQLITE_OK);
	if (rc != SQLITE_OK)
		fprintf(stderr, "Error! %s\n", pzErrMsg);

	sqlite3_free(pzErrMsg);
	rc = sqlite3_exec(db,
			"INSERT INTO sesqlite_master values('unconfined_u:object_r:app_data_db_c:s0', 't2', 'e');",
			0, 0, &pzErrMsg);
	CU_ASSERT(rc == SQLITE_OK);
	if (rc != SQLITE_OK)
		fprintf(stderr, "Error! %s\n", pzErrMsg);

	sqlite3_free(pzErrMsg);
	rc =
			sqlite3_exec(db,
					"INSERT INTO sesqlite_master(security_context, name) values('unconfined_u:object_r:app_data_db_t:s0', 't3');",
					0, 0, &pzErrMsg);
	CU_ASSERT(rc == SQLITE_OK);
	if (rc != SQLITE_OK)
		fprintf(stderr, "Error! %s\n", pzErrMsg);

	sqlite3_free(pzErrMsg);
	rc = sqlite3_exec(db,
			"INSERT INTO sesqlite_master values('unconfined_u:object_r:app_data_db_c:s0', 't3', 'g');",
			0, 0, &pzErrMsg);
	CU_ASSERT(rc == SQLITE_OK);
	if (rc != SQLITE_OK)
		fprintf(stderr, "Error! %s\n", pzErrMsg);

}

void test_insert_table(void) {

	//create table
	CU_ASSERT(
			sqlite3_exec(db, "CREATE TABLE t1(a TEXT,b TEXT);", 0, 0, 0) == SQLITE_OK);
	CU_ASSERT(
			sqlite3_exec(db, "CREATE TABLE t2(ad TEXT,e TEXT);", 0, 0, 0) == SQLITE_OK);
	CU_ASSERT(
			sqlite3_exec(db, "CREATE TABLE t3(f TEXT,g TEXT);", 0, 0, 0) == SQLITE_OK);

	//insert
	CU_ASSERT(
			sqlite3_exec(db, "INSERT INTO t1 values('a', 'b');", 0, 0, 0) == SQLITE_OK);
	CU_ASSERT(
			sqlite3_exec(db, "INSERT INTO t2 values('d', 'e');", 0, 0, 0) == SQLITE_OK);
	CU_ASSERT(
			sqlite3_exec(db, "INSERT INTO t3 values('f', 'g');", 0, 0, 0) == SQLITE_OK);

}

void test_select_table(void) {

	CU_ASSERT(sqlite3_exec(db, "SELECT * FROM t1;", 0, 0, 0) == SQLITE_OK);

	CU_ASSERT(sqlite3_exec(db, "SELECT d FROM t2;", 0, 0, 0) == SQLITE_ABORT);
	CU_ASSERT(sqlite3_exec(db, "SELECT e FROM t2;", 0, 0, 0) == SQLITE_ABORT);

	CU_ASSERT(sqlite3_exec(db, "SELECT * FROM t3;", 0, 0, 0) == SQLITE_ABORT);
}

void test_update_table(void) {

	CU_ASSERT(
			sqlite3_exec(db, "UPDATE t1 SET a='z' WHERE b='b';", 0, 0, 0) == SQLITE_ABORT);

	CU_ASSERT(
			sqlite3_exec(db, "UPDATE t2 SET d='z' WHERE d='d'", 0, 0, 0) == SQLITE_ABORT);
	CU_ASSERT(
			sqlite3_exec(db, "UPDATE t2 SET e='z' WHERE e='e'", 0, 0, 0) == SQLITE_ABORT);

	CU_ASSERT(
			sqlite3_exec(db, "UPDATE t3 SET f='z' WHERE f='f'", 0, 0, 0) == SQLITE_ABORT);
	CU_ASSERT(
			sqlite3_exec(db, "UPDATE t3 SET g='z' WHERE g='g'", 0, 0, 0) == SQLITE_ABORT);
}

void test_delete_table(void) {

	CU_ASSERT(
			sqlite3_exec(db, "DELETE FROM t1 WHERE a='z'", 0, 0, 0) == SQLITE_OK);
}

int main(int argc, char **argv) {

	CU_pSuite pSuite = NULL;

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	/* add a suite to the registry */
	pSuite = CU_add_suite("Suite_SESQLite1", init_suite1, clean_suite1);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* add the tests to the suite */
	/* NOTE - ORDER IS IMPORTANT - MUST TEST fread() AFTER fprintf() */
	if ((NULL == CU_ADD_TEST(pSuite, test_insert_sesqlite_master))
			|| (NULL == CU_ADD_TEST(pSuite, test_insert_table))
			|| (NULL == CU_ADD_TEST(pSuite, test_select_table))
			|| (NULL == CU_ADD_TEST(pSuite, test_update_table))
			|| (NULL == CU_ADD_TEST(pSuite, test_delete_table))) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Run all tests using the CUnit Console interface */
	CU_console_run_tests();
	CU_cleanup_registry();
	return CU_get_error();

	return 0;

}
