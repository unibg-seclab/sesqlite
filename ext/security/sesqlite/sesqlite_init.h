/*
 * sesqlite_init.h
 *
 *      Author: mutti
 */

#include "sesqlite.h"

sqlite3_stmt *stmt_init_id;
seSQLiteHash *init_hash; /* HashMap*/
seSQLiteBiHash *init_hash_id; /* HashMap used to map security_context -> int*/

int set_hash(seSQLiteHash *arg);

int set_hash_id(seSQLiteBiHash *arg);

int initialize(sqlite3 *db);

int compute_sql_context(int isColumn, char *dbName, char *tblName,
	char *colName, struct sesqlite_context_element * con, char **res);

#define SELINUX_CONTEXT_TABLE		"CREATE TABLE IF NOT EXISTS \
selinux_context(security_context INT, \
security_label INT, \
db TEXT, \
name TEXT, \
column TEXT, \
PRIMARY KEY(name, column));" /* use FK */

    /* use rowid */
#define SELINUX_ID_TABLE		"CREATE TABLE IF NOT EXISTS \
selinux_id(security_context INT, \
security_label TEXT UNIQUE);"
