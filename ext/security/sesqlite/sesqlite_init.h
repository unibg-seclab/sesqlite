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

