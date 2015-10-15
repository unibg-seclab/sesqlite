/*
** Authors: Simone Mutti <simone.mutti@unibg.it>
**          Enrico Bacis <enrico.bacis@unibg.it>
**
** Copyright 2015, Università degli Studi di Bergamo
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX)

#include "sesqlite.h"

/**
 * Compute the default context to give to a table or a column
 * based on the sesqlite_context_element structure.
 */
int compute_sql_context(
    int isColumn,
    char *dbName,
    char *tblName,
    char *colName,
    struct sesqlite_context_element *con,
    char **res
){
    int rc = SQLITE_OK;
    int found = 0;
    struct sesqlite_context_element *p = 0;
    p = con;
    while (found == 0 && p != NULL) {
		if (strcasecmp(dbName, p->fparam) == 0 || strcmp(p->fparam, "*") == 0) {
			if(tblName){
				if (strcasecmp(tblName, p->sparam) == 0
					|| strcmp(p->sparam, "*") == 0) {
					if(isColumn){
						if (strcasecmp(colName, p->tparam) == 0
							|| strcmp(p->tparam, "*") == 0)
						break;
					}else
						break; /* no column */
				}
			}else
				break;
		}
	p = p->next;
    }
    if(p != NULL)
		//TODO check for type transition
		*res = p->security_context;
    else{
		/* the sesqlite_context file does not contain a context for the
		 * table/column we want to store, then compute the default one. */
		security_context_t p_con = NULL;
		rc = getcon(&p_con);
		if(security_compute_create_raw(p_con, p_con, 4, res) < 0){
			fprintf(stderr, "SELinux could not compute a default context\n");
			return 0;
		}
    }
    return rc;
}


int lookup_security_context(SESQLITE_BIHASH *hash, char *db_name, char *tbl_name){

    int *id = NULL;
    char *sec_context = NULL;

    compute_sql_context(0, db_name, tbl_name, NULL, 
	    contexts->tuple_context, &sec_context);

    SESQLITE_BIHASH_FINDKEY(hash, sec_context, -1, (void**) &id, 0);
    assert(id != NULL); /* check if SELinux can compute a security context */

    return *id;
}

int lookup_security_label(sqlite3 *db, 
	sqlite3_stmt *stmt, 
	SESQLITE_BIHASH *hash, 
	int type, 
	char *db_name, 
	char *tbl_name, 
	char *col_name){

    int rc = SQLITE_OK;
    int rowid = 0;
    int *id = NULL;
    char *context = NULL;

	compute_sql_context(type, db_name, tbl_name, col_name,
	    type ? contexts->column_context : contexts->table_context, &context);

    assert(context != NULL);
    SESQLITE_BIHASH_FINDKEY(hash, context, -1, (void**) &id, 0);
    if( id!=NULL )
      return *id;

	sqlite3_bind_int(stmt, 1, lookup_security_context(hash, db_name, SELINUX_ID));
	sqlite3_bind_text(stmt, 2, context, strlen(context),
	    SQLITE_TRANSIENT);

	rc = sqlite3_step(stmt);
	rc = sqlite3_reset(stmt);

	rowid = sqlite3_last_insert_rowid(db);
	SESQLITE_BIHASH_INSERT(hash, &rowid, sizeof(int), context, -1);
    return rowid;
}

#endif /* !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX) */

