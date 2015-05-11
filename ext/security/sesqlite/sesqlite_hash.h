#include "hash/sesqlite_hash_wrapper.h"

/* Bidirection hash table (it just uses two hash tables */
typedef struct SESQLITE_BIHASH SESQLITE_BIHASH;

struct SESQLITE_BIHASH {
  SESQLITE_HASH *key2val;
  SESQLITE_HASH *val2key;
};

/*
** Access routines for bidirectional hash tables.
*/
void SESQLITE_BIHASH_INIT(SESQLITE_BIHASH*, int keytype, int valtype, int copyKey, int copyValue);
void SESQLITE_BIHASH_INSERT(SESQLITE_BIHASH*, const void *pKey, int nKey, const void *pVal, int nVal);
void SESQLITE_BIHASH_FIND(const SESQLITE_BIHASH*, const void *pKey, int nKey, void **pRes, int *nRes);
void SESQLITE_BIHASH_FINDKEY(const SESQLITE_BIHASH*, const void *pVal, int nVal, void **pRes, int *nRes);
void SESQLITE_BIHASH_CLEAR(SESQLITE_BIHASH*);
void SESQLITE_BIHASH_FREE(SESQLITE_BIHASH*);

