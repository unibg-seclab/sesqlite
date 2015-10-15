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

