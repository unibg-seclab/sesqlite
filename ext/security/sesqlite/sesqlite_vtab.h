/*
 * sesqlite_vtab.h
 *
 *  Created on: Jun 24, 2014
 *      Author: mutti
 */

#include "sesqlite.h"
#include "sesqlite_hash.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//TODO can be shared among all users in the db
seSQLiteHash *phash; /* HashMap */

static const char *sesqlite_sql =
		"CREATE TABLE selinuxModule ( security_context TEXT, name TEXT, column TEXT );";

#define N_STATEMENT 3
static const char *azSql[N_STATEMENT] = {
/* Read and write the xxx_node table */
"SELECT * FROM selinux_context ",

/* Read and write the xxx_rowid table */
"SELECT security_context FROM selinux_context WHERE "
		"name = ?1",

/* Read and write the xxx_parent table */
"SELECT security_context FROM selinux_context WHERE "
		"name = ?1 AND column = ?2", };

typedef enum sesqliteQueryType {
	QUERY_ALL, /* table scan */
	QUERY_TABLE, /* lookup by rowid */
	QUERY_COLUMN /* QUERY_FULLTEXT + [i] is a full-text search for column i*/
} sesqliteQueryType;

typedef struct sesqlite_vtab_s {
	sqlite3_vtab vtab; /* this must go first */
	sesqliteQueryType iCursorType; /* Copy of sqlite3_index_info.idxNum */
	sqlite3 *db; /* module specific fields then follow */
	sqlite3_stmt *pSelectAll; /* sql statement */
	sqlite3_stmt *pSelectTable; /* sql statement */
	sqlite3_stmt *pSelectColumn; /* sql statement */
} sesqlite_vtab;

typedef struct sesqlite_cursor_s {
	sqlite3_vtab_cursor cur; /* this must go first */
	sqlite3_stmt *stmt; /* sql statement */
	int eof; /* EOF flag */
	char *res;
} sesqlite_cursor;

static int sesqlite_connect(sqlite3 *db, void *udp, int argc,
		const char * const *argv, sqlite3_vtab **vtab, char **errmsg);

static int sesqlite_disconnect(sqlite3_vtab *vtab);

static int sesqlite_bestindex(sqlite3_vtab *vtab, sqlite3_index_info *info);

static int sesqlite_open(sqlite3_vtab *vtab, sqlite3_vtab_cursor **cur);

static int sesqlite_close(sqlite3_vtab_cursor *cur);

static int sesqlite_filter(sqlite3_vtab_cursor *cur, int idxnum,
		const char *idxstr, int argc, sqlite3_value **value);

static int sesqlite_next(sqlite3_vtab_cursor *cur);

static int sesqlite_eof(sqlite3_vtab_cursor *cur);

static int sesqlite_column(sqlite3_vtab_cursor *cur, sqlite3_context *ctx,
		int cidx);

static int sesqlite_rowid(sqlite3_vtab_cursor *cur, sqlite3_int64 *rowid);

static int sesqlite_rename(sqlite3_vtab *vtab, const char *newname);

static int seslite_update(sqlite3_vtab *pVtab, int argc, sqlite3_value ** argv,
		sqlite3_int64 *pRowId);

static sqlite3_module sesqlite_mod = {
/* iVersion      */0,
/* xCreate       */sesqlite_connect,
/* xConnect      */sesqlite_connect,
/* xBestIndex    */sesqlite_bestindex,
/* xDisconnect   */sesqlite_disconnect,
/* xDestroy      */sesqlite_disconnect,
/* xOpen         */sesqlite_open,
/* xClose        */sesqlite_close,
/* xFilter       */sesqlite_filter,
/* xNext         */sesqlite_next,
/* xEof          */sesqlite_eof,
/* xColumn       */sesqlite_column,
/* xRowid        */sesqlite_rowid,
/* xUpdate       */seslite_update,
/* xBegin        */0,
/* xSync         */0,
/* xCommit       */0,
/* xRollback     */0,
/* xFindFunction */0,
/* xRename		 */sesqlite_rename, };

#ifdef __cplusplus
} /* extern "C" */
#endif  /* __cplusplus */
