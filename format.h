/*
 * Manage device file. Load items from device when
 * starts, and store items to device when quits.
 *
 * Author: Wu Bingzheng
 *
 */

#ifndef _FCA_FORMAT_H_
#define _FCA_FORMAT_H_

#include "fcache.h"

/* fca_item_t on disk */
struct fca_format_item_s {
	unsigned char	hash_id[16];
	off_t		offset;
	uint32_t	length;
	int32_t		expire;
	unsigned short	headers_len;
	short		server_index;
};

int format_store_device(unsigned short *ports, fca_device_t *device);
int format_load_device(fca_device_t *device);

#endif
