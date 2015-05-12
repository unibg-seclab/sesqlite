#include "sesqlite.h"

/*
 * Print messages for SeSQLite.
 */
void sesqlite_print(
	const char *before,
	const char *dbName,
	const char *tblName,
	const char *colName,
	const char *after
);

void sesqlite_clearavc();

/*
 * Makes the key based on the database, the table and the column.
 * The user must invoke free on the returned pointer to free the memory.
 * It returns db:tbl:col if the column is not NULL, otherwise db:tbl.
 */
char *make_key(
	const char *dbName,
	const char *tblName,
	const char *colName
);
