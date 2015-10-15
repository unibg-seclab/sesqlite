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

#include "sesqlite.h"

#define NELEMS(x)  (sizeof(x) / sizeof(x[0]))
#define  SOURCE_MASK   0xfff00000
#define  TARGET_MASK   0xfff00
#define  CLASS_MASK    0xe0
#define  PERM_MASK     0xf1

#define  SOURCE_SHIFT  20
#define  TARGET_SHIFT  8
#define  CLASS_SHIFT   5
#define  PERM_SHIFT    0

/* Used for internal representation of security contexts*/
unsigned int compress(
	unsigned short source,
	unsigned short target,
	unsigned char tclass,
	unsigned char perm
){
	return ( perm   << PERM_SHIFT   )
	     + ( tclass  << CLASS_SHIFT  )
	     + ( target << TARGET_SHIFT )
	     + ( source << SOURCE_SHIFT );
}

void decompress(
	unsigned int data,
	unsigned short* source,
	unsigned short* target,
	unsigned char* tclass,
	unsigned char* perm
){
	*source = ( data & SOURCE_MASK ) >> SOURCE_SHIFT;
	*target = ( data & TARGET_MASK ) >> TARGET_SHIFT;
	*tclass  = ( data & CLASS_MASK  ) >> CLASS_SHIFT;
	*perm   = ( data & PERM_MASK   ) >> PERM_SHIFT;
}

