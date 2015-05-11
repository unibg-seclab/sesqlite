/*
 ** 2001 September 22
 **
 ** The author disclaims copyright to this source code.  In place of
 ** a legal notice, here is a blessing:
 **
 **    May you do good and not evil.
 **    May you find forgiveness for yourself and forgive others.
 **    May you share freely, never taking more than you give.
 **
 *************************************************************************
 ** This is the implementation of generic hash-tables used in SQLite.
 ** We've modified it slightly to serve as a standalone hash table
 ** implementation for the full-text indexing module.
 */

#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX)

#include "hash/sesqlite_hash.h"

void *sqlite_malloc_and_zero(int n) {
	void *p = malloc(n);
	if (p) {
		memset(p, 0, n);
	}
	return p;
}

/* Turn bulk memory into a hash table object by initializing the
 ** fields of the Hash structure.
 **
 ** "pNew" is a pointer to the hash table that is to be initialized.
 ** keyClass is one of the constants HASH_INT, HASH_POINTER,
 ** HASH_BINARY, or HASH_STRING.  The value of keyClass
 ** determines what kind of key the hash table will use.  "copyKey" is
 ** true if the hash table should make its own private copy of keys and
 ** false if it should just use the supplied pointer.  CopyKey only makes
 ** sense for HASH_STRING and HASH_BINARY and is ignored
 ** for other key classes.
 ** "copyValue" is true if the hash table should make its own private copy
 ** of values and false if it should just use the supplied pointer.
 */
void sqliteHashInit(sqliteHash *pNew, int keyClass, int copyKey, int copyValue) {
	assert(pNew != 0);
	assert(keyClass>= SQLITE_HASH_INT && keyClass<= SQLITE_HASH_BINARY);
	pNew->keyClass = keyClass;
	if( keyClass== SQLITE_HASH_INT ) copyKey = 0;
	pNew->copyKey = copyKey;
	pNew->copyValue = copyValue;
	pNew->first = 0;
	pNew->count = 0;
	pNew->htsize = 0;
	pNew->sqlite_ht = 0;
	pNew->xMalloc = sqlite_malloc_and_zero;
	pNew->xFree = free;
}

/* Remove all entries from a hash table.  Reclaim all memory.
 ** Call this routine to delete a hash table or to reset a hash table
 ** to the empty state.
 */
void sqliteHashClear(sqliteHash *pH) {
	sqliteHashElem *elem; /* For looping over all elements of the table */

	assert(pH != 0);
	elem = pH->first;
	pH->first = 0;
	if (pH->sqlite_ht)
		pH->xFree(pH->sqlite_ht);
	pH->sqlite_ht = 0;
	pH->htsize = 0;
	while (elem) {
		sqliteHashElem *next_elem = elem->next;
		if (pH->copyKey && elem->pKey) {
			pH->xFree(elem->pKey);
		}
		if (pH->copyValue && elem->pData) {
			pH->xFree(elem->pData);
		}
		pH->xFree(elem);
		elem = next_elem;
	}
	pH->count = 0;
}

/*
 ** Hash and comparison functions when the mode is HASH_INT
 */
static int sqliteIntHash(const void *pKey, int nKey) {
	return nKey ^ (nKey<<8) ^ (nKey>>8);
}
static int sqliteIntCompare(const void *pKey1, int n1, const void *pKey2, int n2) {
	return n2 - n1;
}


/*
 ** Hash and comparison functions when the mode is HASH_STRING
 */
static int sqliteStrHash(const void *pKey, int nKey) {
	const char *z = (const char *) pKey;
	int h = 0;
	if (nKey <= 0)
		nKey = (int) strlen(z);
	while (nKey > 0) {
		h = (h << 3) ^ h ^ *z++;
		nKey--;
	}
	return h & 0x7fffffff;
}
static int sqliteStrCompare(const void *pKey1, int n1, const void *pKey2,
		int n2) {
	if (n1 != n2)
		return 1;
	return strncmp((const char*) pKey1, (const char*) pKey2, n1);
}

/*
 ** Hash and comparison functions when the mode is HASH_BINARY
 */
static int sqliteBinHash(const void *pKey, int nKey) {
	int h = 0;
	const char *z = (const char *) pKey;
	while (nKey-- > 0) {
		h = (h << 3) ^ h ^ *(z++);
	}
	return h & 0x7fffffff;
}

static int sqliteBinCompare(const void *pKey1, int n1, const void *pKey2,
		int n2) {
	if (n1 != n2)
		return 1;
	return memcmp(pKey1, pKey2, n1);
}

/*
 ** Return a pointer to the appropriate hash function given the key class.
 **
 ** The C syntax in this function definition may be unfamilar to some
 ** programmers, so we provide the following additional explanation:
 **
 ** The name of the function is "hashFunction".  The function takes a
 ** single parameter "keyClass".  The return value of hashFunction()
 ** is a pointer to another function.  Specifically, the return value
 ** of hashFunction() is a pointer to a function that takes two parameters
 ** with types "const void*" and "int" and returns an "int".
 */
static int (*sqliteHashFunction(int keyClass))(const void*,int) {
	switch( keyClass ) {
		case SQLITE_HASH_INT: return &sqliteIntHash;
		case SQLITE_HASH_STRING: return &sqliteStrHash;
		case SQLITE_HASH_BINARY: return &sqliteBinHash;
		default: break;
	}
	return 0;
}


/*
** Return a pointer to the appropriate hash function given the key class.
**
** For help in interpreted the obscure C code in the function definition,
** see the header comment on the previous function.
*/
static int (*sqliteCompareFunction(int keyClass))(const void*,int,const void*,int){
  switch( keyClass ){
    case SQLITE_HASH_INT:     return &sqliteIntCompare;
    case SQLITE_HASH_STRING:  return &sqliteStrCompare;
    case SQLITE_HASH_BINARY:  return &sqliteBinCompare;
    default: break;
  }
  return 0;
}


/* Link an element into the hash table
*/
static void sqliteInsertElement(
		sqliteHash *pH,              /* The complete hash table */
  struct sqlite_ht *pEntry,    /* The entry into which pNew is inserted */
  sqliteHashElem *pNew         /* The element to be inserted */
){
  sqliteHashElem *pHead;       /* First element already in pEntry */
  pHead = pEntry->chain;
  if( pHead ){
    pNew->next = pHead;
    pNew->prev = pHead->prev;
    if( pHead->prev ){ pHead->prev->next = pNew; }
    else             { pH->first = pNew; }
    pHead->prev = pNew;
  }else{
    pNew->next = pH->first;
    if( pH->first ){ pH->first->prev = pNew; }
    pNew->prev = 0;
    pH->first = pNew;
  }
  pEntry->count++;
  pEntry->chain = pNew;
}


/* Resize the hash table so that it cantains "new_size" buckets.
** "new_size" must be a power of 2.  The hash table might fail
** to resize if sqliteMalloc() fails.
*/
static void sqliteRehash(sqliteHash *pH, int new_size){
  struct sqlite_ht *new_ht;            /* The new hash table */
  sqliteHashElem *elem, *next_elem;    /* For looping over existing elements */
  int (*xHash)(const void*,int); /* The hash function */

  assert( (new_size & (new_size-1))==0 );
  new_ht = (struct sqlite_ht *)pH->xMalloc( new_size*sizeof(struct sqlite_ht) );
  if( new_ht==0 ) return;
  if( pH->sqlite_ht ) pH->xFree(pH->sqlite_ht);
  pH->sqlite_ht = new_ht;
  pH->htsize = new_size;
  xHash = sqliteHashFunction(pH->keyClass);
  for(elem=pH->first, pH->first=0; elem; elem = next_elem){
    int h = (*xHash)(elem->pKey, elem->nKey) & (new_size-1);
    next_elem = elem->next;
    sqliteInsertElement(pH, &new_ht[h], elem);
  }
}

/* This function (for internal use only) locates an element in an
** hash table that matches the given key.  The hash for this key has
** already been computed and is passed as the 4th parameter.
*/
static sqliteHashElem *sqliteFindElementGivenHash(
  const sqliteHash *pH,     /* The pH to be searched */
  const void *pKey,   /* The key we are searching for */
  int nKey,
  int h               /* The hash for this key. */
){
	sqliteHashElem *elem;                /* Used to loop thru the element list */
  int count;                     /* Number of elements left to test */
  int (*xCompare)(const void*,int,const void*,int);  /* comparison function */

  if( pH->sqlite_ht ){
    struct sqlite_ht *pEntry = &pH->sqlite_ht[h];
    elem = pEntry->chain;
    count = pEntry->count;
    xCompare = sqliteCompareFunction(pH->keyClass);
    while( count-- && elem ){
      if( (*xCompare)(elem->pKey,elem->nKey,pKey,nKey)==0 ){
        return elem;
      }
      elem = elem->next;
    }
  }
  return 0;
}

/* Remove a single entry from the hash table given a pointer to that
** element and a hash on the element's key.
*/
static void sqliteRemoveElementGivenHash(
		sqliteHash *pH,         /* The pH containing "elem" */
		sqliteHashElem* elem,   /* The element to be removed from the pH */
  int h             /* Hash value for the element */
){
  struct sqlite_ht *pEntry;
  if( elem->prev ){
    elem->prev->next = elem->next;
  }else{
    pH->first = elem->next;
  }
  if( elem->next ){
    elem->next->prev = elem->prev;
  }
  pEntry = &pH->sqlite_ht[h];
  if( pEntry->chain==elem ){
    pEntry->chain = elem->next;
  }
  pEntry->count--;
  if( pEntry->count<=0 ){
    pEntry->chain = 0;
  }
  if( pH->copyKey && elem->pKey ){
    pH->xFree(elem->pKey);
  }
  if( pH->copyValue && elem->pData ){
    pH->xFree(elem->pData);
  }
  pH->xFree( elem );
  pH->count--;
  if( pH->count<=0 ){
    assert( pH->first==0 );
    assert( pH->count==0 );
    sqliteHashClear(pH);
  }
}

/* Attempt to locate an element of the hash table pH with a key
** that matches pKey,nKey.  *pRes is set to be the data for this element if it is
** found, or NULL if there is no match, *nRes will be the n for the element.
** If nKey<0, the key is assumed to be a string and nKey is computed accordingly.
*/
void sqliteHashFind(const sqliteHash *pH, const void *pKey, int nKey, void **pRes, int *nRes){
  int h;             /* A hash on key */
  sqliteHashElem *elem;    /* The element that matches key */
  int (*xHash)(const void*,int);  /* The hash function */

  if( pH==0 || pH->sqlite_ht==0 ){
    if( pRes ){ *pRes = 0; }
    if( nRes ){ *nRes = 0; }
  }else{
    xHash = sqliteHashFunction(pH->keyClass);
    assert( xHash!=0 );
    if( nKey<0 && pKey!=0 ){
      nKey = sizeof(char) * (1 + strlen(pKey));
    }
    h = (*xHash)(pKey,nKey);
    assert( (pH->htsize & (pH->htsize-1))==0 );
    elem = sqliteFindElementGivenHash(pH,pKey,nKey, h & (pH->htsize-1));
    if( pRes ){ *pRes = elem ? elem->pData : 0; }
    if( nRes ){ *nRes = elem ? elem->nData : 0; }
  }
}

/* Insert an element into the hash table pH.  The key is pKey,nKey
** and the data is "data".
**
** If no element exists with a matching key, then a new
** element is created.  A copy of the key is made if the copyKey
** flag is set. A copy of the value is made if the copyValue
** flag is set. NULL is returned.
**
** If another element already exists with the same key, then the
** new data replaces the old data.
**
** If the "data" parameter to this function is NULL, then the
** element corresponding to "key" is removed from the hash table.
**
** if nKey is <0, the key is assumed to be a string and the correct
** number of bytes is computed. The same holds for nData.
*/
void sqliteHashInsert(sqliteHash *pH, const void *pKey, int nKey, void *pData, int nData){
  int hraw;             /* Raw hash value of the key */
  int h;                /* the hash of the key modulo hash table size */
  sqliteHashElem *elem;       /* Used to loop thru the element list */
  sqliteHashElem *new_elem;   /* New element added to the pH */
  int (*xHash)(const void*,int);  /* The hash function */

  assert( pH!=0 );
  xHash = sqliteHashFunction(pH->keyClass);
  assert( xHash!=0 );

  if( nKey<0  && pKey!=0  ){
    nKey  = sizeof(char) * (1 + strlen(pKey));
  }
  if( nData<0 && pData!=0 ){
    nData = sizeof(char) * (1 + strlen(pData));
  }

  hraw = (*xHash)(pKey, nKey);
  assert( (pH->htsize & (pH->htsize-1))==0 );
  h = hraw & (pH->htsize-1);
  elem = sqliteFindElementGivenHash(pH,pKey,nKey,h);
  if( elem ){
	if( pData==0 ){
		sqliteRemoveElementGivenHash(pH,elem,h);
	}else{
		if( pH->copyValue ){
			if( elem->pData ){
				pH->xFree(elem->pData);
			}
			elem->pData = pH->xMalloc(nData);
			if( elem->pData==0 ){
				pH->xFree(elem);
				return;
			}
			memcpy((void*)elem->pData, pData, nData);
		}else{
			elem->pData = pData;
		}
	}
	return;
  }
  if( pData==0 ) return;
  new_elem = (sqliteHashElem*)pH->xMalloc( sizeof(sqliteHashElem) );
  if( new_elem==0 ) return;
  if( pH->copyKey && pKey!=0 ){
    new_elem->pKey = pH->xMalloc( nKey );
    if( new_elem->pKey==0 ){
      pH->xFree(new_elem);
      return;
    }
    memcpy((void*)new_elem->pKey, pKey, nKey);
  }else{
    new_elem->pKey = (void*)pKey;
  }
  if( pH->copyValue && pData!=0 ){
    new_elem->pData = pH->xMalloc( nData );
    if( new_elem->pData==0 ){
      pH->xFree(new_elem);
      return;
    }
    memcpy((void*)new_elem->pData, pData, nData);
  }else{
    new_elem->pData = pData;
  }
  new_elem->nKey = nKey;
  new_elem->nData = nData;
  pH->count++;
  if( pH->htsize==0 ){
	  sqliteRehash(pH,8);
    if( pH->htsize==0 ){
      pH->count = 0;
      pH->xFree(new_elem);
      return;
    }
  }
  if( pH->count > pH->htsize ){
	  sqliteRehash(pH,pH->htsize*2);
  }
  assert( pH->htsize>0 );
  assert( (pH->htsize & (pH->htsize-1))==0 );
  h = hraw & (pH->htsize-1);
  sqliteInsertElement(pH, &pH->sqlite_ht[h], new_elem);
  return;
}

#endif /* !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX) */

