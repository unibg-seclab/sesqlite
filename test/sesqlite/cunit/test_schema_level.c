/*
 * createtabletest.c
 *
 *  Created on: Jun 13, 2014
 *      Author: mutti
 */

#include "sqlite_test_utils.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <CUnit/Console.h>

#define CU_ADD_TEST(suite, test) (CU_add_test(suite, #test, (CU_TestFunc)test))

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

void test_create_table(void) {

    SQLITE_INIT
    CU_ASSERT(SQLITE_EXEC(db, "CREATE TABLE t1(a INT, b INT);") == SQLITE_OK);
    CU_ASSERT(SQLITE_EXEC(db, "CREATE TABLE t2(d INT, e INT);") == SQLITE_OK);
    CU_ASSERT(SQLITE_EXEC(db, "CREATE TABLE t3(f INT, g INT);") == SQLITE_OK);
    CU_ASSERT(SQLITE_EXEC(db, "CREATE TABLE t4(x INT);") == SQLITE_AUTH); /* no create */
    CU_ASSERT(SQLITE_EXEC(db, "CREATE TABLE t5(y INT);") == SQLITE_OK); /* no create */
    CU_ASSERT(SQLITE_EXEC(db, "CREATE TABLE t6(z INT);") == SQLITE_OK); /* no create */
}

void test_insert_table(void) {

    SQLITE_INIT
    CU_ASSERT(SQLITE_EXEC(db, "INSERT INTO t1(a, b) values(100, 101);") == SQLITE_OK);
    CU_ASSERT(SQLITE_EXEC(db, "INSERT INTO t2(d, e) values(200, 201);") == SQLITE_OK);
    CU_ASSERT(SQLITE_EXEC(db, "INSERT INTO t3(f, g) values(300, 301);") == SQLITE_OK);
    CU_ASSERT(SQLITE_EXEC(db, "INSERT INTO t5(y) values(500);") == SQLITE_OK);
    CU_ASSERT(SQLITE_EXEC(db, "INSERT INTO t6(z) values(600);") == SQLITE_AUTH);

}

void test_update_table(void) {

    SQLITE_INIT
    CU_ASSERT(SQLITE_EXEC(db, "UPDATE t1 SET a=110;") == SQLITE_OK);
    CU_ASSERT(SQLITE_EXEC(db, "UPDATE t1 SET b=120;") == SQLITE_OK);

    CU_ASSERT(SQLITE_EXEC(db, "UPDATE t2 SET d=210;") == SQLITE_AUTH);
    CU_ASSERT(SQLITE_EXEC(db, "UPDATE t2 SET e=220;") == SQLITE_AUTH);

    CU_ASSERT(SQLITE_EXEC(db, "UPDATE t3 SET f=310;") == SQLITE_OK);
    CU_ASSERT(SQLITE_EXEC(db, "UPDATE t3 SET g=320;") == SQLITE_AUTH);

    CU_ASSERT(SQLITE_EXEC(db, "UPDATE t5 SET y=510;") == SQLITE_OK);
    CU_ASSERT(SQLITE_EXEC(db, "UPDATE t6 SET z=610;") == SQLITE_OK);

}

void test_delete_table(void) {

    SQLITE_INIT
    CU_ASSERT(SQLITE_EXEC(db, "DELETE FROM t1 WHERE a=110") == SQLITE_OK);
    CU_ASSERT(SQLITE_EXEC(db, "DELETE FROM t5 WHERE y=510") == SQLITE_OK);
}

void test_select_table(void) {


    SQLITE_INIT
    CU_ASSERT(SQLITE_EXEC(db, "SELECT * FROM t3;") == SQLITE_AUTH);
    CU_ASSERT(SQLITE_EXEC(db, "SELECT e FROM t6;") == SQLITE_AUTH);
    CU_ASSERT(SQLITE_EXEC(db, "SELECT * FROM t1;") == SQLITE_OK);
    CU_ASSERT(SQLITE_EXEC(db, "SELECT * FROM t2;") == SQLITE_OK);
}



int main(int argc, char **argv) {

    int rc;
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
	    return CU_get_error();

    /* add a suite to the registry */
    pSuite = CU_add_suite("suite_schema_level", init_suite, clean_suite);
    if (NULL == pSuite) {
	    CU_cleanup_registry();
	    return CU_get_error();
    }

    /* add the tests to the suite */
    if ((NULL == CU_ADD_TEST(pSuite, test_create_table))
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
