/*
 * Utils for request event management.
 *
 * Author: Wu Bingzheng
 *
 */

#ifndef _FCA_EVENT_H_
#define _FCA_EVENT_H_

#include "fcache.h"

int event_add_read(fca_request_t *r, req_handler_f *handler);
int event_add_write(fca_request_t *r, req_handler_f *handler);
int event_add_keepalive(fca_request_t *r, req_handler_f *handler);
void event_del(fca_request_t *r);

#endif
