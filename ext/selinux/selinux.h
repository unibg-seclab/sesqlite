/* * sesqlite.h
 *
 *  Created on: Jun 10, 2014
 *      Author: mutti
 */

#include "sqlite3.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define NELEMS(x)  (sizeof(x) / sizeof(x[0]))

#define SELINUX_CONTEXT_TABLE		"CREATE TABLE IF NOT EXISTS \
selinux_context( \
security_context INT, \
db TEXT, \
name TEXT, \
column TEXT, \
PRIMARY KEY(name, column));" /* use FK */

    /* use rowid */
#define SELINUX_ID_TABLE		"CREATE TABLE IF NOT EXISTS \
selinux_id( \
security_context TEXT UNIQUE)"

#define SECURITY_CONTEXT_COLUMN_NAME "security_context"
#define SECURITY_CONTEXT_COLUMN_TYPE "hidden text"
#define SECURITY_CONTEXT_COLUMN_DEFAULT_FUNC "getcon()"
#define SECURITY_CONTEXT_COLUMN_DEFAULT "DEFAULT (getcon())"
#define SECURITY_CONTEXT_COLUMN_DEFINITION SECURITY_CONTEXT_COLUMN_NAME " " SECURITY_CONTEXT_COLUMN_TYPE " " SECURITY_CONTEXT_COLUMN_DEFAULT

/* SELinux classes */
#define SELINUX_DB_DATABASE      	0
#define SELINUX_DB_TABLE      		1
#define SELINUX_DB_COLUMN     		2
#define SELINUX_DB_TUPLE    		3

/* SELinux permissions */
/* common database */
#define SELINUX_CREATE 				0
#define SELINUX_DROP 				1
#define SELINUX_GETATTR 			2
#define SELINUX_SETATTR 			3
#define SELINUX_RELABEL_FROM 		4
#define SELINUX_RELABEL_TO 			5

/* db_xxx */
#define SELINUX_SELECT 				6
#define SELINUX_UPDATE 				7
#define SELINUX_INSERT 				8
#define SELINUX_DELETE 				9

/* other */
#define SELINUX_LOAD_MODULE 		10

/* default target security context */
#define DEFAULT_TCON          "unconfined_u:object_r:sesqlite_public:s0"

/* indices to bind paramteres in sesqlite_stmt */
#define SESQLITE_IDX_NAME     1
#define SESQLITE_IDX_COLUMN   2

/* 0 to disable authorizer checks, 1 otherwise */
int auth_enabled = 1;

/* authorizer type */
const char *authtype[] = { "SQLITE_COPY", "SQLITE_CREATE_INDEX",
		"SQLITE_CREATE_TABLE", "SQLITE_CREATE_TEMP_INDEX",
		"SQLITE_CREATE_TEMP_TABLE", "SQLITE_CREATE_TEMP_TRIGGER",
		"SQLITE_CREATE_TEMP_VIEW", "SQLITE_CREATE_TRIGGER",
		"SQLITE_CREATE_VIEW", "SQLITE_DELETE", "SQLITE_DROP_INDEX",
		"SQLITE_DROP_TABLE", "SQLITE_DROP_TEMP_INDEX", "SQLITE_DROP_TEMP_TABLE",
		"SQLITE_DROP_TEMP_TRIGGER", "SQLITE_DROP_TEMP_VIEW",
		"SQLITE_DROP_TRIGGER", "SQLITE_DROP_VIEW", "SQLITE_INSERT",
		"SQLITE_PRAGMA", "SQLITE_READ", "SQLITE_SELECT", "SQLITE_TRANSACTION",
		"SQLITE_UPDATE", "SQLITE_ATTACH", "SQLITE_DETACH", "SQLITE_ALTER_TABLE",
		"SQLITE_REINDEX", "SQLITE_ANALYZE", "SQLITE_CREATE_VTABLE",
		"SQLITE_DROP_VTABLE", "SQLITE_FUNCTION", "SQLITE_SAVEPOINT",
		"SQLITE_RECURSIVE" };

/* */
int sqlite3SelinuxInit(sqlite3 *db);

static struct sesqlite_context *sesqlite_contexts = NULL;

struct sesqlite_context {

	//db
	int ndb_context;
	struct sesqlite_context_element *db_context;

	//table
	int ntable_context;
	struct sesqlite_context_element *table_context;

	//column
	int ncolumn_context;
	struct sesqlite_context_element *column_context;

	//tuple
	int ntuple_context;
	struct sesqlite_context_element *tuple_context;

};

/**
 * Used to store
 */
struct sesqlite_context_element {
	//*.*
	char *origin;
	char *fparam;
	char *sparam;
	char *tparam;
	char *security_context;
	struct sesqlite_context_element *next;
};

/* struct for the management of selinux classes and the relative permissions*/
static struct {
	const char *c_name;
	uint16_t c_code;
	struct {
		const char *p_name;
		uint16_t p_code;
	} perm[32];

} access_vector[] = { { "db_database", SELINUX_DB_DATABASE, { { "create",
SELINUX_CREATE }, { "drop", SELINUX_DROP }, { "getattr", SELINUX_GETATTR }, {
		"setattr", SELINUX_SETATTR }, { "relabelfrom",
SELINUX_RELABEL_FROM }, { "relabelto",
SELINUX_RELABEL_TO }, } }, { "db_table",
SELINUX_DB_TABLE, { { "create", SELINUX_CREATE }, { "drop",
SELINUX_DROP }, { "getattr", SELINUX_GETATTR }, { "setattr", SELINUX_SETATTR },
		{ "relabelfrom",
		SELINUX_RELABEL_FROM }, { "relabelto",
		SELINUX_RELABEL_TO }, { "select", SELINUX_SELECT }, { "update",
		SELINUX_UPDATE }, { "insert", SELINUX_INSERT }, { "delete",
		SELINUX_DELETE }, } }, { "db_column",
SELINUX_DB_COLUMN, { { "create", SELINUX_CREATE }, { "drop", SELINUX_DROP }, {
		"getattr", SELINUX_GETATTR }, { "setattr",
SELINUX_SETATTR }, { "relabelfrom", SELINUX_RELABEL_FROM }, { "relabelto",
SELINUX_RELABEL_TO }, { "select", SELINUX_SELECT },
		{ "update", SELINUX_UPDATE }, { "insert", SELINUX_INSERT }, } }, {
		"db_tuple",
		SELINUX_DB_TUPLE, { { "relabelfrom", SELINUX_RELABEL_FROM }, {
				"relabelto",
				SELINUX_RELABEL_TO }, { "select", SELINUX_SELECT }, { "update",
		SELINUX_UPDATE }, { "insert", SELINUX_INSERT }, { "delete",
		SELINUX_DELETE }, } }, };

#ifdef __cplusplus
} /* extern "C" */
#endif  /* __cplusplus */
