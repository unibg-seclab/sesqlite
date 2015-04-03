/* SeSqlite extension to add SELinux checks in SQLite */

#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX)

#include "sesqlite_utils.h"
#include "sesqlite.h"

/*
 * Print messages for SeSQLite.
 */
void printmsg(
	const char *before,
	const char *dbName,
	const char *tblName,
	const char *colName,
	const char *after
){
	fprintf(stdout, "SeSQLite: ");

	if( before )  fprintf(stdout, "%s ", before);
	if( dbName )  fprintf(stdout, "db=%s ", dbName);
	if( tblName ) fprintf(stdout, "table=%s ", tblName);
	if( colName ) fprintf(stdout, "column=%s ", colName);
	if( after )   fprintf(stdout, "%s\n", after);
}

#endif

