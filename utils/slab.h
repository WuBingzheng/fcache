/**
 *
 * A simple slab.
 *
 * Auther: Wu Bingzheng
 *
 **/

#ifndef _SLAB_H_
#define _SLAB_H_

#include "list.h"

typedef struct fca_slab_s {
	struct hlist_head	block_head;
	unsigned		item_size;
} fca_slab_t;

/* make sure: sizeof(type) >= sizeof(struct hlist_node) */
#define FCA_SLAB_INIT(type) \
	{HLIST_HEAD_INIT, sizeof(type)+sizeof(fca_slab_t *)}

void *slab_alloc(fca_slab_t *slab);
void slab_free(void *p);

#endif
