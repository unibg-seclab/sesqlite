/* SeSqlite extension to add SELinux checks in SQLite */

#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX)

#ifndef SQLITE_CORE
#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#else
#include "sqlite3.h"
#endif

#include <stdlib.h>
#include <selinux/selinux.h>
#include "selinux.h"

/* SELinux classes */
#define SELINUX_DB_TABLE      "db_table"
#define SELINUX_DB_COLUMN     "db_column"
#define SELINUX_DB_TUPLE    "db_tuple"

/* SELinux permissions */
/* common database */
const char *SELINUX_CREATE = "create";
const char *SELINUX_DROP = "drop";
const char *SELINUX_GETATTR = "getattr";
const char *SELINUX_SETATTR = "setattr";
const char *SELINUX_RELABELFROM = "relabelfrom";
const char *SELINUX_RELABELTO = "relabelto";

/* db_xxx */
const char *SELINUX_SELECT = "select";
const char *SELINUX_UPDATE = "update";
const char *SELINUX_INSERT = "insert";
const char *SELINUX_DELETE = "delete";

/* source (process) security context */
security_context_t scon;

/* default target security context */
#define DEFAULT_TCON          "sesqlite_public"

/* prepared statements to query schema sesqlite_master (bind it before use) */
sqlite3_stmt *sesqlite_table_stmt;
sqlite3_stmt *sesqlite_column_stmt;

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

/*
 * Get the column context from sesqlite for the specified table.
 * Returns 1 if a context is defined for the table, 0 otherwise. The string
 * representing the context is allocated using sqlite3_mprintf in *con only
 * if the result is 1. The caller has to free the allocated space after usage.
 */
int getColumnContext(const char *dbname, const char *table, const char *column,
		char **con) {
	// TODO another way would be to use a virtual table and query its structure.
	auth_enabled = 0;
	int rc = 0;

	// TODO use dbname when multiple databases are supported by SeSqlite.

	sqlite3_stmt *sesqlite_stmt = sesqlite_table_stmt;

	if (column) {
		sesqlite_stmt = sesqlite_column_stmt;
		sqlite3_bind_text(sesqlite_stmt, SESQLITE_IDX_COLUMN, column, -1, NULL);
	}

	sqlite3_bind_text(sesqlite_stmt, SESQLITE_IDX_NAME, table, -1, NULL);
	int res = sqlite3_step(sesqlite_stmt);

	if (res == SQLITE_ROW) {
		const char *security_context = sqlite3_column_text(sesqlite_stmt, 0);
		*con = sqlite3_mprintf(security_context);
		rc = 1; /* security_context found */
	}

	printf("getcontext: table=%s column=%s -> %sfound (%d)\n", table,
			(column ? column : "NULL"), (rc ? "" : "not "), res);

	sqlite3_reset(sesqlite_stmt);
	sqlite3_clear_bindings(sesqlite_stmt);
	auth_enabled = 1;
	return rc;
}

/*
 * Get the table context from sesqlite (the column should be NULL).
 * Returns 1 if a context is defined for the table, 0 otherwise. The string
 * representing the context is allocated using sqlite3_mprintf in *con only
 * if the result is 1. The caller has to free the allocated space after usage.
 */
int getTableContext(const char *dbname, const char *table, char **con) {
	return getColumnContext(dbname, table, NULL, con);
}

/*
 * Get the default context from sesqlite. Always return 1. The string
 * representing the context is allocated using sqlite3_mprintf in *con.
 * The caller has to free the allocated space after usage.
 */
int getDefaultContext(char **con) {
	*con = sqlite3_mprintf(DEFAULT_TCON);
	return 1;
}

/*
 * Checks whether the source context has been granted the specified permission
 * for the class 'db_table' and the target context associated with the table.
 * Returns 1 if the access has been granted, 0 otherwise.
 */
int checkTableAccess(const char *dbname, const char *table,
		const char *permission) {
	security_context_t tcon;
	getTableContext(dbname, table, &tcon) || getDefaultContext(&tcon);

	int res = selinux_check_access(scon, tcon, SELINUX_DB_TABLE, permission,
	NULL);
	printf("selinux_check_access(%s, %s, %s, %s) => %d\n", scon, tcon,
	SELINUX_DB_TABLE, permission, res);

	sqlite3_free(tcon);
	return 0 == res;
}

/*
 * Checks whether the source context has been granted the specified permission
 * for the class 'db_column' and the target context associated with the column
 * in the table. Returns 1 if the access has been granted, 0 otherwise.
 */
int checkColumnAccess(const char *dbname, const char *table, const char *column,
		const char *permission) {

	security_context_t tcon;
	getColumnContext(dbname, table, column, &tcon)
			|| getTableContext(dbname, table, &tcon)
			|| getDefaultContext(&tcon);

	int res = selinux_check_access(scon, tcon, SELINUX_DB_COLUMN, permission,
	NULL);
	printf("selinux_check_access(%s, %s, %s, %s) => %d\n", scon, tcon,
	SELINUX_DB_COLUMN, permission, res);

	sqlite3_free(tcon);
	return 0 == res;
}

/*
 * Authorizer to be set with sqlite3_set_authorizer that checks the SELinux
 * permission at schema level (tables and columns).
 */
int selinuxAuthorizer(void *pUserData, int type, const char *arg1,
		const char *arg2, const char *dbname, const char *source) {
	int rc = SQLITE_OK;

	if (!auth_enabled)
		return rc;

	printf("authorizer: type=%s arg1=%s arg2=%s\n", authtype[type],
			(arg1 ? arg1 : "NULL"), (arg2 ? arg2 : "NULL"));

	switch (type) /* arg1          | arg2            */
	{

	case SQLITE_CREATE_INDEX: /* Index Name    | Table Name      */

		break;

	case SQLITE_CREATE_TABLE: /* Table Name    | NULL            */
		if (!checkTableAccess(dbname, NULL, SELINUX_CREATE)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_CREATE_TEMP_INDEX: /* Index Name    | Table Name      */
		break;

	case SQLITE_CREATE_TEMP_TABLE: /* Table Name    | NULL            */
		break;

	case SQLITE_CREATE_TEMP_TRIGGER: /* Trigger Name  | Table Name      */
		break;

	case SQLITE_CREATE_TEMP_VIEW: /* View Name     | NULL            */
		break;

	case SQLITE_CREATE_TRIGGER: /* Trigger Name  | Table Name      */
		break;

	case SQLITE_CREATE_VIEW: /* View Name     | NULL            */
		break;

	case SQLITE_DELETE: /* Table Name    | NULL            */
		if (!checkTableAccess(dbname, arg1, SELINUX_DELETE)) {
			rc = SQLITE_DENY;
		}
		// TODO check delete permission on all columns.
		break;

	case SQLITE_DROP_INDEX: /* Index Name    | Table Name      */
		if (!checkTableAccess(dbname, arg2, SELINUX_SETATTR)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_DROP_TABLE: /* Table Name    | NULL            */
		if (!checkTableAccess(dbname, arg1, SELINUX_DROP)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_DROP_TEMP_INDEX: /* Index Name    | Table Name      */
		break;

	case SQLITE_DROP_TEMP_TABLE: /* Table Name    | NULL            */
		break;

	case SQLITE_DROP_TEMP_TRIGGER: /* Trigger Name  | Table Name      */
		break;

	case SQLITE_DROP_TEMP_VIEW: /* View Name     | NULL            */
		break;

	case SQLITE_DROP_TRIGGER: /* Trigger Name  | Table Name      */
		if (!checkTableAccess(dbname, arg2, SELINUX_SETATTR)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_DROP_VIEW: /* View Name     | NULL            */
		if (!checkTableAccess(dbname, arg1, SELINUX_DROP)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_INSERT: /* Table Name    | NULL            */
		if (!checkTableAccess(dbname, arg1, SELINUX_INSERT)) {
			rc = SQLITE_DENY;
		}
		// TODO check insert permission on all columns.
		break;

	case SQLITE_PRAGMA: /* Pragma Name   | 1st arg or NULL */
		if (0 == sqlite3_stricmp(arg1, "writable_schema")) {
			fprintf(stderr, "Pragma disabled to guarantee SeSqlite checks.");
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_READ: /* Table Name    | Column Name     */
		if (!checkColumnAccess(dbname, arg1, arg2, SELINUX_SELECT)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_SELECT: /* NULL          | NULL            */
		break;

	case SQLITE_TRANSACTION: /* Operation     | NULL            */
		break;

	case SQLITE_UPDATE: /* Table Name    | Column Name     */
		if (!checkColumnAccess(dbname, arg1, arg2, SELINUX_UPDATE)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_ATTACH: /* Filename      | NULL            */
		// TODO change when multiple databases are supported by SeSqlite.
		fprintf(stderr, "SeSqlite does not support multiple databases yet.");
		rc = SQLITE_DENY;
		break;

	case SQLITE_DETACH: /* Database Name | NULL            */
		// TODO change when multiple databases are supported by SeSqlite.
		fprintf(stderr, "SeSqlite does not support multiple databases yet.");
		rc = SQLITE_DENY;
		break;

	case SQLITE_ALTER_TABLE: /* Database Name | Table Name      */
		if (!checkTableAccess(arg1, arg2, SELINUX_GETATTR)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_REINDEX: /* Index Name    | NULL            */
		break;

	case SQLITE_ANALYZE: /* Table Name    | NULL            */
		break;

	case SQLITE_CREATE_VTABLE: /* Table Name    | Module Name     */
		break;

	case SQLITE_DROP_VTABLE: /* Table Name    | Module Name     */
		if (!checkTableAccess(dbname, arg1, SELINUX_DROP)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_FUNCTION: /* NULL          | Function Name   */
		break;

	case SQLITE_SAVEPOINT: /* Operation     | Savepoint Name  */
		break;

	case SQLITE_COPY: /*       - No longer used -        */
		break;

	default:
		break;
	}

	printf("\n");
	return rc;
}

/*
 * Function invoked when using the SQL function selinux_check_access
 */
static void selinuxCheckAccessFunction(sqlite3_context *context, int argc,
		sqlite3_value **argv) {
	int rc = selinux_check_access(
	sqlite3_value_text(argv[0]), /* source security context */
	sqlite3_value_text(argv[1]), /* target security context */
	sqlite3_value_text(argv[2]), /* target security class string */
	sqlite3_value_text(argv[3]), /* requested permissions string */
	NULL /* auxiliary audit data */
	);

	sqlite3_result_int(context, rc == 0);
}

/*
 * Inizialize the database objects used by SeSqlite:
 * 1. SeSqlite master table that keeps the permission for the schema level
 * 2. Trigger to delete unused SELinux contexts after a drop table statement
 * 3. Trigger to update SELinux contexts after table rename
 */
int initializeSeSqliteObjects(sqlite3 *db) {
	int rc = SQLITE_OK;

	// TODO attached databases could not have the triggers an the table, we should
	//      consider adding an hook for the attach or the open database and
	//	    move the table and trigger creation there.

	rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS sesqlite_master ( "
			"security_context TEXT NOT NULL, "
			"name TEXT NOT NULL, "
			"column TEXT DEFAULT NULL, "
			"PRIMARY KEY (name, column) "
			");", 0, 0, 0);

	// TODO experiments on why triggers are disabled for sqlite_* tables are required
	//      the easy solution is to allow them. Indexes are also disables, and
	//      enabling them causes the execution to interrupt, so imposing a UNIQUE
	//      or PRIMARY KEY constraint for column 'name' in sqlite_master in order
	//      to use it as foreign key in sesqlite_master is not feasible.

	// create trigger to delete unused SELinux contexts after table drop
	if (rc == SQLITE_OK) {
		rc = sqlite3_exec(db,
				"CREATE TRIGGER IF NOT EXISTS delete_contexts_after_table_drop "
						"AFTER DELETE ON sqlite_master "
						"FOR EACH ROW WHEN OLD.type IN ('table', 'view') "
						"BEGIN "
						" DELETE FROM sesqlite_master WHERE name = OLD.name; "
						"END;", 0, 0, 0);
	}

	// create trigger to update SELinux contexts after table rename
	if (rc == SQLITE_OK) {
		rc =
				sqlite3_exec(db,
						"CREATE TRIGGER IF NOT EXISTS update_contexts_after_rename "
								"AFTER UPDATE OF name ON sqlite_master "
								"FOR EACH ROW WHEN NEW.type IN ('table', 'view') "
								"BEGIN "
								" UPDATE sesqlite_master SET name = NEW.name WHERE name = OLD.name; "
								"END;", 0, 0, 0);
	}

	return rc;
}

/*
 * Initialize SeSqlite and register objects, authorizer and functions.
 * This function is called by the SQLite core in case the SQLITE_CORE
 * compile flag has been enabled or at runtime when the extension is loaded.
 */
int sqlite3SelinuxInit(sqlite3 *db) {
	//retrieve current security context
	int rc = getcon(&scon);
	if (rc == -1) {
		fprintf(stderr,
				"Error: unable to retrieve the current security context.\n");
		return -1;
	}

	// initialize sesqlite table and trigger
	rc = initializeSeSqliteObjects(db);

	// prepare sesqlite query statements
	if (rc == SQLITE_OK) {
		rc = sqlite3_prepare_v2(db,
				"SELECT security_context FROM sesqlite_master "
						"WHERE name = ?1 AND column IS NULL LIMIT 1", -1,
				&sesqlite_table_stmt, NULL)
				|| sqlite3_prepare_v2(db,
						"SELECT security_context FROM sesqlite_master "
								"WHERE name = ?1 AND column = ?2 LIMIT 1", -1,
						&sesqlite_column_stmt, NULL);
	}

	// create the SQL function selinux_check_access
	if (rc == SQLITE_OK) {
		rc = sqlite3_create_function(db, "selinux_check_access", 4,
				SQLITE_UTF8 /* | SQLITE_DETERMINISTIC */, 0,
				selinuxCheckAccessFunction, 0, 0);
	}

	// set the authorizer
	if (rc == SQLITE_OK) {
		rc = sqlite3_set_authorizer(db, selinuxAuthorizer, NULL);
	}

	return rc;
}

/* Runtime-loading extension support */

#if !SQLITE_CORE
int sqlite3_extension_init(sqlite3 *db, char **pzErrMsg,
		const sqlite3_api_routines *pApi) {
	SQLITE_EXTENSION_INIT2(pApi)
	return sqlite3SelinuxInit(db);
}
#endif

#endif
