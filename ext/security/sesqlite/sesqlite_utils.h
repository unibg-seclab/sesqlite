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
);

void sesqlite_clearavc();

/*
 * Makes the key based on the database, the table and the column.
 * The user must invoke free on the returned pointer to free the memory.
 * It returns db:tbl:col if the column is not NULL, otherwise db:tbl.
 */
char *make_key(
	const char *dbName,
	const char *tblName,
	const char *colName
);
