/*
 * Parse configure file.
 *
 * Author: Wu Bingzheng
 *
 */

#ifndef _FCA_CONF_H_
#define _FCA_CONF_H_

#include "fcache.h"

struct fca_conf_s {
	int		device_badblock_percent;
	fca_flag_t	device_check_270G;
	time_t		quit_timeout;

	char		error_log[PATH_LENGTH];
	FILE		*error_filp;

	struct list_head	servers;
	struct list_head	devices;

};

fca_conf_t *conf_parse(const char *filename);

#endif
