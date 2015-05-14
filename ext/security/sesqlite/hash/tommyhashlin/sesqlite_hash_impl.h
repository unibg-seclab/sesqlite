#ifndef _SESQLITE_HASH_IMPL_H_
#define _SESQLITE_HASH_IMPL_H_

/* start of file tommyhashlin.h */

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

/** \file
 * Linear chained hashtable.
 *
 * This hashtable resizes dynamically and progressively using a variation of the
 * linear hashing algorithm described in http://en.wikipedia.org/wiki/Linear_hashing
 *
 * It starts with the minimal size of 16 buckets, it doubles the size then it
 * reaches a load factor greater than 0.5 and it halves the size with a load
 * factor lower than 0.125.
 *
 * The progressive resize is good for real-time and interactive applications
 * as it makes insert and delete operations taking always the same time.
 *
 * For resizing it's used a dynamic array that supports access to not contigous
 * segments.
 * In this way we only allocate additional table segments on the heap, without
 * freeing the previous table, and then not increasing the heap fragmentation.
 *
 * The resize takes place inside tommy_hashlin_insert() and tommy_hashlin_remove().
 * No resize is done in the tommy_hashlin_search() operation.
 *
 * To initialize the hashtable you have to call tommy_hashlin_init().
 *
 * \code
 * tommy_hashslin hashlin;
 *
 * tommy_hashlin_init(&hashlin);
 * \endcode
 *
 * To insert elements in the hashtable you have to call tommy_hashlin_insert() for
 * each element.
 * In the insertion call you have to specify the address of the node, the
 * address of the object, and the hash value of the key to use.
 * The address of the object is used to initialize the tommy_node::data field
 * of the node, and the hash to initialize the tommy_node::key field.
 *
 * \code
 * struct object {
 *     tommy_node node;
 *     // other fields
 *     int value;
 * };
 *
 * struct object* obj = malloc(sizeof(struct object)); // creates the object
 *
 * obj->value = ...; // initializes the object
 *
 * tommy_hashlin_insert(&hashlin, &obj->node, obj, tommy_inthash_u32(obj->value)); // inserts the object
 * \endcode
 *
 * To find and element in the hashtable you have to call tommy_hashtable_search()
 * providing a comparison function, its argument, and the hash of the key to search.
 *
 * \code
 * int compare(const void* arg, const void* obj)
 * {
 *     return *(const int*)arg != ((const struct object*)obj)->value;
 * }
 *
 * int value_to_find = 1;
 * struct object* obj = tommy_hashlin_search(&hashlin, compare, &value_to_find, tommy_inthash_u32(value_to_find));
 * if (!obj) {
 *     // not found
 * } else {
 *     // found
 * }
 * \endcode
 *
 * To iterate over all the elements in the hashtable with the same key, you have to
 * use tommy_hashlin_bucket() and follow the tommy_node::next pointer until NULL.
 * You have also to check explicitely for the key, as the bucket may contains
 * different keys.
 *
 * \code
 * int value_to_find = 1;
 * tommy_node* i = tommy_hashlin_bucket(&hashlin, tommy_inthash_u32(value_to_find));
 * while (i) {
 *     struct object* obj = i->data; // gets the object pointer
 *
 *     if (obj->value == value_to_find) {
 *         printf("%d\n", obj->value); // process the object
 *     }
 *
 *     i = i->next; // goes to the next element
 * }
 * \endcode
 *
 * To remove an element from the hashtable you have to call tommy_hashlin_remove()
 * providing a comparison function, its argument, and the hash of the key to search
 * and remove.
 *
 * \code
 * struct object* obj = tommy_hashlin_remove(&hashlin, compare, &value_to_remove, tommy_inthash_u32(value_to_remove));
 * if (obj) {
 *     free(obj); // frees the object allocated memory
 * }
 * \endcode
 *
 * To destroy the hashtable you have to remove all the elements, and deinitialize
 * the hashtable calling tommy_hashlin_done().
 *
 * \code
 * tommy_hashlin_done(&hashlin);
 * \endcode
 *
 * If you need to iterate over all the elements in the hashtable, you can use
 * tommy_hashlin_foreach() or tommy_hashlin_foreach_arg().
 * If you need a more precise control with a real iteration, you have to insert
 * all the elements also in a ::tommy_list, and use the list to iterate.
 * See the \ref multiindex example for more detail.
 */

#ifndef __TOMMYHASHLIN_H
#define __TOMMYHASHLIN_H


/* start of file tommyhash.h */

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

/** \file
 * Hash functions for the use with ::tommy_hashtable, ::tommy_hashdyn and ::tommy_hashlin.
 */

#ifndef __TOMMYHASH_H
#define __TOMMYHASH_H


/* start of file tommytypes.h */

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

/** \file
 * Generic types.
 */

#ifndef __TOMMYTYPES_H
#define __TOMMYTYPES_H

/******************************************************************************/
/* types */

#include <stddef.h>

#if defined(_MSC_VER)
typedef unsigned tommy_uint32_t; /**< Generic uint32_t type. */
typedef unsigned _int64 tommy_uint64_t; /**< Generic uint64_t type. */
typedef size_t tommy_uintptr_t; /**< Generic uintptr_t type. */
#else
#include <stdint.h>
typedef uint32_t tommy_uint32_t; /**< Generic uint32_t type. */
typedef uint64_t tommy_uint64_t; /**< Generic uint64_t type. */
typedef uintptr_t tommy_uintptr_t; /**< Generic uintptr_t type. */
#endif
typedef size_t tommy_size_t; /**< Generic size_t type. */
typedef ptrdiff_t tommy_ptrdiff_t; /**< Generic ptrdiff_t type. */
typedef int tommy_bool_t; /**< Generic boolean type. */

/**
 * Generic unsigned integer type.
 *
 * It has no specific size, as is used to store only small values.
 * To make the code more efficient, a full 32 bit integer is used.
 */
typedef tommy_uint32_t tommy_uint_t;

/**
 * Generic unsigned integer for counting objects.
 *
 * TommyDS doesn't support more than 2^32-1 objects.
 */
typedef tommy_uint32_t tommy_count_t;

/** \internal
 * Type cast required for the C++ compilation.
 * When compiling in C++ we cannot convert a void* pointer to another pointer.
 * In such case we need an explicit cast.
 */
#ifdef __cplusplus
#define tommy_cast(type, value) static_cast<type>(value)
#else
#define tommy_cast(type, value) (value)
#endif

/******************************************************************************/
/* heap */

/* by default uses malloc/calloc/realloc/free */

/**
 * Generic malloc(), calloc(), realloc() and free() functions.
 * Redefine them to what you need. By default they map to the C malloc(), calloc(), realloc() and free().
 */
#if !defined(tommy_malloc) || !defined(tommy_calloc) || !defined(tommy_realloc) || !defined(tommy_free)
#include <stdlib.h>
#endif
#if !defined(tommy_malloc)
#define tommy_malloc malloc
#endif
#if !defined(tommy_calloc)
#define tommy_calloc calloc
#endif
#if !defined(tommy_realloc)
#define tommy_realloc realloc
#endif
#if !defined(tommy_free)
#define tommy_free free
#endif

/******************************************************************************/
/* modificators */

/** \internal
 * Definition of the inline keyword if available.
 */
#if !defined(tommy_inline)
#if defined(_MSC_VER) || defined(__GNUC__)
#define tommy_inline static __inline
#else
#define tommy_inline static
#endif
#endif

/** \internal
 * Definition of the restrict keyword if available.
 */
#if !defined(tommy_restrict)
#if __STDC_VERSION__ >= 199901L
#define tommy_restrict restrict
#elif defined(_MSC_VER) || defined(__GNUC__)
#define tommy_restrict __restrict
#else
#define tommy_restrict
#endif
#endif

/** \internal
 * Hints the compiler that a condition is likely true.
 */
#if !defined(tommy_likely)
#if defined(__GNUC__)
#define tommy_likely(x) __builtin_expect(!!(x), 1)
#else
#define tommy_likely(x) (x)
#endif
#endif

/** \internal
 * Hints the compiler that a condition is likely false.
 */
#if !defined(tommy_unlikely)
#if defined(__GNUC__)
#define tommy_unlikely(x) __builtin_expect(!!(x), 0)
#else
#define tommy_unlikely(x) (x)
#endif
#endif

/******************************************************************************/
/* key */

/**
 * Key type used in indexed data structures to store the key or the hash value.
 */
typedef tommy_uint32_t tommy_key_t;

/**
 * Bits into the ::tommy_key_t type.
 */
#define TOMMY_KEY_BIT (sizeof(tommy_key_t) * 8)

/******************************************************************************/
/* node */

/**
 * Data structure node.
 * This node type is shared between all the data structures and used to store some
 * info directly into the objects you want to store.
 *
 * A typical declaration is:
 * \code
 * struct object {
 *     tommy_node node;
 *     // other fields
 * };
 * \endcode
 */
typedef struct tommy_node_struct {
	/**
	 * Next node.
	 * The tail node has it at 0, like a 0 terminated list.
	 */
	struct tommy_node_struct* next;

	/**
	 * Previous node.
	 * The head node points to the tail node, like a circular list.
	 */
	struct tommy_node_struct* prev;

	/**
	 * Pointer at the object containing the node.
	 * This field is initialized when inserting nodes into a data structure.
	 */
	void* data;

	/**
	 * Key used to store the node.
	 * With hashtables this field is used to store the hash value.
	 * With lists this field is not used.
	 */
	tommy_key_t key;
} tommy_node;

/******************************************************************************/
/* compare */

/**
 * Compare function for elements.
 * \param obj_a Pointer at the first object to compare.
 * \param obj_b Pointer at the second object to compare.
 * \return <0 if the first element is less than the second, ==0 equal, >0 if greather.
 *
 * This function is like the C strcmp().
 *
 * \code
 * struct object {
 *     tommy_node node;
 *     int value;
 * };
 *
 * int compare(const void* obj_a, const void* obj_b)
 * {
 *     if (((const struct object*)obj_a)->value < ((const struct object*)obj_b)->value)
 *         return -1;
 *     if (((const struct object*)obj_a)->value > ((const struct object*)obj_b)->value)
 *         return 1;
 *     return 0;
 * }
 *
 * tommy_list_sort(&list, compare);
 * \endcode
 *
 */
typedef int tommy_compare_func(const void* obj_a, const void* obj_b);

/**
 * Search function for elements.
 * \param arg Pointer at the value to search.
 * \param obj Pointer at the object to compare to.
 * \return ==0 if the value matches the element. !=0 if different.
 *
 * Note that the first argument is a pointer to the value to search and
 * the second one is a pointer to the object to compare.
 * They are pointers of two different types.
 *
 * \code
 * struct object {
 *     tommy_node node;
 *     int value;
 * };
 *
 * int compare(const void* arg, const void* obj)
 * {
 *     return *(const int*)arg != ((const struct object*)obj)->value;
 * }
 *
 * int value_to_find = 1;
 * struct object* obj = tommy_hashtable_search(&hashtable, compare, &value_to_find, tommy_inthash_u32(value_to_find));
 * if (!obj) {
 *     // not found
 * } else {
 *     // found
 * }
 * \endcode
 *
 */
typedef int tommy_search_func(const void* arg, const void* obj);

/**
 * Foreach function.
 * \param obj Pointer at the object to iterate.
 *
 * A typical example is to use free() to deallocate all the objects in a list.
 * \code
 * tommy_list_foreach(&list, (tommy_foreach_func*)free);
 * \endcode
 */
typedef void tommy_foreach_func(void* obj);

/**
 * Foreach function with an argument.
 * \param arg Pointer at a generic argument.
 * \param obj Pointer at the object to iterate.
 */
typedef void tommy_foreach_arg_func(void* arg, void* obj);

/******************************************************************************/
/* bit hacks */

#if defined(_MSC_VER) && !defined(__cplusplus)
#include <intrin.h>
#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanForward)
#endif

/** \internal
 * Integer log2 for constants.
 * You can use it only for exact power of 2 up to 256.
 */
#define TOMMY_ILOG2(value) ((value) == 256 ? 8 : (value) == 128 ? 7 : (value) == 64 ? 6 : (value) == 32 ? 5 : (value) == 16 ? 4 : (value) == 8 ? 3 : (value) == 4 ? 2 : (value) == 2 ? 1 : 0)

/**
 * Bit scan reverse or integer log2.
 * Return the bit index of the most significant 1 bit.
 *
 * If no bit is set, the result is undefined.
 * To force a return 0 in this case, you can use tommy_ilog2_u32(value | 1).
 *
 * Other interesting ways for bitscan are at:
 *
 * Bit Twiddling Hacks
 * http://graphics.stanford.edu/~seander/bithacks.html
 *
 * Chess Programming BitScan
 * http://chessprogramming.wikispaces.com/BitScan
 *
 * \param value Value to scan. 0 is not allowed.
 * \return The index of the most significant bit set.
 */
tommy_inline tommy_uint_t tommy_ilog2_u32(tommy_uint32_t value)
{
#if defined(_MSC_VER)
	unsigned long count;
	_BitScanReverse(&count, value);
	return count;
#elif defined(__GNUC__)
	/*
	 * GCC implements __builtin_clz(x) as "__builtin_clz(x) = bsr(x) ^ 31"
	 *
	 * Where "x ^ 31 = 31 - x", but gcc does not optimize "31 - __builtin_clz(x)" to bsr(x),
	 * but generates 31 - (bsr(x) xor 31).
	 *
	 * So we write "__builtin_clz(x) ^ 31" instead of "31 - __builtin_clz(x)",
	 * to allow the double xor to be optimized out.
	 */
	return __builtin_clz(value) ^ 31;
#else
	/* Find the log base 2 of an N-bit integer in O(lg(N)) operations with multiply and lookup */
	/* from http://graphics.stanford.edu/~seander/bithacks.html */
	static unsigned char TOMMY_DE_BRUIJN_INDEX_ILOG2[32] = {
		0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
		8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
	};

	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;

	return TOMMY_DE_BRUIJN_INDEX_ILOG2[(tommy_uint32_t)(value * 0x07C4ACDDU) >> 27];
#endif
}

/**
 * Bit scan forward or trailing zero count.
 * Return the bit index of the least significant 1 bit.
 *
 * If no bit is set, the result is undefined.
 * \param value Value to scan. 0 is not allowed.
 * \return The index of the least significant bit set.
 */
tommy_inline tommy_uint_t tommy_ctz_u32(tommy_uint32_t value)
{
#if defined(_MSC_VER)
	unsigned long count;
	_BitScanForward(&count, value);
	return count;
#elif defined(__GNUC__)
	return __builtin_ctz(value);
#else
	/* Count the consecutive zero bits (trailing) on the right with multiply and lookup */
	/* from http://graphics.stanford.edu/~seander/bithacks.html */
	static const unsigned char TOMMY_DE_BRUIJN_INDEX_CTZ[32] = {
		0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
		31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
	};

	return TOMMY_DE_BRUIJN_INDEX_CTZ[(tommy_uint32_t)(((value & - value) * 0x077CB531U)) >> 27];
#endif
}

/**
 * Rounds up to the next power of 2.
 * For the value 0, the result is undefined.
 * \return The smallest power of 2 not less than the specified value.
 */
tommy_inline tommy_uint32_t tommy_roundup_pow2_u32(tommy_uint32_t value)
{
	/* Round up to the next highest power of 2 */
	/* from http://www-graphics.stanford.edu/~seander/bithacks.html */

	--value;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	++value;

	return value;
}
#endif


/* end of file tommytypes.h */


/******************************************************************************/
/* hash */

/**
 * Hash type used in hashtables.
 */
typedef tommy_key_t tommy_hash_t;

/**
 * Hash function with a 32 bits result.
 * Implementation of the Robert Jenkins "lookup3" hash 32 bits version,
 * from http://www.burtleburtle.net/bob/hash/doobs.html, function hashlittle().
 * \param init_val Initialization value.
 * Using a different initialization value, you can generate a completely different set of hash values.
 * Use 0 if not relevant.
 * \param void_key Pointer at the data to hash.
 * \param key_len Size of the data to hash.
 * \note
 * This function is endianess independent.
 * \return The hash value of 32 bits.
 */
tommy_uint32_t tommy_hash_u32(tommy_uint32_t init_val, const void* void_key, tommy_size_t key_len);

/**
 * Hash function with a 64 bits result.
 * Implementation of the Robert Jenkins "lookup3" hash 64 bits versions,
 * from http://www.burtleburtle.net/bob/hash/doobs.html, function hashlittle2().
 * \param init_val Initialization value.
 * Using a different initialization value, you can generate a completely different set of hash values.
 * Use 0 if not relevant.
 * \param void_key Pointer at the data to hash.
 * \param key_len Size of the data to hash.
 * \note
 * This function is endianess independent.
 * \return The hash value of 64 bits.
 */
tommy_uint64_t tommy_hash_u64(tommy_uint64_t init_val, const void* void_key, tommy_size_t key_len);

/**
 * Integer hash of 32 bits.
 * Implementation of the Robert Jenkins "4-byte Integer Hashing",
 * from http://burtleburtle.net/bob/hash/integer.html
 */
tommy_inline tommy_uint32_t tommy_inthash_u32(tommy_uint32_t key)
{
	key -= key << 6;
	key ^= key >> 17;
	key -= key << 9;
	key ^= key << 4;
	key -= key << 3;
	key ^= key << 10;
	key ^= key >> 15;

	return key;
}

/**
 * Integer hash of 64 bits.
 * Implementation of the Thomas Wang "Integer Hash Function",
 * from http://www.cris.com/~Ttwang/tech/inthash.htm
 */
tommy_inline tommy_uint64_t tommy_inthash_u64(tommy_uint64_t key)
{
	key = ~key + (key << 21);
	key = key ^ (key >> 24);
	key = key + (key << 3) + (key << 8);
	key = key ^ (key >> 14);
	key = key + (key << 2) + (key << 4);
	key = key ^ (key >> 28);
	key = key + (key << 31);

	return key;
}

#endif


/* end of file tommyhash.h */


/* tommyhash.h already was included */


/******************************************************************************/
/* hashlin */

/** \internal
 * Initial and minimal size of the hashtable expressed as a power of 2.
 * The initial size is 2^TOMMY_HASHLIN_BIT.
 */
#define TOMMY_HASHLIN_BIT 6

/**
 * Hashtable node.
 * This is the node that you have to include inside your objects.
 */
typedef tommy_node tommy_hashlin_node;

/** \internal
 * Max number of elements as a power of 2.
 */
#define TOMMY_HASHLIN_BIT_MAX 32

/**
 * Hashtable container type.
 * \note Don't use internal fields directly, but access the container only using functions.
 */
typedef struct tommy_hashlin_struct {
	tommy_hashlin_node** bucket[TOMMY_HASHLIN_BIT_MAX]; /**< Dynamic array of hash buckets. One list for each hash modulus. */
	tommy_uint_t bucket_bit; /**< Bits used in the bit mask. */
	tommy_count_t bucket_max; /**< Number of buckets. */
	tommy_count_t bucket_mask; /**< Bit mask to access the buckets. */
	tommy_count_t low_max; /**< Low order max value. */
	tommy_count_t low_mask; /**< Low order mask value. */
	tommy_count_t split; /**< Split position. */
	tommy_count_t count; /**< Number of elements. */
	tommy_uint_t state; /**< Reallocation state. */
} tommy_hashlin;

/**
 * Initializes the hashtable.
 */
void tommy_hashlin_init(tommy_hashlin* hashlin);

/**
 * Deinitializes the hashtable.
 *
 * You can call this function with elements still contained,
 * but such elements are not going to be freed by this call.
 */
void tommy_hashlin_done(tommy_hashlin* hashlin);

/**
 * Inserts an element in the hashtable.
 */
void tommy_hashlin_insert(tommy_hashlin* hashlin, tommy_hashlin_node* node, void* data, tommy_hash_t hash);

/**
 * Searches and removes an element from the hashtable.
 * You have to provide a compare function and the hash of the element you want to remove.
 * If the element is not found, 0 is returned.
 * If more equal elements are present, the first one is removed.
 * \param cmp Compare function called with cmp_arg as first argument and with the element to compare as a second one.
 * The function should return 0 for equal elements, anything other for different elements.
 * \param cmp_arg Compare argument passed as first argument of the compare function.
 * \param hash Hash of the element to find and remove.
 * \return The removed element, or 0 if not found.
 */
void* tommy_hashlin_remove(tommy_hashlin* hashlin, tommy_search_func* cmp, const void* cmp_arg, tommy_hash_t hash);

/** \internal
 * Returns the bucket at the specified position.
 */
tommy_inline tommy_hashlin_node** tommy_hashlin_pos(tommy_hashlin* hashlin, tommy_hash_t pos)
{
	tommy_uint_t bsr;

	/* get the highest bit set, in case of all 0, return 0 */
	bsr = tommy_ilog2_u32(pos | 1);

	return &hashlin->bucket[bsr][pos];
}

/** \internal
 * Returns a pointer to the bucket of the specified hash.
 */
tommy_inline tommy_hashlin_node** tommy_hashlin_bucket_ref(tommy_hashlin* hashlin, tommy_hash_t hash)
{
	tommy_count_t pos;
	tommy_count_t high_pos;

	pos = hash & hashlin->low_mask;
	high_pos = hash & hashlin->bucket_mask;

	/* if this position is already allocated in the high half */
	if (pos < hashlin->split) {
		/* The following assigment is expected to be implemented */
		/* with a conditional move instruction */
		/* that results in a little better and constant performance */
		/* regardless of the split position. */
		/* This affects mostly the worst case, when the split value */
		/* is near at its half, resulting in a totally unpredictable */
		/* condition by the CPU. */
		/* In such case the use of the conditional move is generally faster. */

		/* use also the high bit */
		pos = high_pos;
	}

	return tommy_hashlin_pos(hashlin, pos);
}

/**
 * Gets the bucket of the specified hash.
 * The bucket is guaranteed to contain ALL the elements with the specified hash,
 * but it can contain also others.
 * You can access elements in the bucket following the ::next pointer until 0.
 * \param hash Hash of the element to find.
 * \return The head of the bucket, or 0 if empty.
 */
tommy_inline tommy_hashlin_node* tommy_hashlin_bucket(tommy_hashlin* hashlin, tommy_hash_t hash)
{
	return *tommy_hashlin_bucket_ref(hashlin, hash);
}

/**
 * Searches an element in the hashtable.
 * You have to provide a compare function and the hash of the element you want to find.
 * If more equal elements are present, the first one is returned.
 * \param cmp Compare function called with cmp_arg as first argument and with the element to compare as a second one.
 * The function should return 0 for equal elements, anything other for different elements.
 * \param cmp_arg Compare argument passed as first argument of the compare function.
 * \param hash Hash of the element to find.
 * \return The first element found, or 0 if none.
 */
tommy_inline void* tommy_hashlin_search(tommy_hashlin* hashlin, tommy_search_func* cmp, const void* cmp_arg, tommy_hash_t hash)
{
	tommy_hashlin_node* i = tommy_hashlin_bucket(hashlin, hash);

	while (i) {
		/* we first check if the hash matches, as in the same bucket we may have multiples hash values */
		if (i->key == hash && cmp(cmp_arg, i->data) == 0)
			return i->data;
		i = i->next;
	}
	return 0;
}

/**
 * Removes an element from the hashtable.
 * You must already have the address of the element to remove.
 * \return The tommy_node::data field of the node removed.
 */
void* tommy_hashlin_remove_existing(tommy_hashlin* hashlin, tommy_hashlin_node* node);

/**
 * Calls the specified function for each element in the hashtable.
 *
 * You can use this function to deallocate all the elements inserted.
 *
 * \code
 * tommy_hashlin hashlin;
 *
 * // initializes the hashtable
 * tommy_hashlin_init(&hashlin);
 *
 * ...
 *
 * // creates an object
 * struct object* obj = malloc(sizeof(struct object));
 *
 * ...
 *
 * // insert it in the hashtable
 * tommy_hashlin_insert(&hashlin, &obj->node, obj, tommy_inthash_u32(obj->value));
 *
 * ...
 *
 * // deallocates all the objects iterating the hashtable
 * tommy_hashlin_foreach(&hashlin, free);
 *
 * // deallocates the hashtable
 * tommy_hashlin_done(&hashlin);
 * \endcode
 */
void tommy_hashlin_foreach(tommy_hashlin* hashlin, tommy_foreach_func* func);

/**
 * Calls the specified function with an argument for each element in the hashtable.
 */
void tommy_hashlin_foreach_arg(tommy_hashlin* hashlin, tommy_foreach_arg_func* func, void* arg);

/**
 * Gets the number of elements.
 */
tommy_inline tommy_count_t tommy_hashlin_count(tommy_hashlin* hashlin)
{
	return hashlin->count;
}

/**
 * Gets the size of allocated memory.
 * It includes the size of the ::tommy_hashlin_node of the stored elements.
 */
tommy_size_t tommy_hashlin_memory_usage(tommy_hashlin* hashlin);

#endif


/* end of file tommyhashlin.h */


/* tommyhashlin.h already was included */


/* start of file tommylist.h */

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

/** \file
 * Double linked list for collisions into hashtables.
 *
 * This list is a double linked list mainly targetted for handling collisions
 * into an hashtables, but useable also as a generic list.
 *
 * The main feature of this list is to require only one pointer to represent the
 * list, compared to a classic implementation requiring a head an a tail pointers.
 * This reduces the memory usage in hashtables.
 *
 * Another feature is to support the insertion at the end of the list. This allow to store
 * collisions in a stable order. Where for stable order we mean that equal elements keep
 * their insertion order.
 *
 * To initialize the list, you have to call tommy_list_init(), or to simply assign
 * to it NULL, as an empty list is represented by the NULL value.
 *
 * \code
 * tommy_list list;
 *
 * tommy_list_init(&list); // initializes the list
 * \endcode
 *
 * To insert elements in the list you have to call tommy_list_insert_tail()
 * or tommy_list_insert_head() for each element.
 * In the insertion call you have to specify the address of the node and the
 * address of the object.
 * The address of the object is used to initialize the tommy_node::data field
 * of the node.
 *
 * \code
 * struct object {
 *     tommy_node node;
 *     // other fields
 *     int value;
 * };
 *
 * struct object* obj = malloc(sizeof(struct object)); // creates the object
 *
 * obj->value = ...; // initializes the object
 *
 * tommy_list_insert_tail(&list, &obj->node, obj); // inserts the object
 * \endcode
 *
 * To iterate over all the elements in the list you have to call
 * tommy_list_head() to get the head of the list and follow the
 * tommy_node::next pointer until NULL.
 *
 * \code
 * tommy_node* i = tommy_list_head(&list);
 * while (i) {
 *     struct object* obj = i->data; // gets the object pointer
 *
 *     printf("%d\n", obj->value); // process the object
 *
 *     i = i->next; // go to the next element
 * }
 * \endcode
 *
 * To destroy the list you have to remove all the elements,
 * as the list is completely inplace and it doesn't allocate memory.
 * This can be done with the tommy_list_foreach() function.
 *
 * \code
 * // deallocates all the objects iterating the list
 * tommy_list_foreach(&list, free);
 * \endcode
 */

#ifndef __TOMMYLIST_H
#define __TOMMYLIST_H


/* tommytypes.h already was included */


/******************************************************************************/
/* list */

/**
 * Double linked list type.
 */
typedef tommy_node* tommy_list;

/**
 * Initializes the list.
 * The list is completely inplace, so it doesn't need to be deinitialized.
 */
tommy_inline void tommy_list_init(tommy_list* list)
{
	*list = 0;
}

/**
 * Gets the head of the list.
 * \return The head node. For empty lists 0 is returned.
 */
tommy_inline tommy_node* tommy_list_head(tommy_list* list)
{
	return *list;
}

/**
 * Gets the tail of the list.
 * \return The tail node. For empty lists 0 is returned.
 */
tommy_inline tommy_node* tommy_list_tail(tommy_list* list)
{
	tommy_node* head = tommy_list_head(list);

	if (!head)
		return 0;

	return head->prev;
}

/** \internal
 * Creates a new list with a single element.
 * \param list The list to initialize.
 * \param node The node to insert.
 */
tommy_inline void tommy_list_insert_first(tommy_list* list, tommy_node* node)
{
	/* one element "circular" prev list */
	node->prev = node;

	/* one element "0 terminated" next list */
	node->next = 0;

	*list = node;
}

/** \internal
 * Inserts an element at the head of a not empty list.
 * The element is inserted at the head of the list. The list cannot be empty.
 * \param list The list. The list cannot be empty.
 * \param node The node to insert.
 */
tommy_inline void tommy_list_insert_head_not_empty(tommy_list* list, tommy_node* node)
{
	tommy_node* head = tommy_list_head(list);

	/* insert in the "circular" prev list */
	node->prev = head->prev;
	head->prev = node;

	/* insert in the "0 terminated" next list */
	node->next = head;

	*list = node;
}

/** \internal
 * Inserts an element at the tail of a not empty list.
 * The element is inserted at the tail of the list. The list cannot be empty.
 * \param head The node at the list head. It cannot be 0.
 * \param node The node to insert.
 */
tommy_inline void tommy_list_insert_tail_not_empty(tommy_node* head, tommy_node* node)
{
	/* insert in the "circular" prev list */
	node->prev = head->prev;
	head->prev = node;

	/* insert in the "0 terminated" next list */
	node->next = 0;
	node->prev->next = node;
}

/**
 * Inserts an element at the head of a list.
 * \param node The node to insert.
 * \param data The object containing the node. It's used to set the tommy_node::data field of the node.
 */
tommy_inline void tommy_list_insert_head(tommy_list* list, tommy_node* node, void* data)
{
	tommy_node* head = tommy_list_head(list);

	if (head)
		tommy_list_insert_head_not_empty(list, node);
	else
		tommy_list_insert_first(list, node);

	node->data = data;
}

/**
 * Inserts an element at the tail of a list.
 * \param node The node to insert.
 * \param data The object containing the node. It's used to set the tommy_node::data field of the node.
 */
tommy_inline void tommy_list_insert_tail(tommy_list* list, tommy_node* node, void* data)
{
	tommy_node* head = tommy_list_head(list);

	if (head)
		tommy_list_insert_tail_not_empty(head, node);
	else
		tommy_list_insert_first(list, node);

	node->data = data;
}

/** \internal
 * Removes an element from the head of a not empty list.
 * \param list The list. The list cannot be empty.
 * \return The node removed.
 */
tommy_inline tommy_node* tommy_list_remove_head_not_empty(tommy_list* list)
{
	tommy_node* head = tommy_list_head(list);

	/* remove from the "circular" prev list */
	head->next->prev = head->prev;

	/* remove from the "0 terminated" next list */
	*list = head->next; /* the new head, in case 0 */

	return head;
}

/**
 * Removes an element from the list.
 * You must already have the address of the element to remove.
 * \note The node content is left unchanged, including the tommy_node::next
 * and tommy_node::prev fields that still contain pointers at the list.
 * \param node The node to remove. The node must be in the list.
 * \return The tommy_node::data field of the node removed.
 */
tommy_inline void* tommy_list_remove_existing(tommy_list* list, tommy_node* node)
{
	tommy_node* head = tommy_list_head(list);

	/* remove from the "circular" prev list */
	if (node->next)
		node->next->prev = node->prev;
	else
		head->prev = node->prev; /* the last */

	/* remove from the "0 terminated" next list */
	if (head == node)
		*list = node->next; /* the new head, in case 0 */
	else
		node->prev->next = node->next;

	return node->data;
}

/**
 * Concats two lists.
 * The second list is concatenated at the first list.
 * \param first The first list.
 * \param second The second list. After this call the list content is undefined,
 * and you should not use it anymore.
 */
void tommy_list_concat(tommy_list* first, tommy_list* second);

/**
 * Sorts a list.
 * It's a stable merge sort with O(N*log(N)) worst complexity.
 * It's faster on degenerated cases like partially ordered lists.
 * \param cmp Compare function called with two elements.
 * The function should return <0 if the first element is less than the second, ==0 if equal, and >0 if greather.
 */
void tommy_list_sort(tommy_list* list, tommy_compare_func* cmp);

/**
 * Checks if empty.
 * \return If the list is empty.
 */
tommy_inline tommy_bool_t tommy_list_empty(tommy_list* list)
{
	return tommy_list_head(list) == 0;
}

/**
 * Calls the specified function for each element in the list.
 *
 * You can use this function to deallocate all the elements
 * inserted in a list.
 *
 * \code
 * tommy_list list;
 *
 * // initializes the list
 * tommy_list_init(&list);
 *
 * ...
 *
 * // creates an object
 * struct object* obj = malloc(sizeof(struct object));
 *
 * ...
 *
 * // insert it in the list
 * tommy_list_insert_tail(&list, &obj->node, obj);
 *
 * ...
 *
 * // deallocates all the objects iterating the list
 * tommy_list_foreach(&list, free);
 * \endcode
 */
tommy_inline void tommy_list_foreach(tommy_list* list, tommy_foreach_func* func)
{
	tommy_node* node = tommy_list_head(list);

	while (node) {
		void* data = node->data;
		node = node->next;
		func(data);
	}
}

/**
 * Calls the specified function with an argument for each element in the list.
 */
tommy_inline void tommy_list_foreach_arg(tommy_list* list, tommy_foreach_arg_func* func, void* arg)
{
	tommy_node* node = tommy_list_head(list);

	while (node) {
		void* data = node->data;
		node = node->next;
		func(arg, data);
	}
}

#endif


/* end of file tommylist.h */


/* tommylist.h already was included */


/* start of file tommychain.h */

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

/** \file
 * Chain of nodes.
 * A chain of nodes is an abstraction used to implements complex list operations
 * like sorting.
 *
 * Do not use this directly. Use lists instead.
 */

#ifndef __TOMMYCHAIN_H
#define __TOMMYCHAIN_H


/* tommytypes.h already was included */


/******************************************************************************/
/* chain */

/**
 * Chain of nodes.
 * A chain of nodes is a sequence of nodes with the following properties:
 * - It contains at least one node. A chains of zero nodes cannot exist.
 * - The next field of the tail is of *undefined* value.
 * - The prev field of the head is of *undefined* value.
 * - All the other inner prev and next fields are correctly set.
 */
typedef struct tommy_chain_struct {
	tommy_node* head; /**< Pointer at the head of the chain. */
	tommy_node* tail; /**< Pointer at the tail of the chain. */
} tommy_chain;

/**
 * Splices a chain in the middle of another chain.
 */
tommy_inline void tommy_chain_splice(tommy_node* first_before, tommy_node* first_after, tommy_node* second_head, tommy_node* second_tail)
{
	/* set the prev list */
	first_after->prev = second_tail;
	second_head->prev = first_before;

	/* set the next list */
	first_before->next = second_head;
	second_tail->next = first_after;
}

/**
 * Concats two chains.
 */
tommy_inline void tommy_chain_concat(tommy_node* first_tail, tommy_node* second_head)
{
	/* set the prev list */
	second_head->prev = first_tail;

	/* set the next list */
	first_tail->next = second_head;
}

/**
 * Merges two chains.
 */
tommy_inline void tommy_chain_merge(tommy_chain* first, tommy_chain* second, tommy_compare_func* cmp)
{
	tommy_node* first_i = first->head;
	tommy_node* second_i = second->head;

	/* merge */
	while (1) {
		if (cmp(first_i->data, second_i->data) > 0) {
			tommy_node* next = second_i->next;
			if (first_i == first->head) {
				tommy_chain_concat(second_i, first_i);
				first->head = second_i;
			} else {
				tommy_chain_splice(first_i->prev, first_i, second_i, second_i);
			}
			if (second_i == second->tail)
				break;
			second_i = next;
		} else {
			if (first_i == first->tail) {
				tommy_chain_concat(first_i, second_i);
				first->tail = second->tail;
				break;
			}
			first_i = first_i->next;
		}
	}
}

/**
 * Merges two chains managing special degenerated cases.
 * It's funtionally equivalent at tommy_chain_merge() but faster with already ordered chains.
 */
tommy_inline void tommy_chain_merge_degenerated(tommy_chain* first, tommy_chain* second, tommy_compare_func* cmp)
{
	/* identify the condition first <= second */
	if (cmp(first->tail->data, second->head->data) <= 0) {
		tommy_chain_concat(first->tail, second->head);
		first->tail = second->tail;
		return;
	}

	/* identify the condition second < first */
	/* here we must be strict on comparison to keep the sort stable */
	if (cmp(second->tail->data, first->head->data) < 0) {
		tommy_chain_concat(second->tail, first->head);
		first->head = second->head;
		return;
	}

	tommy_chain_merge(first, second, cmp);
}

/**
 * Max number of elements as a power of 2.
 */
#define TOMMY_CHAIN_BIT_MAX 32

/**
 * Sorts a chain.
 * It's a stable merge sort using power of 2 buckets, with O(N*log(N)) complexity,
 * similar at the one used in the SGI STL libraries and in the Linux Kernel,
 * but faster on degenerated cases like already ordered lists.
 *
 * SGI STL stl_list.h
 * http://www.sgi.com/tech/stl/stl_list.h
 *
 * Linux Kernel lib/list_sort.c
 * http://lxr.linux.no/#linux+v2.6.36/lib/list_sort.c
 */
tommy_inline void tommy_chain_mergesort(tommy_chain* chain, tommy_compare_func* cmp)
{
	/*
	 * Bit buckets of chains.
	 * Each bucket contains 2^i nodes or it's empty.
	 * The chain at address TOMMY_CHAIN_BIT_MAX is an independet variable operating as "carry".
	 * We keep it in the same "bit" vector to avoid reports from the valgrind tool sgcheck.
	 */
	tommy_chain bit[TOMMY_CHAIN_BIT_MAX + 1];

	/**
	 * Value stored inside the bit bucket.
	 * It's used to know which bucket is empty of full.
	 */
	tommy_count_t counter;
	tommy_node* node = chain->head;
	tommy_node* tail = chain->tail;
	tommy_count_t mask;
	tommy_count_t i;

	counter = 0;
	while (1) {
		tommy_node* next;
		tommy_chain* last;

		/* carry bit to add */
		last = &bit[TOMMY_CHAIN_BIT_MAX];
		bit[TOMMY_CHAIN_BIT_MAX].head = node;
		bit[TOMMY_CHAIN_BIT_MAX].tail = node;
		next = node->next;

		/* add the bit, propagating the carry */
		i = 0;
		mask = counter;
		while ((mask & 1) != 0) {
			tommy_chain_merge_degenerated(&bit[i], last, cmp);
			mask >>= 1;
			last = &bit[i];
			++i;
		}

		/* copy the carry in the first empty bit */
		bit[i] = *last;

		/* add the carry in the counter */
		++counter;

		if (node == tail)
			break;
		node = next;
	}

	/* merge the buckets */
	i = tommy_ctz_u32(counter);
	mask = counter >> i;
	while (mask != 1) {
		mask >>= 1;
		if (mask & 1)
			tommy_chain_merge_degenerated(&bit[i + 1], &bit[i], cmp);
		else
			bit[i + 1] = bit[i];
		++i;
	}

	*chain = bit[i];
}

#endif


/* end of file tommychain.h */

#endif /* _SESQLITE_HASH_IMPL_H_ */
