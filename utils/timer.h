/**
 * Timer and time management.
 *
 * Auther: Wu Bingzheng
 *
 **/

#ifndef _FCA_TIMER_H_
#define _FCA_TIMER_H_

#include <time.h>
#include "list.h"

#define LEN_TIME_FARMAT_RFC1123	sizeof("Sun, 06 Nov 1994 08:49:23 GMT")
#define LEN_TIME_FARMAT_LOG	sizeof("1994-11-06 08:49:23")

#define BIG_TIME 3638880000

typedef struct {
	struct list_head	tgroup_head;
	struct list_head	expires;

	time_t		now;
	time_t		now_ms;
	char		format_rfc1123[LEN_TIME_FARMAT_RFC1123];
	char		format_log[LEN_TIME_FARMAT_LOG];
} fca_timer_t;

typedef struct fca_timer_group_s fca_timer_group_t;
typedef struct {
	struct list_head	tnode_node;

	time_t			timeout;
	fca_timer_group_t	*group;
} fca_timer_node_t;

time_t timer_parse_rfc1123(char *p);
void timer_destroy(fca_timer_t *timer);
time_t timer_refresh(fca_timer_t *timer);
time_t timer_closest(fca_timer_t *timer);
struct list_head *timer_expire(fca_timer_t *timer);
int timer_add(fca_timer_t *timer, fca_timer_node_t *tnode, time_t timeout);
void timer_del(fca_timer_node_t *tnode);
int timer_update(fca_timer_node_t *tnode, time_t timeout);
void timer_init(fca_timer_t *timer);
char *timer_format_rfc1123(fca_timer_t *timer);
char *timer_format_log(fca_timer_t *timer);

static inline time_t timer_now(fca_timer_t *timer)
{
	return timer->now;
}

static inline time_t timer_now_ms(fca_timer_t *timer)
{
	return timer->now_ms;
}

#endif
