/*
 * Parse HTTP request, make HTTP response.
 *
 * Author: Wu Bingzheng
 *
 */

#ifndef _FCA_HTTP_H_
#define _FCA_HTTP_H_

#include "fcache.h"
#include "request.h"

enum http_methods {
	FCA_HTTP_METHOD_GET,
	FCA_HTTP_METHOD_HEAD,
	FCA_HTTP_METHOD_PUT,
	FCA_HTTP_METHOD_POST,
	FCA_HTTP_METHOD_PURGE,
	FCA_HTTP_METHOD_DELETE,
	FCA_HTTP_METHOD_INVALID,
};

struct http_method_s {
	string_t	str;
	void		*data;
};
extern struct http_method_s http_methods[];

#define RANGE_NO_SET	-1

int http_request_parse(fca_request_t *r);
string_t *http_code_page(int code);
ssize_t http_decode_uri(const char *uri, ssize_t len, char *output);
ssize_t http_make_200_response_header(ssize_t content_length, char *output);
ssize_t http_make_206_response_header(ssize_t range_start, ssize_t range_end,
		ssize_t body_len, char *output);

#endif
