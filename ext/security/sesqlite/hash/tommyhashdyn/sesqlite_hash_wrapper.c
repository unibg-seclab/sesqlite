#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX)

#include "sesqlite_hash_wrapper.h"
#include "sesqlite_hash_impl.h"
#include <stdlib.h>
#include <string.h>

#define HASHCODE(P, N) ((P==NULL) ? tommy_inthash_u32(N) : tommy_hash_u32(0, P, N))

#define OPT_COPYKEY   0x01
#define OPT_COPYVALUE 0x02

static void freeElement(
	void* arg,
	void* obj
){
	tommyHash *th = (tommyHash*) arg;
	tommyElement *elem = (tommyElement*) obj;
	tommy_hashdyn_remove_existing(th->hash, &elem->node);

	if( (elem->key.p!=NULL) && (th->options & OPT_COPYKEY) )
		free(elem->key.p);
	if( (elem->value.p!=NULL) && (th->options & OPT_COPYVALUE) )
		free(elem->value.p);

	free(obj);
}

static int compareElement(
	const void *arg,
	const void *obj
){
	tommyPointer *tp = (tommyPointer*) arg;
	tommyElement *elem = (tommyElement*) obj;
	if( tp->n!=elem->key.n ) return -1;
	if( tp->p==NULL && elem->key.p==NULL) return 0;
	return memcmp(tp->p, elem->key.p, tp->n);
}

void tommyHashInit(
	tommyHash* th,
	int copyKey,
	int copyValue
){
	th->options = 0;
	if( copyKey!=0 )
		th->options = th->options | OPT_COPYKEY;
	if( copyValue!=0 )
		th->options = th->options | OPT_COPYVALUE;

	th->hash = malloc(sizeof(tommy_hashdyn));
	tommy_hashdyn_init(th->hash);
}

void tommyHashInsert(
	tommyHash* th,
	const void* pKey,
	int nKey,
	void* pValue,
	int nValue
){
	if( nKey<0  && pKey!=0 ){
		nKey = sizeof(char) * (1 + strlen(pKey));
	}

	if( nValue<0 && pValue!=0 ){
		nValue = sizeof(char) * (1 + strlen(pValue));
	}

	tommyHashRemove(th, pKey, nKey);

	tommyElement *obj = malloc(sizeof(tommyElement));
	obj->key.n = nKey;
	obj->value.n = nValue;

	if( (pKey!=NULL) && (th->options & OPT_COPYKEY) ){
		obj->key.p = malloc(nKey);
		memcpy(obj->key.p, (void*) pKey, nKey);
	}else{
		obj->key.p = (void*) pKey;
	}

	if( (pValue!=NULL) && (th->options & OPT_COPYVALUE) ){
		obj->value.p = malloc(nValue);
		memcpy(obj->value.p, pValue, nValue);
	}else{
		obj->value.p = pValue;
	}

	tommy_hashdyn_insert(th->hash, &obj->node, obj, HASHCODE(pKey, nKey));
}

void tommyHashFind(
	const tommyHash* th,
	const void* pKey,
	int nKey,
	void** pRes,
	int* nRes
){
	if( nKey<0 && pKey!=0 ){
		nKey = sizeof(char) * (1 + strlen(pKey));
	}

	tommyPointer tp = { .n = nKey, .p = (void*) pKey };
	tommyElement *obj = tommy_hashdyn_search(th->hash, compareElement, &tp, HASHCODE(pKey, nKey));

	if( pRes ){ *pRes = obj ? obj->value.p : 0; }
	if( nRes ){ *nRes = obj ? obj->value.n : 0; }
}

void tommyHashRemove(
	const tommyHash* th,
	const void *pKey,
	int nKey
){
	tommyPointer tp = { .n = nKey, .p = (void*) pKey };
	tommyElement *obj = tommy_hashdyn_search(th->hash, compareElement, &tp, HASHCODE(pKey, nKey));

	if( obj!=NULL )
		freeElement((void*) th, obj);
}

void tommyHashClear(
	tommyHash* th
){
	tommy_hashdyn_foreach_arg(th->hash, freeElement, th);
}

void tommyHashFree(
	tommyHash* th
){
	tommyHashClear(th);
	tommy_hashdyn_done(th->hash);
	free(th->hash);
}

#endif /* !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_SELINUX) */

