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
const char *SELINUX_TABLE   = "db_table";
const char *SELINUX_COLUMN  = "db_column";

/* SELinux permissions */
const char *SELINUX_SELECT  = "select";
const char *SELINUX_UPDATE  = "update";
const char *SELINUX_INSERT  = "insert";
const char *SELINUX_DELETE  = "delete";
const char *SELINUX_DROP    = "drop";
const char *SELINUX_GETATTR = "getattr";
const char *SELINUX_SETATTR = "setattr";

/* source (process) security context */
char *scon;

/* default target security context */
const char *default_tcon = "sesqlite_public";

/*
 * Get the column context from sesqlite for the specified table.
 * Returns 1 if a context is defined for the table, 0 otherwise. The string
 * representing the context is allocated using sqlite3_mprintf in *con only
 * if the result is 1. The caller has to free the allocated space after usage.
 */
int getColumnContext(
  char **con
){
	int rc = 0;
	return rc;
}

/*
 * Get the table context from sesqlite (the column should be '*', '' or NULL).
 * Returns 1 if a context is defined for the table, 0 otherwise. The string
 * representing the context is allocated using sqlite3_mprintf in *con only
 * if the result is 1. The caller has to free the allocated space after usage.
 */
int getTableContext(
  char **con
){
	int rc = 0;
	return rc;
}

/*
 * Checks whether the source context has been granted the specified permission
 * for the class 'db_table' and the target context associated with the table.
 * Returns 1 if the access has been granted, 0 otherwise.
 */
int checkTableAccess(
  const char *dbname,
  const char *table,
  const char *permission
){
	char *tcon;
	tcon = "change_this"; // TODO query the table
	return 0 == selinux_check_access(scon, tcon, SELINUX_TABLE, permission, NULL);
}

/*
 * Checks whether the source context has been granted the specified permission
 * for the class 'db_column' and the target context associated with the column
 * in the table. Returns 1 if the access has been granted, 0 otherwise.
 */
int checkColumnAccess(
  const char *dbname,
  const char *table,
  const char *column,
  const char *permission
){
	char *tcon;
	tcon = "change_this"; // TODO query the table
	return 0 == selinux_check_access(scon, tcon, SELINUX_COLUMN, permission, NULL);
}

/*
 * Authorizer to be set with sqlite3_set_authorizer that checks the SELinux
 * permission at schema level (tables and columns).
 */
int selinuxAuthorizer(
  void *pUserData,
  int type,
  const char *arg1,
  const char *arg2,
  const char *dbname,
  const char *source
){
	int rc = SQLITE_OK;

	switch ( type )                    /* arg1          | arg2            */
	{

	case SQLITE_CREATE_INDEX:          /* Index Name    | Table Name      */
	    break;

	case SQLITE_CREATE_TABLE:          /* Table Name    | NULL            */
	    break;

	case SQLITE_CREATE_TEMP_INDEX:     /* Index Name    | Table Name      */
	    break;

	case SQLITE_CREATE_TEMP_TABLE:     /* Table Name    | NULL            */
	    break;

	case SQLITE_CREATE_TEMP_TRIGGER:   /* Trigger Name  | Table Name      */
	    break;

	case SQLITE_CREATE_TEMP_VIEW:      /* View Name     | NULL            */
	    break;

	case SQLITE_CREATE_TRIGGER:        /* Trigger Name  | Table Name      */
	    break;

	case SQLITE_CREATE_VIEW:           /* View Name     | NULL            */
	    break;

	case SQLITE_DELETE:                /* Table Name    | NULL            */
		if ( !checkTableAccess(dbname, arg1, SELINUX_DELETE) ) {
			rc = SQLITE_DENY;
		}
	    break;

	case SQLITE_DROP_INDEX:            /* Index Name    | Table Name      */
		if ( !checkTableAccess(dbname, arg2, SELINUX_DROP)) {
			rc = SQLITE_DENY;
		}
		/* no break -- check also index name (arg1) */

	case SQLITE_DROP_TABLE:            /* Table Name    | NULL            */
		if ( !checkTableAccess(dbname, arg1, SELINUX_DROP)) {
			rc = SQLITE_DENY;
		}
	    break;

	case SQLITE_DROP_TEMP_INDEX:       /* Index Name    | Table Name      */
	    break;

	case SQLITE_DROP_TEMP_TABLE:       /* Table Name    | NULL            */
	    break;

	case SQLITE_DROP_TEMP_TRIGGER:     /* Trigger Name  | Table Name      */
	    break;

	case SQLITE_DROP_TEMP_VIEW:        /* View Name     | NULL            */
	    break;

	case SQLITE_DROP_TRIGGER:          /* Trigger Name  | Table Name      */
		if ( !checkTableAccess(dbname, arg2, SELINUX_DROP)) {
			rc = SQLITE_DENY;
		}
		/* no break -- check also trigger name (arg1) */

	case SQLITE_DROP_VIEW:             /* View Name     | NULL            */
		if ( !checkTableAccess(dbname, arg1, SELINUX_DROP)) {
			rc = SQLITE_DENY;
		}
	    break;

	case SQLITE_INSERT:                /* Table Name    | NULL            */
		if ( !checkTableAccess(dbname, arg1, SELINUX_INSERT)) {
			rc = SQLITE_DENY;
		}
		// TODO SE-PostgreSQL also checks the insert on every column
		break;

	case SQLITE_PRAGMA:                /* Pragma Name   | 1st arg or NULL */
	    break;

	case SQLITE_READ:                  /* Table Name    | Column Name     */
		if ( !checkColumnAccess(dbname, arg1, arg2, SELINUX_SELECT)) {
			rc = SQLITE_DENY;
		}
	    break;

	case SQLITE_SELECT:                /* NULL          | NULL            */
	    break;

	case SQLITE_TRANSACTION:           /* Operation     | NULL            */
	    break;

	case SQLITE_UPDATE:                /* Table Name    | Column Name     */
		if ( !checkColumnAccess(dbname, arg1, arg2, SELINUX_UPDATE)) {
			rc = SQLITE_DENY;
		}
	    break;

	case SQLITE_ATTACH:                /* Filename      | NULL            */
	    break;

	case SQLITE_DETACH:                /* Database Name | NULL            */
	    break;

	case SQLITE_ALTER_TABLE:           /* Database Name | Table Name      */
		if ( !checkTableAccess(arg1, arg2, SELINUX_GETATTR)) {
			rc = SQLITE_DENY;
		}
	    break;

	case SQLITE_REINDEX:               /* Index Name    | NULL            */
	    break;

	case SQLITE_ANALYZE:               /* Table Name    | NULL            */
	    break;

	case SQLITE_CREATE_VTABLE:         /* Table Name    | Module Name     */
	    break;

	case SQLITE_DROP_VTABLE:           /* Table Name    | Module Name     */
	    break;

	case SQLITE_FUNCTION:              /* NULL          | Function Name   */
	    break;

	case SQLITE_SAVEPOINT:             /* Operation     | Savepoint Name  */
	    break;

	case SQLITE_COPY:                  /*       - No longer used -        */
	    break;

	default:
	    break;
	}

	return rc;
}

/*
 * Function invoked when using the SQL function selinux_check_access
 */
static void selinuxCheckAccessFunction(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
	int rc = selinux_check_access(
		sqlite3_value_text(argv[0]), 	/* source security context */
		sqlite3_value_text(argv[1]), 	/* target security context */
		sqlite3_value_text(argv[2]), 	/* target security class string */
		sqlite3_value_text(argv[3]), 	/* requested permissions string */
		NULL 							/* auxiliary audit data */
	);

	sqlite3_result_int(context, rc == 0);
}

/*
 * Inizialize the database objects used by SeSqlite:
 * 1. SeSqlite master table that keeps the permission for the schema level
 * 2. Trigger to delete unused SELinux contexts after a drop table statement
 * 3. Trigger to update SELinux contexts after table rename
 */
int initializeSeSqliteObjects(
  sqlite3 *db
){
	int rc = SQLITE_OK;

	// TODO attached databases could not have the triggers an the table, we should
	//      consider adding an hook for the attach or the open database and
	//	    move the table and trigger creation there.

	rc = sqlite3_exec(db,
		"CREATE TABLE IF NOT EXISTS sesqlite_master ( "
  		"name TEXT, "
  		"column TEXT, "
  		"security_context TEXT "
  		");",
  		0, 0, 0);

	// TODO experiments on why triggers are disabled for sqlite_* tables are required
	//      the easy solution is to allow them.

	// create trigger to delete unused SELinux contexts after table drop
	if (rc == SQLITE_OK) {
		rc = sqlite3_exec(db,
			"CREATE TRIGGER IF NOT EXISTS delete_contexts_after_table_drop "
			"AFTER DELETE ON sqlite_master FOR EACH ROW "
			"BEGIN "
			" DELETE FROM sesqlite_master WHERE name = OLD.name; "
			"END;",
			0, 0, 0);
	}

  	// create trigger to update SELinux contexts after table rename
  	if (rc == SQLITE_OK) {
  		rc = sqlite3_exec(db,
  			"CREATE TRIGGER IF NOT EXISTS update_contexts_after_rename "
  			"AFTER UPDATE OF name ON sqlite_master FOR EACH ROW "
  			"BEGIN "
  			" UPDATE sesqlite_master SET name = NEW.name WHERE name = OLD.name;"
  			"END;",
  			0, 0, 0);
  	}

  	return rc;
}

/*
 * Initialize SeSqlite and register objects, authorizer and functions.
 * This function is called by the SQLite core in case the SQLITE_CORE
 * compile flag has been enabled or at runtime when the extension is loaded.
 */
int sqlite3SelinuxInit(
  sqlite3 *db
){
    //retrieve current security context
	int rc = getcon(&scon);
	if (rc == -1) {
		fprintf(stderr, "Error: unable to retrieve the current security context.\n");
		return -1;
	}

	// initialize sesqlite table and trigger
	rc = initializeSeSqliteObjects(db);

	// set the authorizer
	if (rc == SQLITE_OK) {
		rc = sqlite3_set_authorizer(db, selinuxAuthorizer, NULL);
	}

	// create the SQL function selinux_check_access
	if (rc == SQLITE_OK) {
		rc = sqlite3_create_function(db, "selinux_check_access", 4,
			SQLITE_UTF8 /* | SQLITE_DETERMINISTIC */, 0,
			selinuxCheckAccessFunction, 0, 0);
	}

	return rc;
}

/* Runtime-loading extension support */

#if !SQLITE_CORE
int sqlite3_extension_init(
  sqlite3 *db,
  char **pzErrMsg,
  const sqlite3_api_routines *pApi
){
  SQLITE_EXTENSION_INIT2(pApi)
  return sqlite3SelinuxInit(db);
}
#endif

#endif
