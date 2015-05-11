
#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX)

#ifndef SQLITE_CORE
#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#else
#include "sqlite3.h"
#endif

#include "sesqlite_init.h"
#include "sesqlite_utils.h"
#include "sesqlite_contexts.h"

security_context_t scon = NULL;
security_context_t tcon = NULL;
int scon_id = 0;
int tcon_id = 0;

seSQLiteHash *hash = NULL;
seSQLiteBiHash *hash_id = NULL;

sqlite3_stmt *stmt_insert = NULL;
sqlite3_stmt *stmt_update = NULL;
sqlite3_stmt *stmt_select_id = NULL;
sqlite3_stmt *stmt_select_label = NULL;
sqlite3_stmt *stmt_con_insert = NULL;

struct sesqlite_context *contexts = NULL;

/*
 * In order to check if the database was already opened with SeSQLite we
 * check if the table selinux_id is already in the database.
 */
int isReopen(
	sqlite3 *db,
	int *reopen
){
	sqlite3_stmt *check_stmt = NULL;
	int count = 0;
	int rc = SQLITE_OK;

	rc = sqlite3_prepare_v2(db,
		"SELECT count(*) FROM sqlite_master WHERE type='table' AND tbl_name LIKE 'selinux%';", -1,
		&check_stmt, 0);

	if( SQLITE_OK!=rc ){
		fprintf(stderr, "Error: SQL error in function isReopen\n");
		return SQLITE_ERROR;
	}

	while( sqlite3_step(check_stmt)==SQLITE_ROW ){
		count = sqlite3_column_int(check_stmt, 0);
	}

	sqlite3_reset(check_stmt);
	sqlite3_finalize(check_stmt);

	if( count == 0 ){ /* first open */
		*reopen = 0;
		rc = SQLITE_OK;
	}else if( count == 2 ){ /* this is a reopen */
		*reopen = 1;
		rc = SQLITE_OK;
	}else
		rc = SQLITE_ERROR; /* something wrong happened. */

	return rc;
}

int insert_context(sqlite3 *db, int isColumn, char *dbName, char *tblName,
	char *colName, struct sesqlite_context_element * con, 
	struct sesqlite_context_element *tuple_context) {

    int rc = SQLITE_OK;
    int *value = NULL;
    int *tid = NULL;
    int rowid = 0;
    char *sec_label = NULL;
    char *sec_context = NULL;

    rc = compute_sql_context(isColumn, dbName, tblName, colName, con, &sec_label); 
    assert( sec_label != NULL);

    value = seSQLiteBiHashFindKey(hash_id, sec_label, strlen(sec_label));
    if(value == NULL){
	rc = compute_sql_context(0, dbName, tblName, NULL, tuple_context, &sec_context); 

	tid = seSQLiteBiHashFindKey(hash_id, sec_context, strlen(sec_context));
	assert(tid != NULL); /* check if SELinux can compute a security context */

	sqlite3_bind_int(stmt_insert, 1, *(int *)tid);
	sqlite3_bind_text(stmt_insert, 2, sec_label, strlen(sec_label),
	    SQLITE_TRANSIENT);

	rc = sqlite3_step(stmt_insert);
	rc = sqlite3_reset(stmt_insert);

	rowid = sqlite3_last_insert_rowid(db);
	value = sqlite3_malloc(sizeof(int));
	*value = rowid;
	seSQLiteBiHashInsert(hash_id, value, sizeof(int), sec_label, strlen(sec_label));
	sqlite3_free(sec_context);
	
    }	
    return *(int*)value;
}

/*
 * Makes the key based on the database, the table and the column.
 * The user must invoke free on the returned pointer to free the memory.
 * It returns db:tbl:col if the column is not NULL, otherwise db:tbl.
 */
char *make_key(
	const char *dbName,
	const char *tblName,
	const char *colName
){
	char *key;

	if( tblName==NULL )
		key = sqlite3_mprintf("%s", dbName);
	else if( colName==NULL )
		key = sqlite3_mprintf("%s:%s", dbName, tblName);
	else
		key = sqlite3_mprintf("%s:%s:%s", dbName, tblName, colName);

	return key;
}

/*
 * Returns the context id associated to the column or the table (if
 * colName is NULL) or -1 if there is no context associated.
 */
int get_key(
	sqlite3 *db,
	const char *dbName,
	const char *tblName,
	const char *colName
){
	char *key = make_key(dbName, tblName, colName);
	int *id = seSQLiteHashFind(hash, key, strlen(key));
	free(key);
	return id ? *id : -1;
}

/*
 * Insert the context id specified for the column or the table (if
 * colName is NULL). Returns the id previously associated or -1 if
 * no id was associated before the call.
 */
int insert_key(
	sqlite3 *db,
	const char *dbName,
	const char *tblName,
	const char *colName,
	int id
){
	char *key = make_key(dbName, tblName, colName);
	int *value = sqlite3_malloc(sizeof(int));
	*value = id;
	int *old_id = seSQLiteHashInsert(hash, key, strlen(key), value, sizeof(int), 0);

#ifdef SQLITE_DEBUG
	char *after = sqlite3_mprintf("context: %d.", id);
	sesqlite_print(NULL, dbName, tblName, colName, after);
	free(after);
#endif

	if( old_id ){
		free(key);
		return *old_id;
	}

	return -1;
}

int create_internal_table(
	sqlite3 *db
){
	int rc = SQLITE_OK;

	rc = sqlite3_exec(db, SELINUX_CONTEXT_TABLE, 0, 0, 0);
	if( SQLITE_OK!=rc ) return rc;

	rc = sqlite3_exec(db, SELINUX_ID_TABLE, 0, 0, 0);
	return rc;
}

/* Prepare the statements used in SeSQLite */
int prepare_stmt(
	sqlite3 *db
){
	int rc = sqlite3_prepare_v2(db, "INSERT INTO"
		" selinux_id(security_context, security_label)"
		" VALUES (?1, ?2);", -1, &stmt_insert, 0);
	if( SQLITE_OK!=rc ) return rc;

	rc = sqlite3_prepare_v2(db, "UPDATE selinux_id"
		" SET security_context = ?1;", -1, &stmt_update, 0);
	if( SQLITE_OK!=rc ) return rc;

	rc = sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO"
		" selinux_context(security_context, security_label, db, name, column)"
		" VALUES (?1, ?2, ?3, ?4, ?5);", -1, &stmt_con_insert, 0);
	if( SQLITE_OK!=rc ) return rc;

	rc = sqlite3_prepare_v2(db, "SELECT rowid"
		" FROM selinux_id"
		" WHERE security_label = ?1;", -1, &stmt_select_id, 0);
	if( SQLITE_OK!=rc ) return rc;

	rc = sqlite3_prepare_v2(db,
		"SELECT security_label"
		" FROM selinux_id"
		" WHERE rowid = ?1;", -1, &stmt_select_label, 0);
	return rc;
}

int initialize_mapping(
	sqlite3* db
){
	int rc = SQLITE_OK;
	char *result = NULL;
	char *context = NULL;
	int *id = NULL;
	int *value = NULL;

	struct sesqlite_context_element *pp;
	pp = contexts->tuple_context;

	while( pp!=NULL ){
		value = seSQLiteBiHashFindKey(hash_id,
			pp->security_context,
			strlen(pp->security_context));

		if( value==NULL ){
			sqlite3_bind_int( stmt_insert, 1, 0);
			sqlite3_bind_text(stmt_insert, 2, pp->security_context, -1, SQLITE_TRANSIENT);

			rc = sqlite3_step(stmt_insert);
			assert( rc==SQLITE_DONE );

			id = sqlite3_malloc(sizeof(int));
			*id = sqlite3_last_insert_rowid(db);

			rc = sqlite3_reset(stmt_insert);
			assert( rc==SQLITE_OK);

			context = NULL;
			context = sqlite3MPrintf(db, "%s", pp->security_context);
			seSQLiteBiHashInsert(hash_id, id, sizeof(int), context, strlen(context));
		}
		pp = pp->next;
	}

	compute_sql_context(0, "main", SELINUX_ID, NULL, contexts->tuple_context, &result);
	value = seSQLiteBiHashFindKey(hash_id, result, strlen(result));
	assert(value != NULL);
	sqlite3_bind_int(stmt_update, 1, *(int*)value);

	while( sqlite3_step(stmt_update)==SQLITE_ROW ){
		fprintf(stdout, "update security_context\n");
    }
	
	sqlite3_finalize(stmt_update);
	return SQLITE_OK;
}


/*
 * This function loads the selinux_id and selinux_context tables into the
 * respective hashmaps. This is used when the database is reopened and
 * the contexts must not be loaded from the sesqlite_contexts file.
 */
int load_contexts_from_table(
	sqlite3 *db
){
	sqlite3_stmt *select_stmt;
	int rc = SQLITE_OK;

	/* load the data from selinux_id table */
	rc = sqlite3_prepare_v2(db,
		"SELECT rowid, security_context, security_label FROM selinux_id;", -1,
		&select_stmt, 0);
	if( SQLITE_OK!=rc ) return rc;

	while( sqlite3_step(select_stmt)==SQLITE_ROW ){
		int *rowid = sqlite3_malloc(sizeof(int));
		*rowid = sqlite3_column_int(select_stmt, 0);
		char *ttcon = sqlite3MPrintf(db, "%s", sqlite3_column_text(select_stmt, 2));
		seSQLiteBiHashInsert(hash_id, rowid, sizeof(int), ttcon, strlen(ttcon));
	}

	sqlite3_finalize(select_stmt);

	/* load the data from selinux_context table */
	rc = sqlite3_prepare_v2(db,
		"SELECT security_label, db, name, column FROM selinux_context;", -1,
		&select_stmt, 0);
	if( SQLITE_OK!=rc ) return rc;

	while( sqlite3_step(select_stmt)==SQLITE_ROW ){
		int id = sqlite3_column_int(select_stmt, 0);
		const char *dbName  = sqlite3_column_text(select_stmt, 1);
		const char *tblName = sqlite3_column_text(select_stmt, 2);
		const char *colName = sqlite3_column_text(select_stmt, 3);

		insert_key(db, 
			dbName, 
			( tblName==0 || strlen(tblName)==0) ? NULL : tblName,
			( colName==0 || strlen(colName)==0) ? NULL : colName, 
			id);
	}

	sqlite3_finalize(select_stmt);
	return SQLITE_OK;
}

void selinux_restorecon_pragma(
	void* pArg,
	sqlite3 *db,
	char *args
){
	char *dbName  = strtok(args, ". ");
	char *tblName = strtok(NULL, ". ");
	char *colName = strtok(NULL, ". ");

	CHECK_WRONG_USAGE( dbName==NULL || MORE_TOKENS,
		"USAGE: pragma restorecon(\"db.[table.[column]]\")\n" );

	sesqlite_print("Restoring labels for", dbName, tblName, colName, ".");

	free_sesqlite_context(contexts);
	contexts = read_sesqlite_context(db, SESQLITE_CONTEXTS_PATH);

	int count = reload_sesqlite_contexts(db, stmt_con_insert,
		contexts, dbName, tblName, colName);

	fprintf(stdout, "%d contexts updated.\n", count);
}

void selinux_chcon_pragma(
	void* pArg,
	sqlite3 *db,
	char *args
){
	char *label   = strtok(args, " ");
	char *dbName  = strtok(NULL, ". ");
	char *tblName = strtok(NULL, ". ");
	char *colName = strtok(NULL, ". ");

	CHECK_WRONG_USAGE( label==NULL || dbName==NULL || MORE_TOKENS,
		"USAGE: pragma chcon(\"label db.[table.[column]]\")\n" );

	sesqlite_print("Changing label for", dbName, tblName, colName, ".");
	if( get_key(db, dbName, tblName, colName)==-1 ){
		sesqlite_print("ERROR - No known context for", dbName, tblName, colName, ".");
	}else{
		/*TODO  Temporary FIX in order to use chcon with new labels */
		char *t_label = sqlite3MPrintf(db, "%s", label);
		insert_key(db, dbName, tblName, colName, insert_id(db, dbName, t_label));
		sesqlite_print("Label for", dbName, tblName, colName, "successfully changed.");
	}
}

void selinux_getcon_pragma(
	void* pArg,
	sqlite3 *db,
	char *args
){
	char *dbName  = strtok(args, ". ");
	char *tblName = strtok(NULL, ". ");
	char *colName = strtok(NULL, ". ");

	CHECK_WRONG_USAGE( dbName==NULL || MORE_TOKENS,
		"USAGE: pragma getcon(\"db.[table.[column]]\")\n" );

	int tclass = -1;
	if( tblName==NULL )
		tclass = SELINUX_DB_DATABASE;
	else
		tclass = colName==NULL ? SELINUX_DB_TABLE : SELINUX_DB_COLUMN;

	int id = getContext(db, dbName, tblName, colName, tclass);

	sqlite3_bind_int(stmt_select_label, 1, id);
	sqlite3_step(stmt_select_label);

	sesqlite_print("Getting context for", dbName, tblName, colName, ":");
	fprintf(stdout, "id: %d, label: %s\n", id,
		sqlite3_column_text(stmt_select_label, 0), -1, SQLITE_TRANSIENT);

	sqlite3_reset(stmt_select_label);
}

void selinux_getdefaultcon_pragma(
	void* pArg,
	sqlite3 *db,
	char *args
){
	char *dbName  = strtok(args, ". ");
	char *tblName = strtok(NULL, ". ");
	char *colName = strtok(NULL, ". ");

	CHECK_WRONG_USAGE( dbName==NULL || MORE_TOKENS,
		"USAGE: pragma getdefaultcon(\"db.[table.[column]]\")\n" );

	char *defaultcon = NULL;
	if( tblName ){
		compute_sql_context(colName!=NULL, dbName, tblName, colName,
			colName==NULL ? contexts->table_context : contexts->column_context,
			&defaultcon);
	}else{
		compute_sql_context(colName!=NULL, dbName, tblName, colName,
			contexts->db_context,
			&defaultcon);
	}
	
	if( defaultcon==NULL ){
		sesqlite_print("ERROR - Default context not found for", dbName, tblName, colName, ".");
	}else{
		int id = insert_id(db, "main", defaultcon);
		sesqlite_print("Getting default context for", dbName, tblName, colName, ":");
		fprintf(stdout, "id: %d, label: %s\n", id, defaultcon);
	}
}

void selinux_clearavc_pragma(
	void* pArg,
	sqlite3 *db,
	char *args
){
	sesqlite_clearavc();
	fprintf(stdout, "AVC cleared\n");
}

int register_pragmas(sqlite3 *db){
	int rc;

	rc = sqlite3_create_pragma(db, "restorecon", selinux_restorecon_pragma, 0);
	if( SQLITE_OK!=rc ) return rc;

	rc = sqlite3_create_pragma(db, "chcon", selinux_chcon_pragma, 0);
	if( SQLITE_OK!=rc ) return rc;

    rc = sqlite3_create_pragma(db, "getcon", selinux_getcon_pragma, 0);
	if( SQLITE_OK!=rc ) return rc;

    rc = sqlite3_create_pragma(db, "getdefaultcon", selinux_getdefaultcon_pragma, 0);
	if( SQLITE_OK!=rc ) return rc;

	rc = sqlite3_create_pragma(db, "clearavc", selinux_clearavc_pragma, 0);
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

	int rc = SQLITE_OK;
	int reopen = 0;

#ifdef SQLITE_DEBUG
	fprintf(stdout, "\n == SeSqlite Initialization == \n");
#endif

	/* Allocate and initialize the hash-table used to store tokenizers. */
	hash = sqlite3_malloc(sizeof(seSQLiteHash));
	hash_id = sqlite3_malloc(sizeof(seSQLiteBiHash));

	if( !hash || !hash_id ){
		return SQLITE_NOMEM;
	}else{
		seSQLiteHashInit(hash, SESQLITE_HASH_STRING, 0); /* init */
		seSQLiteBiHashInit(hash_id, SESQLITE_HASH_BINARY, SESQLITE_HASH_STRING, 0); /* init mapping */
	}

	rc = isReopen(db, &reopen);
	if( SQLITE_OK!=rc ) return rc;

	rc = create_internal_table(db);
	if( SQLITE_OK!=rc ) return rc;

	rc = prepare_stmt(db);
	if( SQLITE_OK!=rc ) return rc;
	
	if( reopen ){
		rc = load_contexts_from_table(db);
		if( SQLITE_OK!=rc ) return rc;
	}else{
		contexts = read_sesqlite_context(db, SESQLITE_CONTEXTS_PATH);
		if( !contexts ) return SQLITE_ERROR;

		rc = initialize_mapping(db);
		if( SQLITE_OK!=rc ) return rc;

		load_sesqlite_contexts(db, stmt_con_insert, contexts);
	}

	rc = register_pragmas(db);
	if( SQLITE_OK!=rc ) return rc;

	rc = initialize_authorizer(db);
	if( SQLITE_OK!=rc ) return rc;

#ifdef SELINUX_STATIC_CONTEXT
	sqlite3_set_xattr(db, "security.selinux", "unconfined_u:unconfined_r:unconfined_t:s0");
#else
	rc = getcon(&scon);
	sqlite3_set_xattr(db, "security.selinux", scon);
//	deprecated
//	if(security_compute_create_raw(scon, scon, 4, &tcon) < 0){
//		fprintf(stderr, "SELinux could not compute a default context\n");
//		return SQLITE_ERROR;
//	}
#endif

	scon = sqlite3_get_xattr(db, "security.selinux");
	if( !scon ){
		fprintf(stderr, "Error: SeSQLite was unable to retrieve the security context.\n");
		return SQLITE_ERROR;
	}

	scon_id = insert_id(db, "main", scon);
	assert( scon_id != 0);

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

