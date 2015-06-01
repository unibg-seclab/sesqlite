#ifndef __SESQLITE_UTILS_H__
#define __SESQLITE_UTILS_H__

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

/*
 * Get bitmask of allowed permission
 */
int sesqlite_get_allowed_perms(
	const char *scon,
	const char *tcon,
	security_class_t *class_t
);

/*
 * Use selinux_compute_av to check for access.
 */
int sesqlite_check_access_av(
	const char *scon,
	const char *tcon,
	const char *tclass,
	const char *perm
);

//#define SESQLITE_CHECK_ACCESS(SCON, TCON, TCLASS, PERM) selinux_check_access(SCON, TCON, TCLASS, PERM, NULL)
#define SESQLITE_CHECK_ACCESS(SCON, TCON, TCLASS, PERM) sesqlite_check_access_av(SCON, TCON, TCLASS, PERM)

#endif /* __SESQLITE_UTILS_H__ */
