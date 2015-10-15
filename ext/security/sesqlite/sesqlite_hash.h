/*
** Authors: Simone Mutti <simone.mutti@unibg.it>
**          Enrico Bacis <enrico.bacis@unibg.it>
**
** Copyright 2015, Università degli Studi di Bergamo
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

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

