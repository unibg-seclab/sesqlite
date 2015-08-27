/* SeSqlite extension to add SELinux checks in SQLite */

#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX)

#include "sesqlite_contexts.h"

/*
 * Function to insert a new_node in a list. Note that this
 * function expects a pointer to head_ref as this can modify the
 * head of the input linked list (similar to push())
 */
void sorted_insert(
	struct sesqlite_context_element** head_ref,
	struct sesqlite_context_element* new_node
){
	struct sesqlite_context_element* current;

	if( *head_ref==NULL
	    || strcasecmp((*head_ref)->origin, new_node->origin)<=0 ){
		new_node->next = *head_ref;
		*head_ref = new_node;
	}else{
		current = *head_ref;
		while( current->next!=NULL
		       && strcasecmp(current->next->origin, new_node->origin)>0 ){
			current = current->next;
		}
		new_node->next = current->next;
		current->next = new_node;
	}
}

/* Free a list contained in sesqlite_context */
void free_sesqlite_context_list(
	struct sesqlite_context_element *head
){
	struct sesqlite_context_element *temp;
	while( head!=NULL ){
		sqlite3_free(head->origin);
		sqlite3_free(head->fparam);
		sqlite3_free(head->sparam);
		sqlite3_free(head->tparam);
		sqlite3_free(head->security_context);
		temp = head;
		head = head->next;
		free(temp);
	}
}

/* Free the entire sesqlite_context structure */
void free_sesqlite_context(
	struct sesqlite_context *sc
){
	free_sesqlite_context_list(sc->db_context);
	free_sesqlite_context_list(sc->table_context);
	free_sesqlite_context_list(sc->column_context);
	free_sesqlite_context_list(sc->tuple_context);
	free(sc);
}

struct sesqlite_context *read_sesqlite_context(
	sqlite3 *db,
	char *path
){
	int rc = SQLITE_OK;
	int n_line, ndb_line, ntable_line, nview_line, ncolumn_line, ntuple_line;
	char line[255];
	char *p      = NULL;
	char *token  = NULL;
	char *stoken = NULL;
	char *rest   = NULL; /* to point to the rest of the string */
	char *srest   = NULL; /* to point to the rest of the string */
	FILE *fp     = NULL;

	struct sesqlite_context *sc = sqlite3_malloc(
		sizeof(struct sesqlite_context));

	// TODO modify the liselinux in order to retrieve the context from the targeted folder
	fp = fopen(path, "rb");
	if( fp == NULL ){
		fprintf(stderr, "Error. Unable to open '%s' configuration file.\n", path);
		return NULL;
	}

	sc->db_context     = NULL;
	sc->table_context  = NULL;
	sc->view_context   = NULL;
	sc->column_context = NULL;
	sc->tuple_context  = NULL;

	n_line = 0;
	ndb_line = 0;
	ntable_line = 0;
	nview_line = 0;
	ncolumn_line = 0;
	ntuple_line = 0;

	/* Read the file */
	while( fgets(line, sizeof line - 1, fp) ){

		if( line[strlen(line) - 1] == '\n' )
			line[strlen(line) - 1] = 0;

		p = line;

		while( isspace(*p) )
			p++;

		if( *p=='#' || *p==0 )
			continue;

		token = strtok_r(p, " \t", &rest);
		if( !token ){
			fprintf(stderr, "Error. Unable to parse the configuration file.\n");
			fclose(fp);
			return NULL;
		}

		if( !strcasecmp(token, "db_database") ){

			struct sesqlite_context_element *new;
			new = sqlite3_malloc(sizeof(struct sesqlite_context_element));
			new->next = NULL;

			token = strtok_r(NULL, " \t", &rest);
			new->origin = sqlite3MPrintf(db, "%s", token);
			char *con = strtok_r(NULL, " \t", &rest);
			new->security_context = sqlite3MPrintf(db, "%s", con);

			new->fparam = sqlite3MPrintf(db, "%s", token);
			new->sparam = sqlite3MPrintf(db, "%s", token);
			new->tparam = sqlite3MPrintf(db, "%s", token);

			sorted_insert(&sc->db_context, new);
			ndb_line++;

		}else if( !strcasecmp(token, "db_table") ){

			struct sesqlite_context_element *new;
			new = sqlite3_malloc(sizeof(struct sesqlite_context_element));
			new->next = NULL;

			token = strtok_r(NULL, " \t", &rest);
			new->origin = sqlite3MPrintf(db, "%s", token);
			char *con = strtok_r(NULL, " \t", &rest);
			new->security_context = sqlite3MPrintf(db, "%s", con);

			stoken = strtok_r(token, ".", &srest);
			new->fparam = sqlite3MPrintf(db, "%s", stoken);
			stoken = strtok_r(NULL, ".", &srest);
			new->sparam = sqlite3MPrintf(db, "%s", stoken);
			new->tparam = NULL;

			sorted_insert(&sc->table_context, new);
			ntable_line++;

		}else if( !strcasecmp(token, "db_view") ){

			struct sesqlite_context_element *new;
			new = sqlite3_malloc(sizeof(struct sesqlite_context_element));
			new->next = NULL;

			token = strtok_r(NULL, " \t", &rest);
			new->origin = sqlite3MPrintf(db, "%s", token);
			char *con = strtok_r(NULL, " \t", &rest);
			new->security_context = sqlite3MPrintf(db, "%s", con);

			stoken = strtok_r(token, ".", &srest);
			new->fparam = sqlite3MPrintf(db, "%s", stoken);
			stoken = strtok_r(NULL, ".", &srest);
			new->sparam = sqlite3MPrintf(db, "%s", stoken);
			new->tparam = NULL;

			sorted_insert(&sc->view_context, new);
			nview_line++;

		}else if( !strcasecmp(token, "db_column") ){

			struct sesqlite_context_element *new;
			new = sqlite3_malloc(sizeof(struct sesqlite_context_element));
			new->next = NULL;

			token = strtok_r(NULL, " \t", &rest);
			new->origin = sqlite3MPrintf(db, "%s", token);
			char *con = strtok_r(NULL, " \t", &rest);
			new->security_context = sqlite3MPrintf(db, "%s", con);

			stoken = strtok_r(token, ".", &srest);
			new->fparam = sqlite3MPrintf(db, "%s", stoken);
			stoken = strtok_r(NULL, ".", &srest);
			new->sparam = sqlite3MPrintf(db, "%s", stoken);
			stoken = strtok_r(NULL, ".", &srest);
			new->tparam = sqlite3MPrintf(db, "%s", stoken);

			sorted_insert(&sc->column_context, new);
			ncolumn_line++;

		}else if( !strcasecmp(token, "db_tuple") ){

			struct sesqlite_context_element *new;
			new = sqlite3_malloc(sizeof(struct sesqlite_context_element));
			new->next = NULL;

			token = strtok_r(NULL, " \t", &rest);
			new->origin = sqlite3MPrintf(db, "%s", token);
			char *con = strtok_r(NULL, " \t", &rest);
			new->security_context = sqlite3MPrintf(db, "%s", con);
			
			stoken = strtok_r(token, ".", &srest);
			new->fparam = sqlite3MPrintf(db, "%s", stoken);
			stoken = strtok_r(NULL, ".", &srest);
			new->sparam = sqlite3MPrintf(db, "%s", stoken);
			new->tparam = NULL;

			sorted_insert(&sc->tuple_context, new);
			ntuple_line++;

		}else{
			fprintf(stderr,
				"Error, unable to recognize '%s' in sesqlite_context file.\n",
				token);
		}
	}

	fclose(fp);
	return sc;
}

/*
 * Returns 1 if the name is accepted by the filter, otherwise 0.
 * The name is accepted if the filter and the name are the same
 * (case insensitive) up to the first '*' or till the end of the string.
 * A NULL filter can be used to always returns 0.
 */
int filter_accepts(
	const char *filter,
	const char *name
){
	if( filter==NULL )
		return 0;
	
	int wildcard = strcspn(filter, "*");
	if( wildcard==strlen(filter) )
		return ( sqlite3StrICmp(filter, name)==0 );

	return ( sqlite3StrNICmp(filter, name, wildcard)==0 );
}

int reload_sesqlite_contexts(
	sqlite3 *db,                  /* the database connection */
	struct sesqlite_context *sc,  /* the sesqlite_context */
	char *dbFilter,               /* the db filter or NULL as wildcard */
	char *tblFilter,              /* the table filter or NULL as wildcard */
	char *colFilter               /* the column filter or NULL as wildcard */
){
	/* The following code is used to scan all the databases, tables
	 * and columns directly from the structs used to store the schema */

	int i, j;
	Hash *pTbls;
	HashElem * x;
	Db *pDb;
	char *dbName = NULL;
	char *tblName = NULL;
	char *colName = NULL;
	char *result = NULL;
	int rc = SQLITE_OK;
	int count = 0;

	int sec_label_id = 0;
	int sec_con_id = 0;

	/* Scan the databases */
	for( i = (db->nDb - 1), pDb = &db->aDb[i]; i>=0; i--, pDb-- ){

		if( OMIT_TEMPDB && i==1 )
			continue;

		/* Filter database */
		dbName = pDb->zName;
		if( !filter_accepts(dbFilter, dbName) )
			continue;

			sec_label_id = insert_context(db, 0, dbName, NULL, NULL,
				sc->db_context, sc->tuple_context);

			sec_con_id = insert_context(db, 0, dbName, SELINUX_CONTEXT, NULL,
				sc->tuple_context, sc->tuple_context);

			insert_key(db, dbName, NULL, NULL, sec_label_id);

			sqlite3_bind_int( stmt_con_insert, 1, sec_con_id);
			sqlite3_bind_int( stmt_con_insert, 2, sec_label_id);
			sqlite3_bind_text(stmt_con_insert, 3, dbName,  -1, SQLITE_TRANSIENT);
			sqlite3_bind_text(stmt_con_insert, 4, "",      -1, SQLITE_TRANSIENT);
			sqlite3_bind_text(stmt_con_insert, 5, "",      -1, SQLITE_TRANSIENT);

			rc = sqlite3_step(stmt_con_insert);
			assert( rc==SQLITE_DONE );

			rc = sqlite3_clear_bindings(stmt_con_insert);
			assert( rc==SQLITE_OK );

			rc = sqlite3_reset(stmt_con_insert);
			assert( rc==SQLITE_OK );

		/* Scan the tables */
		pTbls = &db->aDb[i].pSchema->tblHash;
		for( x = sqliteHashFirst(pTbls); x; x = sqliteHashNext(x) ){

			Table *pTab = sqliteHashData(x);
			if( !pTab )
				continue;

			/* Filter table */
			tblName = pTab->zName;
			if( !filter_accepts(tblFilter, tblName) )
				continue;

			sec_label_id = insert_context(db, 0, dbName, tblName, NULL,
				sc->table_context, sc->tuple_context);

			sec_con_id = insert_context(db, 0, dbName, SELINUX_CONTEXT, NULL,
				sc->tuple_context, sc->tuple_context);

			insert_key(db, dbName, tblName, NULL, sec_label_id);

			sqlite3_bind_int( stmt_con_insert, 1, sec_con_id);
			sqlite3_bind_int( stmt_con_insert, 2, sec_label_id);
			sqlite3_bind_text(stmt_con_insert, 3, dbName,  -1, SQLITE_TRANSIENT);
			sqlite3_bind_text(stmt_con_insert, 4, tblName, -1, SQLITE_TRANSIENT);
			sqlite3_bind_text(stmt_con_insert, 5, "",      -1, SQLITE_TRANSIENT);

			rc = sqlite3_step(stmt_con_insert);
			assert( rc==SQLITE_DONE );

			rc = sqlite3_clear_bindings(stmt_con_insert);
			assert( rc==SQLITE_OK );

			rc = sqlite3_reset(stmt_con_insert);
			assert( rc==SQLITE_OK );
			++count;

			/* Scan the columns */
			Column *pCol;
			for (j = 0, pCol = pTab->aCol; j < pTab->nCol; j++, pCol++) {

				/* Filter column */
				colName = pCol->zName;
				if( !filter_accepts(colFilter, colName) )
					continue;

				sec_label_id = insert_context(db, 1, dbName, tblName, colName,
					sc->column_context, sc->tuple_context);

				sec_con_id = insert_context(db, 0, dbName, SELINUX_CONTEXT, NULL,
					sc->tuple_context, sc->tuple_context);

				insert_key(db, dbName, tblName, colName, sec_label_id);

				sqlite3_bind_int( stmt_con_insert, 1, sec_con_id);
				sqlite3_bind_int( stmt_con_insert, 2, sec_label_id);
				sqlite3_bind_text(stmt_con_insert, 3, dbName,  -1, SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt_con_insert, 4, tblName, -1, SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt_con_insert, 5, colName, -1, SQLITE_TRANSIENT);

				rc = sqlite3_step(stmt_con_insert);
				assert( rc==SQLITE_DONE );

				rc = sqlite3_clear_bindings(stmt_con_insert);
				assert( rc==SQLITE_OK );

				rc = sqlite3_reset(stmt_con_insert);
				assert( rc==SQLITE_OK );
				++count;
			}

			/* assign security context to rowid if exists */
			if( HasRowid(pTab) && filter_accepts(colFilter, "ROWID") ){
				sec_label_id = insert_context(db, 1, dbName, tblName, "ROWID",
					sc->column_context, sc->tuple_context);

				sec_con_id = insert_context(db, 0, dbName, SELINUX_CONTEXT, NULL,
					sc->tuple_context, sc->tuple_context);

				insert_key(db, dbName, tblName, "ROWID", sec_label_id);

				sqlite3_bind_int( stmt_con_insert, 1, sec_con_id);
				sqlite3_bind_int( stmt_con_insert, 2, sec_label_id);
				sqlite3_bind_text(stmt_con_insert, 3, dbName,  -1, SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt_con_insert, 4, tblName, -1, SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt_con_insert, 5, "ROWID", -1, SQLITE_TRANSIENT);

				rc = sqlite3_step(stmt_con_insert);
				assert( rc==SQLITE_DONE );

				rc = sqlite3_clear_bindings(stmt_con_insert);
				assert( rc==SQLITE_OK );

				rc = sqlite3_reset(stmt_con_insert);
				assert( rc==SQLITE_OK );
				++count;
			}
		}
	}

	return count;
}

int load_sesqlite_contexts(
	sqlite3 *db,                   /* the database connection */
	struct sesqlite_context *sc    /* the sesqlite_context */
){
	return reload_sesqlite_contexts(db, sc, "*", "*", "*");
}

#endif

