#ifndef __SESQLITE_DYNARRAY_H__
#define __SESQLITE_DYNARRAY_H__

#define SESQLITE_DYNARRAY_INITIAL_SIZE 16

typedef struct sesqlite_dynarray{
	int size;
	int elements;
	int *array;
} sesqlite_dynarray;

void sesqlite_dynarray_init(
	sesqlite_dynarray *d
);

void sesqlite_dynarray_append(
	sesqlite_dynarray *d,
	int element
);

void sesqlite_dynarray_delete(
	sesqlite_dynarray *d
);

void sesqlite_dynarray_clear(
	sesqlite_dynarray *d
);

#endif /*__SESQLITE_DYNARRAY_H__*/
