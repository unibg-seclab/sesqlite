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
#include <assert.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <CUnit/Console.h>

#define CU_ADD_TEST(suite, test) (CU_add_test(suite, #test, (CU_TestFunc)test))

#include "sqlite_test_utils.h"

static sqlite3 *db;

/* The suite initialization function.
 * Returns zero on success, non-zero otherwise.
 */
int init_suite(void) {

    SQLITE_INIT
    int rc = SQLITE_ERROR;

    rc = SQLITE_OPEN(db, ":memory:");
    if (rc != SQLITE_OK) {
	fprintf(stderr, "Cannot initialize database.\n");
	sqlite3_close(db);
	return rc;
    }

    return rc;
}

/* The suite cleanup function.
 * Returns zero on success, non-zero otherwise.
 */
int clean_suite(void) {

    SQLITE_INIT
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

//	if(strcmp(col[0], test) != 0){
//		printf("COLNAME: %s\n", colName[0]);
//		printf("COL: %s, test: %s\n", col[0], test);
//	}
//	//only one result
//	CU_ASSERT(strcmp(col[0], test) == 0);

	return SQLITE_OK;
}

void test_create_table(void) {

    SQLITE_INIT
    CU_ASSERT(SQLITE_EXEC(db, "CREATE TABLE t1(a INT, b INT);") == SQLITE_OK);
    CU_ASSERT(SQLITE_EXEC(db, "CREATE TABLE t2(d INT, e INT);") == SQLITE_OK);
    CU_ASSERT(SQLITE_EXEC(db, "CREATE TABLE t3(f INT, g INT);") == SQLITE_OK);
}

void test_insert_table(void) {

	SQLITE_INIT
	CU_ASSERT(SQLITE_EXEC(db, "INSERT INTO t1(a, b) values(100, 101), (102, 103), (104, 105);") == SQLITE_OK);
	CU_ASSERT(SQLITE_EXEC(db, "INSERT INTO t2(d, e) values(200, 201), (202,203);") == SQLITE_OK);
	CU_ASSERT(SQLITE_EXEC(db, "INSERT INTO t3(f, g) values(300, 301), (302, 303);") == SQLITE_OK);

}

void test_select(void) {

	SQLITE_INIT
	CU_ASSERT(SQLITE_ASSERT(db, "SELECT * FROM t1;", ROW("100","101"), ROW("102","103"), ROW("104","105")) == SQLITE_OK);
	CU_ASSERT(SQLITE_ASSERT(db, "SELECT * FROM t2;", ROW("200","201"), ROW("202","203")) == SQLITE_OK);
	CU_ASSERT(SQLITE_ASSERT(db, "SELECT f FROM t3;", ROW("300"), ROW("302")) == SQLITE_OK);

}

void test_chcon(void) {

	SQLITE_INIT
	CU_ASSERT(SQLITE_EXEC(db, "PRAGMA chcon('unconfined_u:object_r:column_all:s0 main.t1.security_context');") == SQLITE_OK);

	CU_ASSERT(SQLITE_EXEC(db, "PRAGMA chcon('unconfined_u:object_r:column_all:s0 main.t2.security_context');") == SQLITE_OK);
	CU_ASSERT(SQLITE_EXEC(db, "PRAGMA chcon('unconfined_u:object_r:column_all:s0 main.t2.d');") == SQLITE_OK);
	CU_ASSERT(SQLITE_EXEC(db, "PRAGMA chcon('unconfined_u:object_r:column_all:s0 main.t2.e');") == SQLITE_OK);

	
	CU_ASSERT(SQLITE_EXEC(db, "PRAGMA chcon('unconfined_u:object_r:column_all:s0 main.t3.security_context');") == SQLITE_OK);

}

void test_update_label(void) {

	SQLITE_INIT
	CU_ASSERT(SQLITE_EXEC(db, "UPDATE t1 SET security_context=getcon_id('unconfined_u:object_r:sqlite_tuple_no_select_t:s0') WHERE a=100;") == SQLITE_OK);
	CU_ASSERT(SQLITE_EXEC(db, "UPDATE t2 SET security_context=getcon_id('unconfined_u:object_r:sqlite_tuple_no_update_t:s0') WHERE d=202;") == SQLITE_OK);
	CU_ASSERT(SQLITE_EXEC(db, "UPDATE t3 SET security_context=getcon_id('unconfined_u:object_r:sqlite_tuple_no_delete_t:s0') WHERE f=300;") == SQLITE_OK);

}

void test_select_tuple(void) {

	SQLITE_INIT
	CU_ASSERT(SQLITE_ASSERT(db, "SELECT * FROM t1;", ROW("102","103"), ROW("104","105")) == SQLITE_OK);

}

void test_update_tuple(void) {

	SQLITE_INIT
	CU_ASSERT(SQLITE_EXEC(db, "UPDATE t2 SET d=100000;") == SQLITE_OK);
	CU_ASSERT(SQLITE_ASSERT(db, "SELECT * FROM t2;", ROW("100000","201"), ROW("202","203")) == SQLITE_OK);

}

void test_delete_tuple(void) {

	SQLITE_INIT
	CU_ASSERT(SQLITE_EXEC(db, "DELETE FROM t3;") == SQLITE_OK);
	CU_ASSERT(SQLITE_ASSERT(db, "SELECT f FROM t3;", ROW("300")) == SQLITE_OK);

}

int main(int argc, char **argv) {

	CU_pSuite pSuite = NULL;

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	/* add a suite to the registry */
	pSuite = CU_add_suite("suite_tuple_level", init_suite, clean_suite);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* add the tests to the suite */
	if ((NULL == CU_ADD_TEST(pSuite, test_create_table))
			|| (NULL == CU_ADD_TEST(pSuite, test_insert_table))
			|| (NULL == CU_ADD_TEST(pSuite, test_select))
			|| (NULL == CU_ADD_TEST(pSuite, test_chcon))
			|| (NULL == CU_ADD_TEST(pSuite, test_update_label))
			|| (NULL == CU_ADD_TEST(pSuite, test_select_tuple))
			|| (NULL == CU_ADD_TEST(pSuite, test_update_tuple))
			|| (NULL == CU_ADD_TEST(pSuite, test_delete_tuple))
		) {
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
