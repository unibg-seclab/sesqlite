/* SeSqlite extension to add SELinux checks in SQLite */

#include <sqlite3ext.h>
#include <stdlib.h>

SQLITE_EXTENSION_INIT1

#ifdef _WIN32
__declspec(dllexport)
#endif

int selinux_authorizer(
  void *pUserData, int type,
  const char *arg1,
  const char *arg2,
  const char *dbname,
  const char *source
){
  int rc = SQLITE_OK;
  return rc;
}

/* ^ AUTHORIZER ^ */

int sqlite3_selinux_init(
  sqlite3 *db,
  char **pzErrMsg,
  const sqlite3_api_routines *pApi
){
  int rc = SQLITE_OK;
  SQLITE_EXTENSION_INIT2(pApi);

  sqlite3_set_authorizer(db, selinux_authorizer, NULL);

  return rc;
}
