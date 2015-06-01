#include <stdlib.h>
#include "sesqlite_dynarray.h"

void sesqlite_dynarray_init(sesqlite_dynarray *d){
	d->size = DYNARRAY_INITIAL_SIZE;
	d->array = malloc(sizeof(int) * d->size);
	d->elements = 0;
}

void sesqlite_dynarray_append(sesqlite_dynarray *d, int element){
	if( d->elements == d->size ){
		d->size *= 2;
		d->array = realloc(d->array, sizeof(int) * d->size);
	}
	d->array[d->elements++] = element;
}

void sesqlite_dynarray_delete(sesqlite_dynarray *d){
	free(d->array);
}

void sesqlite_dynarray_clear(sesqlite_dynarray *d){
	d->elements = 0;
}

