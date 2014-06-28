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

#define USE_AVC

/* source (process) security context */
security_context_t scon;

/* prepared statements to query schema sesqlite_master (bind it before use) */
sqlite3_stmt *sesqlite_stmt;

/**
 * HashMap used by the virtual table implementation
 */
static seSQLiteHash hash; /* HashMap*/

#ifdef USE_AVC
static seSQLiteHash avc; /* HashMap*/
#endif

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

	// TODO use dbname when multiple databases are supported by SeSqlite.

	auth_enabled = 1;
	sqlite3_free(key);
	return rc;
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
 * for the classes 'db_table' and 'db_column' and the target context associated with the table/column.
 * Returns 1 if the access has been granted, 0 otherwise.
 */
int checkAccess(const char *dbname, const char *table, const char *column,
		int tclass, int perm) {

	//check
	assert(tclass <= NELEMS(access_vector));
	security_context_t tcon;
	int rc;
	int* res = NULL;
	char *key;

	getContext(dbname, tclass, table, column, &tcon);
	assert(tcon != NULL);

	key = sqlite3_malloc(
			strlen(scon) + strlen(tcon) + strlen(access_vector[tclass].c_name)
					+ strlen(access_vector[tclass].perm[perm].p_name) + 1);
	strcpy(key, scon);
	strcat(key, tcon);
	strcat(key, access_vector[tclass].c_name);
	strcat(key, access_vector[tclass].perm[perm].p_name);
	res = seSQLiteHashFind(&avc, key, sizeof(key));
	if (res == NULL) {
		rc = selinux_check_access(scon, tcon, access_vector[tclass].c_name,
				access_vector[tclass].perm[perm].p_name, NULL);
		seSQLiteHashInsert(&avc, key, sizeof(key), &rc);
	} else {
		rc = 0;
	}
	sqlite3_free(key);

#ifdef SQLITE_DEBUG
	fprintf(stdout, "%s selinux_check_access(%s, %s, %s, %s) => %d\n", res != NULL ? "Hash hint: " : "", scon, tcon,
			access_vector[tclass].c_name,
			access_vector[tclass].perm[perm].p_name, rc);
#endif

	sqlite3_free(tcon);
	return 0 == rc;
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

		//MANCA sqlite3 *db
		//Table *pTable = sqlite3FindTable(db, arg1, dbname);

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
		;
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

	void *res;
	int rc = 1;
	char *key;

#ifdef USE_AVC
	key = sqlite3_malloc(
			strlen(scon) + strlen(argv[1]->z) + strlen(argv[2]->z)
					+ strlen(argv[3]->z));
	strcpy(key, scon);
	strcat(key, argv[1]->z);
	strcat(key, argv[2]->z);
	strcat(key, argv[3]->z);
	res = seSQLiteHashFind(&avc, key, sizeof(key));
	if (res == NULL) {
		rc = selinux_check_access(argv[0]->z, /* source security context */
		argv[1]->z, /* target security context */
		argv[2]->z, /* target security class string */
		argv[3]->z, /* requested permissions string */
		NULL /* auxiliary audit data */
		);
		seSQLiteHashInsert(&avc, key, sizeof(key), &rc);
	}else
		rc = 0;
	sqlite3_free(key);
#else
	rc = selinux_check_access(argv[0]->z, /* source security context */
			argv[1]->z, /* target security context */
			argv[2]->z, /* target security class string */
			argv[3]->z, /* requested permissions string */
			NULL /* auxiliary audit data */
	);
#endif

#ifdef SQLITE_DEBUG
	fprintf(stdout, "selinux_check_access(%s, %s, %s, %s) => %d\n", argv[0]->z, argv[1]->z,
			argv[2]->z,
			argv[3]->z, rc);
#endif

	sqlite3_result_int(context, rc == 0);
}

/**
 *
 */
int initializeContext(sqlite3 *db) {
	int rc = SQLITE_OK;
	int fd;
	int n_line, ntable_line, ncolumn_line, ntuple_line;
	char line[255];
	char *p, *token, *stoken;
	FILE* fp = NULL;

	/* open the sqlite_contexts configuration file*/
	fp = fopen("../test/sesqlite/policy/sesqlite_contexts", "r");

	if (fd == -1) {
		fprintf(stderr, "Error. Unable to open '%s' configuration file.", "");
		return SQLITE_ERROR;
	}

	sesqlite_contexts = (struct sesqlite_context*) malloc(
			sizeof(struct sesqlite_context));
	sesqlite_contexts->table_context = NULL;
	sesqlite_contexts->column_context = NULL;
	sesqlite_contexts->tuple_context = NULL;

	n_line = 0;
	ntable_line = 0;
	ncolumn_line = 0;
	ntuple_line = 0;
	while (fgets(line, sizeof line - 1, fp)) {
		p = line;
		while (isspace(*p))
			p++;
		if (*p == '#' || *p == 0)
			continue;

		token = strtok(p, " \t");
		if (!strcasecmp(token, "db_table"))
			ntable_line++;
		else if (!strcasecmp(token, "db_column"))
			ncolumn_line++;
		else if (!strcasecmp(token, "db_tuple"))
			ntuple_line++;
		else {
			fprintf(stderr, "Not recognized!!!");
		}
		n_line++;
	}

	rewind(fp);
	while (fgets(line, sizeof line - 1, fp)) {
		if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = 0;
		p = line;
		while (isspace(*p))
			p++;
		if (*p == '#' || *p == 0)
			continue;

		token = strtok(p, " \t");
		if (!strcasecmp(token, "db_table")) {
			if (sesqlite_contexts->table_context == NULL) {
				sesqlite_contexts->table_context =
						(struct sesqlite_context_element*) malloc(
								sizeof(struct sesqlite_context_element));
				token = strtok(NULL, " \t");
				char *con = strtok(NULL, " \t");
				sesqlite_contexts->table_context->security_context = strdup(
						con);

				stoken = strtok(token, ".");
				sesqlite_contexts->table_context->fparam = strdup(stoken);
				stoken = strtok(NULL, ".");
				sesqlite_contexts->table_context->sparam = strdup(stoken);

				sesqlite_contexts->table_context->next = NULL;

			} else {
				struct sesqlite_context_element *new;
				new = (struct sesqlite_context_element*) malloc(
						sizeof(struct sesqlite_context_element));
				new->next = NULL;

				token = strtok(NULL, " \t");
				char *con = strtok(NULL, " \t");
				new->security_context = strdup(con);
				stoken = strtok(token, ".");
				new->fparam = strdup(stoken);
				stoken = strtok(NULL, ".");
				new->sparam = strdup(stoken);

				new->next = sesqlite_contexts->table_context;
				sesqlite_contexts->table_context = new;
			}

		} else if (!strcasecmp(token, "db_column")) {
			if (sesqlite_contexts->column_context == NULL) {
				sesqlite_contexts->column_context =
						(struct sesqlite_context_element*) malloc(
								sizeof(struct sesqlite_context_element));
				token = strtok(NULL, " \t");
				char *con = strtok(NULL, " \t");
				sesqlite_contexts->column_context->security_context = strdup(
						con);

				stoken = strtok(token, ".");
				sesqlite_contexts->column_context->fparam = strdup(stoken);
				stoken = strtok(NULL, ".");
				sesqlite_contexts->column_context->sparam = strdup(stoken);

				sesqlite_contexts->column_context->next = NULL;

			} else {
				struct sesqlite_context_element *new;
				new = (struct sesqlite_context_element*) malloc(
						sizeof(struct sesqlite_context_element));
				new->next = NULL;

				token = strtok(NULL, " \t");
				char *con = strtok(NULL, " \t");
				new->security_context = strdup(con);
				stoken = strtok(token, ".");
				new->fparam = strdup(stoken);
				stoken = strtok(NULL, ".");
				new->sparam = strdup(stoken);

				new->next = sesqlite_contexts->column_context;
				sesqlite_contexts->column_context = new;
			}

		} else if (!strcasecmp(token, "db_tuple")) {
			if (sesqlite_contexts->tuple_context == NULL) {
				sesqlite_contexts->tuple_context =
						(struct sesqlite_context_element*) malloc(
								sizeof(struct sesqlite_context_element));
				token = strtok(NULL, " \t");
				char *con = strtok(NULL, " \t");
				sesqlite_contexts->tuple_context->security_context = strdup(
						con);

				stoken = strtok(token, ".");
				sesqlite_contexts->tuple_context->fparam = strdup(stoken);
				stoken = strtok(NULL, ".");
				sesqlite_contexts->tuple_context->sparam = strdup(stoken);

				sesqlite_contexts->tuple_context->next = NULL;

			} else {
				struct sesqlite_context_element *new;
				new = (struct sesqlite_context_element*) malloc(
						sizeof(struct sesqlite_context_element));
				new->next = NULL;

				token = strtok(NULL, " \t");
				char *con = strtok(NULL, " \t");
				new->security_context = strdup(con);
				stoken = strtok(token, ".");
				new->fparam = strdup(stoken);
				stoken = strtok(NULL, ".");
				new->sparam = strdup(stoken);

				new->next = sesqlite_contexts->tuple_context;
				sesqlite_contexts->tuple_context = new;
			}

		} else {
			//fprintf(stderr, "Error!!!\n");
		}
	}

	//TODO sort the contexts, more specific >> less specific

//	struct sesqlite_context_element *pp;
//	pp = sesqlite_contexts->table_context;
//	while (pp != NULL) {
//		fprintf(stdout, "Fparam: %s\n", pp->fparam);
//		fprintf(stdout, "Sparam: %s\n", pp->sparam);
//		fprintf(stdout, "SecCon: %s\n", pp->security_context);
//		pp = pp->next;
//	}
//
//	fprintf(stdout, "\n");
//	pp = NULL;
//	pp = sesqlite_contexts->column_context;
//	while (pp != NULL) {
//		fprintf(stdout, "Fparam: %s\n", pp->fparam);
//		fprintf(stdout, "Sparam: %s\n", pp->sparam);
//		fprintf(stdout, "SecCon: %s\n", pp->security_context);
//		pp = pp->next;
//	}
//
//	fprintf(stdout, "\n");
//	pp = NULL;
//	pp = sesqlite_contexts->tuple_context;
//	while (pp != NULL) {
//		fprintf(stdout, "Fparam: %s\n", pp->fparam);
//		fprintf(stdout, "Sparam: %s\n", pp->sparam);
//		fprintf(stdout, "SecCon: %s\n", pp->security_context);
//		pp = pp->next;
//	}

	/**
	 * get the tables and respectively columns in all databases.
	 */
	int i = -1; /* Database number */
	int j;
	Hash *pTbls;
	HashElem *x;
	Db *pDb;
	char *result = NULL;
	for (i = (db->nDb - 1), pDb = &db->aDb[i]; i >= 0; i--, pDb--) {
		if (!OMIT_TEMPDB || i != 1) {
			pTbls = &db->aDb[i].pSchema->tblHash;
			for (x = sqliteHashFirst(pTbls); x; x = sqliteHashNext(x)) {
				Table *pTab = sqliteHashData(x);
				computeTableContext(db, pDb->zName, pTab->zName,
						sesqlite_contexts->table_context, &result);

				if (result != NULL) {
					sqlite3_bind_text(sesqlite_stmt, 1, result, strlen(result),
							SQLITE_TRANSIENT);
					sqlite3_bind_text(sesqlite_stmt, 2, pTab->zName,
							strlen(pTab->zName), SQLITE_TRANSIENT);
					sqlite3_bind_text(sesqlite_stmt, 3, "", strlen(""),
							SQLITE_TRANSIENT);

					rc = sqlite3_step(sesqlite_stmt);
					rc = sqlite3_reset(sesqlite_stmt);

					rc = insertKey(pTab->zName, NULL, result);
					result = NULL;
				}

				if (pTab) {
					Column *pCol;
					for (j = 0, pCol = pTab->aCol; j < pTab->nCol;
							j++, pCol++) {

						computeColumnContext(db, pDb->zName, pTab->zName,
								pCol->zName, sesqlite_contexts->column_context,
								&result);
						if (result != NULL) {
							sqlite3_bind_text(sesqlite_stmt, 1, result,
									strlen(result), SQLITE_TRANSIENT);
							sqlite3_bind_text(sesqlite_stmt, 2, pTab->zName,
									strlen(pTab->zName), SQLITE_TRANSIENT);
							sqlite3_bind_text(sesqlite_stmt, 3, pCol->zName,
									strlen(pCol->zName), SQLITE_TRANSIENT);

							rc = sqlite3_step(sesqlite_stmt);
							rc = sqlite3_reset(sesqlite_stmt);

							rc = insertKey(pTab->zName, pCol->zName, result);
							result = NULL;
						}
					}
				}
			}
		}
	}
	return rc;

}

/**
 *
 */
int computeTableContext(sqlite3 *db, char *dbName, char *tblName,
		struct sesqlite_context_element * tcon, char **res) {

	int rc = SQLITE_OK;
	int found = 0;
	struct sesqlite_context_element *p = tcon;
	while (found == 0 && p != NULL) {
		if (strcasecmp(dbName, p->fparam) == 0 || strcmp(p->fparam, "*") == 0) {
			if (strcasecmp(tblName, p->sparam) == 0
					|| strcmp(p->sparam, "*") == 0) {
//				fprintf(stdout, "Database: %s, Table: %s, Context found: %s\n",
//						dbName, tblName, p->security_context);
				*res = p->security_context;
				found = 1;
			}
		}
		p = p->next;
	}
	return rc;
}

/**
 *
 */
int computeColumnContext(sqlite3 *db, char *dbName, char *tblName,
		char *colName, struct sesqlite_context_element * ccon, char **res) {

	int rc = SQLITE_OK;
	int found = 0;
	struct sesqlite_context_element *p = ccon;
	while (found == 0 && p != NULL) {
		if (strcasecmp(dbName, p->fparam) == 0 || strcmp(p->fparam, "*") == 0) {
			if (strcasecmp(tblName, p->sparam) == 0
					|| strcmp(p->sparam, "*") == 0) {
//				fprintf(stdout,
//						"Database: %s, Table: %s, Column: %s, Context found: %s\n",
//						dbName, tblName, colName, p->security_context);
				*res = p->security_context;
				found = 1;
			}
		}
		p = p->next;
	}
	return rc;
}

int insertKey(char *tName, char *cName, char *con) {
	int rc = SQLITE_OK;
	char *key = NULL;
	int length = 0;

	if (cName == NULL) {
		key = strdup(tName);
	} else {
		length = strlen(tName) + strlen(cName) + 1;
		key = sqlite3_malloc(length);
		strcpy(key, tName);
		strcat(key, cName);
	}

	char *tcon = strdup(con);
	char *res = NULL;

	res = (char *) seSQLiteHashFind(&hash, key, strlen(key));
	if (res == 0) {
		seSQLiteHashInsert(&hash, key, strlen(key), tcon);
	}

	sqlite3_free(key);
	return rc;
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

//**************
	setHashMap(&hash);
	seSQLiteHashInit(&hash, SESQLITE_HASH_STRING, 1); /* init */
	seSQLiteHashInit(&avc, SESQLITE_HASH_STRING, 1); /* init avc */

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

		rc = sqlite3_prepare_v2(db,
				"INSERT INTO selinux_context values(?1, ?2, ?3);", -1,
				&sesqlite_stmt, 0);

		if (rc == SQLITE_OK)
			rc = initializeContext(db);

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
								" UPDATE selinux_context SET name = NEW.name WHERE name = OLD.name; "
								"END;", 0, 0, 0);

#ifdef SQLITE_DEBUG
		if (rc == SQLITE_OK) {
			//fprintf(stdout, "Trigger 'update_contexts_after_rename' created successfully.\n");
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
	SQLITE_EXTENSION_INIT2(pApi);
	return sqlite3SelinuxInit(db);
}
#endif

#endif /* !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX) */
