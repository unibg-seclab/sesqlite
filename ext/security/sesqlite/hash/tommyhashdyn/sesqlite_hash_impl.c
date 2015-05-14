#include "sesqlite_hash_impl.h"


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


/* start of file tommyhashdyn.c */

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


/******************************************************************************/
/* hashdyn */

void tommy_hashdyn_init(tommy_hashdyn* hashdyn)
{
	/* fixed initial size */
	hashdyn->bucket_bit = TOMMY_HASHDYN_BIT;
	hashdyn->bucket_max = 1 << hashdyn->bucket_bit;
	hashdyn->bucket_mask = hashdyn->bucket_max - 1;
	hashdyn->bucket = tommy_cast(tommy_hashdyn_node**, tommy_calloc(hashdyn->bucket_max, sizeof(tommy_hashdyn_node*)));

	hashdyn->count = 0;
}

void tommy_hashdyn_done(tommy_hashdyn* hashdyn)
{
	tommy_free(hashdyn->bucket);
}

/**
 * Resize the bucket vector.
 */
static void tommy_hashdyn_resize(tommy_hashdyn* hashdyn, tommy_count_t new_bucket_bit)
{
	tommy_count_t bucket_bit;
	tommy_count_t bucket_max;
	tommy_count_t new_bucket_max;
	tommy_count_t new_bucket_mask;
	tommy_hashdyn_node** new_bucket;

	bucket_bit = hashdyn->bucket_bit;
	bucket_max = hashdyn->bucket_max;

	new_bucket_max = 1 << new_bucket_bit;
	new_bucket_mask = new_bucket_max - 1;

	/* allocate the new vector using malloc() and not calloc() */
	/* because data is fully initialized in the update process */
	new_bucket = tommy_cast(tommy_hashdyn_node**, tommy_malloc(new_bucket_max * sizeof(tommy_hashdyn_node*)));

	/* reinsert all the elements */
	if (new_bucket_bit > bucket_bit) {
		tommy_count_t i;

		/* grow */
		for (i = 0; i < bucket_max; ++i) {
			tommy_hashdyn_node* j;

			/* setup the new two buckets */
			new_bucket[i] = 0;
			new_bucket[i + bucket_max] = 0;

			/* reinsert the bucket */
			j = hashdyn->bucket[i];
			while (j) {
				tommy_hashdyn_node* j_next = j->next;
				tommy_count_t pos = j->key & new_bucket_mask;
				if (new_bucket[pos])
					tommy_list_insert_tail_not_empty(new_bucket[pos], j);
				else
					tommy_list_insert_first(&new_bucket[pos], j);
				j = j_next;
			}
		}
	} else {
		tommy_count_t i;

		/* shrink */
		for (i = 0; i < new_bucket_max; ++i) {
			/* setup the new bucket with the lower bucket*/
			new_bucket[i] = hashdyn->bucket[i];

			/* concat the upper bucket */
			tommy_list_concat(&new_bucket[i], &hashdyn->bucket[i + new_bucket_max]);
		}
	}

	tommy_free(hashdyn->bucket);

	/* setup */
	hashdyn->bucket_bit = new_bucket_bit;
	hashdyn->bucket_max = new_bucket_max;
	hashdyn->bucket_mask = new_bucket_mask;
	hashdyn->bucket = new_bucket;
}

/**
 * Grow.
 */
tommy_inline void hashdyn_grow_step(tommy_hashdyn* hashdyn)
{
	/* grow if more than 50% full */
	if (hashdyn->count >= hashdyn->bucket_max / 2)
		tommy_hashdyn_resize(hashdyn, hashdyn->bucket_bit + 1);
}

/**
 * Shrink.
 */
tommy_inline void hashdyn_shrink_step(tommy_hashdyn* hashdyn)
{
	/* shrink if less than 12.5% full */
	if (hashdyn->count <= hashdyn->bucket_max / 8 && hashdyn->bucket_bit > TOMMY_HASHDYN_BIT)
		tommy_hashdyn_resize(hashdyn, hashdyn->bucket_bit - 1);
}

void tommy_hashdyn_insert(tommy_hashdyn* hashdyn, tommy_hashdyn_node* node, void* data, tommy_hash_t hash)
{
	tommy_count_t pos = hash & hashdyn->bucket_mask;

	tommy_list_insert_tail(&hashdyn->bucket[pos], node, data);

	node->key = hash;

	++hashdyn->count;

	hashdyn_grow_step(hashdyn);
}

void* tommy_hashdyn_remove_existing(tommy_hashdyn* hashdyn, tommy_hashdyn_node* node)
{
	tommy_count_t pos = node->key & hashdyn->bucket_mask;

	tommy_list_remove_existing(&hashdyn->bucket[pos], node);

	--hashdyn->count;

	hashdyn_shrink_step(hashdyn);

	return node->data;
}

void* tommy_hashdyn_remove(tommy_hashdyn* hashdyn, tommy_search_func* cmp, const void* cmp_arg, tommy_hash_t hash)
{
	tommy_count_t pos = hash & hashdyn->bucket_mask;
	tommy_hashdyn_node* node = hashdyn->bucket[pos];

	while (node) {
		/* we first check if the hash matches, as in the same bucket we may have multiples hash values */
		if (node->key == hash && cmp(cmp_arg, node->data) == 0) {
			tommy_list_remove_existing(&hashdyn->bucket[pos], node);

			--hashdyn->count;

			hashdyn_shrink_step(hashdyn);

			return node->data;
		}
		node = node->next;
	}

	return 0;
}

void tommy_hashdyn_foreach(tommy_hashdyn* hashdyn, tommy_foreach_func* func)
{
	tommy_count_t bucket_max = hashdyn->bucket_max;
	tommy_hashdyn_node** bucket = hashdyn->bucket;
	tommy_count_t pos;

	for (pos = 0; pos < bucket_max; ++pos) {
		tommy_hashdyn_node* node = bucket[pos];

		while (node) {
			void* data = node->data;
			node = node->next;
			func(data);
		}
	}
}

void tommy_hashdyn_foreach_arg(tommy_hashdyn* hashdyn, tommy_foreach_arg_func* func, void* arg)
{
	tommy_count_t bucket_max = hashdyn->bucket_max;
	tommy_hashdyn_node** bucket = hashdyn->bucket;
	tommy_count_t pos;

	for (pos = 0; pos < bucket_max; ++pos) {
		tommy_hashdyn_node* node = bucket[pos];

		while (node) {
			void* data = node->data;
			node = node->next;
			func(arg, data);
		}
	}
}

tommy_size_t tommy_hashdyn_memory_usage(tommy_hashdyn* hashdyn)
{
	return hashdyn->bucket_max * (tommy_size_t)sizeof(hashdyn->bucket[0])
	       + tommy_hashdyn_count(hashdyn) * (tommy_size_t)sizeof(tommy_hashdyn_node);
}


/* end of file tommyhashdyn.c */

