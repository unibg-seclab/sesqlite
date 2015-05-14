#ifndef _SESQLITE_HASH_WRAPPER_H_
#define _SESQLITE_HASH_WRAPPER_H_

#include "sesqlite_hash_impl.h"

#define SESQLITE_HASH_INT                                      0
#define SESQLITE_HASH_STRING                                   0
#define SESQLITE_HASH_BINARY                                   0

#define SESQLITE_HASH                                          tommyHash
#define SESQLITE_HASH_INIT(HASH, KEYTYPE, COPYKEY, COPYVALUE)  tommyHashInit(HASH, COPYKEY, COPYVALUE)
#define SESQLITE_HASH_INSERT(HASH, PKEY, NKEY, PDATA, NDATA)   tommyHashInsert(HASH, PKEY, NKEY, PDATA, NDATA)
#define SESQLITE_HASH_REMOVE(HASH, PKEY, NKEY)                 tommyHashRemove(HASH, PKEY, NKEY)
#define SESQLITE_HASH_FIND(HASH, PKEY, NKEY, PRES, NRES)       tommyHashFind(HASH, PKEY, NKEY, (void**)(PRES), NRES)
#define SESQLITE_HASH_CLEAR(HASH)                              tommyHashClear(HASH)
#define SESQLITE_HASH_FREE(HASH)                               tommyHashFree(HASH)

typedef struct tommyHash    tommyHash;
typedef struct tommyElement tommyElement;
typedef struct tommyPointer tommyPointer;

struct tommyHash {
	int options;
	tommy_hashlin *hash;
};

struct tommyPointer {
	void* p;
	int n;
};

struct tommyElement {
	tommyPointer key;
	tommyPointer value;
	tommy_node node;
};

void tommyHashInit(tommyHash* th, int copyKey, int copyValue);
void tommyHashInsert(tommyHash* th, const void* pKey, int nKey, void* pValue, int nValue);
void tommyHashFind(const tommyHash* th, const void* pKey, int nKey, void** pRes, int* nRes);
void tommyHashRemove(const tommyHash* th, const void *pKey, int nKey);
void tommyHashClear(tommyHash* th);
void tommyHashFree(tommyHash* th);

#endif /* _SESQLITE_HASH_WRAPPER_H_ */
