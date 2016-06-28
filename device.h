/*
 * Devices and free blocks management.
 *
 * Author: Wu Bingzheng
 *
 */

#ifndef _FCA_STORE_H_
#define _FCA_STORE_H_

#include "fcache.h"

struct fca_device_s {
	unsigned	deleted:1;
	unsigned	kicked:1;

	int		fd;
	int		index;
	int		used;
	char		filename[PATH_LENGTH];
	dev_t		dev;
	ino_t		inode;
	long		item_nr;
	long		fblock_nr;
	size_t		capacity;
	size_t		consumed;
	size_t		badblock;

	fca_worker_t		*worker;

	struct list_head	order_head;
	struct list_head	dnode;

	struct fca_device_s	*conf;
};

typedef struct {
	/* @order_node and @fblock must be together, to
	 * distinguish fca_item_t and fca_free_block_t. */
	struct list_head	order_node;
	unsigned		fblock:1;

	short			device_index;

	/* since sendfile(2) supports only 0x4020010000, so 40bits is enough */
	unsigned long		offset:40;

	off_t			block_size;
	struct list_head	bucket_node;
} fca_free_block_t;

#define DEVICES_LIMIT IPT_ARRAY_SIZE

fca_device_t *device_of_item(fca_item_t *item);

int device_conf_check(fca_conf_t *conf_cycle);
void device_conf_load(fca_conf_t *conf_cycle);
void device_conf_rollback(fca_conf_t *conf_cycle);

int device_free_block_extend(size_t target);
size_t device_get_free_block(fca_item_t *item);
size_t device_return_free_block(fca_item_t *item);
size_t device_cut_free_block(fca_item_t *item);
void device_load_post(fca_device_t *device);

void device_worker_quit(time_t quit_time);
void device_format_load(void);
void device_format_store(void);
void device_routine(void);
void device_status(FILE *filp);

#endif
