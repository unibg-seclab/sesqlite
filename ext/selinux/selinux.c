/* SeSqlite extension to add SELinux checks in SQLite */

#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX)

#ifndef SQLITE_CORE
#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#else
#include "sqlite3.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <selinux/selinux.h>
#include "selinux.h"
#include "sesqlite_vtab.h"
#include "sesqlite_hash.h"

/* source (process) security context */
security_context_t scon;

/* prepared statements to query schema sesqlite_master (bind it before use) */
sqlite3_stmt *sesqlite_stmt;

/**
 * HashMap used by the virtual table implementation
 */
static seSQLiteHash hash; /* HashMap*/

/*
 * Get the column context from sesqlite for the specified table.
 * Returns 1 if a context is defined for the table, 0 otherwise. The string
 * representing the context is allocated using sqlite3_mprintf in *con only
 * if the result is 1. The caller has to free the allocated space after usage.
 */
int getContext(const char *dbname, int tclass, const char *table,
		const char *column, char **con) {
// TODO another way would be to use a virtual table and query its structure.
	auth_enabled = 0;
	int rc = 0;
	size_t length = 0;
	char *res = NULL, *key = NULL;

	switch (tclass) {
	case 0: /* database */
		break;
	case 1: /* table */
		key = strdup(table);
		break;
	case 2: /* column */
		length = strlen(table) + strlen(column) + 1;
		key = sqlite3_malloc(length);
		strcpy(key, table);
		strcat(key, column);
		break;
	default:
		break;
	}

	res = seSQLiteHashFind(&hash, key, strlen(key));
	if (res != NULL) {
		*con = strdup(res);
	} else
		getDefaultContext(con);

#ifdef SQLITE_DEBUG
	fprintf(stdout, "%s: table=%s, column=%s -> %sfound (%s)\n", (res != NULL ? "Hash hint" : "default_context"), table,
			(column ? column : "NULL"), (res != NULL ? "" : "not "), res);
#endif

//	// TODO use dbname when multiple databases are supported by SeSqlite.
//
//	sqlite3_bind_text(sesqlite_stmt, SESQLITE_IDX_NAME, table, -1, NULL);
//	sqlite3_bind_text(sesqlite_stmt, SESQLITE_IDX_COLUMN, column, -1, NULL);
//	int res = sqlite3_step(sesqlite_stmt);
//
//	if (res == SQLITE_ROW) {
//		const char *security_context = sqlite3_column_text(sesqlite_stmt, 0);
//		*con = sqlite3_mprintf(security_context);
//		rc = 1; /* security_context found */
//	}
//
//#ifdef SQLITE_DEBUG
//	fprintf(stdout, "getcontext: table=%s column=%s -> %sfound (%d)\n", table,
//			(column ? column : "NULL"), (rc ? "" : "not "), res);
//#endif
//
//	sqlite3_reset(sesqlite_stmt);
//	sqlite3_clear_bindings(sesqlite_stmt);
	auth_enabled = 1;
	sqlite3_free(key);
	return rc;
}

/*
 * Get the table context from sesqlite (the column should be NULL).
 * Returns 1 if a context is defined for the table, 0 otherwise. The string
 * representing the context is allocated using sqlite3_mprintf in *con only
 * if the result is 1. The caller has to free the allocated space after usage.
 */
//int getTableContext(const char *dbname, const char *table, char **con) {
//	return getColumnContext(dbname, table, NULL, con);
//}
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
 * for the classes 'db_table' and 'db_column' and the target context associated with the table/column.
 * Returns 1 if the access has been granted, 0 otherwise.
 */
int checkAccess(const char *dbname, const char *table, const char *column,
		int tclass, int perm) {

	//check
	assert(tclass <= NELEMS(access_vector));
	security_context_t tcon;
	int res = -1;

	getContext(dbname, tclass, table, column, &tcon);
	assert(tcon != NULL);

	res = selinux_check_access(scon, tcon, access_vector[tclass].c_name,
			access_vector[tclass].perm[perm].p_name, NULL);

#ifdef SQLITE_DEBUG
	fprintf(stdout, "selinux_check_access(%s, %s, %s, %s) => %d\n", scon, tcon,
			access_vector[tclass].c_name,
			access_vector[tclass].perm[perm].p_name, res);
#endif

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

#ifdef SQLITE_DEBUG
	fprintf(stdout, "authorizer: type=%s arg1=%s arg2=%s\n", authtype[type],
			(arg1 ? arg1 : "NULL"), (arg2 ? arg2 : "NULL"));
#endif

	switch (type) /* arg1          | arg2            */
	{

	case SQLITE_CREATE_INDEX: /* Index Name    | Table Name      */
		break;

	case SQLITE_CREATE_TABLE: /* Table Name    | NULL            */
		//check if the sesqlite_master contains the security_context to assign,
		//otherwise use the default security_context.
		//TODO in android the default security_context is not always the same (e.g., untrusted_app->app_data_file,
		//release_app->platform_app_data_file)
		if (!checkAccess(dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_CREATE)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_CREATE_TEMP_INDEX: /* Index Name    | Table Name      */
		break;

	case SQLITE_CREATE_TEMP_TABLE: /* Table Name    | NULL            */
		//see SQLITE_CREATE_TABLE comment
		if (!checkAccess(dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_CREATE)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_CREATE_TEMP_TRIGGER: /* Trigger Name  | Table Name      */
		break;

	case SQLITE_CREATE_TEMP_VIEW: /* View Name     | NULL            */
		//TODO TABLE == VIEW??
		if (!checkAccess(dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_CREATE)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_CREATE_TRIGGER: /* Trigger Name  | Table Name      */
		if (!checkAccess(dbname, arg2, NULL, SELINUX_DB_TABLE,
		SELINUX_SETATTR)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_CREATE_VIEW: /* View Name     | NULL            */
		//TODO TABLE == VIEW??
		if (!checkAccess(dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_CREATE)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_DELETE: /* Table Name    | NULL            */
		if (!checkAccess(dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_DELETE)) {
			rc = SQLITE_DENY;
		}
		// TODO check delete permission on all columns.
		break;

	case SQLITE_DROP_INDEX: /* Index Name    | Table Name      */
		if (!checkAccess(dbname, arg2, NULL, SELINUX_DB_TABLE,
		SELINUX_SETATTR)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_DROP_TABLE: /* Table Name    | NULL            */
		if (!checkAccess(dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_DROP)) {
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
		if (!checkAccess(dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_DROP)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_DROP_TRIGGER: /* Trigger Name  | Table Name      */
		if (!checkAccess(dbname, arg2, NULL, SELINUX_DB_TABLE,
		SELINUX_SETATTR)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_DROP_VIEW: /* View Name     | NULL            */
		if (!checkAccess(dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_DROP)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_INSERT: /* Table Name    | NULL            */
		if (!checkAccess(dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_INSERT)) {
			rc = SQLITE_DENY;
		}
		// TODO check insert permission on all columns.
		break;

	case SQLITE_PRAGMA: /* Pragma Name   | 1st arg or NULL */
		if (0 == sqlite3_stricmp(arg1, "writable_schema")) {
			fprintf(stderr,
					"Pragma disabled to guarantee SeSqlite checks. [pragma command: %s]\n",
					arg1);
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_READ: /* Table Name    | Column Name     */

		if (!checkAccess(dbname, arg1, arg2, SELINUX_DB_COLUMN,
		SELINUX_SELECT)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_SELECT: /* NULL          | NULL            */
		//check the table
		break;

	case SQLITE_TRANSACTION: /* Operation     | NULL            */
		break;

	case SQLITE_UPDATE: /* Table Name    | Column Name     */
		if (!checkAccess(dbname, arg1, arg2, SELINUX_DB_COLUMN,
		SELINUX_UPDATE)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_ATTACH: /* Filename      | NULL            */
		// TODO change when multiple databases are supported by SeSqlite.
		if ((arg1 != NULL) && (strlen(arg1) != 0)) {
			fprintf(stderr,
					"SeSqlite does not support multiple databases yet. [db filename: %s]\n",
					arg1);
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_DETACH: /* Database Name | NULL            */
		// TODO change when multiple databases are supported by SeSqlite.
		if ((arg1 != NULL) && (strlen(arg1) != 0)) {
			fprintf(stderr,
					"SeSqlite does not support multiple databases yet. [db filename: %s]\n",
					arg1);
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_ALTER_TABLE: /* Database Name | Table Name      */
		if (!checkAccess(arg1, arg2, NULL, SELINUX_DB_TABLE,
		SELINUX_GETATTR)) {
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
		if (!checkAccess(dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_DROP)) {
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

#ifdef SQLITE_DEBUG
	printf("\n");
#endif

	return rc;
}

/*
 * Function invoked when using the SQL function selinux_check_access
 */
static void selinuxCheckAccessFunction(sqlite3_context *context, int argc,
		sqlite3_value **argv) {
	int rc = selinux_check_access(sqlite3_value_text(argv[0]), /* source security context */
	sqlite3_value_text(argv[1]), /* target security context */
	sqlite3_value_text(argv[2]), /* target security class string */
	sqlite3_value_text(argv[3]), /* requested permissions string */
	NULL /* auxiliary audit data */
	);

	sqlite3_result_int(context, rc == 0);
}

void insertData(sqlite3 *db) {
	int rc;
	//TODO TEST
	rc =
			sqlite3_exec(db,
					"INSERT INTO selinux_context(security_context, name) values ('unconfined_u:object_r:sesqlite_public:s0', 'selinux_context')",
					NULL, NULL, NULL);

	rc =
			sqlite3_exec(db,
					"INSERT INTO selinux_context values ('unconfined_u:object_r:sesqlite_public:s0', 'selinux_context', 'security_context')",
					NULL, NULL, NULL);

	rc =
			sqlite3_exec(db,
					"INSERT INTO selinux_context values ('unconfined_u:object_r:sesqlite_public:s0', 'selinux_context', 'name')",
					NULL, NULL, NULL);

	rc =
			sqlite3_exec(db,
					"INSERT INTO selinux_context values ('unconfined_u:object_r:sesqlite_public:s0', 'selinux_context', 'column')",
					NULL, NULL, NULL);

	/* sqlite_master */

	rc =
			sqlite3_exec(db,
					"INSERT INTO selinux_context(security_context, name) values ('unconfined_u:object_r:sesqlite_public:s0', 'sqlite_master')",
					NULL, NULL, NULL);

	rc =
			sqlite3_exec(db,
					"INSERT INTO selinux_context values ('unconfined_u:object_r:sesqlite_public:s0', 'sqlite_master', 'type')",
					NULL, NULL, NULL);

	rc =
			sqlite3_exec(db,
					"INSERT INTO selinux_context values ('unconfined_u:object_r:sesqlite_public:s0', 'sqlite_master', 'name')",
					NULL, NULL, NULL);

	rc =
			sqlite3_exec(db,
					"INSERT INTO selinux_context values ('unconfined_u:object_r:sesqlite_public:s0', 'sqlite_master', 'tbl_name')",
					NULL, NULL, NULL);

	rc =
			sqlite3_exec(db,
					"INSERT INTO selinux_context values ('unconfined_u:object_r:sesqlite_public:s0', 'sqlite_master', 'rootpage')",
					NULL, NULL, NULL);

	rc =
			sqlite3_exec(db,
					"INSERT INTO selinux_context values ('unconfined_u:object_r:sesqlite_public:s0', 'sqlite_master', 'sql')",
					NULL, NULL, NULL);

	/* FOR TEST*/
	//CREATE TABLE t1(a INTEGER, b INTEGER, c TEXT);
	//CREATE TABLE t2(a INTEGER, b INTEGER, c TEXT);

	rc =
				sqlite3_exec(db,
						"INSERT INTO selinux_context(security_context, name) values ('unconfined_u:object_r:sesqlite_public:s0', 't1')",
						NULL, NULL, NULL);
	rc =
			sqlite3_exec(db,
					"INSERT INTO selinux_context values ('unconfined_u:object_r:sesqlite_public:s0', 't1', 'a')",
					NULL, NULL, NULL);
	rc =
			sqlite3_exec(db,
					"INSERT INTO selinux_context values ('unconfined_u:object_r:sesqlite_public:s0', 't1', 'b')",
					NULL, NULL, NULL);
	rc =
			sqlite3_exec(db,
					"INSERT INTO selinux_context values ('unconfined_u:object_r:sesqlite_public:s0', 't1', 'c')",
					NULL, NULL, NULL);

	rc =
				sqlite3_exec(db,
						"INSERT INTO selinux_context(security_context, name) values ('unconfined_u:object_r:sesqlite_public:s0', 't2')",
						NULL, NULL, NULL);
	rc =
			sqlite3_exec(db,
					"INSERT INTO selinux_context values ('unconfined_u:object_r:sesqlite_public:s0', 't2', 'a')",
					NULL, NULL, NULL);
	rc =
			sqlite3_exec(db,
					"INSERT INTO selinux_context values ('unconfined_u:object_r:sesqlite_public:s0', 't2', 'b')",
					NULL, NULL, NULL);
	rc =
			sqlite3_exec(db,
					"INSERT INTO selinux_context values ('unconfined_u:object_r:sesqlite_public:s0', 't2', 'c')",
					NULL, NULL, NULL);

	sqlite3_exec(db, "select * from sesqlite_master",
	NULL, NULL, NULL);

	//		rc =
	//				sqlite3_exec(db,
	//						"INSERT INTO selinux_context values ('', 'selinux_context', 'column')",
	//						NULL, NULL, NULL);

}

/*
 * Inizialize the database objects used by SeSqlite:
 * 1. SeSqlite master table that keeps the permission for the schema level
 * 2. Trigger to delete unused SELinux contexts after a drop table statement
 * 3. Trigger to update SELinux contexts after table rename
 */
int initializeSeSqliteObjects(sqlite3 *db) {
	int rc = SQLITE_OK;
	char *pzErr;

#ifdef SQLITE_DEBUG
	fprintf(stdout, "\n == SeSqlite Initialization == \n");
#endif

	//
	setHashMap(&hash);

	/* register module */
	rc = sqlite3_create_module(db, "selinuxModule", &sesqlite_mod, NULL);
	if (rc != SQLITE_OK) {
		return rc;
	}

#ifdef SQLITE_DEBUG
	if (rc == SQLITE_OK)
	fprintf(stdout, "Module 'selinuxModule' registered successfully.\n");
	else
	fprintf(stderr, "Error: unable to register 'sesqliteModule' module.\n");
#endif

// 		TODO attached databases could not have the triggers an the table, we should
//      consider adding an hook for the attach or the open database and
//	    move the table and trigger creation there.
	if (rc == SQLITE_OK) {
		/* automatically create an instance of the virtual table */
		rc =
		sqlite3_exec(db,
				"CREATE VIRTUAL TABLE sesqlite_master USING selinuxModule",
				NULL, NULL, NULL);

		//TODO WHERE??
		rc =
				sqlite3_exec(db,
						"CREATE TABLE IF NOT EXISTS selinux_context(security_context TEXT, name TEXT, column TEXT, PRIMARY KEY(name, column))",
						NULL, NULL, NULL);

		insertData(db);

#ifdef SQLITE_DEBUG
		if (rc == SQLITE_OK)
		fprintf(stdout, "Virtual table 'sesqlite_master' created successfully.\n");
		else
		fprintf(stderr, "Error: unable to create VirtualTable 'sesqlite_master'.\n");
#endif
	}

// 		TODO experiments on why triggers are disabled for sqlite_* tables are required
//      the easy solution is to allow them. Indexes are also disables, and
//      enabling them causes the execution to interrupt, so imposing a UNIQUE
//      or PRIMARY KEY constraint for column 'name' in sqlite_master in order
//      to use it as foreign key in sesqlite_master is not feasible.
// 		create trigger to delete unused SELinux contexts after table drop
	if (rc == SQLITE_OK) {
		rc =
		sqlite3_exec(db, "CREATE TEMP TRIGGER delete_contexts_after_table_drop "
				"AFTER DELETE ON sqlite_master "
				"FOR EACH ROW WHEN OLD.type IN ('table', 'view') "
				"BEGIN "
				" DELETE FROM sesqlite_master WHERE name = OLD.name; "
				"END;", 0, 0, 0);

#ifdef SQLITE_DEBUG
		if (rc == SQLITE_OK)
		fprintf(stdout, "Trigger 'delete_contexts_after_table_drop' created successfully.\n");
		else
		fprintf(stderr, "Error: unable to create 'delete_contexts_after_table_drop' trigger.\n");
#endif
	}

// create trigger to update SELinux contexts after table rename
	if (rc == SQLITE_OK) {
		rc =
				sqlite3_exec(db,
						"CREATE TEMP TRIGGER update_contexts_after_rename "
								"AFTER UPDATE OF name ON sqlite_master "
								"FOR EACH ROW WHEN NEW.type IN ('table', 'view') "
								"BEGIN "
								" UPDATE sesqlite_master SET name = NEW.name WHERE name = OLD.name; "
								"END;", 0, 0, 0);

#ifdef SQLITE_DEBUG
		if (rc == SQLITE_OK) {
			fprintf(stdout, "Trigger 'update_contexts_after_rename' created successfully.\n");
			fprintf(stdout, " == SeSqlite Initialized == \n\n");
		} else
		fprintf(stderr, "Error: unable to create 'update_contexts_after_rename' trigger.\n");
#endif
	}

#ifdef SQLITE_DEBUG
	if (rc != SQLITE_OK)
	fprintf(stderr, "Error: unable to initialize the selinux support for SQLite.\n");
#endif

	return rc;
}

/*
 * Function: sqlite3SelinuxInit
 * Purpose: Initialize SeSqlite and register objects, authorizer and functions.
 * 			This function is called by the SQLite core in case the SQLITE_CORE
 * 			compile flag has been enabled or at runtime when the extension is loaded.
 * Parameters:
 * 				sqlite3 *db: a pointer to the SQLite database.
 * Return value: 0->OK, other->ERROR (see **pzErr for info about error)
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

// create the SQL function selinux_check_access
	if (rc == SQLITE_OK) {
		rc = sqlite3_create_function(db, "selinux_check_access", 4,
		SQLITE_UTF8 /* | SQLITE_DETERMINISTIC */, 0, selinuxCheckAccessFunction,
				0, 0);
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
	SQLITE_EXTENSION_INIT2(pApi);
	return sqlite3SelinuxInit(db);
}
#endif

#endif /* !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX) */
