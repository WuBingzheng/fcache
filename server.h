/*
 * Manage servers and items. Create/destroy/update servers,
 * get/put/delete items, etc.
 *
 * Author: Wu Bingzheng
 *
 */

#ifndef _FCA_SERVER_H_
#define _FCA_SERVER_H_

#include "fcache.h"

struct fca_server_s {
	struct list_head	snode;

	struct list_head	lru_head;
	struct list_head	passby_lru_head;

	unsigned short	listen_port;
	int		listen_fd;

	fca_hash_t	*hash;

	size_t		capacity;
	size_t		consumed;
	size_t		content;
	long		item_nr;

	fca_server_t	*conf;

	int		index;

	unsigned short	clear;

	unsigned	deleted:1;

	fca_flag_t	server_dump;
	fca_flag_t	shutdown_if_not_store;
	fca_flag_t	key_include_host;
	fca_flag_t	key_include_fca_key;
	fca_flag_t	key_include_query;

	fca_flag_t	passby_enable;
	long		passby_begin_item_nr;
	size_t		passby_begin_consumed;
	long		passby_limit_nr;
	time_t		passby_expire;

	long		passby_item_nr;

	size_t		sndbuf;
	size_t		rcvbuf;
	int		connections;
	int		connections_limit;
	int		send_timeout;
	int		recv_timeout;
	int		request_timeout;
	int		keepalive_timeout;
	char		access_log[PATH_LENGTH];
	FILE		*access_filp;
	size_t		item_max_size;
	time_t		expire_default;
	time_t		expire_force;

	/* statistics */
	long		gets;
	long		hits;
	long		passby_hits;
	long		gets_last_period;
	long		hits_last_period;
	long		passby_hits_last_period;
	long		gets_current_period;
	long		hits_current_period;
	long		passby_hits_current_period;

	long		puts;
	long		stores;
	long		passby_stores;
	long		puts_last_period;
	long		stores_last_period;
	long		passby_stores_last_period;
	long		puts_current_period;
	long		stores_current_period;
	long		passby_stores_current_period;

	long		deletes;
	long		deletes_current_period;
	long		deletes_last_period;

	size_t		output_size_last_period;
	size_t		output_size_current_period;
	size_t		input_size_last_period;
	size_t		input_size_current_period;

	time_t		last_clear;
	time_t		status_period;
};

struct fca_item_s {
	fca_hash_node_t		hnode;
	struct list_head	order_node;
	struct list_head	lru_node;

	/* since the number of items is huge, so we try our
	 * best to minimize the size of fca_item_s. */

	/* we supports 4G at most */
	uint32_t		length;

	/* 2038 is enough... */
	int32_t			expire;

	/* since sendfile(2) supports only 0x4020010000, so 40bits is enough */
	unsigned long		offset:40;

	unsigned		putting:1;
	unsigned		deleted:1;
	unsigned		badblock:1;

	short			server_index;
	short			device_index;

	unsigned short		headers_len;
	unsigned short		used;
	unsigned short		clear;
};

#define SERVERS_LIMIT IPT_ARRAY_SIZE

void server_dump_ports(unsigned short *ports);
fca_server_t *server_of_item(fca_item_t *item);
fca_server_t *server_by_port(unsigned short port);

int server_conf_check(fca_conf_t *conf_cycle);
void server_conf_load(fca_conf_t *conf_cycle);
void server_conf_rollback(fca_conf_t *conf_cycle);

int server_clear(unsigned short port);
void server_stop_service(void);

int server_request_get_handler(fca_request_t *r);
int server_request_put_handler(fca_request_t *r);
int server_request_delete_handler(fca_request_t *r);
void server_request_finalize(fca_request_t *r);

int server_item_valid(fca_item_t *item);
int server_load_fm_item(fca_server_t *s, fca_device_t *d,
		fca_format_item_t *fm_item);

void server_item_delete(fca_item_t *item);

void server_listen_handler(fca_server_t *s);

void server_routine(void);
void server_status(FILE *filp);
#endif
