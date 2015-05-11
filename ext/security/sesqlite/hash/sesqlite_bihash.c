
#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX)

#include "hash/sesqlite_bihash.h"

void SESQLITE_BIHASH_INIT(SESQLITE_BIHASH* bihash, int keytype, int valtype, int copyKey, int copyValue) {
  bihash->key2val = malloc(sizeof(SESQLITE_HASH));
  bihash->val2key = malloc(sizeof(SESQLITE_HASH));
  SESQLITE_HASH_INIT(bihash->key2val, keytype, copyKey, copyValue);
  SESQLITE_HASH_INIT(bihash->val2key, valtype, copyKey, copyValue);
}

void SESQLITE_BIHASH_INSERT(SESQLITE_BIHASH* bihash, const void *pKey, int nKey, const void *pVal, int nVal){
  int nOld;
  void *pOld;

  // insert new key -> value association
  SESQLITE_HASH_FIND(bihash->key2val, pKey, nKey, &pOld, &nOld);
  SESQLITE_HASH_INSERT(bihash->key2val, pKey, nKey, (void*)pVal, nVal);

  // remove old value -> key association if exists
  if( pOld!=0 ){
    if( nOld!=0 ){
      SESQLITE_HASH_REMOVE(bihash->val2key, pOld, nOld);
    }else{
      fprintf(stderr, "ERROR: cannot remove old value -> key association in BiHash (no nVal set).\n");
    }
  }

  // insert new value -> key association if provided
  if( pVal!=0 ){
    SESQLITE_HASH_FIND(bihash->val2key, pVal, nVal, &pOld, &nOld);
    if( pOld!=0 ){
      fprintf(stderr, "ERROR: value already associated to another key in BiHash (not injective).\n");
    }else{
      SESQLITE_HASH_INSERT(bihash->val2key, pVal, nVal, (void*)pKey, nKey);
    }
  }
}

void SESQLITE_BIHASH_FIND(const SESQLITE_BIHASH* bihash, const void *pKey, int nKey, void **pRes, int *nRes){
  SESQLITE_HASH_FIND(bihash->key2val, pKey, nKey, pRes, nRes);
}

void SESQLITE_BIHASH_FINDKEY(const SESQLITE_BIHASH* bihash, const void *pValue, int nValue, void **pRes, int *nRes){
  SESQLITE_HASH_FIND(bihash->val2key, pValue, nValue, pRes, nRes);
}

void SESQLITE_BIHASH_CLEAR(SESQLITE_BIHASH* bihash){
  SESQLITE_HASH_CLEAR(bihash->val2key);
  SESQLITE_HASH_CLEAR(bihash->key2val);
}

void SESQLITE_BIHASH_FREE(SESQLITE_BIHASH* bihash) {
  SESQLITE_BIHASH_CLEAR(bihash);
  if (bihash->key2val)
    free(bihash->key2val);
  if (bihash->val2key)
    free(bihash->val2key);
}

#endif /* !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX) */

