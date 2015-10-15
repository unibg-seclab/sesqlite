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

#define SESQLITE_CONTEXTS_PATH "./sesqlite_contexts"

int initialize(sqlite3 *db);

int compute_sql_context(int isColumn, char *dbName, char *tblName,
	char *colName, struct sesqlite_context_element * con, char **res);

#define SELINUX_CONTEXT_TABLE \
	"CREATE TABLE IF NOT EXISTS selinux_context(" \
	" security_context INT," \
	" security_label INT," \
	" db TEXT," \
	" name TEXT," \
	" column TEXT," \
	" PRIMARY KEY(db, name, column)" \
	");"
/* TODO use FK */

/* use rowid */
#define SELINUX_ID_TABLE \
	"CREATE TABLE IF NOT EXISTS selinux_id(" \
	" security_context INT," \
	" security_label TEXT UNIQUE" \
	");"

#define CHECK_WRONG_USAGE(CONDITION, USAGE) \
  if( CONDITION ){ \
    fprintf(stdout, USAGE); \
    return; \
  }

#define MORE_TOKENS (NULL!=strtok(NULL, ""))

