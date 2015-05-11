#include "sesqlite_hash_impl.h"

typedef sqliteHash SESQLITE_HASH;

#define SESQLITE_HASH_INT                                      SQLITE_HASH_INT
#define SESQLITE_HASH_STRING                                   SQLITE_HASH_STRING
#define SESQLITE_HASH_BINARY                                   SQLITE_HASH_BINARY

#define SESQLITE_HASH_INIT(HASH, KEYTYPE, COPYKEY, COPYVALUE)  sqliteHashInit(HASH, KEYTYPE, COPYKEY, COPYVALUE)
#define SESQLITE_HASH_INSERT(HASH, PKEY, NKEY, PDATA, NDATA)   sqliteHashInsert(HASH, PKEY, NKEY, PDATA, NDATA)
#define SESQLITE_HASH_REMOVE(HASH, PKEY, NKEY)                 sqliteHashInsert(HASH, PKEY, NKEY, NULL, 0)
#define SESQLITE_HASH_FIND(HASH, PKEY, NKEY, PRES, NRES)       sqliteHashFind(HASH, PKEY, NKEY, PRES, NRES)
#define SESQLITE_HASH_CLEAR(HASH)                              sqliteHashClear(HASH)

