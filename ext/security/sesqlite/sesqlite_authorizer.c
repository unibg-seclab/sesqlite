/* SeSqlite extension to add SELinux checks in SQLite */

#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX)

#ifndef SQLITE_CORE
#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#else
#include "sqlite3.h"
#endif

#include "sesqlite_authorizer.h"

#define USE_AVC

/* source (process) security context */
security_context_t scon = NULL;
security_context_t tcon = NULL;
int scon_id = 0;
int tcon_id = 0;

/* prepared statements to query schema sesqlite_master (bind it before use) */
sqlite3_stmt *stmt_insert_context;
sqlite3_stmt *stmt_insert_id;

seSQLiteHash *hash = NULL;
seSQLiteBiHash *hash_id = NULL;

#ifdef USE_AVC
seSQLiteHash *avc; /* HashMap*/
#endif


int insert_id(sqlite3 *db, char *db_name, char *sec_label){

    int rc = SQLITE_OK;
    int *value = NULL;
    int rowid = 0;

    value = seSQLiteBiHashFindKey(hash_id, sec_label, strlen(sec_label));
    if(value == NULL){
	sqlite3_bind_int(stmt_insert_id, 1, lookup_security_context(hash_id, db_name, SELINUX_ID));
	sqlite3_bind_text(stmt_insert_id, 2, sec_label, strlen(sec_label),
	    SQLITE_TRANSIENT);

	rc = sqlite3_step(stmt_insert_id);
	rc = sqlite3_reset(stmt_insert_id);

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
#ifdef SELINUX_STATIC_CONTEXT
    security_context_new = sqlite3_mprintf("%s", "unconfined_u:object_r:unconfined_t:s0");
#else
    if(security_compute_create_raw(scon, scon, (column ? SELINUX_DB_COLUMN : SELINUX_DB_TABLE), &security_context_new) < 0){
	fprintf(stderr, "SELinux could not compute a default context\n");
	return SQLITE_ERROR;
    }
#endif
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
int checkAccess(sqlite3 *db, const char *dbname, const char *table, const char *column,
		int tclass, int perm) {

	assert(tclass <= NELEMS(access_vector));

	int id = getContext(db, dbname, tclass, table, column);
	assert(id != 0);

	unsigned int key = compress(scon_id,
		id,
		access_vector[tclass].c_code,
		access_vector[tclass].perm[perm].p_code);

	int *res = seSQLiteHashFind(avc, NULL, key);
	if (res == NULL) {
		res = sqlite3_malloc(sizeof(int));
		char *ttcon = seSQLiteBiHashFind(hash_id, NULL, id);
		sqlite3Dequote(ttcon);
		*res = selinux_check_access(scon, ttcon, access_vector[tclass].c_name,
		    access_vector[tclass].perm[perm].p_name, NULL);

		seSQLiteHashInsert(avc, NULL, key, res, 0, 0);
	}

	// DO NOT FREE key AND res
	return 0 == *res;
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
		}

		rc = checkAllColumns(pdb, dbname, arg1, SELINUX_DB_COLUMN,
		SELINUX_DROP);

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
		}

		rc = checkAllColumns(pdb, dbname, arg1, SELINUX_DB_COLUMN,
		SELINUX_DROP);

		break;

	case SQLITE_DROP_TEMP_INDEX: /* Index Name    | Table Name      */
		break;

	case SQLITE_DROP_TEMP_TABLE: /* Table Name    | NULL            */
		if (!checkAccess(pdb, dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_DROP)) {
			rc = SQLITE_DENY;
		}

		rc = checkAllColumns(pdb, dbname, arg1, SELINUX_DB_COLUMN,
		SELINUX_DROP);

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
		}

		rc = checkAllColumns(pdb, dbname, arg1, SELINUX_DB_COLUMN,
		SELINUX_DROP);

		break;

	case SQLITE_INSERT: /* Table Name    | NULL            */
		if (!checkAccess(pdb, dbname, arg1, NULL, SELINUX_DB_TABLE,
		SELINUX_INSERT)) {
			rc = SQLITE_DENY;
		}

		rc = checkAllColumns(pdb, dbname, arg1, SELINUX_DB_COLUMN,
		SELINUX_INSERT);

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

		if (!checkAccess(pdb, dbname, arg1, arg2, SELINUX_DB_COLUMN,
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
		}

		rc = checkAllColumns(pdb, dbname, arg1, SELINUX_DB_COLUMN,
		SELINUX_DROP);

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

    int *res;
    int id = 0;
    id = sqlite3_value_int(argv[0]);
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

    unsigned int key = compress(scon_id,
	id,
	tclass,
	tperm);

    res = seSQLiteHashFind(avc, NULL, key);
    if (res == NULL) { 
	res = sqlite3_malloc(sizeof(int));
    	char *ttcon = seSQLiteBiHashFind(hash_id, NULL, id);
	*res = selinux_check_access(scon, /* source security context */
	    ttcon, /* target security context */
	    argv[1]->z, /* target security class string */
	    argv[2]->z, /* requested permissions string */
	    NULL /* auxiliary audit data */
	    );
	seSQLiteHashInsert(avc, NULL, key, res, 0, 0);
    }
#else
    *res = selinux_check_access(scon, /* source security context */
	argv[0]->z, /* target security context */
	argv[1]->z, /* target security class string */
	argv[2]->z, /* requested permissions string */
	NULL /* auxiliary audit data */
    );
#endif

#ifdef SQLITE_DEBUG
    fprintf(stdout, "selinux_check_access(%s, %s, %s, %s) => %d\n", scon, argv[0]->z,
	argv[1]->z,
	argv[2]->z, *res);
#endif

    sqlite3_result_int(context, 0 == *res);
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



// 	TODO experiments on why triggers are disabled for sqlite_* tables are required
//      the easy solution is to allow them. Indexes are also disables, and
//      enabling them causes the execution to interrupt, so imposing a UNIQUE
//      or PRIMARY KEY constraint for column 'name' in sqlite_master in order
//      to use it as foreign key in sesqlite_master is not feasible.
// 		create trigger to delete unused SELinux contexts after table drop
//    if (rc == SQLITE_OK) {
//	rc = sqlite3_exec(db, 
//		"CREATE TEMP TRIGGER delete_contexts_after_table_drop "
//		"AFTER DELETE ON sqlite_master "
//		"FOR EACH ROW WHEN OLD.type IN ('table', 'view') "
//		"BEGIN "
//		" DELETE FROM selinux_context WHERE name = OLD.name; "
//		"END;", 0, 0, 0);
//
//#ifdef SQLITE_DEBUG
//if (rc == SQLITE_OK)
//    fprintf(stdout, "Trigger 'delete_contexts_after_table_drop' created successfully.\n");
//else
//    fprintf(stderr, "Error: unable to create 'delete_contexts_after_table_drop' trigger.\n");
//#endif
//    }
//
// create trigger to update SELinux contexts after table rename
//    if (rc == SQLITE_OK) {
//	rc = sqlite3_exec(db,
//		"CREATE TEMP TRIGGER update_contexts_after_rename "
//		"AFTER UPDATE OF name ON sqlite_master "
//		"FOR EACH ROW WHEN NEW.type IN ('table', 'view') "
//		"BEGIN "
//		" UPDATE selinux_context SET name = NEW.name WHERE name = OLD.name; "
//		"END;", 0, 0, 0); 
//
//#ifdef SQLITE_DEBUG
//if (rc == SQLITE_OK)
//    fprintf(stdout, " == SeSqlite Initialized == \n\n");
//else
//    fprintf(stderr, "Error: unable to create 'update_contexts_after_rename' trigger.\n");
//#endif
//    }
//
//#ifdef SQLITE_DEBUG
//if (rc != SQLITE_OK)
//    fprintf(stderr, "Error: unable to initialize the selinux support for SQLite.\n");
//#endif
//
    return rc;
}



int create_security_context_column(void *pUserData, void *parse, int type, void *pNewCol, 
	char **zColumn) {

    sqlite3* db = pUserData;
    Parse *pParse = parse;
   // Column *pCol;
    char *zName = 0;
    char *zType = 0;
    int op = 0;
    int nExtra = 0;
    Expr *pExpr;
    int c = 0;
    int i = 0;
    int iDb = 0;
   
    *zColumn = NULL;
    *zColumn = sqlite3MPrintf(db, SECURITY_CONTEXT_COLUMN_DEFINITION);
    sqlite3Dequote(*zColumn);

//    Table *p = pNew;
//    iDb = sqlite3SchemaToIndex(db, p->pSchema);
//#if SQLITE_MAX_COLUMN
//  if( p->nCol+1>db->aLimit[SQLITE_LIMIT_COLUMN] ){
//    sqlite3ErrorMsg(pParse, "too many columns on %s", p->zName);
//    return;
//  }
//#endif
    zName = sqlite3MPrintf(db, SECURITY_CONTEXT_COLUMN_NAME);
    sqlite3Dequote(zName);
//    for(i=0; i<p->nCol; i++){
//	if( STRICMP(zName, p->aCol[i].zName) ){
//      sqlite3ErrorMsg(pParse, "object name reserved for internal use: %s", zName);
//	    sqlite3DbFree(db, zName);
//	    sqlite3DbFree(db, *zColumn);
//	    return;
//	}
//    }
//
//    if( (p->nCol & 0x7)==0 ){
//	Column *aNew;
//	aNew = sqlite3DbRealloc(db,p->aCol,(p->nCol+8)*sizeof(p->aCol[0]));
//	if( aNew==0 ){
//	    sqlite3ErrorMsg(pParse, "memory error");
//	    sqlite3DbFree(db, zName);
//	    sqlite3DbFree(db, *zColumn);
//	return;
//	}
//	p->aCol = aNew;
//    }
//    pCol = &p->aCol[p->nCol];
//    memset(pCol, 0, sizeof(p->aCol[0]));
    Column *pCol = pNewCol;
    pCol->zName = zName;

    zType = sqlite3MPrintf(db, SECURITY_CONTEXT_COLUMN_TYPE);
    sqlite3Dequote(sqlite3MPrintf(db, SECURITY_CONTEXT_COLUMN_TYPE));
    pCol->zType = sqlite3MPrintf(db, zType);
    pCol->affinity = SQLITE_AFF_INTEGER;
//    p->nCol++;
//    p->aCol[p->nCol].colFlags |= COLFLAG_HIDDEN;

    /**
    *generate expression for DEFAULT value
    */
//    op = 151;
//    nExtra = 7;
//    pExpr = sqlite3DbMallocZero(db, sizeof(Expr)+nExtra);
//    pExpr->op = (u8)op;
//    pExpr->iAgg = -1;
//    pExpr->u.zToken = (char*)&pExpr[1];
//    memcpy(pExpr->u.zToken, SECURITY_CONTEXT_COLUMN_DEFAULT_FUNC, strlen(SECURITY_CONTEXT_COLUMN_DEFAULT_FUNC) - 2);
//    pExpr->u.zToken[strlen(SECURITY_CONTEXT_COLUMN_DEFAULT_FUNC) - 2] = 0;
//    sqlite3Dequote(pExpr->u.zToken);
//    pExpr->flags |= EP_DblQuoted;
//#if SQLITE_MAX_EXPR_DEPTH>0
//    pExpr->nHeight = 1;
//#endif 
//    pCol->pDflt = pExpr;
//    pCol->zDflt = sqlite3DbStrNDup(db, SECURITY_CONTEXT_COLUMN_DEFAULT_FUNC
//	      , strlen(SECURITY_CONTEXT_COLUMN_DEFAULT_FUNC));
      
      
    /* Loop through the columns of the table to see if any of them contain the token "hidden".
     ** If so, set the Column.isHidden flag and remove the token from
     ** the type string.  */
//    int iCol;
//    for (iCol = 0; iCol < p->nCol; iCol++) {
//	char *zType = p->aCol[iCol].zType;
//	char *zName = p->aCol[iCol].zName;
//	int nType;
//	int i = 0;
//	if (!zType)
//	continue;
//	nType = sqlite3Strlen30(zType);
//	if ( sqlite3StrNICmp("hidden", zType, 6)
//			|| (zType[6] && zType[6] != ' ')) {
//	    for (i = 0; i < nType; i++) {
//		if ((0 == sqlite3StrNICmp(" hidden", &zType[i], 7))
//				&& (zType[i + 7] == '\0' || zType[i + 7] == ' ')) {
//		    i++;
//		    break;
//		}
//	    }
//	}
//	if (i < nType) {
//	    int j;
//	    int nDel = 6 + (zType[i + 6] ? 1 : 0);
//	    for (j = i; (j + nDel) <= nType; j++) {
//		    zType[j] = zType[j + nDel];
//	    }
//	    if (zType[i] == '\0' && i > 0) {
//		    assert(zType[i-1]==' ');
//		    zType[i - 1] = '\0';
//	    }
//            p->aCol[iCol].colFlags |= COLFLAG_HIDDEN;
//	}
//    }
    //assign security context to sql schema object
    //insert table context
//    sqlite3NestedParse(pParse,
//      "INSERT INTO %Q.%s (security_context, security_label, db, name) VALUES(\
//	%d, %d,\
//	'%s',\
//	'%s')",
//      pParse->db->aDb[iDb].zName, SELINUX_CONTEXT,
//      lookup_security_context(hash_id, pParse->db->aDb[iDb].zName, SELINUX_CONTEXT),
//      lookup_security_label(hash_id, 0, pParse->db->aDb[iDb].zName, p->zName, NULL),
//      pParse->db->aDb[iDb].zName, p->zName
//    );
//    sqlite3ChangeCookie(pParse, iDb);
//
//    //add security context to columns
//    for (iCol = 0; iCol < p->nCol; iCol++) {
//    	sqlite3NestedParse(pParse,
//    	  "INSERT INTO %Q.%s(security_context, security_label, db, name, column) VALUES(\
//	    %d, %d,\
//	    '%s',\
//	    '%s',\
//	    '%s')",
//    	  pParse->db->aDb[iDb].zName, SELINUX_CONTEXT,
//	  lookup_security_context(hash_id, pParse->db->aDb[iDb].zName, SELINUX_CONTEXT),
//	  lookup_security_label(hash_id, 1, pParse->db->aDb[iDb].zName, p->zName, p->aCol[iCol].zName),
//    	  pParse->db->aDb[iDb].zName, p->zName, p->aCol[iCol].zName
//    	);
//    }
//    sqlite3ChangeCookie(pParse, iDb);
//
//    sqlite3NestedParse(pParse,
//      "INSERT INTO %Q.%s(security_context, security_label, db, name, column) VALUES(\
//	%d, %d,\
//	'%s',\
//	'%s',\
//	'%s')",
//      pParse->db->aDb[iDb].zName, SELINUX_CONTEXT,
//      lookup_security_context(hash_id, pParse->db->aDb[iDb].zName, SELINUX_CONTEXT),
//      lookup_security_label(hash_id, 1, pParse->db->aDb[iDb].zName, p->zName, "ROWID"),
//      pParse->db->aDb[iDb].zName, p->zName, "ROWID" 
//    );
//    sqlite3ChangeCookie(pParse, iDb);

    return SQLITE_OK;
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

#ifdef SQLITE_DEBUG
fprintf(stdout, "\n == SeSqlite Initialization == \n");
#endif

    /* Allocate and initialize the hash-table used to store tokenizers. */
    hash = sqlite3_malloc(sizeof(seSQLiteHash));
    hash_id = sqlite3_malloc(sizeof(seSQLiteBiHash));
    avc = sqlite3_malloc(sizeof(seSQLiteHash));
    if( !hash )
	return SQLITE_NOMEM;
    else
	seSQLiteHashInit(hash, SESQLITE_HASH_STRING, 0); /* init */

    if( !hash_id )
	return SQLITE_NOMEM;
    else
	seSQLiteBiHashInit(hash_id, SESQLITE_HASH_BINARY, SESQLITE_HASH_STRING, 0); /* init mapping */

    if( !avc )
	return SQLITE_NOMEM;
    else
	seSQLiteHashInit(avc, SESQLITE_HASH_INT, 0); /* init avc */

    set_hash(hash);
    set_hash_id(hash_id);

    rc = context_reload(db);
    if(rc != SQLITE_OK)
	return rc;

//    /* create the SQL function getcon */
//    rc = sqlite3_create_function(db, "getSecurityContext", 0,
//	SQLITE_UTF8 /* | SQLITE_DETERMINISTIC */, db, selinuxGetSecurityContextFunction,
//	0, 0);
//    if(rc != SQLITE_OK)
//	return rc;
//
//    /* create the SQL function getcon */
//    rc = sqlite3_create_function(db, "getSecurityLabel", 0,
//	SQLITE_UTF8 /* | SQLITE_DETERMINISTIC */, db, selinuxGetSecurityLabelFunction,
//	0, 0);
//    if(rc != SQLITE_OK)
//	return rc;

    rc = initialize(db);
    if(rc != SQLITE_OK)
	return rc;

    rc = sqlite3_prepare_v2(db,
	"INSERT INTO selinux_id(security_context, security_label) values(?1, ?2);", -1,
	&stmt_insert_id, 0);
    if(rc != SQLITE_OK)
	return rc;

    rc = sqlite3_prepare_v2(db,
	"INSERT INTO selinux_context(security_context, security_label, db, name, column) values(?1, ?2, ?3, ?4, ?5);", -1,
	&stmt_insert_context, 0);
    if(rc != SQLITE_OK)
	return rc;

    rc =sqlite3_set_add_extra_column(db, create_security_context_column, db); 
    if (rc != SQLITE_OK)
	return rc;

    /* create the SQL function selinux_check_access */
    rc = sqlite3_create_function(db, "selinux_check_access", 3,
	SQLITE_UTF8 /* | SQLITE_DETERMINISTIC */, 0, selinuxCheckAccessFunction,
	0, 0);
    if (rc != SQLITE_OK)
	return rc;

    /* register module */
    rc = sqlite3_create_module(db, "selinuxModule", &sesqlite_mod, NULL);
    if (rc != SQLITE_OK)
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

    /* set the authorizer */
//    if (rc == SQLITE_OK)
//	rc = sqlite3_set_authorizer(db, selinuxAuthorizer, db);

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
