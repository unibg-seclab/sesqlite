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
		free(head->origin);
		free(head->fparam);
		free(head->sparam);
		free(head->tparam);
		free(head->security_context);
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
	int n_line, ndb_line, ntable_line, ncolumn_line, ntuple_line;
	char line[255];
	char *p      = NULL;
	char *token  = NULL;
	char *stoken = NULL;
	FILE *fp     = NULL;

	struct sesqlite_context *sc = sqlite3_malloc(
		sizeof(struct sesqlite_context));

	// TODO modify the liselinux in order to retrieve the context from the targeted folder
	fp = fopen(path, "rb");
	if( fp==NULL ){
		fprintf(stderr, "Error. Unable to open '%s' configuration file.\n", path);
		return sc;
	}

	sc->db_context     = NULL;
	sc->table_context  = NULL;
	sc->column_context = NULL;
	sc->tuple_context  = NULL;

	n_line = 0;
	ndb_line = 0;
	ntable_line = 0;
	ncolumn_line = 0;
	ntuple_line = 0;

	/* Read the file */
	while( fgets(line, sizeof line - 1, fp) ){

		if( line[strlen(line) - 1]=='\n' )
			line[strlen(line) - 1] = 0;

		p = line;

		while( isspace(*p) )
			p++;

		if( *p=='#' || *p==0 )
			continue;

		token = strtok(p, " \t");

		if( !strcasecmp(token, "db_database") ){

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

			sorted_insert(&sc->db_context, new);
			ndb_line++;

		}else if( !strcasecmp(token, "db_table") ){

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

			sorted_insert(&sc->table_context, new);
			ntable_line++;

		}else if( !strcasecmp(token, "db_column") ){

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

			sorted_insert(&sc->column_context, new);
			ncolumn_line++;

		}else if( !strcasecmp(token, "db_tuple") ){

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

int filter_accepts(
	const char *filter,
	const char *name
){
	return ( filter==NULL ||
		sqlite3StrNICmp(filter, name, strcspn(filter, "*"))==0);
}

int reload_sesqlite_contexts(
	sqlite3 *db,                  /* the database connection */
	sqlite3_stmt *stmt,           /* the INSERT OR REPLACE statement */
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

	/* Scan the databases */
	for( i = (db->nDb - 1), pDb = &db->aDb[i]; i>=0; i--, pDb-- ){

		if( OMIT_TEMPDB && i==1 )
			continue;

		/* Filter database */
		dbName = pDb->zName;
		if( !filter_accepts(dbFilter, dbName) )
			continue;

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

			int sec_label_id = insert_context(db, 0, dbName, tblName, NULL,
				sc->table_context, sc->tuple_context);

			int sec_con_id = insert_context(db, 0, dbName, tblName, NULL,
				sc->tuple_context, sc->tuple_context);

			insert_key(db, dbName, tblName, NULL, sec_label_id);

			sqlite3_bind_int( stmt, 1, sec_con_id);
			sqlite3_bind_int( stmt, 2, sec_label_id);
			sqlite3_bind_text(stmt, 3, dbName,  -1, SQLITE_TRANSIENT);
			sqlite3_bind_text(stmt, 4, tblName, -1, SQLITE_TRANSIENT);
			sqlite3_bind_text(stmt, 5, "",      -1, SQLITE_TRANSIENT);

			rc = sqlite3_step(stmt);
			assert( rc==SQLITE_DONE );

			rc = sqlite3_reset(stmt);
			assert( rc==SQLITE_OK );

			/* Scan the columns */
			Column *pCol;
			for (j = 0, pCol = pTab->aCol; j < pTab->nCol; j++, pCol++) {

				/* Filter column */
				colName = pCol->zName;
				if( !filter_accepts(colFilter, colName) )
					continue;

				int sec_label_id = insert_context(db, 1, dbName, tblName, colName,
					sc->column_context, sc->tuple_context);

				int sec_con_id = insert_context(db, 0, dbName, tblName, NULL,
					sc->tuple_context, sc->tuple_context);

				insert_key(db, dbName, tblName, colName, sec_label_id);

				sqlite3_bind_int( stmt, 1, sec_con_id);
				sqlite3_bind_int( stmt, 2, sec_label_id);
				sqlite3_bind_text(stmt, 3, dbName,  -1, SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt, 4, tblName, -1, SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt, 5, colName, -1, SQLITE_TRANSIENT);

				rc = sqlite3_step(stmt);
				assert( rc==SQLITE_DONE );

				rc = sqlite3_reset(stmt);
				assert( rc==SQLITE_OK );
			}

			/* assign security context to rowid if exists */
			if( HasRowid(pTab) && filter_accepts(colFilter, "ROWID") ){
				int sec_label_id = insert_context(db, 1, dbName, tblName, "ROWID",
					sc->column_context, sc->tuple_context);

				int sec_con_id = insert_context(db, 0, dbName, tblName, NULL,
					sc->tuple_context, sc->tuple_context);

				insert_key(db, dbName, tblName, "ROWID", sec_label_id);

				sqlite3_bind_int( stmt, 1, sec_con_id);
				sqlite3_bind_int( stmt, 2, sec_label_id);
				sqlite3_bind_text(stmt, 3, dbName,  -1, SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt, 4, tblName, -1, SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt, 5, "ROWID", -1, SQLITE_TRANSIENT);

				rc = sqlite3_step(stmt);
				assert( rc==SQLITE_DONE );

				rc = sqlite3_reset(stmt);
				assert( rc==SQLITE_OK );
			}
		}
	}

	return SQLITE_OK;
}

/* Just a convenience function to load the sesqlite_contexts unfiltered */
int load_sesqlite_contexts(
	sqlite3 *db,                   /* the database connection */
	sqlite3_stmt *stmt,            /* the INSERT OR REPLACE statement */
	struct sesqlite_context *sc    /* the sesqlite_context */
){
	return reload_sesqlite_contexts(db, stmt, sc, NULL, NULL, NULL);
}

#endif

