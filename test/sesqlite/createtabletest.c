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
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include "sqlite.h"
#include <assert.h>

#define MAX 		10

int main(int argc, char **argv) {

	int i;
	sqlite *db;
	char *zErr;
	int status;
	int parent = getpid();

	unlink("test.db");
	unlink("test.db-journal");
	db = sqlite_open("test.db", 0, &zErr);
	if (db == 0) {
		printf("Cannot initialize: %s\n", zErr);
		return 1;
	}


	/* insert data in sesqlite_master*/
	sqlite3_exec(db, "INSERT INTO sesqlite_master('t1', NULL, 'ok')", 0, 0, 0);
	sqlite3_exec(db, "CREATE TABLE t1(a,b)", 0, 0, 0);
	sqlite3_exec(db, "CREATE TABLE t1(a,b)", 0, 0, 0);
	sqlite3_exec(db, "CREATE TABLE t1(a,b)", 0, 0, 0);




	sqlite_exec(db, "CREATE TABLE t1(a,b)", 0, 0, 0);

	/* */


}

