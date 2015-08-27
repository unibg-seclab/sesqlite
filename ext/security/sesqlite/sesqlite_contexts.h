#include "sesqlite.h"

/*
 * Read the sesqlite_contexts file and return a struct
 * representing it.
 */
struct sesqlite_context *read_sesqlite_context(
	sqlite3 *db,
	char *path
);

/*
 * Insert the security context from sesqlite_contexts into the
 * SElinux tables and hashmaps. The update parameter can be
 * UPDATE_SELINUX_TABLE, UPDATE_SELINUX_HASH or UPDATE_SELINUX_ALL.
 * The *filter parameters can be NULL to indicate a wildcard or a string
 * to indicate that only the (db/table/column) that match the filter must
 * have its value reloaded.
 * It returns the number of tuples updated in the sesqlite_context table.
 */
int reload_sesqlite_contexts(
	sqlite3 *db,                  /* the database connection */
	struct sesqlite_context *sc,  /* the sesqlite_context */
	char *dbFilter,               /* the db filter or NULL as wildcard */
	char *tblFilter,              /* the table filter or NULL as wildcard */
	char *colFilter               /* the column filter or NULL as wildcard */
);

/*
 * Just a convenience function to load the sesqlite_contexts unfiltered.
 * It returns the number of tuples inserted in the sesqlite_context table.
 */
int load_sesqlite_contexts(
	sqlite3 *db,                  /* the database connection */
	struct sesqlite_context *sc   /* the sesqlite_context */
);

/* Free the entire sesqlite_context structure */
void free_sesqlite_context(
	struct sesqlite_context *contexts
);

