/*
 * Linear dynamic hashing
 *
 * MurmurHash3 x64-versoin is used to calculate string into hash-key.
 * MurmurHash3 was written by Austin Appleby, and is placed in the public
 * domain. The author hereby disclaims copyright to this source code.
 *
 * Author: Wu Bingzheng
 *
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "hash.h"
#include "list.h"

/* == MurmurHash3 begins == */

inline static uint64_t rotl64 ( uint64_t x, int8_t r )
{
  return (x << r) | (x >> (64 - r));
}

inline static uint64_t fmix64 ( uint64_t k )
{
  k ^= k >> 33;
  k *= 0xff51afd7ed558ccdLLU;
  k ^= k >> 33;
  k *= 0xc4ceb9fe1a85ec53LLU;
  k ^= k >> 33;

  return k;
}

static void MurmurHash3_x64_128(const void * key, const int len, void *out)
{
  const uint8_t * data = (const uint8_t*)key;
  const int nblocks = len / 16;

  uint64_t h1 = 0xae27729cb202352bLLU;
  uint64_t h2 = 0xd5e7551b9199b903LLU;

  const uint64_t c1 = 0x87c37b91114253d5LLU;
  const uint64_t c2 = 0x4cf5ad432745937fLLU;

  // body

  const uint64_t * blocks = (const uint64_t *)(data);

  int i;
  for (i = 0; i < nblocks; i++)
  {
    uint64_t k1 = blocks[i*2+0];
    uint64_t k2 = blocks[i*2+1];

    k1 *= c1; k1  = rotl64(k1,31); k1 *= c2; h1 ^= k1;

    h1 = rotl64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

    k2 *= c2; k2  = rotl64(k2,33); k2 *= c1; h2 ^= k2;

    h2 = rotl64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
  }

  // tail

  const uint8_t * tail = (const uint8_t*)(data + nblocks*16);

  uint64_t k1 = 0;
  uint64_t k2 = 0;

  switch(len & 15)
  {
  case 15: k2 ^= ((uint64_t)tail[14]) << 48;
  case 14: k2 ^= ((uint64_t)tail[13]) << 40;
  case 13: k2 ^= ((uint64_t)tail[12]) << 32;
  case 12: k2 ^= ((uint64_t)tail[11]) << 24;
  case 11: k2 ^= ((uint64_t)tail[10]) << 16;
  case 10: k2 ^= ((uint64_t)tail[ 9]) << 8;
  case  9: k2 ^= ((uint64_t)tail[ 8]) << 0;
           k2 *= c2; k2  = rotl64(k2,33); k2 *= c1; h2 ^= k2;

  case  8: k1 ^= ((uint64_t)tail[ 7]) << 56;
  case  7: k1 ^= ((uint64_t)tail[ 6]) << 48;
  case  6: k1 ^= ((uint64_t)tail[ 5]) << 40;
  case  5: k1 ^= ((uint64_t)tail[ 4]) << 32;
  case  4: k1 ^= ((uint64_t)tail[ 3]) << 24;
  case  3: k1 ^= ((uint64_t)tail[ 2]) << 16;
  case  2: k1 ^= ((uint64_t)tail[ 1]) << 8;
  case  1: k1 ^= ((uint64_t)tail[ 0]) << 0;
           k1 *= c1; k1  = rotl64(k1,31); k1 *= c2; h1 ^= k1;
  };

  // finalization

  h1 ^= len; h2 ^= len;

  h1 += h2;
  h2 += h1;

  h1 = fmix64(h1);
  h2 = fmix64(h2);

  h1 += h2;
  h2 += h1;

  ((uint64_t*)out)[0] = h1;
  ((uint64_t*)out)[1] = h2;
}

/* == hash begins == */

typedef int hindex_t;

/* hash bucket size range from 2^4 to 2^28*/
#define HASH_BUCKET_SIZE_BEGIN	(1<<4)
#define HASH_BUCKET_SIZE_MAX	(1<<28)

struct fca_hash_s {
	struct hlist_head	*buckets;
	hindex_t		bucket_size;
	long			items;

	/* previous buckets, in split process */
	struct hlist_head	*prev_buckets;
	/* split pointer in linear hashing */
	hindex_t		split;
};

inline static int key_equal(unsigned char *id1, unsigned char *id2)
{
	uint64_t *p = (uint64_t *)id1;
	uint64_t *q = (uint64_t *)id2;
	return (*p == *q) && (*(p+1) == *(q+1));
}

inline static hindex_t hash_index(fca_hash_t *hash, unsigned char *id)
{
	uint64_t *p = (uint64_t *)id;
	return (*p) & (hash->bucket_size - 1);
}

fca_hash_t *hash_init(void)
{
	fca_hash_t *hash;

	hash = malloc(sizeof(fca_hash_t));
	if (hash == NULL) {
		return NULL;
	}

	hash->items = 0;
	hash->split = 0;
	hash->prev_buckets = NULL;
	hash->bucket_size = HASH_BUCKET_SIZE_BEGIN;

	/* @calloc do the same thing with INIT_HLIST_HEAD. */
	hash->buckets = calloc(hash->bucket_size, sizeof(struct hlist_head));
	if (hash->buckets == NULL) {
		free(hash);
		return NULL;
	}

	return hash;
}

void hash_destroy(fca_hash_t *hash)
{
	free(hash->buckets);
	if (hash->prev_buckets) {
		free(hash->prev_buckets);
	}
	free(hash);
}

/* expansion: double the hash buckets */
static void hash_expansion(fca_hash_t *hash)
{
	fca_hash_node_t *hnode;
	struct hlist_node *p, *safe;
	void *newb;

#define HASH_COLLISIONS 10
	/* expansion */
	if (hash->items / hash->bucket_size >= HASH_COLLISIONS
			&& hash->prev_buckets == NULL
			&& hash->bucket_size < HASH_BUCKET_SIZE_MAX) {

		newb = calloc(hash->bucket_size * 2, sizeof(struct hlist_head));
		if (newb == NULL) {
			/* if calloc fails, do nothing */
			return;
		}
		hash->bucket_size *= 2;
		hash->prev_buckets = hash->buckets;
		hash->buckets = newb;
		hash->split = 0;
	}

	/* split a bucket */
	if (hash->prev_buckets != NULL) {

		for (p = hash->prev_buckets[hash->split].first; p; p = safe) {
			safe = p->next;
			hlist_del(p);

			hnode = list_entry(p, fca_hash_node_t, node);
			hlist_add_head(p, &hash->buckets[hash_index(hash, hnode->id)]);
		}

		hash->split++;
		if (hash->split == hash->bucket_size / 2) {
			/* expansion finish */
			free(hash->prev_buckets);
			hash->prev_buckets = NULL;
		}
	}
}

void hash_add(fca_hash_t *hash, fca_hash_node_t *hnode, unsigned char *str, int len)
{
	if (str) {
		MurmurHash3_x64_128(str, len, hnode->id);
	}

	hlist_add_head(&hnode->node, &hash->buckets[hash_index(hash, hnode->id)]);

	hash->items++;
	hash_expansion(hash);
}

static fca_hash_node_t *hash_search(struct hlist_head *slot, unsigned char *id)
{
	struct hlist_node *p;
	fca_hash_node_t *hnode;

	hlist_for_each(p, slot) {
		hnode = list_entry(p, fca_hash_node_t, node);
		if (key_equal(hnode->id, id)) {
			return hnode;
		}
	}

	return NULL;
}

fca_hash_node_t *hash_get(fca_hash_t *hash, unsigned char *str, int len, unsigned char *hash_id)
{
	unsigned char id_buf[16];
	unsigned char *id;
	hindex_t index;
	hindex_t pbsize;
	fca_hash_node_t *ret;

	hash_expansion(hash);

	id = hash_id ? hash_id : id_buf;
	MurmurHash3_x64_128(str, len, id);
	index = hash_index(hash, id);

	ret = hash_search(&hash->buckets[index], id);
	if (ret != NULL) {
		return ret;
	}

	if (hash->prev_buckets != NULL) {
		pbsize = hash->bucket_size / 2;
		if (index >= pbsize) {
			index -= pbsize;
		}
		return hash_search(&hash->prev_buckets[index], id);
	}

	return NULL;
}

void hash_del(fca_hash_t *hash, fca_hash_node_t *hnode)
{
	hlist_del(&hnode->node);
	hash->items--;
}
