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

#include "sesqlite_hash.h"

void *sesqlite_malloc_and_zero(int n) {
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
 */
void seSQLiteHashInit(seSQLiteHash *pNew, int keyClass, int copyKey) {
	assert(pNew != 0);
	assert(keyClass>= SESQLITE_HASH_INT && keyClass<= SESQLITE_HASH_BINARY);
	pNew->keyClass = keyClass;
	if( keyClass== SESQLITE_HASH_INT ) copyKey = 0;
	pNew->copyKey = copyKey;
	pNew->first = 0;
	pNew->count = 0;
	pNew->htsize = 0;
	pNew->sesqlite_ht = 0;
	pNew->xMalloc = sesqlite_malloc_and_zero;
	pNew->xFree = free;
}

/* Remove all entries from a hash table.  Reclaim all memory.
 ** Call this routine to delete a hash table or to reset a hash table
 ** to the empty state.
 */
void seSQLiteHashClear(seSQLiteHash *pH) {
	seSQLiteHashElem *elem; /* For looping over all elements of the table */

	assert(pH != 0);
	elem = pH->first;
	pH->first = 0;
	if (pH->sesqlite_ht)
		pH->xFree(pH->sesqlite_ht);
	pH->sesqlite_ht = 0;
	pH->htsize = 0;
	while (elem) {
		seSQLiteHashElem *next_elem = elem->next;
		if (pH->copyKey && elem->pKey) {
			pH->xFree(elem->pKey);
		}
		pH->xFree(elem);
		elem = next_elem;
	}
	pH->count = 0;
}

/*
 ** Hash and comparison functions when the mode is HASH_INT
 */
static int seSQLiteIntHash(const void *pKey, int nKey) {
	return nKey ^ (nKey<<8) ^ (nKey>>8);
}
static int seSQLiteIntCompare(const void *pKey1, int n1, const void *pKey2, int n2) {
	return n2 - n1;
}


/*
 ** Hash and comparison functions when the mode is HASH_STRING
 */
static int seSQLiteStrHash(const void *pKey, int nKey) {
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
static int seSQLiteStrCompare(const void *pKey1, int n1, const void *pKey2,
		int n2) {
	if (n1 != n2)
		return 1;
	return strncmp((const char*) pKey1, (const char*) pKey2, n1);
}

/*
 ** Hash and comparison functions when the mode is HASH_BINARY
 */
static int seSQLiteBinHash(const void *pKey, int nKey) {
	int h = 0;
	const char *z = (const char *) pKey;
	while (nKey-- > 0) {
		h = (h << 3) ^ h ^ *(z++);
	}
	return h & 0x7fffffff;
}

static int seSQLiteBinCompare(const void *pKey1, int n1, const void *pKey2,
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
static int (*seSQLiteHashFunction(int keyClass))(const void*,int) {
	switch( keyClass ) {
		case SESQLITE_HASH_INT: return &seSQLiteIntHash;
		case SESQLITE_HASH_STRING: return &seSQLiteStrHash;
		case SESQLITE_HASH_BINARY: return &seSQLiteBinHash;
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
static int (*seSQLiteCompareFunction(int keyClass))(const void*,int,const void*,int){
  switch( keyClass ){
    case SESQLITE_HASH_INT:     return &seSQLiteIntCompare;
    case SESQLITE_HASH_STRING:  return &seSQLiteStrCompare;
    case SESQLITE_HASH_BINARY:  return &seSQLiteBinCompare;
    default: break;
  }
  return 0;
}


/* Link an element into the hash table
*/
static void seSQLiteInsertElement(
		seSQLiteHash *pH,              /* The complete hash table */
  struct sesqlite_ht *pEntry,    /* The entry into which pNew is inserted */
  seSQLiteHashElem *pNew         /* The element to be inserted */
){
  seSQLiteHashElem *pHead;       /* First element already in pEntry */
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
static void seSQLiteRehash(seSQLiteHash *pH, int new_size){
  struct sesqlite_ht *new_ht;            /* The new hash table */
  seSQLiteHashElem *elem, *next_elem;    /* For looping over existing elements */
  int (*xHash)(const void*,int); /* The hash function */

  assert( (new_size & (new_size-1))==0 );
  new_ht = (struct sesqlite_ht *)pH->xMalloc( new_size*sizeof(struct sesqlite_ht) );
  if( new_ht==0 ) return;
  if( pH->sesqlite_ht ) pH->xFree(pH->sesqlite_ht);
  pH->sesqlite_ht = new_ht;
  pH->htsize = new_size;
  xHash = seSQLiteHashFunction(pH->keyClass);
  for(elem=pH->first, pH->first=0; elem; elem = next_elem){
    int h = (*xHash)(elem->pKey, elem->nKey) & (new_size-1);
    next_elem = elem->next;
    seSQLiteInsertElement(pH, &new_ht[h], elem);
  }
}

/* This function (for internal use only) locates an element in an
** hash table that matches the given key.  The hash for this key has
** already been computed and is passed as the 4th parameter.
*/
static seSQLiteHashElem *seSQLiteFindElementGivenHash(
  const seSQLiteHash *pH,     /* The pH to be searched */
  const void *pKey,   /* The key we are searching for */
  int nKey,
  int h               /* The hash for this key. */
){
	seSQLiteHashElem *elem;                /* Used to loop thru the element list */
  int count;                     /* Number of elements left to test */
  int (*xCompare)(const void*,int,const void*,int);  /* comparison function */

  if( pH->sesqlite_ht ){
    struct sesqlite_ht *pEntry = &pH->sesqlite_ht[h];
    elem = pEntry->chain;
    count = pEntry->count;
    xCompare = seSQLiteCompareFunction(pH->keyClass);
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
static void seSQLiteRemoveElementGivenHash(
		seSQLiteHash *pH,         /* The pH containing "elem" */
		seSQLiteHashElem* elem,   /* The element to be removed from the pH */
  int h             /* Hash value for the element */
){
  struct sesqlite_ht *pEntry;
  if( elem->prev ){
    elem->prev->next = elem->next;
  }else{
    pH->first = elem->next;
  }
  if( elem->next ){
    elem->next->prev = elem->prev;
  }
  pEntry = &pH->sesqlite_ht[h];
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
  pH->xFree( elem );
  pH->count--;
  if( pH->count<=0 ){
    assert( pH->first==0 );
    assert( pH->count==0 );
    seSQLiteHashClear(pH);
  }
}

/* Attempt to locate an element of the hash table pH with a key
** that matches pKey,nKey.  Return the data for this element if it is
** found, or NULL if there is no match.
*/
void *seSQLiteHashFind(const seSQLiteHash *pH, const void *pKey, int nKey){
  int h;             /* A hash on key */
  seSQLiteHashElem *elem;    /* The element that matches key */
  int (*xHash)(const void*,int);  /* The hash function */

  if( pH==0 || pH->sesqlite_ht==0 ) return 0;
  xHash = seSQLiteHashFunction(pH->keyClass);
  assert( xHash!=0 );
  h = (*xHash)(pKey,nKey);
  assert( (pH->htsize & (pH->htsize-1))==0 );
  elem = seSQLiteFindElementGivenHash(pH,pKey,nKey, h & (pH->htsize-1));
  return elem ? elem->pData : 0;
}

/* Insert an element into the hash table pH.  The key is pKey,nKey
** and the data is "data".
**
** If no element exists with a matching key, then a new
** element is created.  A copy of the key is made if the copyKey
** flag is set.  NULL is returned.
**
** If another element already exists with the same key, then the
** new data replaces the old data and the old data is returned.
** The key is not copied in this instance.  If a malloc fails, then
** the new data is returned and the hash table is unchanged.
**
** If the "data" parameter to this function is NULL, then the
** element corresponding to "key" is removed from the hash table.
*/
void *seSQLiteHashInsert(seSQLiteHash *pH, const void *pKey, int nKey, void *data, int nData, int *nDataOld){
  int hraw;             /* Raw hash value of the key */
  int h;                /* the hash of the key modulo hash table size */
  seSQLiteHashElem *elem;       /* Used to loop thru the element list */
  seSQLiteHashElem *new_elem;   /* New element added to the pH */
  int (*xHash)(const void*,int);  /* The hash function */

  assert( pH!=0 );
  xHash = seSQLiteHashFunction(pH->keyClass);
  assert( xHash!=0 );
  hraw = (*xHash)(pKey, nKey);
  assert( (pH->htsize & (pH->htsize-1))==0 );
  h = hraw & (pH->htsize-1);
  elem = seSQLiteFindElementGivenHash(pH,pKey,nKey,h);
  if( elem ){
    void *old_data = elem->pData;
    if ( nDataOld != 0 )
      *nDataOld = elem->nData;
    if( data==0 ){
      seSQLiteRemoveElementGivenHash(pH,elem,h);
    }else{
      elem->pData = data;
    }
    return old_data;
  }
  if ( nDataOld != 0 )
    *nDataOld = 0;
  if( data==0 ) return 0;
  new_elem = (seSQLiteHashElem*)pH->xMalloc( sizeof(seSQLiteHashElem) );
  if( new_elem==0 ) return data;
  if( pH->copyKey && pKey!=0 ){
    new_elem->pKey = pH->xMalloc( nKey );
    if( new_elem->pKey==0 ){
      pH->xFree(new_elem);
      return data;
    }
    memcpy((void*)new_elem->pKey, pKey, nKey);
  }else{
    new_elem->pKey = (void*)pKey;
  }
  new_elem->nKey = nKey;
  new_elem->nData = nData;
  pH->count++;
  if( pH->htsize==0 ){
	  seSQLiteRehash(pH,8);
    if( pH->htsize==0 ){
      pH->count = 0;
      pH->xFree(new_elem);
      return data;
    }
  }
  if( pH->count > pH->htsize ){
	  seSQLiteRehash(pH,pH->htsize*2);
  }
  assert( pH->htsize>0 );
  assert( (pH->htsize & (pH->htsize-1))==0 );
  h = hraw & (pH->htsize-1);
  seSQLiteInsertElement(pH, &pH->sesqlite_ht[h], new_elem);
  new_elem->pData = data;
  return 0;
}

void seSQLiteBiHashInit(seSQLiteBiHash* bihash, int keytype, int valtype, int copy) {
  bihash->key2val = sesqlite_malloc_and_zero(sizeof(seSQLiteHash));
  bihash->val2key = sesqlite_malloc_and_zero(sizeof(seSQLiteHash));
  seSQLiteHashInit(bihash->key2val, keytype, copy);
  seSQLiteHashInit(bihash->val2key, valtype, copy);
}

void seSQLiteBiHashInsert(seSQLiteBiHash* bihash, const void *pKey, int nKey, const void *pVal, int nVal) {

  // insert new key -> value association
  int nValOld = 0;
  void *pValOld = seSQLiteHashInsert(bihash->key2val, pKey, nKey, (void*) pVal, nVal, &nValOld);

  // remove old value -> key association if exists
  if (pValOld != 0) {
    if (nValOld != 0)
      seSQLiteHashInsert(bihash->val2key, pValOld, nValOld, 0, 0, 0);
    else
      fprintf(stderr, "ERROR: cannot remove old value -> key association in BiHash (no nVal set).\n");
  }

  // insert new value -> key association if provided
  if (pVal != 0) {
    pValOld = seSQLiteHashInsert(bihash->val2key, pVal, nVal, (void*) pKey, nKey, 0);
    if (pValOld != 0)
      fprintf(stderr, "ERROR: value already associated to another key in BiHash (not injective).\n");
  }
}

void *seSQLiteBiHashFind(const seSQLiteBiHash* bihash, const void *pKey, int nKey) {
  return seSQLiteHashFind(bihash->key2val, pKey, nKey);
}

void *seSQLiteBiHashFindKey(const seSQLiteBiHash* bihash, const void *pValue, int nValue) {
  return seSQLiteHashFind(bihash->val2key, pValue, nValue);
}

void seSQLiteBiHashClear(seSQLiteBiHash* bihash) {
  seSQLiteHashClear(bihash->val2key);
  seSQLiteHashClear(bihash->key2val);
}

void seSQLiteBiHashFree(seSQLiteBiHash* bihash) {
  seSQLiteBiHashClear(bihash);
  if (bihash->key2val)
    free(bihash->key2val);
  if (bihash->val2key)
    free(bihash->val2key);
}

