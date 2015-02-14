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

	rc = sqlite3_open("test.db", &db);
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



/*
 * This is a callback function for use by sqlite3_exec. You can have
 * as many of these as you want / need, so long as the prototype matches.
 *
 * received parameters are:
 *    *param    : a void pointer provided as arg 4 to the
 *                sqlite3_exec call. Useful if you need to
 *                get other data into / out of the function
 *    numCols   : Number of columns returned by the query
 *    **col     : Array of char[] containing values in the row
 *    **colName : Array of char[] containing column names
 *
 * Note that sqlite returns text regardless of the actual type of the column.
 * If you want / need numeric forms, you'll have to convert the text to them
 */
int callback(void *param, int numCols, char **col, char **colName) {
	int i;
	char * test = (char*) param;

	if(strcmp(col[0], test) != 0){
		printf("COLNAME: %s\n", colName[0]);
		printf("COL: %s, test: %s\n", col[0], test);
	}
	//only one result
	CU_ASSERT(strcmp(col[0], test) == 0);

	return SQLITE_OK;
}

/*
 * Table structure:
 * 	t1: a (TEXT)|b (TEXT)
 *	t2: d (TEXT)|e (TEXT)
 *	t3: f (TEXT)|g (TEXT)
 */
void test_create_table(void) {

	//create table
	//CU_ASSERT(sqlite3_exec(db, "CREATE TABLE t1(a TEXT,b TEXT);", 0, 0, 0) == SQLITE_OK);
	
	//CU_ASSERT(sqlite3_exec(db, "CREATE TABLE t2(d TEXT,e TEXT);", 0, 0, 0) == SQLITE_OK);

	//CU_ASSERT(sqlite3_exec(db, "CREATE TABLE t3(f TEXT,g TEXT);", 0, 0, 0) == SQLITE_OK);
}

//check label tables
void test_context_table(void) {
	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='t1' AND column='';", callback, "unconfined_u:object_r:app_data_db_t:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='t2' AND column='';", callback, "unconfined_u:object_r:app_data_db_t:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='t3' AND column='';", callback, "unconfined_u:object_r:app_data_db_t:s0", 0) == SQLITE_OK);

}

void test_context_column(void) {
	//check label columns
	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='t1' AND column='a';", callback, "unconfined_u:object_r:other_c:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='t1' AND column='b';", callback, "unconfined_u:object_r:other_c:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='t2' AND column='d';", callback, "unconfined_u:object_r:app_data_db_c:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='t2' AND column='e';", callback, "unconfined_u:object_r:app_data_db_c:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='t3' AND column='f';", callback, "unconfined_u:object_r:other_c:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='t3' AND column='g';", callback, "unconfined_u:object_r:app_data_db_c:s0", 0) == SQLITE_OK);
}

void test_context_sqlite_master(void) {
	//sqlite_master table
	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='sqlite_master' AND column='';", callback, "unconfined_u:object_r:sqlite_master_t:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='sqlite_master' AND column='type';", callback, "unconfined_u:object_r:sqlite_master_t:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='sqlite_master' AND column='name';", callback, "unconfined_u:object_r:sqlite_master_t:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='sqlite_master' AND column='tbl_name';", callback, "unconfined_u:object_r:sqlite_master_t:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='sqlite_master' AND column='rootpage';", callback, "unconfined_u:object_r:sqlite_master_t:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='sqlite_master' AND column='sql';", callback, "unconfined_u:object_r:sqlite_master_t:s0", 0) == SQLITE_OK);
}

void test_context_sqlite_temp_master(void) {
	//temp table
	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='sqlite_temp_master' AND column='';", callback, "unconfined_u:object_r:sqlite_temp_master_t:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='sqlite_temp_master' AND column='type';", callback, "unconfined_u:object_r:sqlite_temp_master_t:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='sqlite_temp_master' AND column='name';", callback, "unconfined_u:object_r:sqlite_temp_master_t:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='sqlite_temp_master' AND column='tbl_name';", callback, "unconfined_u:object_r:sqlite_temp_master_t:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='sqlite_temp_master' AND column='rootpage';", callback, "unconfined_u:object_r:sqlite_temp_master_t:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='sqlite_temp_master' AND column='sql';", callback, "unconfined_u:object_r:sqlite_temp_master_t:s0", 0) == SQLITE_OK);
}

void test_context_selinux_context(void) {
	//selinux_context table
	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='selinux_context' AND column='';", callback, "unconfined_u:object_r:selinux_context_t:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='selinux_context' AND column='security_context';", callback, "unconfined_u:object_r:selinux_context_t:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='selinux_context' AND column='db';", callback, "unconfined_u:object_r:selinux_context_t:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='selinux_context' AND column='name';", callback, "unconfined_u:object_r:selinux_context_t:s0", 0) == SQLITE_OK);

	CU_ASSERT(
			sqlite3_exec(db, "SELECT security_context FROM selinux_context WHERE name='selinux_context' AND column='column';", callback, "unconfined_u:object_r:selinux_context_t:s0", 0) == SQLITE_OK);

}

void test_insert_table(void) {

//insert
	CU_ASSERT(
			sqlite3_exec(db, "INSERT INTO t1(a, b) values('a', 'b');", 0, 0, 0) == SQLITE_OK);
	CU_ASSERT(
			sqlite3_exec(db, "INSERT INTO t2(d, e) values('d', 'e');", 0, 0, 0) == SQLITE_OK);
	CU_ASSERT(
			sqlite3_exec(db, "INSERT INTO t3(f, g) values('f', 'g');", 0, 0, 0) == SQLITE_OK);

}

void test_select_table(void) {

	char *pzErrMsg;

	//CU_ASSERT(sqlite3_exec(db, "SELECT * FROM t1;", 0, 0, 0) == SQLITE_OK);

	CU_ASSERT(sqlite3_exec(db, "SELECT d FROM t2;", 0, 0, 0) == SQLITE_AUTH);

	CU_ASSERT(sqlite3_exec(db, "SELECT e FROM t2;", 0, 0, 0) == SQLITE_AUTH);

	//can read f but not g
	CU_ASSERT(sqlite3_exec(db, "SELECT f,g FROM t3;", 0, 0, 0) == SQLITE_AUTH);
}

void test_update_table(void) {

	CU_ASSERT(sqlite3_exec(db, "UPDATE t1 SET a='z';", 0, 0, 0) == SQLITE_OK);

	CU_ASSERT(sqlite3_exec(db, "UPDATE t2 SET d='z';", 0, 0, 0) == SQLITE_AUTH);
	CU_ASSERT(sqlite3_exec(db, "UPDATE t2 SET e='z';", 0, 0, 0) == SQLITE_AUTH);

	CU_ASSERT(sqlite3_exec(db, "UPDATE t3 SET f='z';", 0, 0, 0) == SQLITE_OK);

	CU_ASSERT(sqlite3_exec(db, "UPDATE t3 SET g='z';", 0, 0, 0) == SQLITE_AUTH);
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
	if ((NULL == CU_ADD_TEST(pSuite, test_create_table))
			|| (NULL == CU_ADD_TEST(pSuite, test_context_table))
			|| (NULL == CU_ADD_TEST(pSuite, test_context_column))
			|| (NULL == CU_ADD_TEST(pSuite, test_context_sqlite_master))
			|| (NULL == CU_ADD_TEST(pSuite, test_context_sqlite_temp_master))
			|| (NULL == CU_ADD_TEST(pSuite, test_context_selinux_context))
			|| (NULL == CU_ADD_TEST(pSuite, test_insert_table))
			|| (NULL == CU_ADD_TEST(pSuite, test_select_table))
			|| (NULL == CU_ADD_TEST(pSuite, test_update_table))
			|| (NULL == CU_ADD_TEST(pSuite, test_delete_table))) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (argc > 1 && (strcmp(argv[1], "-auto") == 0)) {
		/* Run all test using the CUnit automated interface (output to xml file) */
		CU_set_output_filename("cunit_testall");
		//CU_list_tests_to_file();
		CU_automated_run_tests();
		printf("[%u tests failed]\n\n", CU_get_number_of_failures());
	} else {
		/* Run all tests using the CUnit Console interface */
		CU_console_run_tests();
	}

	CU_cleanup_registry();
	return CU_get_error();

}
