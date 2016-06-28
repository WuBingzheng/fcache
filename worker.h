/*
 * Worker thread management, and communication between master.
 *
 * Author: Wu Bingzheng
 *
 */

#ifndef _FCA_WORKER_H_
#define _FCA_WORKER_H_ 

#include "fcache.h"

struct fca_worker_s {
	struct list_head	working_requests;
	struct list_head	blocked_requests;
	fca_timer_t		timer;

	time_t		quit_time;
	pthread_t	tid;
	int		epoll_fd;

	/* only master update this, and worker check this. */
	int		request_nr;

	/**
	 * two pipes.
	 * 1. master write request into @dispatch_fd, worker read
	 *    @receive_fd to receive request;
	 * 2. worker write the finished requests into @return_fd,
	 *    and master receive the requests from @receive_fd.
	 **/
	int		recycle_fd;
	int		dispatch_fd;
	int		receive_fd;
	int		return_fd;

};

fca_worker_t *worker_create(void);
void worker_delete(fca_worker_t *worker, time_t quit_time);

/* master calls */
int worker_request_dispatch(fca_request_t *r, req_handler_f *handler);
void worker_request_recycle(fca_worker_t *worker);

/* worker calls */
int worker_request_return(fca_request_t *r, req_handler_f *handler);
void worker_request_receive(fca_worker_t *worker);

#endif
