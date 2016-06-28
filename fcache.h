/*
 * Include and define all things.
 *
 * Author: Wu Bingzheng
 *
 */

#ifndef _OLIVEHC_H_
#define _OLIVEHC_H_

#define FCA_VERSION	"fcache version: 1.2"

#define FCA_OK		0
#define FCA_ERROR	-1
#define FCA_DONE	-2
#define FCA_AGAIN	-3
#define FCA_DECLINE	-4

#define PATH_LENGTH	1024

#define LOOP_LIMIT	1000

#define EVENT_TYPE_SOCKET	0
#define EVENT_TYPE_LISTEN	1
#define EVENT_TYPE_PIPE		2
#define EVENT_TYPE_MASK		3UL

typedef char fca_flag_t;


#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>  
#include <linux/fs.h>
#include <netinet/tcp.h>


/* utils */
#include "utils/list.h"
#include "utils/socktcp.h"
#include "utils/string.h"
#include "utils/slab.h"
#include "utils/hash.h"
#include "utils/epoll.h"
#include "utils/timer.h"
#include "utils/ipbucket.h"
#include "utils/idx_pointer.h"


typedef struct fca_request_s fca_request_t;
typedef struct fca_item_s fca_item_t;
typedef struct fca_server_s fca_server_t;
typedef struct fca_device_s fca_device_t;
typedef struct fca_worker_s fca_worker_t;
typedef struct fca_format_item_s fca_format_item_t;
typedef struct fca_conf_s fca_conf_t;
typedef void req_handler_f(fca_request_t *r);

extern int master_epoll_fd;
extern fca_timer_t master_timer;
extern struct list_head master_requests;

#include "conf.h"
#include "format.h"
#include "http.h"
#include "server.h"
#include "worker.h"
#include "device.h"
#include "request.h"
#include "event.h"

void log_error(FILE *filp, const char *prefix, int errnum, const char *fmt, ...);

extern FILE *error_filp;
#define log_error_run(errnum, fmt, ...) \
	log_error(error_filp, timer_format_log(&master_timer), errnum, fmt, ##__VA_ARGS__)

extern FILE *admin_out_filp;
#define log_error_admin(errnum, fmt, ...) \
	log_error(admin_out_filp, "ERROR:", errnum, fmt, ##__VA_ARGS__)

#endif
