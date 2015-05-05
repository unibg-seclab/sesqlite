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
** This is the header file for the generic hash-table implemenation
** used in SQLite.  We've modified it slightly to serve as a standalone
** hash table implementation for the full-text indexing module.
**
*/
#ifndef _HASH_H_
#define _HASH_H_

/* Forward declarations of structures. */
typedef struct seSQLiteHash seSQLiteHash;
typedef struct seSQLiteBiHash seSQLiteBiHash;
typedef struct seSQLiteHashElem seSQLiteHashElem;

/* A complete hash table is an instance of the following structure.
** The internals of this structure are intended to be opaque -- client
** code should not attempt to access or modify the fields of this structure
** directly.  Change this structure only by using the routines below.
** However, many of the "procedures" and "functions" for modifying and
** accessing this structure are really macros, so we can't really make
** this structure opaque.
*/
struct seSQLiteHash {
  char keyClass;          /* HASH_INT, _POINTER, _STRING, _BINARY */
  char copyKey;           /* True if copy of key made on insert */
  char copyValue;         /* True if copy of values made on insert */
  int count;              /* Number of entries in this table */
  seSQLiteHashElem *first;        /* The first element of the array */
  void *(*xMalloc)(int);  /* malloc() function to use */
  void (*xFree)(void *);  /* free() function to use */
  int htsize;             /* Number of buckets in the hash table */
  struct sesqlite_ht {            /* the hash table */
    int count;               /* Number of entries with this hash */
    seSQLiteHashElem *chain;         /* Pointer to first entry with this hash */
  } *sesqlite_ht;
};

/* Bidirection hash table (it just uses two hash tables */
struct seSQLiteBiHash {
  seSQLiteHash *key2val;
  seSQLiteHash *val2key;
};

/* Each element in the hash table is an instance of the following 
** structure.  All elements are stored on a single doubly-linked list.
**
** Again, this structure is intended to be opaque, but it can't really
** be opaque because it is used by macros.
*/
struct seSQLiteHashElem {
  seSQLiteHashElem *next, *prev;   /* Next and previous elements in the table */
  void *pData; int nData;          /* Data associated with this element */
  void *pKey; int nKey;            /* Key associated with this element */
};

/*
** There are 4 different modes of operation for a hash table:
**
**   HASH_INT         nKey is used as the key and pKey is ignored.
**
**   HASH_POINTER     pKey is used as the key and nKey is ignored.
**
**   HASH_STRING      pKey points to a string that is nKey bytes long
**                           (including the null-terminator, if any).  Case
**                           is respected in comparisons.
**
**   HASH_BINARY      pKey points to binary data nKey bytes long. 
**                           memcmp() is used to compare keys.
**
** A copy of the key is made for HASH_STRING and HASH_BINARY
** if the copyKey parameter to HashInit is 1.  
**
** A copy of the value is made if copyValue parameter to HashInit is 1.
*/
#define SESQLITE_HASH_INT       1
/* #define SESQLITE_HASH_POINTER   2 NOT USED */
#define SESQLITE_HASH_STRING    3
#define SESQLITE_HASH_BINARY    4

/*
** Access routines.  To delete, insert a NULL pointer.
*/
void seSQLiteHashInit(seSQLiteHash*, int keytype, int copyKey, int copyValue);
void seSQLiteHashInsert(seSQLiteHash*, const void *pKey, int nKey, void *pData, int nData);
void seSQLiteHashFind(const seSQLiteHash*, const void *pKey, int nKey, void **pRes, int *nRes);
void seSQLiteHashClear(seSQLiteHash*);

/*
** Access routines for bidirectional hash tables.
*/
void seSQLiteBiHashInit(seSQLiteBiHash*, int keytype, int valtype, int copyKey, int copyValue);
void seSQLiteBiHashInsert(seSQLiteBiHash*, const void *pKey, int nKey, const void *pVal, int nVal);
void seSQLiteBiHashFind(const seSQLiteBiHash*, const void *pKey, int nKey, void **pRes, int *nRes);
void seSQLiteBiHashFindKey(const seSQLiteBiHash*, const void *pVal, int nVal, void **pRes, int *nRes);
void seSQLiteBiHashClear(seSQLiteBiHash*);
void seSQLiteBiHashFree(seSQLiteBiHash*);

/*
** Macros for looping over all elements of a hash table.  The idiom is
** like this:
**
**   Hash h;
**   HashElem *p;
**   ...
**   for(p=HashFirst(&h); p; p=HashNext(p)){
**     SomeStructure *pData = HashData(p);
**     // do something with pData
**   }
*/
#define seSQLiteHashFirst(H)  ((H)->first)
#define seSQLiteHashNext(E)   ((E)->next)
#define seSQLiteHashData(E)   ((E)->data)
#define seSQLiteHashKey(E)    ((E)->pKey)
#define seSQLiteHashKeysize(E) ((E)->nKey)

/*
** Number of entries in a hash table
*/
#define seSQLiteHashCount(H)  ((H)->count)

#endif /* _HASH_H_ */
