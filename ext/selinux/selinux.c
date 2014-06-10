/* SeSqlite extension to add SELinux checks in SQLite */

#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX)

#include "selinux.h"
#include <stdlib.h>
#include <selinux/selinux.h>

#ifndef SQLITE_CORE
  #include "sqlite3ext.h"
  SQLITE_EXTENSION_INIT1
#else
  #include "sqlite3.h"
#endif

#ifdef _WIN32
__declspec(dllexport)
#endif

int selinux_authorizer(void *pUserData, int type, const char *arg1,
		const char *arg2, const char *dbname, const char *source) {
	int rc = SQLITE_OK;
	return rc;
}

static void selinuxCheckAccessFunction(sqlite3_context *context, int argc,
		sqlite3_value **argv) {
	int rc = selinux_check_access(
	sqlite3_value_text(argv[0]), /* source security context */
	sqlite3_value_text(argv[1]), /* target security context */
	sqlite3_value_text(argv[2]), /* target security class string */
	sqlite3_value_text(argv[3]), /* requested permissions string */
	NULL /* auxiliary audit data */
	);

	sqlite3_result_int(context, rc == 0);
}

int sqlite3SelinuxInit(sqlite3 *db) {
	int rc = SQLITE_OK;

	sqlite3_create_function(db, "selinux_check_access", 4,
			SQLITE_UTF8 /* | SQLITE_DETERMINISTIC */, 0,
			selinuxCheckAccessFunction, 0, 0);

	sqlite3_set_authorizer(db, selinux_authorizer, NULL);

	//sqlite3_create_module(db, "");

	return rc;
}

#if !SQLITE_CORE
int sqlite3_extension_init(
  sqlite3 *db,
  char **pzErrMsg,
  const sqlite3_api_routines *pApi
){
  SQLITE_EXTENSION_INIT2(pApi)
  return sqlite3SelinuxInit(db);
}
#endif

#endif
