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

/* source (process) security context */
char *source_context;

int selinux_authorizer(
  void *pUserData,
  int type,
  const char *arg1,
  const char *arg2,
  const char *dbname,
  const char *source
){
	int rc = SQLITE_OK;

	switch ( type ) {                  /* ID | arg1          | arg2            */

	case SQLITE_CREATE_INDEX:          /*  1 | Index Name    | Table Name      */
	    break;
	case SQLITE_CREATE_TABLE:          /*  2 | Table Name    | NULL            */
	    break;
	case SQLITE_CREATE_TEMP_INDEX:     /*  3 | Index Name    | Table Name      */
	    break;
	case SQLITE_CREATE_TEMP_TABLE:     /*  4 | Table Name    | NULL            */
	    break;
	case SQLITE_CREATE_TEMP_TRIGGER:   /*  5 | Trigger Name  | Table Name      */
	    break;
	case SQLITE_CREATE_TEMP_VIEW:      /*  6 | View Name     | NULL            */
	    break;
	case SQLITE_CREATE_TRIGGER:        /*  7 | Trigger Name  | Table Name      */
	    break;
	case SQLITE_CREATE_VIEW:           /*  8 | View Name     | NULL            */
	    break;
	case SQLITE_DELETE:                /*  9 | Table Name    | NULL            */
	    break;
	case SQLITE_DROP_INDEX:            /* 10 | Index Name    | Table Name      */
	    break;
	case SQLITE_DROP_TABLE:            /* 11 | Table Name    | NULL            */
	    break;
	case SQLITE_DROP_TEMP_INDEX:       /* 12 | Index Name    | Table Name      */
	    break;
	case SQLITE_DROP_TEMP_TABLE:       /* 13 | Table Name    | NULL            */
	    break;
	case SQLITE_DROP_TEMP_TRIGGER:     /* 14 | Trigger Name  | Table Name      */
	    break;
	case SQLITE_DROP_TEMP_VIEW:        /* 15 | View Name     | NULL            */
	    break;
	case SQLITE_DROP_TRIGGER:          /* 16 | Trigger Name  | Table Name      */
	    break;
	case SQLITE_DROP_VIEW:             /* 17 | View Name     | NULL            */
	    break;
	case SQLITE_INSERT:                /* 18 | Table Name    | NULL            */
	    break;
	case SQLITE_PRAGMA:                /* 19 | Pragma Name   | 1st arg or NULL */
	    break;
	case SQLITE_READ:                  /* 20 | Table Name    | Column Name     */
	    break;
	case SQLITE_SELECT:                /* 21 | NULL          | NULL            */
	    break;
	case SQLITE_TRANSACTION:           /* 22 | Operation     | NULL            */
	    break;
	case SQLITE_UPDATE:                /* 23 | Table Name    | Column Name     */
	    break;
	case SQLITE_ATTACH:                /* 24 | Filename      | NULL            */
	    break;
	case SQLITE_DETACH:                /* 25 | Database Name | NULL            */
	    break;
	case SQLITE_ALTER_TABLE:           /* 26 | Database Name | Table Name      */
	    break;
	case SQLITE_REINDEX:               /* 27 | Index Name    | NULL            */
	    break;
	case SQLITE_ANALYZE:               /* 28 | Table Name    | NULL            */
	    break;
	case SQLITE_CREATE_VTABLE:         /* 29 | Table Name    | Module Name     */
	    break;
	case SQLITE_DROP_VTABLE:           /* 30 | Table Name    | Module Name     */
	    break;
	case SQLITE_FUNCTION:              /* 31 | NULL          | Function Name   */
	    break;
	case SQLITE_SAVEPOINT:             /* 32 | Operation     | Savepoint Name  */
	    break;
	case SQLITE_COPY:                  /*  0 |       - No longer used -        */
	    break;

	default:
	    break;
	}

	return rc;
}

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

int sqlite3SelinuxInit(
  sqlite3 *db
){
	//retrieve current security context
	int rc = getcon(&source_context);
	if (rc == -1) {
		fprintf(stderr, "Error: unable to retrieve the current security context.\n");
		return -1;
	}

	// Set the authorizer
	rc = sqlite3_set_authorizer(db, selinux_authorizer, NULL);

	// create the SQL function selinux_check_access
	if (rc == SQLITE_OK) {
		rc = sqlite3_create_function(db, "selinux_check_access", 4,
			SQLITE_UTF8 /* | SQLITE_DETERMINISTIC */, 0,
			selinuxCheckAccessFunction, 0, 0);
	}

	return rc;
}

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
