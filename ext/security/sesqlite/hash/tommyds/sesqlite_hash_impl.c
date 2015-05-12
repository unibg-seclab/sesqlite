#include "sesqlite_hash_impl.h"


/* start of file main.c */


/* start of file tommyhash.c */

/*
 * Copyright (c) 2010, Andrea Mazzoleni. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/******************************************************************************/
/* hash */

tommy_inline tommy_uint32_t tommy_le_uint32_read(const void* ptr)
{
	/* allow unaligned read on Intel x86 and x86_64 platforms */
#if defined(__i386__) || defined(_M_IX86) || defined(_X86_) || defined(__x86_64__) || defined(_M_X64)
	/* defines from http://predef.sourceforge.net/ */
	return *(const tommy_uint32_t*)ptr;
#else
	const unsigned char* ptr8 = tommy_cast(const unsigned char*, ptr);
	return ptr8[0] + ((tommy_uint32_t)ptr8[1] << 8) + ((tommy_uint32_t)ptr8[2] << 16) + ((tommy_uint32_t)ptr8[3] << 24);
#endif
}

#define tommy_rot(x, k) \
	(((x) << (k)) | ((x) >> (32 - (k))))

#define tommy_mix(a, b, c) \
	do { \
		a -= c;  a ^= tommy_rot(c, 4);  c += b; \
		b -= a;  b ^= tommy_rot(a, 6);  a += c; \
		c -= b;  c ^= tommy_rot(b, 8);  b += a; \
		a -= c;  a ^= tommy_rot(c, 16);  c += b; \
		b -= a;  b ^= tommy_rot(a, 19);  a += c; \
		c -= b;  c ^= tommy_rot(b, 4);  b += a; \
	} while (0)

#define tommy_final(a, b, c) \
	do { \
		c ^= b; c -= tommy_rot(b, 14); \
		a ^= c; a -= tommy_rot(c, 11); \
		b ^= a; b -= tommy_rot(a, 25); \
		c ^= b; c -= tommy_rot(b, 16); \
		a ^= c; a -= tommy_rot(c, 4);  \
		b ^= a; b -= tommy_rot(a, 14); \
		c ^= b; c -= tommy_rot(b, 24); \
	} while (0)

tommy_uint32_t tommy_hash_u32(tommy_uint32_t init_val, const void* void_key, tommy_size_t key_len)
{
	const unsigned char* key = tommy_cast(const unsigned char*, void_key);
	tommy_uint32_t a, b, c;

	a = b = c = 0xdeadbeef + ((tommy_uint32_t)key_len) + init_val;

	while (key_len > 12) {
		a += tommy_le_uint32_read(key + 0);
		b += tommy_le_uint32_read(key + 4);
		c += tommy_le_uint32_read(key + 8);

		tommy_mix(a, b, c);

		key_len -= 12;
		key += 12;
	}

	switch (key_len) {
	case 0 :
		return c; /* used only when called with a zero length */
	case 12 :
		c += tommy_le_uint32_read(key + 8);
		b += tommy_le_uint32_read(key + 4);
		a += tommy_le_uint32_read(key + 0);
		break;
	case 11 : c += ((tommy_uint32_t)key[10]) << 16;
	case 10 : c += ((tommy_uint32_t)key[9]) << 8;
	case 9 : c += key[8];
	case 8 :
		b += tommy_le_uint32_read(key + 4);
		a += tommy_le_uint32_read(key + 0);
		break;
	case 7 : b += ((tommy_uint32_t)key[6]) << 16;
	case 6 : b += ((tommy_uint32_t)key[5]) << 8;
	case 5 : b += key[4];
	case 4 :
		a += tommy_le_uint32_read(key + 0);
		break;
	case 3 : a += ((tommy_uint32_t)key[2]) << 16;
	case 2 : a += ((tommy_uint32_t)key[1]) << 8;
	case 1 : a += key[0];
	}

	tommy_final(a, b, c);

	return c;
}

tommy_uint64_t tommy_hash_u64(tommy_uint64_t init_val, const void* void_key, tommy_size_t key_len)
{
	const unsigned char* key = tommy_cast(const unsigned char*, void_key);
	tommy_uint32_t a, b, c;

	a = b = c = 0xdeadbeef + ((tommy_uint32_t)key_len) + (init_val & 0xffffffff);
	c += init_val >> 32;

	while (key_len > 12) {
		a += tommy_le_uint32_read(key + 0);
		b += tommy_le_uint32_read(key + 4);
		c += tommy_le_uint32_read(key + 8);

		tommy_mix(a, b, c);

		key_len -= 12;
		key += 12;
	}

	switch (key_len) {
	case 0 :
		return c + ((tommy_uint64_t)b << 32); /* used only when called with a zero length */
	case 12 :
		c += tommy_le_uint32_read(key + 8);
		b += tommy_le_uint32_read(key + 4);
		a += tommy_le_uint32_read(key + 0);
		break;
	case 11 : c += ((tommy_uint32_t)key[10]) << 16;
	case 10 : c += ((tommy_uint32_t)key[9]) << 8;
	case 9 : c += key[8];
	case 8 :
		b += tommy_le_uint32_read(key + 4);
		a += tommy_le_uint32_read(key + 0);
		break;
	case 7 : b += ((tommy_uint32_t)key[6]) << 16;
	case 6 : b += ((tommy_uint32_t)key[5]) << 8;
	case 5 : b += key[4];
	case 4 :
		a += tommy_le_uint32_read(key + 0);
		break;
	case 3 : a += ((tommy_uint32_t)key[2]) << 16;
	case 2 : a += ((tommy_uint32_t)key[1]) << 8;
	case 1 : a += key[0];
	}

	tommy_final(a, b, c);

	return c + ((tommy_uint64_t)b << 32);
}


/* end of file tommyhash.c */


/* start of file tommyhashlin.c */

/*
 * Copyright (c) 2010, Andrea Mazzoleni. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/* start of file tommylist.c */

/*
 * Copyright (c) 2010, Andrea Mazzoleni. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


void tommy_list_concat(tommy_list* first, tommy_list* second)
{
	tommy_node* first_head;
	tommy_node* first_tail;
	tommy_node* second_head;

	if (tommy_list_empty(second))
		return;

	if (tommy_list_empty(first)) {
		*first = *second;
		return;
	}

	first_head = tommy_list_head(first);
	second_head = tommy_list_head(second);
	first_tail = tommy_list_tail(first);

	/* set the "circular" prev list */
	first_head->prev = second_head->prev;
	second_head->prev = first_tail;

	/* set the "0 terminated" next list */
	first_tail->next = second_head;
}

/** \internal
 * Setup a list.
 */
tommy_inline void tommy_list_set(tommy_list* list, tommy_node* head, tommy_node* tail)
{
	head->prev = tail;
	tail->next = 0;
	*list = head;
}

void tommy_list_sort(tommy_list* list, tommy_compare_func* cmp)
{
	tommy_chain chain;
	tommy_node* head;

	if (tommy_list_empty(list))
		return;

	head = tommy_list_head(list);

	/* create a chain from the list */
	chain.head = head;
	chain.tail = head->prev;

	tommy_chain_mergesort(&chain, cmp);

	/* restore the list */
	tommy_list_set(list, chain.head, chain.tail);
}


/* end of file tommylist.c */


#include <assert.h> /* for assert */

/******************************************************************************/
/* hashlin */

/**
 * Reallocation states.
 */
#define TOMMY_HASHLIN_STATE_STABLE 0
#define TOMMY_HASHLIN_STATE_GROW 1
#define TOMMY_HASHLIN_STATE_SHRINK 2

/**
 * Set the hashtable in stable state.
 */
tommy_inline void tommy_hashlin_stable(tommy_hashlin* hashlin)
{
	hashlin->state = TOMMY_HASHLIN_STATE_STABLE;

	/* setup low_mask/max/split to allow tommy_hashlin_bucket_ref() */
	/* and tommy_hashlin_foreach() to work regardless we are in stable state */
	hashlin->low_max = hashlin->bucket_max;
	hashlin->low_mask = hashlin->bucket_mask;
	hashlin->split = 0;
}

void tommy_hashlin_init(tommy_hashlin* hashlin)
{
	tommy_uint_t i;

	/* fixed initial size */
	hashlin->bucket_bit = TOMMY_HASHLIN_BIT;
	hashlin->bucket_max = 1 << hashlin->bucket_bit;
	hashlin->bucket_mask = hashlin->bucket_max - 1;
	hashlin->bucket[0] = tommy_cast(tommy_hashlin_node**, tommy_calloc(hashlin->bucket_max, sizeof(tommy_hashlin_node*)));
	for (i = 1; i < TOMMY_HASHLIN_BIT; ++i)
		hashlin->bucket[i] = hashlin->bucket[0];

	/* stable state */
	tommy_hashlin_stable(hashlin);

	hashlin->count = 0;
}

void tommy_hashlin_done(tommy_hashlin* hashlin)
{
	tommy_uint_t i;

	tommy_free(hashlin->bucket[0]);
	for (i = TOMMY_HASHLIN_BIT; i < hashlin->bucket_bit; ++i) {
		tommy_hashlin_node** segment = hashlin->bucket[i];
		tommy_free(&segment[((tommy_ptrdiff_t)1) << i]);
	}
}

/**
 * Grow one step.
 */
tommy_inline void hashlin_grow_step(tommy_hashlin* hashlin)
{
	/* grow if more than 50% full */
	if (hashlin->state != TOMMY_HASHLIN_STATE_GROW
		&& hashlin->count > hashlin->bucket_max / 2
	) {
		/* if we are stable, setup a new grow state */
		/* otherwise continue with the already setup shrink one */
		/* but in backward direction */
		if (hashlin->state == TOMMY_HASHLIN_STATE_STABLE) {
			tommy_hashlin_node** segment;

			/* set the lower size */
			hashlin->low_max = hashlin->bucket_max;
			hashlin->low_mask = hashlin->bucket_mask;

			/* allocate the new vector using malloc() and not calloc() */
			/* because data is fully initialized in the split process */
			segment = tommy_cast(tommy_hashlin_node**, tommy_malloc(hashlin->low_max * sizeof(tommy_hashlin_node*)));

			/* store it adjusting the offset */
			/* cast to ptrdiff_t to ensure to get a negative value */
			hashlin->bucket[hashlin->bucket_bit] = &segment[-(tommy_ptrdiff_t)hashlin->low_max];

			/* grow the hash size */
			++hashlin->bucket_bit;
			hashlin->bucket_max = 1 << hashlin->bucket_bit;
			hashlin->bucket_mask = hashlin->bucket_max - 1;

			/* start from the beginning going forward */
			hashlin->split = 0;
		}

		/* grow state */
		hashlin->state = TOMMY_HASHLIN_STATE_GROW;
	}

	/* if we are growing */
	if (hashlin->state == TOMMY_HASHLIN_STATE_GROW) {
		/* compute the split target required to finish the reallocation before the next resize */
		tommy_count_t split_target = 2 * hashlin->count;

		/* reallocate buckets until the split target */
		while (hashlin->split + hashlin->low_max < split_target) {
			tommy_hashlin_node** split[2];
			tommy_hashlin_node* j;
			tommy_count_t mask;

			/* get the low bucket */
			split[0] = tommy_hashlin_pos(hashlin, hashlin->split);

			/* get the high bucket */
			split[1] = tommy_hashlin_pos(hashlin, hashlin->split + hashlin->low_max);

			/* save the low bucket */
			j = *split[0];

			/* reinitialize the buckets */
			*split[0] = 0;
			*split[1] = 0;

			/* the bit used to identify the bucket */
			mask = hashlin->low_max;

			/* flush the bucket */
			while (j) {
				tommy_hashlin_node* j_next = j->next;
				tommy_count_t pos = (j->key & mask) != 0;
				if (*split[pos])
					tommy_list_insert_tail_not_empty(*split[pos], j);
				else
					tommy_list_insert_first(split[pos], j);
				j = j_next;
			}

			/* go forward */
			++hashlin->split;

			/* if we have finished, change the state */
			if (hashlin->split == hashlin->low_max) {
				/* go in stable mode */
				tommy_hashlin_stable(hashlin);
				break;
			}
		}
	}
}

/**
 * Shrink one step.
 */
tommy_inline void hashlin_shrink_step(tommy_hashlin* hashlin)
{
	/* shrink if less than 12.5% full */
	if (hashlin->state != TOMMY_HASHLIN_STATE_SHRINK
		&& hashlin->count < hashlin->bucket_max / 8
	) {
		/* avoid to shrink the first bucket */
		if (hashlin->bucket_bit > TOMMY_HASHLIN_BIT) {
			/* if we are stable, setup a new shrink state */
			/* otherwise continue with the already setup grow one */
			/* but in backward direction */
			if (hashlin->state == TOMMY_HASHLIN_STATE_STABLE) {
				/* set the lower size */
				hashlin->low_max = hashlin->bucket_max / 2;
				hashlin->low_mask = hashlin->bucket_mask / 2;

				/* start from the half going backward */
				hashlin->split = hashlin->low_max;
			}

			/* start reallocation */
			hashlin->state = TOMMY_HASHLIN_STATE_SHRINK;
		}
	}

	/* if we are shrinking */
	if (hashlin->state == TOMMY_HASHLIN_STATE_SHRINK) {
		/* compute the split target required to finish the reallocation before the next resize */
		tommy_count_t split_target = 8 * hashlin->count;

		/* reallocate buckets until the split target */
		while (hashlin->split + hashlin->low_max > split_target) {
			tommy_hashlin_node** split[2];

			/* go backward position */
			--hashlin->split;

			/* get the low bucket */
			split[0] = tommy_hashlin_pos(hashlin, hashlin->split);

			/* get the high bucket */
			split[1] = tommy_hashlin_pos(hashlin, hashlin->split + hashlin->low_max);

			/* concat the high bucket into the low one */
			tommy_list_concat(split[0], split[1]);

			/* if we have finished, clean up and change the state */
			if (hashlin->split == 0) {
				tommy_hashlin_node** segment;

				/* shrink the hash size */
				--hashlin->bucket_bit;
				hashlin->bucket_max = 1 << hashlin->bucket_bit;
				hashlin->bucket_mask = hashlin->bucket_max - 1;

				/* free the last segment */
				segment = hashlin->bucket[hashlin->bucket_bit];
				tommy_free(&segment[((tommy_ptrdiff_t)1) << hashlin->bucket_bit]);

				/* go in stable mode */
				tommy_hashlin_stable(hashlin);
				break;
			}
		}
	}
}

void tommy_hashlin_insert(tommy_hashlin* hashlin, tommy_hashlin_node* node, void* data, tommy_hash_t hash)
{
	tommy_list_insert_tail(tommy_hashlin_bucket_ref(hashlin, hash), node, data);

	node->key = hash;

	++hashlin->count;

	hashlin_grow_step(hashlin);
}

void* tommy_hashlin_remove_existing(tommy_hashlin* hashlin, tommy_hashlin_node* node)
{
	tommy_list_remove_existing(tommy_hashlin_bucket_ref(hashlin, node->key), node);

	--hashlin->count;

	hashlin_shrink_step(hashlin);

	return node->data;
}

void* tommy_hashlin_remove(tommy_hashlin* hashlin, tommy_search_func* cmp, const void* cmp_arg, tommy_hash_t hash)
{
	tommy_hashlin_node** let_ptr = tommy_hashlin_bucket_ref(hashlin, hash);
	tommy_hashlin_node* node = *let_ptr;

	while (node) {
		/* we first check if the hash matches, as in the same bucket we may have multiples hash values */
		if (node->key == hash && cmp(cmp_arg, node->data) == 0) {
			tommy_list_remove_existing(let_ptr, node);

			--hashlin->count;

			hashlin_shrink_step(hashlin);

			return node->data;
		}
		node = node->next;
	}

	return 0;
}

void tommy_hashlin_foreach(tommy_hashlin* hashlin, tommy_foreach_func* func)
{
	tommy_count_t bucket_max;
	tommy_count_t pos;

	/* number of valid buckets */
	bucket_max = hashlin->low_max + hashlin->split;

	for (pos = 0; pos < bucket_max; ++pos) {
		tommy_hashlin_node* node = *tommy_hashlin_pos(hashlin, pos);

		while (node) {
			void* data = node->data;
			node = node->next;
			func(data);
		}
	}
}

void tommy_hashlin_foreach_arg(tommy_hashlin* hashlin, tommy_foreach_arg_func* func, void* arg)
{
	tommy_count_t bucket_max;
	tommy_count_t pos;

	/* number of valid buckets */
	bucket_max = hashlin->low_max + hashlin->split;

	for (pos = 0; pos < bucket_max; ++pos) {
		tommy_hashlin_node* node = *tommy_hashlin_pos(hashlin, pos);

		while (node) {
			void* data = node->data;
			node = node->next;
			func(arg, data);
		}
	}
}

tommy_size_t tommy_hashlin_memory_usage(tommy_hashlin* hashlin)
{
	return hashlin->bucket_max * (tommy_size_t)sizeof(hashlin->bucket[0][0])
	       + hashlin->count * (tommy_size_t)sizeof(tommy_hashlin_node);
}


/* end of file tommyhashlin.c */

#include <stdio.h>

struct object {
	tommy_node node;
	// other fields
	int value;
};

int compare(const void* arg, const void* obj)
{
	return *(const int*)arg != ((const struct object*)obj)->value;
}

int main(){
	tommy_hashlin hashlin;
	tommy_hashlin_init(&hashlin);
	struct object* obj = malloc(sizeof(struct object)); // creates the object
	obj->value = 1; // initializes the object
	tommy_hashlin_insert(&hashlin, &obj->node, obj, tommy_inthash_u32(obj->value)); // inserts the object
	int value_to_find = 1;
	obj = tommy_hashlin_search(&hashlin, compare, &value_to_find, tommy_inthash_u32(value_to_find));
	if (!obj) {
		printf("not found\n");
	} else {
		printf("found\n");
	}
	obj = tommy_hashlin_remove(&hashlin, compare, &value_to_find, tommy_inthash_u32(value_to_find));
	if (obj) {
	 free(obj); // frees the object allocated memory
	}
	tommy_hashlin_done(&hashlin);
}

/* end of file main.c */

