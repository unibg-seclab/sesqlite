/* SeSqlite extension to add SELinux checks in SQLite */

#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX)

#include "sesqlite_authorizer.h"

/* Comment the following line to disable the userspace AVC */
#define USE_AVC

#ifdef USE_AVC
static seSQLiteHash *avc; /* userspace AVC HashMap*/
static int *avc_deny;
static int *avc_allow;
#endif

int insert_id(sqlite3 *db, char *db_name, char *sec_label){

    int rc = SQLITE_OK;
    int *value = NULL;
    int rowid = 0;

    value = seSQLiteBiHashFindKey(hash_id, sec_label, strlen(sec_label));
    if(value == NULL){
	sqlite3_bind_int(stmt_insert, 1, lookup_security_context(hash_id, db_name, SELINUX_ID));
	sqlite3_bind_text(stmt_insert, 2, sec_label, strlen(sec_label),
	    SQLITE_TRANSIENT);

	rc = sqlite3_step(stmt_insert);
	rc = sqlite3_reset(stmt_insert);

	rowid = sqlite3_last_insert_rowid(db);
	value = sqlite3_malloc(sizeof(int));
	*value = rowid;
	seSQLiteBiHashInsert(hash_id, value, sizeof(int), sec_label, strlen(sec_label));
    }

    return *(int *) value;
}

/*
 * Return the security context of the given (table), (table, column) element.
 */
/*
 * Function: getContext 
 * Purpose: Given the table name or table and column name, returns the security
 * context associated.
 * Parameters:
 * 		dbname: database name;
 * 		tclass: 1 = table, 2 = column;
 * 		table: table name;
 * 		column: column name, it is NULL if tclass == 1.
 * Return value: Return the security context associated to the table/column
 * given. 
 */
int getContext(sqlite3 *db, const char *dbname, int tclass, const char *table,
	const char *column) {

	char *key = NULL;
	int *res = 0;

	switch (tclass) {
	case 0: /* database */
		break;
	case 1: /* table */
		key = sqlite3_mprintf("%s:%s", dbname, table);
		break;
	case 2:
		key = sqlite3_mprintf("%s:%s:%s", dbname, table, column);
		break;
	}

	assert(key != NULL);
	res = seSQLiteHashFind(hash, key, strlen(key));

	if (res != NULL) {
#ifdef SQLITE_DEBUG
fprintf(stdout, "Hash hint: db=%s, table=%s, column=%s -> %d\n", dbname, table,
    (column ? column : "NULL"), *res);
#endif
	    sqlite3_free(key);
	    return *res;
	}else {
	    security_context_t security_context_new = 0;
	    int id = 0;
	    int *value = 0;
	    switch (tclass) {
	    case 1: /* table */
		compute_sql_context(0, 
		    (char *) dbname, 
		    (char *) table, 
		    NULL, 
		    sesqlite_contexts->table_context, 
		    &security_context_new); 
		break;
	    case 2:
		compute_sql_context(1, 
		    (char *) dbname, 
		    (char *) table, 
		    (char *) column, 
		    sesqlite_contexts->column_context, 
		    &security_context_new); 
		break;
	    }
	    id = insert_id(db, (char *) dbname, security_context_new);
	    value = sqlite3_malloc(sizeof(int));
	    *value = id;
	    seSQLiteHashInsert(hash, key, strlen(key), value, sizeof(int), 0);
#ifdef SQLITE_DEBUG
fprintf(stdout, "Compute New Context: db=%s, table=%s, column=%s -> %d\n", dbname, table,
    (column ? column : "NULL"), id);
#endif
	    return id; /* something wrong, a table/column was not labeled correctly */	
	}
}

/*
 * Checks whether the source context has been granted the specified permission
 * for the classes 'db_table' and 'db_column' and the target context associated with the table/column.
 * Returns 1 if the access has been granted, 0 otherwise.
 */
int checkAccess(
	sqlite3 *db,
	const char *dbname,
	const char *table,
	const char *column,
	int tclass,
	int perm
){
    assert(tclass <= NELEMS(access_vector));

    int res = 0;
    int id = getContext(db, dbname, tclass, table, column);
    assert(id != 0);

    unsigned int key = compress(
	scon_id,
	id,
	access_vector[tclass].c_code,
	access_vector[tclass].perm[perm].p_code
    );

#ifdef USE_AVC
    int *avc_res = seSQLiteHashFind(avc, NULL, key);
    if ( avc_res!=NULL ){
	res = ( avc_res==avc_allow ); /* Yes, let's just compare the pointers */
    }else{
#endif
	char *ttcon = seSQLiteBiHashFind(hash_id, &id, sizeof(int));
	sqlite3Dequote(ttcon);
	res = ( 0==selinux_check_access(
	    scon,
	    ttcon,
	    access_vector[tclass].c_name,
	    access_vector[tclass].perm[perm].p_name,
	    NULL
	));
#ifdef USE_AVC
	seSQLiteHashInsert(avc, NULL, key, (res==1 ? avc_allow : avc_deny), 0, 0);
    }
#endif

    return res;
}

/**
 * Scan all the columns and call checkAccess
 */
int checkAllColumns(sqlite3* pdb, const char *dbName, const char* tblName,
		int type, int action) {

	int rc = SQLITE_OK;
	int i = -1; /* Database number */
	int j;
	Hash *pTbls;
	HashElem *x;
	Db *pDb;

	// TODO type = db_column

	Table *pTab = sqlite3FindTable(pdb, tblName, dbName);
	if (pTab) {
		Column *pCol;
		for (j = 0, pCol = pTab->aCol; j < pTab->nCol; j++, pCol++) {
			if (!checkAccess(pdb, dbName, tblName, pCol->zName, type, action)) {
				return SQLITE_DENY;
			}
		}
	}

	return rc;
}

/*
 * Authorizer to be set with sqlite3_set_authorizer that checks the SELinux
 * permission at schema level (tables and columns).
 */
int selinuxAuthorizer(void *pUserData, int type, const char *arg1,
		const char *arg2, const char *dbname, const char *source) {
	int rc = SQLITE_OK;

	sqlite3 *pdb = (sqlite3*) pUserData;

//	if (!auth_enabled)
//		return rc;

#ifdef SQLITE_DEBUG
	//fprintf(stdout, "authorizer: type=%s arg1=%s arg2=%s\n", authtype[type],
	//		(arg1 ? arg1 : "NULL"), (arg2 ? arg2 : "NULL"));
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
		if (!checkAccess(pdb, dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_CREATE)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_CREATE_TEMP_INDEX: /* Index Name    | Table Name      */
		break;

	case SQLITE_CREATE_TEMP_TABLE: /* Table Name    | NULL            */
		//see SQLITE_CREATE_TABLE comment
		if (!checkAccess(pdb, dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_CREATE)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_CREATE_TEMP_TRIGGER: /* Trigger Name  | Table Name      */
		break;

	case SQLITE_CREATE_TEMP_VIEW: /* View Name     | NULL            */
		//TODO TABLE == VIEW??
		if (!checkAccess(pdb, dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_CREATE)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_CREATE_TRIGGER: /* Trigger Name  | Table Name      */
		if (!checkAccess(pdb, dbname, arg2, NULL, SELINUX_DB_TABLE,
		SELINUX_SETATTR)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_CREATE_VIEW: /* View Name     | NULL            */
		//TODO TABLE == VIEW??
		if (!checkAccess(pdb, dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_CREATE)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_DELETE: /* Table Name    | NULL            */
		if (!checkAccess(pdb, dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_DELETE)) {
			rc = SQLITE_DENY;
		}else if(checkAllColumns(pdb, dbname, arg1, SELINUX_DB_COLUMN,
		    SELINUX_DROP)){
			rc = SQLITE_DENY;
		}

		break;

	case SQLITE_DROP_INDEX: /* Index Name    | Table Name      */
		if (!checkAccess(pdb, dbname, arg2, NULL, SELINUX_DB_TABLE,
		SELINUX_SETATTR)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_DROP_TABLE: /* Table Name    | NULL            */
		if (!checkAccess(pdb, dbname, arg1, NULL, SELINUX_DB_TABLE,
		    SELINUX_DROP)) {
			rc = SQLITE_DENY;
		}else if(checkAllColumns(pdb, dbname, arg1, SELINUX_DB_COLUMN,
		    SELINUX_DROP)){
			rc = SQLITE_DENY;
		}

		break;

	case SQLITE_DROP_TEMP_INDEX: /* Index Name    | Table Name      */
		break;

	case SQLITE_DROP_TEMP_TABLE: /* Table Name    | NULL            */
		if (!checkAccess(pdb, dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_DROP)) {
			rc = SQLITE_DENY;
		}else if(checkAllColumns(pdb, dbname, arg1, SELINUX_DB_COLUMN,
		    SELINUX_DROP)){
			rc = SQLITE_DENY;
		}

		break;

	case SQLITE_DROP_TEMP_TRIGGER: /* Trigger Name  | Table Name      */
		break;

	case SQLITE_DROP_TEMP_VIEW: /* View Name     | NULL            */
		if (!checkAccess(pdb, dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_DROP)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_DROP_TRIGGER: /* Trigger Name  | Table Name      */
		if (!checkAccess(pdb, dbname, arg2, NULL, SELINUX_DB_TABLE,
		SELINUX_SETATTR)) {
			rc = SQLITE_DENY;
		}
		break;

	case SQLITE_DROP_VIEW: /* View Name     | NULL            */
		if (!checkAccess(pdb, dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_DROP)) {
			rc = SQLITE_DENY;
		}else if(checkAllColumns(pdb, dbname, arg1, SELINUX_DB_COLUMN,
		    SELINUX_DROP)){
			rc = SQLITE_DENY;
		}

		break;

	case SQLITE_INSERT: /* Table Name    | NULL            */
		if (!checkAccess(pdb, dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_INSERT)) {
			rc = SQLITE_DENY;
		}else if(checkAllColumns(pdb, dbname, arg1, SELINUX_DB_COLUMN,
		    SELINUX_INSERT)){
			rc = SQLITE_DENY;
		}

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
		if (!checkAccess(pdb, dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_SELECT)) {
			rc = SQLITE_DENY;
		}else if (!checkAccess(pdb, dbname, arg1, arg2, SELINUX_DB_COLUMN,
		    SELINUX_SELECT)) {
			rc = SQLITE_DENY;
		}

		break;

	case SQLITE_SELECT: /* NULL          | NULL            */
		break;

	case SQLITE_TRANSACTION: /* Operation     | NULL            */
		break;

	case SQLITE_UPDATE: /* Table Name    | Column Name     */
		if (!checkAccess(pdb, dbname, arg1, arg2, SELINUX_DB_COLUMN,
		SELINUX_UPDATE)) {
			rc = SQLITE_DENY;
		}

		break;

	case SQLITE_ATTACH: /* Filename      | NULL            */
		// TODO change when multiple databases are supported by SeSqlite.
		//modify access vector policy
		//UNION selinux_context
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
		if (!checkAccess(pdb, arg1, arg2, NULL, SELINUX_DB_TABLE,
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
		if (!checkAccess(pdb, dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_DROP)) {
			rc = SQLITE_DENY;
		}else if(checkAllColumns(pdb, dbname, arg1, SELINUX_DB_COLUMN,
		    SELINUX_DROP)){
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
static void selinuxCheckAccessFunction(
	sqlite3_context *context,
	int argc,
	sqlite3_value **argv
){
    int res = 0;
    int id = sqlite3_value_int(argv[0]);
    assert(id != 0);

#ifdef USE_AVC
    int tclass = 0;
    int tperm = 0;

    int i = 0, j = 0;
    for(i = 0; i <= SELINUX_NELEM_CLASS; i++){
	if(strcmp(access_vector[i].c_name, argv[1]->z) == 0){
	    for(j = 0; j <= SELINUX_NELEM_PERM; j++){
		if(strcmp(access_vector[i].perm[j].p_name, argv[2]->z) == 0){
		    tclass = access_vector[i].c_code;
		    tperm = access_vector[i].perm[j].p_code;
		    break;
		 }
	     }
	}
    }

    unsigned int key = compress(
	scon_id,
	id,
	tclass,
	tperm
    );

    int *avc_res = seSQLiteHashFind(avc, NULL, key);
    if( avc_res!=NULL ){
	res = ( avc_res==avc_allow ); /* Yes, let's just compare the pointers */
    }else{
	char *ttcon = seSQLiteBiHashFind(hash_id, &id, sizeof(int));
	res = ( 0==selinux_check_access(
	    scon,       /* source security context */
	    ttcon,      /* target security context */
	    argv[1]->z, /* target security class string */
	    argv[2]->z, /* requested permissions string */
	    NULL        /* auxiliary audit data */
	));
	seSQLiteHashInsert(avc, NULL, key, (res==1 ? avc_allow : avc_deny), 0, 0);
    }
#else
    res = ( 0==selinux_check_access(
	scon,       /* source security context */
	argv[0]->z, /* target security context */
	argv[1]->z, /* target security class string */
	argv[2]->z, /* requested permissions string */
	NULL        /* auxiliary audit data */
    ));
#endif

#ifdef SQLITE_DEBUG
    fprintf(stdout, "selinux_check_access(%s, %s, %s, %s) => %s\n",
	scon, 
	argv[0]->z,
	argv[1]->z,
	argv[2]->z, (res ? "ALLOW": "DENY")
    );
#endif

    sqlite3_result_int(context, res);
}

int create_security_context_column(
    void *pArg, 
    void *parse, 
    int type, 
    void *pNew, 
    char **zColumn
) {

    Column *pCol;
    char *zName = 0;
    char *zType = 0;
    int iDb = 0;
    int i = 0;
    *zColumn = NULL;

    sqlite3* db = (sqlite3*) pArg;
    Parse *pParse = ((Vdbe*) sqlite3_next_stmt(db, 0))->pParse;
    Table *p = pNew;

    *zColumn = sqlite3MPrintf(db, SECURITY_CONTEXT_COLUMN_DEFINITION);
    zName = sqlite3MPrintf(db, SECURITY_CONTEXT_COLUMN_NAME);
    sqlite3Dequote(*zColumn);
    sqlite3Dequote(zName);

    iDb = sqlite3SchemaToIndex(db, p->pSchema);
    pCol = &p->aCol[0];
    for(i=1; i<p->nCol; i++){
	if( STRICMP(zName, p->aCol[i].zName) ){
	    sqlite3ErrorMsg(pParse, "object name reserved for internal use: %s", zName);
	    sqlite3DbFree(db, zName);
	    sqlite3DbFree(db, *zColumn);
	    return;
	}
    }

    pCol->zName = zName;
    zType = sqlite3MPrintf(db, SECURITY_CONTEXT_COLUMN_TYPE);
    pCol->zType = sqlite3MPrintf(db, zType);
    pCol->affinity = SQLITE_AFF_INTEGER;
    pCol->colFlags |= COLFLAG_HIDDEN;

    sqlite3NestedParse(pParse,
      "INSERT INTO %Q.%s (security_context, security_label, db, name) VALUES(\
	%d, %d,\
	'%s',\
	'%s')",
      pParse->db->aDb[iDb].zName, SELINUX_CONTEXT,
      lookup_security_context(hash_id, 
	  pParse->db->aDb[iDb].zName, 
	  SELINUX_CONTEXT),
      lookup_security_label(db, 
	  stmt_insert, 
	  hash_id, 
	  0, 
	  pParse->db->aDb[iDb].zName, 
	  p->zName, 
	  NULL),
      pParse->db->aDb[iDb].zName, p->zName);
    sqlite3ChangeCookie(pParse, iDb);

    //add security context to columns
    int iCol;
    for (iCol = 0; iCol < p->nCol; iCol++) {
    	sqlite3NestedParse(pParse,
    	  "INSERT INTO %Q.%s(security_context, security_label, db, name, column) VALUES(\
	    %d, %d,\
	    '%s',\
	    '%s',\
	    '%s')",
    	  pParse->db->aDb[iDb].zName, SELINUX_CONTEXT,
	  lookup_security_context(hash_id, 
	      pParse->db->aDb[iDb].zName, 
	      SELINUX_CONTEXT),
	  lookup_security_label(db, 
	      stmt_insert, 
	      hash_id, 
	      1, 
	      pParse->db->aDb[iDb].zName, 
	      p->zName, 
	      p->aCol[iCol].zName),
    	  pParse->db->aDb[iDb].zName, p->zName, p->aCol[iCol].zName);
    }
    sqlite3ChangeCookie(pParse, iDb);

    if(HasRowid(p)){
	sqlite3NestedParse(pParse,
	  "INSERT INTO %Q.%s(security_context, security_label, db, name, column) VALUES(\
	    %d, %d,\
	    '%s',\
	    '%s',\
	    '%s')",
	  pParse->db->aDb[iDb].zName, SELINUX_CONTEXT,
	  lookup_security_context(hash_id, 
	      pParse->db->aDb[iDb].zName, 
	      SELINUX_CONTEXT),
	  lookup_security_label(db, 
	      stmt_insert, 
	      hash_id, 
	      1, 
	      pParse->db->aDb[iDb].zName, 
	      p->zName, 
	      "ROWID"),
	  pParse->db->aDb[iDb].zName, p->zName, "ROWID");
	sqlite3ChangeCookie(pParse, iDb);
    }

    return SQLITE_OK;
}

static int selinux_schemachange_callback(
    void* pArg,                  /* the user argument (the db in our case) */
    int op,                      /* the schema operation that triggered the callback */
    const char* zDb,             /* the name of the database */
    const char* zTable,          /* the name of the table */
    void* arg1,                  /* arg1 depends on the op */
    void* arg2                   /* arg2 depends on the op */
){
    sqlite3 *db = (sqlite3*) pArg;
    Parse *pParse = ((Vdbe*) sqlite3_next_stmt(db, 0))->pParse;

    switch( op ){

    case SQLITE_SCHEMA_CREATE_TABLE:
	break;

    case SQLITE_SCHEMA_DROP_TABLE:
	sqlite3NestedParse(pParse,
	    "DELETE FROM %s.%s WHERE db = '%s' AND name = '%s'",
	    zDb, SELINUX_CONTEXT, zDb, zTable);
	break;

    case SQLITE_SCHEMA_ALTER_RENAME:
	sqlite3NestedParse(pParse,
	    "UPDATE %s.%s SET name = '%s' WHERE db = '%s' AND name = '%s'",
	    zDb, SELINUX_CONTEXT, zTable, zDb, arg1);
	break;

    case SQLITE_SCHEMA_ALTER_ADD:
    	sqlite3NestedParse(pParse,
    	  "INSERT INTO %s.%s(security_context, security_label, db, name, column) VALUES(\
	    %d, %d,\
	    '%s',\
	    '%s',\
	    '%s')",
    	  zDb, SELINUX_CONTEXT,
	  lookup_security_context(hash_id, 
	      (char *) zDb, 
	      SELINUX_CONTEXT),
	  lookup_security_label(db, 
	      stmt_insert, 
	      hash_id, 
	      1, 
	      (char *) zDb, 
	      (char *) zTable, 
	      arg1),
    	  (char *) zDb, (char *) zTable, arg1);
	break;
    }

    return SQLITE_OK;
}

int selinux_commit_callback(void *pArg){
#ifdef SQLITE_DEBUG
    fprintf(stdout, "Cleaning AVC after commit\n");
#endif
    seSQLiteHashClear(avc);
    return 0;
}

void selinux_rollback_callback(void *pArg){
#ifdef SQLITE_DEBUG
    fprintf(stdout, "Cleaning AVC after rollback\n");
#endif
    seSQLiteHashClear(avc);
}

int initialize_authorizer(sqlite3 *db){

    int rc = SQLITE_OK;

#ifdef USE_AVC
    /* initialize avc cache */
    avc = sqlite3_malloc(sizeof(seSQLiteHash));
    avc_allow = sqlite3_malloc(sizeof(int));
    avc_deny  = sqlite3_malloc(sizeof(int));
    if( !avc || !avc_allow || !avc_deny ){
	return SQLITE_NOMEM;
    }else{
	seSQLiteHashInit(avc, SESQLITE_HASH_INT, 1); /* init avc with copy key */
	*avc_allow = 0;
	*avc_deny = -1;
    }
#endif

    rc =sqlite3_set_add_extra_column(db, create_security_context_column, db);
    if (rc != SQLITE_OK)
	return rc;

    /* set the schemachange_callback */
    sqlite3_schemachange_hook(db, selinux_schemachange_callback, db);

    /* set the commit callback */
    sqlite3_commit_hook(db, selinux_commit_callback, 0);

    /* set the rollback callback */
    sqlite3_rollback_hook(db, selinux_rollback_callback, 0);

    /* create the SQL function selinux_check_access */
    rc = sqlite3_create_function(db, "selinux_check_access", 3,
	SQLITE_UTF8 /* | SQLITE_DETERMINISTIC */, 0, selinuxCheckAccessFunction,
	0, 0);
    if (rc != SQLITE_OK)
	return rc;

    /* set the authorizer */
    rc = sqlite3_set_authorizer(db, selinuxAuthorizer, db);

    return rc;
}

#endif /* !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX) */
