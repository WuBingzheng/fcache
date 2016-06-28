/*
 * Linear dynamic hashing
 *
 * Author: Wu Bingzheng
 *
 */

#ifndef _HASH_H_
#define _HASH_H_

#include "list.h"

typedef struct fca_hash_s fca_hash_t;

typedef struct {
	unsigned char		id[16];
	struct hlist_node	node;
} fca_hash_node_t;

fca_hash_t *hash_init();
void hash_destroy(fca_hash_t *hash);

void hash_add(fca_hash_t *hash, fca_hash_node_t *hnode, unsigned char *str, int len);
fca_hash_node_t *hash_get(fca_hash_t *hash, unsigned char *str, int len, unsigned char *hash_id);
void hash_del(fca_hash_t *hash, fca_hash_node_t *hnode);

#endif
