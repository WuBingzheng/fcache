/*
 * Each device has a worker thread, for disk-IO.
 *
 * Author: Wu Bingzheng
 *
 */

#include "worker.h"

static void worker_destory(fca_worker_t *worker)
{
	epoll_del(worker->epoll_fd, worker->receive_fd);
	epoll_del(master_epoll_fd, worker->recycle_fd);
	close(worker->receive_fd);
	close(worker->dispatch_fd);
	close(worker->recycle_fd);
	close(worker->return_fd);
	close(worker->epoll_fd);
	timer_destroy(&worker->timer);
	free(worker);
}

static int worker_check_quit(fca_worker_t *worker)
{
	if (worker->quit_time <= timer_now(&worker->timer)) {
		request_clean(&worker->working_requests, 0);
	}
	return worker->request_nr == 0;
}

static void *worker_entry(void *data)
{
#define MAX_EVENTS 512
	fca_worker_t *worker = data;
	fca_request_t *r;
	struct epoll_event events[MAX_EVENTS];
	struct list_head *p, *expires, *safep;
	fca_timer_node_t *tnode;
	int rc, i, type;
	void *ptr;

	pthread_detach(pthread_self());

	while (worker->quit_time == 0 || !worker_check_quit(worker)) {

		rc = epoll_wait(worker->epoll_fd, events, MAX_EVENTS, 1000);
		if (rc == -1) {
			log_error_run(errno, "worker epoll_wait");
		}

		/* try return_fd, if any blocked requests */
		request_clean(&worker->blocked_requests, 0);

		/* ready events */
		timer_refresh(&worker->timer);
		for (i = 0; i < rc; i++) {

			ptr = events[i].data.ptr;
			type = ((uintptr_t)ptr) & EVENT_TYPE_MASK;
			ptr = (void *)(((uintptr_t)ptr) & ~EVENT_TYPE_MASK);

			/* @receive_fd is ready */
			if (type == EVENT_TYPE_PIPE) {
				worker_request_receive(worker);

			/* ready requests */
			} else {
				r = ptr;
				r->event_handler(r);
			}
		}

		/* timeout requests */
		expires = timer_expire(&worker->timer);
		list_for_each_safe(p, safep, expires) {
			tnode = list_entry(p, fca_timer_node_t, tnode_node);
			r = list_entry(tnode, fca_request_t, tnode);

			/* request_timeout_handler() will delete @p from @expires */
			request_timeout_handler(r);
		}
	}

	worker_destory(worker);
	return NULL;
}

fca_worker_t *worker_create(void)
{
	fca_worker_t *worker;
	int fd[2];

	worker = malloc(sizeof(fca_worker_t));
	if (worker == NULL) {
		goto fail0;
	}

	/* 2 pipes. */
	if (pipe(fd) < 0) {
		goto fail1;
	}
	worker->receive_fd = fd[0];
	worker->dispatch_fd = fd[1];
	if (set_nonblock(fd[0]) < 0 || set_nonblock(fd[1]) < 0) {
		goto fail2;
	}

	if (pipe(fd) < 0) {
		goto fail2;
	}
	worker->recycle_fd = fd[0];
	worker->return_fd = fd[1];
	if (set_nonblock(fd[0]) < 0 || set_nonblock(fd[1]) < 0) {
		goto fail3;
	}

	/* create epoll, and add recycle_fd */
	worker->epoll_fd = epoll_create(100);
	if (worker->epoll_fd < 0) {
		goto fail3;
	}

	if (epoll_add_read(worker->epoll_fd, worker->receive_fd,
			(void *)EVENT_TYPE_PIPE) < 0) {
		goto fail4;
	}
	if (epoll_add_read(master_epoll_fd, worker->recycle_fd,
			(void *)((uintptr_t)worker | EVENT_TYPE_PIPE))) {
		goto fail5;
	}

	/* each worker thread has its own timer */
	timer_init(&worker->timer);

	worker->quit_time = 0;
	worker->request_nr = 0;
	INIT_LIST_HEAD(&worker->working_requests);
	INIT_LIST_HEAD(&worker->blocked_requests);

	/* create thread */
	if (pthread_create(&worker->tid, NULL, worker_entry, worker) != 0) {
		goto fail6;
	}

	return worker;;

fail6:
	timer_destroy(&worker->timer);
	epoll_del(master_epoll_fd, worker->recycle_fd);
fail5:
	epoll_del(worker->epoll_fd, worker->receive_fd);
fail4:
	close(worker->epoll_fd);
fail3:
	close(worker->recycle_fd);
	close(worker->return_fd);
fail2:
	close(worker->receive_fd);
	close(worker->dispatch_fd);
fail1:
	free(worker);
fail0:
	return NULL;
}

void worker_delete(fca_worker_t *worker, time_t quit_time)
{
	worker->quit_time = quit_time ? quit_time : BIG_TIME;
}

/* worker_request_dispatch() and worker_request_return() call this, to
 * send @r into @target thread. */
static int worker_do_request_write(fca_request_t *r, req_handler_f *handler, fca_worker_t *target)
{
	fca_worker_t *old = r->worker_thread;
	int fd = target ? target->dispatch_fd : old->return_fd;
	int rc;

	event_del(r);
	r->event_handler = handler;

	r->worker_thread = target;
	list_del(&r->rnode);

	rc = write(fd, &r, sizeof(fca_request_t *));
	if (rc < 0) {
		r->worker_thread = old;
		list_add(&r->rnode, old ? &old->blocked_requests : &master_requests);
		log_error_run(errno, "worker_do_request_write (%p)", target);
		return FCA_ERROR;
	}

	if (rc != sizeof(fca_request_t *)) {
		log_error_run(0, "!!! worker_do_request_write (%p) %d", target, rc);
		exit(1);
	}

	return FCA_OK;
}

/* master call this to dispatch a request to some worker */
int worker_request_dispatch(fca_request_t *r, req_handler_f *handler)
{
	if (r->item == NULL) {
		/* no item, so no device, no worker thread,
		 * so cannot dispatch to a worker */
		handler(r);
		return FCA_OK;
	}

	fca_worker_t *target = device_of_item(r->item)->worker;
	int rc = worker_do_request_write(r, handler, target);
	if (rc == FCA_OK) {
		target->request_nr++;
	}

	return rc;
}

/* worker call this to return a finished request to master */
int worker_request_return(fca_request_t *r, req_handler_f *handler)
{
	return worker_do_request_write(r, handler, NULL);
}


/* worker_request_recycle() and worker_request_receive() call this, to
 * read requests from @fd, and add it @target. */
static int worker_do_request_read(int fd, struct list_head *target)
{
	fca_request_t *reqs[50], *r;
	int rc, i, count = 0;

	do {
		rc = read(fd, reqs, sizeof(reqs));
		if (rc < 0) {
			if (errno != EAGAIN) {
				log_error_run(errno, "worker_do_request_read (%p)", target);
			}
			break;
		}

		if (rc & (sizeof(fca_request_t *) - 1)) {
			log_error_run(0, "!!! worker_do_request_read (%p) %d", target, rc);
			exit(1);
		}

		for (i = 0; i < rc / sizeof(fca_request_t *); i++) {
			r = reqs[i];
			list_add(&r->rnode, target);
			r->event_handler(r);
			count++;
		}

	} while (rc == sizeof(reqs));

	return count;
}

/* master call this to recycle a finished request from worker */
void worker_request_recycle(fca_worker_t *worker)
{
	int count = worker_do_request_read(worker->recycle_fd, &master_requests);
	worker->request_nr -= count;
}

/* worker call this to receive a request from master */
void worker_request_receive(fca_worker_t *worker)
{
	worker_do_request_read(worker->receive_fd, &worker->working_requests);
}
