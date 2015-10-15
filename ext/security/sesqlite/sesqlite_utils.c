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

/* SeSqlite extension to add SELinux checks in SQLite */

#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX)

#include "sesqlite_utils.h"
#include "sesqlite.h"

/*
 * Print messages for SeSQLite.
 */
void sesqlite_print(
	const char *before,
	const char *dbName,
	const char *tblName,
	const char *colName,
	const char *after
){
	fprintf(stdout, "SeSQLite: ");

	if( before )  fprintf(stdout, "%s ", before);
	if( dbName )  fprintf(stdout, "db=%s ", dbName);
	if( tblName ) fprintf(stdout, "table=%s ", tblName);
	if( colName ) fprintf(stdout, "column=%s ", colName);
	if( after )   fprintf(stdout, "%s\n", after);
}

/*
 * Makes the key based on the database, the table and the column.
 * The user must invoke free on the returned pointer to free the memory.
 * It returns db:tbl:col if the column is not NULL, otherwise db:tbl.
 */
int make_key(
	sqlite3 *db,
	const char *dbName,
	const char *tblName,
	const char *colName,
	char **key
){

	if( tblName==NULL )
		sqlite3SetString(key, db, "%s", dbName);
	else if( colName==NULL )
		sqlite3SetString(key, db, "%s:%s", dbName, tblName);
	else
		sqlite3SetString(key, db, "%s:%s:%s", dbName, tblName, colName);

	if(key == NULL)
		return SQLITE_ERROR;

	return SQLITE_OK;
}

#endif

