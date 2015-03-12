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
	int rc = SQLITE_OK;
	sesqlite_vtab *v = (sesqlite_vtab *) c->cur.pVtab;
	size_t length = 0;
	char *key = NULL;

	if (c->eof)
		return SQLITE_OK;
	rc = sqlite3_step(c->stmt);
	if (rc == SQLITE_ROW) {
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
//	int i;
//	for (i = 0; i < pInfo->nConstraint; i++) {
//		pInfo->aConstraintUsage[i].argvIndex = i + 1;
//		pInfo->aConstraintUsage[i].omit = 1;
//	}
//
//	switch (pInfo->nConstraint) {
//	case 0:
//		pInfo->idxNum = QUERY_ALL;
//		break;
//	case 1:
//		pInfo->idxNum = QUERY_TABLE;
//		break;
//	case 2:
//		pInfo->idxNum = QUERY_COLUMN;
//		break;
//	default:
//		break;
//	}
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

	//static query
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

	return SQLITE_OK;
}

static int sesqlite_rowid(sqlite3_vtab_cursor *cur, sqlite3_int64 *rowid) {
	*rowid = sqlite3_column_int64(((sesqlite_cursor*) cur)->stmt, 0);
	return SQLITE_OK;
}

static int sesqlite_rename(sqlite3_vtab *vtab, const char *newname) {
	return SQLITE_OK;
}

static int seslite_update(sqlite3_vtab *pVtab, int argc, sqlite3_value ** argv,
		sqlite3_int64 *pRowId) {
	return SQLITE_OK;
}
