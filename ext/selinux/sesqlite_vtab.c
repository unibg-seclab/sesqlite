/*
 * sesqlite_vtab.c
 *
 *  Created on: Jun 24, 2014
 *      Author: mutti
 */

#include "sesqlite_hash.h"
#include "sesqlite_vtab.h"

int setHashMap(seSQLiteHash *h) {
	phash = h;
	return 0;
}

static int sesqlite_get_row(sesqlite_cursor *c) {
	int rc;
	sesqlite_vtab *v = (sesqlite_vtab *) c->cur.pVtab;
	size_t length = 0;
	char *key = NULL;

	if (c->eof)
		return SQLITE_OK;
	rc = sqlite3_step(c->stmt);
	if (rc == SQLITE_ROW) {

		char *table = (char*) sqlite3_column_text(c->stmt, 1);
		char *column = (char*) sqlite3_column_text(c->stmt, 2);

		if (column == NULL) {
			key = strdup(table);
		} else {
			length = strlen(table) + strlen(column) + 1;
			key = sqlite3_malloc(length);
			strcpy(key, table);
			strcat(key, column);
		}

		char *con = strdup(sqlite3_column_text(c->stmt, 0));
		char *res = NULL;

		res = (char *) seSQLiteHashFind(phash, key, strlen(key));
		if (res == 0) {
			seSQLiteHashInsert(phash, key, strlen(key), con);
		}

		sqlite3_free(key);
		return SQLITE_OK; /* we have a valid row */
	}

	sqlite3_reset(c->stmt);
	c->eof = 1;
	return (rc == SQLITE_DONE ? SQLITE_OK : rc); /* DONE -> OK */
}

static int sesqlite_connect(sqlite3 *db, void *udp, int argc,
		const char * const *argv, sqlite3_vtab **vtab, char **errmsg) {

	sesqlite_vtab *v = NULL;
	sqlite3_stmt **appStmt[N_STATEMENT];
	int rc, i;

	*vtab = NULL;
	*errmsg = NULL;
	if (argc != 3)
		return SQLITE_ERROR;
	if (sqlite3_declare_vtab(db, sesqlite_sql) != SQLITE_OK) {
		return SQLITE_ERROR;
	}

	v = sqlite3_malloc(sizeof(sesqlite_vtab)); /* alloc our custom vtab */
	*vtab = (sqlite3_vtab*) v;
	if (v == NULL)
		return SQLITE_NOMEM;

	//init HashMap
	seSQLiteHashInit(phash, SESQLITE_HASH_STRING, 1);

	appStmt[0] = &v->pSelectAll;
	appStmt[1] = &v->pSelectTable;
	appStmt[2] = &v->pSelectColumn;

	for (i = 0; i < N_STATEMENT && rc == SQLITE_OK; i++) {
		char *zSql = sqlite3_mprintf(azSql[i], argv[1]);
		if (zSql) {
			rc = sqlite3_prepare_v2(db, zSql, -1, appStmt[i], 0);
		} else {
			rc = SQLITE_NOMEM;
		}
		sqlite3_free(zSql);
	}

	v->db = db; /* stash this for later */
	(*vtab)->zErrMsg = NULL; /* initalize this */
	return SQLITE_OK;
}

static int sesqlite_disconnect(sqlite3_vtab *vtab) {
	sqlite3_free(vtab);
	return SQLITE_OK;
}

static int sesqlite_bestindex(sqlite3_vtab *vtab, sqlite3_index_info *pInfo) {

	//TODO VERY SIMPLE IMPLEMENTATION
	/*
	 * if we have 1 constraint -> WHERE table = xxx
	 * if we have 2 constraints -> WHERE table = xxx AND column = yyy
	 */
	int i;
	for (i = 0; i < pInfo->nConstraint; i++) {
		pInfo->aConstraintUsage[i].argvIndex = i + 1;
		pInfo->aConstraintUsage[i].omit = 1;
	}

	switch (pInfo->nConstraint) {
	case 0:
		pInfo->idxNum = QUERY_ALL;
		break;
	case 1:
		pInfo->idxNum = QUERY_TABLE;
		break;
	case 2:
		pInfo->idxNum = QUERY_COLUMN;
		break;
	default:
		break;
	}

	return SQLITE_OK;

}

static int sesqlite_open(sqlite3_vtab *vtab, sqlite3_vtab_cursor **cur) {
	sesqlite_vtab *v = (sesqlite_vtab*) vtab;
	sesqlite_cursor *c = NULL;
	int rc = 0;

	c = sqlite3_malloc(sizeof(sesqlite_cursor));
	*cur = (sqlite3_vtab_cursor*) c;
	if (c == NULL)
		return SQLITE_NOMEM;

	//PRAGMA database_list
//	rc = sqlite3_prepare_v2(v->db,
//			"select * from selinux_context where name='b'", -1, &c->stmt, NULL);
//	if (rc != SQLITE_OK) {
//		*cur = NULL;
//		sqlite3_free(c);
//		return rc;
//	}
	return SQLITE_OK;
}

static int sesqlite_close(sqlite3_vtab_cursor *cur) {
	sqlite3_finalize(((sesqlite_cursor*) cur)->stmt);
	sqlite3_free(cur);
	return SQLITE_OK;
}

static int sesqlite_filter(sqlite3_vtab_cursor *cur, int idxnum,
		const char *idxstr, int argc, sqlite3_value **value) {
	sesqlite_cursor *c = (sesqlite_cursor*) cur;
	sesqlite_vtab *v = (sesqlite_vtab *) cur->pVtab;
	int rc = 0;
	int i;

//	fprintf(stdout, "idxstr: %s\n", idxstr);
//	fprintf(stdout, "idxnum: %d\n", idxnum);
//	fprintf(stdout, "argc: %d\n", argc);
//
//	for (i = 0; i < argc; ++i)
//		fprintf(stdout, "ARGV: %s\n", value[i]->z);

	switch (argc) {
	case 0: /* all */
		rc = sqlite3_prepare_v2(v->db, "select * from selinux_context", -1,
				&c->stmt,
				NULL);
		if (rc != SQLITE_OK) {
			sqlite3_free(c);
			return rc;
		}
		rc = sqlite3_reset(c->stmt); /* start a new scan */
		if (rc != SQLITE_OK)
			return rc;
		c->eof = 0; /* clear EOF flag */

		sesqlite_get_row(c); /* fetch first row */
		break;
	case 1: /* table */
//		fprintf(stdout, "Key: %s.\n", value[0]->z);
////		fprintf(stdout, "Size: %d\n", strlen(value[0]->z));
////
////		int count = seSQLiteHashCount(&hash);
////		fprintf(stdout, "count: %d\n", count);
//
//		char *key = NULL;
//		key = strdup(value[0]->z);
//		c->res = seSQLiteHashFind(phash, key, strlen(key));
//		fprintf(stdout, "Result: %s\n", c->res);
//		c->eof = 1; /* clear EOF flag */
//		free(key);
//
//		sesqlite_get_row(c); /* fetch first row */

//		rc = sqlite3_prepare_v2(v->db,
//				"select * from selinux_context where name = :1", -1, &c->stmt,
//				NULL);
//		if (rc != SQLITE_OK) {
//			sqlite3_free(c);
//			return rc;
//		}
//		sqlite3_bind_text(c->stmt, 1, value[0]->z, strlen(value[0]->z),
//		SQLITE_TRANSIENT);
//		rc = sqlite3_reset(c->stmt); /* start a new scan */
//		if (rc != SQLITE_OK)
//			return rc;
//		c->eof = 0; /* clear EOF flag */
//
//		sesqlite_get_row(c); /* fetch first row */
		break;
	case 2: /* column */
////		c->eof = 1;
////		c->res = seSQLiteHashFind(&hash, value[0]->z, sizeof(value[0]->z));
//		rc = sqlite3_prepare_v2(v->db,
//				"select * from selinux_context where name = :1 and column = :2",
//				-1, &c->stmt,
//				NULL);
//		if (rc != SQLITE_OK) {
//			sqlite3_free(c);
//			return rc;
//		}
//		sqlite3_bind_text(c->stmt, 1, value[0]->z, strlen(value[0]->z),
//		SQLITE_TRANSIENT);
//		sqlite3_bind_text(c->stmt, 2, value[1]->z, strlen(value[1]->z),
//		SQLITE_TRANSIENT);
		break;
	default:
		break;
	}

	return SQLITE_OK;
}

static int sesqlite_next(sqlite3_vtab_cursor *cur) {
	return sesqlite_get_row((sesqlite_cursor*) cur);
}

static int sesqlite_eof(sqlite3_vtab_cursor *cur) {
	return ((sesqlite_cursor*) cur)->eof;
}

static int sesqlite_column(sqlite3_vtab_cursor *cur, sqlite3_context *ctx,
		int cidx) {
	sesqlite_cursor *c = (sesqlite_cursor*) cur;
	sqlite3_result_value(ctx, sqlite3_column_value(c->stmt, cidx));
//	if(c->res != 0)
//		fprintf(stdout, "OKKKKKKK: %s", c->res);

	return SQLITE_OK;
}

static int sesqlite_rowid(sqlite3_vtab_cursor *cur, sqlite3_int64 *rowid) {
	*rowid = sqlite3_column_int64(((sesqlite_cursor*) cur)->stmt, 0);
	return SQLITE_OK;
}

static int sesqlite_rename(sqlite3_vtab *vtab, const char *newname) {
	return SQLITE_OK;
}

