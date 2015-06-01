/* SeSqlite extension to add SELinux checks in SQLite */

#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX)

#include "sesqlite_utils.h"
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
){
	fprintf(stdout, "SeSQLite: ");

	if( before )  fprintf(stdout, "%s ", before);
	if( dbName )  fprintf(stdout, "db=%s ", dbName);
	if( tblName ) fprintf(stdout, "table=%s ", tblName);
	if( colName ) fprintf(stdout, "column=%s ", colName);
	if( after )   fprintf(stdout, "%s\n", after);
}

/*
 * Makes the key based on the database, the table and the column.
 * The user must invoke free on the returned pointer to free the memory.
 * It returns db:tbl:col if the column is not NULL, otherwise db:tbl.
 */
char *make_key(
	const char *dbName,
	const char *tblName,
	const char *colName
){
	char *key;

	if( tblName==NULL )
		key = sqlite3_mprintf("%s", dbName);
	else if( colName==NULL )
		key = sqlite3_mprintf("%s:%s", dbName, tblName);
	else
		key = sqlite3_mprintf("%s:%s:%s", dbName, tblName, colName);

	return key;
}

/*
 * Get bitmask of allowed permission
 */
int sesqlite_get_allowed(
	const char *scon,
	const char *tcon,
	security_class_t class_t
){
	struct av_decision avd;
	security_compute_av(scon, tcon, class_t, 0, &avd);
	return (avd.allowed | avd.auditallow);
}

/*
 * Use selinux_compute_av to check for access.
 */
int sesqlite_check_access_av(
	const char *scon,
	const char *tcon,
	const char *tclass,
	const char *perm
){
	security_class_t class_t = string_to_security_class(tclass);
	return sesqlite_get_allowed(scon, tcon, class_t) & string_to_av_perm(class_t, perm);
}

#endif

