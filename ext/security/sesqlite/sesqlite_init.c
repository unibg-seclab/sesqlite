
#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX)

#ifndef SQLITE_CORE
#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#else
#include "sqlite3.h"
#endif

#include "sesqlite_init.h"

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

struct sesqlite_context *sesqlite_contexts = NULL;

int open_or_reopen(sqlite3 *db){
    int isNew = 0;
    int rc = SQLITE_OK;
    sqlite3_stmt *check_stmt = NULL;

    rc = sqlite3_prepare_v2(db,
	"SELECT count(*) FROM sqlite_master WHERE type='table' and name = 'selinux_id';", -1,
	    &check_stmt, 0);

    while(sqlite3_step(check_stmt) == SQLITE_ROW){
	isNew = sqlite3_column_int(check_stmt, 0);
    }

    sqlite3_finalize(check_stmt);
    return isNew;
}

///**
// *  * Function used to store the mapping between security_context and id in the
// *  'selinux_id' table.
// *   */
//int initialize_id_context(sqlite3 *db, char *con){
//    int rc = SQLITE_OK;
//    int *res = 0;
//    int *id = NULL;
//
//    char *tcon = sqlite3_mprintf("'%s'", con); /* remember, storing quoted string */
//
//    id = seSQLiteHashFind(hash_id, //TODO
//    if(rowid == 0){
//	sqlite3_bind_text(stmt_id_insert, 1, con, strlen(con),
//	    SQLITE_TRANSIENT);
//
//	rc = sqlite3_step(stmt_id_insert);
//	rc = sqlite3_reset(stmt_id_insert);
//
//	rowid = sqlite3_last_insert_rowid(db);
//	seSQLiteHashInsert(hash_id, NULL, rowid, ttcon);
//   }
//
//   return rowid;
//}



/**
 *
 */
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


void insert_key(sqlite3 *db, char *dbName, char *tName, char *cName, int id) {
    char *key = NULL;
    int *value = NULL;

    if (cName == NULL)
	key = sqlite3_mprintf("%s:%s", dbName, tName);
    else
	key = sqlite3_mprintf("%s:%s:%s", dbName, tName, cName);

    value =sqlite3_malloc(sizeof(int));
    *value = id;
    seSQLiteHashInsert(hash, key, strlen(key), value, sizeof(int), 0);
#ifdef SQLITE_DEBUG
fprintf(stdout, "Database: %s Table: %s %s%s Context found: %d\n",
    dbName, tName, cName == NULL ? "" : "Column:", cName == NULL ? "" : cName, id);
#endif
}


/* function to insert a new_node in a list. Note that this
 function expects a pointer to head_ref as this can modify the
 head of the input linked list (similar to push())*/
void sorted_insert(struct sesqlite_context_element** head_ref,
		struct sesqlite_context_element* new_node) {

    struct sesqlite_context_element* current;

    if (*head_ref == NULL
	    || strcasecmp((*head_ref)->origin, new_node->origin) <= 0) {
	new_node->next = *head_ref;
	*head_ref = new_node;
    } else {
	current = *head_ref;
	while (current->next != NULL
	    && strcasecmp(current->next->origin, new_node->origin) > 0) {
	    current = current->next;
	}
	new_node->next = current->next;
	current->next = new_node;
    }
}

int context_reload(sqlite3 *db){

    int rc = SQLITE_OK;
    int n_line, ndb_line, ntable_line, ncolumn_line, ntuple_line;
    char line[255];
    char *p = NULL, *token = NULL, *stoken = NULL;
    FILE* fp = NULL;

    //TODO modify the liselinux in order to retrieve the context from the
    //targeted folder
    fp = fopen("./sesqlite_contexts", "rb");
    if (fp == NULL) {
	    fprintf(stderr, "Error. Unable to open '%s' configuration file.\n",
			    "sesqlite_contexts");
	    return SQLITE_OK;
    }

    sesqlite_contexts = sqlite3_malloc(sizeof(struct sesqlite_context));
    sesqlite_contexts->db_context = NULL;
    sesqlite_contexts->table_context = NULL;
    sesqlite_contexts->column_context = NULL;
    sesqlite_contexts->tuple_context = NULL;

    n_line = 0;
    ndb_line = 0;
    ntable_line = 0;
    ncolumn_line = 0;
    ntuple_line = 0;
    while (fgets(line, sizeof line - 1, fp)) {
	if (line[strlen(line) - 1] == '\n')
		line[strlen(line) - 1] = 0;
	p = line;
	while (isspace(*p))
		p++;
	if (*p == '#' || *p == 0)
		continue;

	token = strtok(p, " \t");
	if (!strcasecmp(token, "db_database")) {
	    struct sesqlite_context_element *new;
	    new = sqlite3_malloc(sizeof(struct sesqlite_context_element));
	    new->next = NULL;

	    token = strtok(NULL, " \t");
	    new->origin = strdup(token);
	    char *con = strtok(NULL, " \t");
	    new->security_context = strdup(con);
	    new->fparam = strdup(token);
	    new->sparam = strdup(token);
	    new->tparam = strdup(token);

	    sorted_insert(&sesqlite_contexts->db_context, new);
	    ndb_line++;

	} else if (!strcasecmp(token, "db_table")) {

	    struct sesqlite_context_element *new;
	    new = sqlite3_malloc(sizeof(struct sesqlite_context_element));
	    new->next = NULL;

	    token = strtok(NULL, " \t");
	    new->origin = strdup(token);
	    char *con = strtok(NULL, " \t");
	    new->security_context = strdup(con);
	    stoken = strtok(token, ".");
	    new->fparam = strdup(stoken);
	    stoken = strtok(NULL, ".");
	    new->sparam = strdup(stoken);
	    new->tparam = NULL;

	    sorted_insert(&sesqlite_contexts->table_context, new);
	    ntable_line++;

	} else if (!strcasecmp(token, "db_column")) {

	    struct sesqlite_context_element *new;
	    new = sqlite3_malloc(sizeof(struct sesqlite_context_element));
	    new->next = NULL;

	    token = strtok(NULL, " \t");
	    new->origin = strdup(token);
	    char *con = strtok(NULL, " \t");
	    new->security_context = strdup(con);
	    stoken = strtok(token, ".");
	    new->fparam = strdup(stoken);
	    stoken = strtok(NULL, ".");
	    new->sparam = strdup(stoken);
	    stoken = strtok(NULL, ".");
	    new->tparam = strdup(stoken);

	    sorted_insert(&sesqlite_contexts->column_context, new);
	    ncolumn_line++;

	} else if (!strcasecmp(token, "db_tuple")) {

	    struct sesqlite_context_element *new;
	    new = sqlite3_malloc(sizeof(struct sesqlite_context_element));
	    new->next = NULL;

	    token = strtok(NULL, " \t");
	    new->origin = strdup(token);
	    char *con = strtok(NULL, " \t");
	    new->security_context = strdup(con);
	    stoken = strtok(token, ".");
	    new->fparam = strdup(stoken);
	    stoken = strtok(NULL, ".");
	    new->sparam = strdup(stoken);
	    new->tparam = NULL;

	    sorted_insert(&sesqlite_contexts->tuple_context, new);
	    ntuple_line++;

	} else {
	    fprintf(stderr,
		"Error, unable to recognize '%s' in sesqlite_context file.\n",
		    token);
	}
    }

    fclose(fp);
}

int create_internal_table(sqlite3 *db){

    int rc = SQLITE_OK;
    rc = sqlite3_exec(db, SELINUX_CONTEXT_TABLE, 0, 0, 0);
    if(rc != SQLITE_OK)
	return rc;

    rc = sqlite3_exec(db, SELINUX_ID_TABLE, 0, 0, 0);
    if(rc != SQLITE_OK)
	return rc;


    return rc;
}

int prepare_stmt(sqlite3 *db){

    int rc = SQLITE_OK;

    /* prepare statements */
    rc = sqlite3_prepare_v2(db,
	"INSERT INTO selinux_id(security_context, security_label) values(?1, ?2);", -1,
	&stmt_insert, 0);
    if(rc != SQLITE_OK)
	return rc;

    rc = sqlite3_prepare_v2(db,
	"UPDATE selinux_id SET security_context = ?1;", -1,
	&stmt_update, 0);
    if(rc != SQLITE_OK)
	return rc;

    rc = sqlite3_prepare_v2(db,
	"INSERT INTO selinux_context(security_context, security_label, db, name, column) values(?1, ?2, ?3, ?4, ?5);", -1,
	&stmt_con_insert, 0);
    if(rc != SQLITE_OK)
	return rc;

    rc = sqlite3_prepare_v2(db,
	"SELECT rowid FROM selinux_id WHERE security_label = ?1;", -1,
	&stmt_select_id, 0);
    if(rc != SQLITE_OK)
	return rc;

    rc = sqlite3_prepare_v2(db,
	"SELECT security_label FROM selinux_id WHERE rowid = ?1;", -1,
	&stmt_select_label, 0);
    if(rc != SQLITE_OK)
	return rc;

    return rc;
}

int initialize_mapping(sqlite3* db){

    int rc = SQLITE_OK;
    char *result = NULL;
    int id = 0;
    int *value = NULL;

    struct sesqlite_context_element *pp;
    pp = sesqlite_contexts->tuple_context;
    while (pp != NULL) {
	value = seSQLiteBiHashFindKey(hash_id, 
		pp->security_context, 
		strlen(pp->security_context));

	if(value == NULL){
	    sqlite3_bind_int(stmt_insert, 1, 0);
	    sqlite3_bind_text(stmt_insert, 
		    2, 
		    pp->security_context, 
		    strlen(pp->security_context),
		    SQLITE_TRANSIENT);

	    rc = sqlite3_step(stmt_insert);
	    id = sqlite3_last_insert_rowid(db);
	    rc = sqlite3_reset(stmt_insert);

	    value = sqlite3_malloc(sizeof(int));
	    *value = id;
	    seSQLiteBiHashInsert(hash_id, 
		    value, 
		    sizeof(int), 
		    pp->security_context, 
		    strlen(pp->security_context));
	}
	pp = pp->next;
    }

    compute_sql_context(0, "main", SELINUX_ID, NULL, sesqlite_contexts->tuple_context, &result);
    value = seSQLiteBiHashFindKey(hash_id, result, strlen(result));
    assert(value != NULL);
    sqlite3_bind_int(stmt_update, 1, *(int *)value);

    while(sqlite3_step(stmt_update) == SQLITE_ROW){
	fprintf(stdout, "update security_context\n");
    }
	
    sqlite3_finalize(stmt_update);
    //sqlite3_finalize(stmt_insert);

    return SQLITE_OK;
}


int initialize_internal_table(sqlite3 *db, int isOpen){

    int rc = SQLITE_OK;
    int *value = NULL;
    /* insert the security context loaded by context_reload function */

    sqlite3_stmt *check_id_stmt;
    sqlite3_stmt *check_context_stmt;
    if(isOpen){
	rc = sqlite3_prepare_v2(db,
	    "SELECT rowid, security_context, security_label FROM selinux_id;", -1,
		&check_id_stmt, 0);
	while(sqlite3_step(check_id_stmt) == SQLITE_ROW){
	    int rowid = sqlite3_column_int(check_id_stmt, 0);
	    char *ttcon = sqlite3MPrintf(db, "%s", sqlite3_column_text(check_id_stmt, 2));

	    value = sqlite3_malloc(sizeof(int));
	    *value = rowid;
	    seSQLiteBiHashInsert(hash_id, value, sizeof(int), ttcon, strlen(ttcon));
	}

	rc = sqlite3_prepare_v2(db,
	    "SELECT security_label, db, name, column FROM selinux_context;", -1,
		&check_context_stmt, 0);
	while(sqlite3_step(check_context_stmt) == SQLITE_ROW){
	    const char *dbName = sqlite3_column_text(check_context_stmt, 1);
	    const char *tblName = sqlite3_column_text(check_context_stmt, 2);
	    const char *colName = sqlite3_column_text(check_context_stmt, 3);
	    int id = sqlite3_column_int(check_context_stmt, 0);
	    insert_key(
	        db,
	        (char *) dbName,
	        (char *) tblName,
	        (colName == 0 || strlen(colName) == 0) ?
	            NULL : (char *) colName,
	        id
	    );
	}

	sqlite3_finalize(check_id_stmt);
	sqlite3_finalize(check_context_stmt);
    }else{
	
	int i = -1;
	int j;
	Hash *pTbls;
	HashElem * x;
	Db *pDb;
	char *result = NULL;
	for (i = (db->nDb - 1), pDb = &db->aDb[i]; i >= 0; i--, pDb--) {
	    if (!OMIT_TEMPDB || i != 1) {
		pTbls = &db->aDb[i].pSchema->tblHash;
		for (x = sqliteHashFirst(pTbls); x; x = sqliteHashNext(x)) {
		    Table *pTab = sqliteHashData(x);
		    int sec_label_id = insert_context(db, 
			    0, 
			    pDb->zName, 
			    pTab->zName, 
			    NULL,
			    sesqlite_contexts->table_context, 
			    sesqlite_contexts->tuple_context);

		    int sec_con_id = insert_context(db, 
			    0, 
			    pDb->zName, 
			    pTab->zName, 
			    NULL,
			    sesqlite_contexts->tuple_context, 
			    sesqlite_contexts->tuple_context);

		    sqlite3_bind_int(stmt_con_insert, 1, sec_con_id);
		    sqlite3_bind_int(stmt_con_insert, 2, sec_label_id);
		    sqlite3_bind_text(stmt_con_insert, 3, pDb->zName,
			strlen(pDb->zName), SQLITE_TRANSIENT);
		    sqlite3_bind_text(stmt_con_insert, 4, pTab->zName,
			strlen(pTab->zName), SQLITE_TRANSIENT);
		    sqlite3_bind_text(stmt_con_insert, 5, "", strlen(""),
			SQLITE_TRANSIENT);

		    rc = sqlite3_step(stmt_con_insert);
		    rc = sqlite3_reset(stmt_con_insert);
		    insert_key(db, 
			    pDb->zName, 
			    pTab->zName, 
			    NULL, 
			    sec_label_id);

		    if (pTab) {
			Column *pCol;
			for (j = 0, pCol = pTab->aCol; j < pTab->nCol; j++, pCol++) {
			    int sec_label_id = insert_context(db, 
				    1, 
				    pDb->zName, 
				    pTab->zName, 
				    pCol->zName,
				    sesqlite_contexts->column_context, 
				    sesqlite_contexts->tuple_context);

			    int sec_con_id = insert_context(db, 
				    0, 
				    pDb->zName, 
				    pTab->zName, 
				    NULL,
				    sesqlite_contexts->tuple_context, 
				    sesqlite_contexts->tuple_context);

			    sqlite3_bind_int(stmt_con_insert, 1, sec_con_id);
			    sqlite3_bind_int(stmt_con_insert, 2, sec_label_id);
			    sqlite3_bind_text(stmt_con_insert, 3, pDb->zName,
				strlen(pDb->zName), SQLITE_TRANSIENT);
			    sqlite3_bind_text(stmt_con_insert, 4, pTab->zName,
				strlen(pTab->zName), SQLITE_TRANSIENT);
			    sqlite3_bind_text(stmt_con_insert, 5, pCol->zName,
				strlen(pCol->zName), SQLITE_TRANSIENT);

			    rc = sqlite3_step(stmt_con_insert);
			    rc = sqlite3_reset(stmt_con_insert);
			    insert_key(db, 
				    pDb->zName, 
				    pTab->zName, 
				    pCol->zName, 
				    sec_label_id);

			}

			if(HasRowid(pTab)){
			    /* assign security context to rowid */
			    int sec_label_id = insert_context(db, 
				    1, 
				    pDb->zName, 
				    pTab->zName,
				    "ROWID", 
				    sesqlite_contexts->column_context, 
				    sesqlite_contexts->tuple_context);

			    int sec_con_id = insert_context(db, 
				    0, 
				    pDb->zName, 
				    pTab->zName, 
				    NULL,
				    sesqlite_contexts->tuple_context, 
				    sesqlite_contexts->tuple_context);


				sqlite3_bind_int(stmt_con_insert, 1, sec_con_id);
				sqlite3_bind_int(stmt_con_insert, 2, sec_label_id);
				sqlite3_bind_text(stmt_con_insert, 3, pDb->zName,
				    strlen(pDb->zName), SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt_con_insert, 4, pTab->zName,
				    strlen(pTab->zName), SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt_con_insert, 5, "ROWID",
				    strlen("ROWID"), SQLITE_TRANSIENT);

				rc = sqlite3_step(stmt_con_insert);
				rc = sqlite3_reset(stmt_con_insert);
				insert_key(db, 
					pDb->zName, 
					pTab->zName, 
					"ROWID", 
					sec_label_id);
			}
		    }
		}
	    }
	}
    }
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
    int isOpen = -1;

#ifdef SQLITE_DEBUG
fprintf(stdout, "\n == SeSqlite Initialization == \n");
#endif

    /* Allocate and initialize the hash-table used to store tokenizers. */
    hash = sqlite3_malloc(sizeof(seSQLiteHash));
    hash_id = sqlite3_malloc(sizeof(seSQLiteBiHash));
    if( !hash )
	return SQLITE_NOMEM;
    else
	seSQLiteHashInit(hash, SESQLITE_HASH_STRING, 0); /* init */

    if( !hash_id )
	return SQLITE_NOMEM;
    else
	seSQLiteBiHashInit(hash_id, SESQLITE_HASH_BINARY, SESQLITE_HASH_STRING, 0); /* init mapping */

    isOpen = open_or_reopen(db);

    rc = context_reload(db);
    if(rc != SQLITE_OK)
	return rc;

    rc = create_internal_table(db);
    if(rc != SQLITE_OK)
	return rc;

    rc = prepare_stmt(db);
    if(rc != SQLITE_OK)
	return rc;
	
    if(!isOpen){
	rc = initialize_mapping(db);
	if(rc != SQLITE_OK)
	    return rc;
    }

    /* in-memory representation of sesqlite_contexts file */
    rc = initialize_internal_table(db, isOpen);
    if(rc != SQLITE_OK)
	return rc;

    rc = initialize_authorizer(db);
    if(rc != SQLITE_OK)
	return rc;

#ifdef SELINUX_STATIC_CONTEXT
    scon = sqlite3_mprintf("%s", "unconfined_u:unconfined_r:unconfined_t:s0");
    tcon = sqlite3_mprintf("%s", "unconfined_u:object_r:unconfined_t:s0");
#else
    rc = getcon(&scon);
    if(security_compute_create_raw(scon, scon, 4, &tcon) < 0){
	fprintf(stderr, "SELinux could not compute a default context\n");
	return SQLITE_ERROR;
    }
#endif

    scon_id = insert_id(db, "main", scon);
    assert( scon_id != 0);
    tcon_id = insert_id(db, "main", tcon);
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

