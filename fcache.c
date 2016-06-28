/*
 * Entry of fcache, and master thread.
 *
 * Author: Wu Bingzheng
 *
 */

#include "fcache.h"

/* set by configure file or command options */
static char *conf_filename = "fcache.conf";
static char error_log[PATH_LENGTH];
static time_t quit_timeout;

int master_epoll_fd;
fca_timer_t master_timer;
LIST_HEAD(master_requests);
FILE *error_filp;
FILE *admin_out_filp;
static time_t quit_time = 0;

static int fcache_global_conf_check(fca_conf_t *conf_cycle)
{
	conf_cycle->error_filp = NULL;
	if (strcmp(conf_cycle->error_log, error_log)) {
		conf_cycle->error_filp = fopen(conf_cycle->error_log, "a");
		if (conf_cycle->error_filp == NULL) {
			log_error_admin(errno, "open error log");
			return FCA_ERROR;
		}
	}
	return FCA_OK;
}

static void fcache_global_conf_load(fca_conf_t *conf_cycle)
{
	quit_timeout = conf_cycle->quit_timeout;

	if (conf_cycle->error_filp) {
		fclose(error_filp);
		error_filp = conf_cycle->error_filp;
		strcpy(error_log, conf_cycle->error_log);
		conf_cycle->error_filp = NULL;
	}
}

static void fcache_global_conf_rollback(fca_conf_t *conf_cycle) 
{
	if (conf_cycle->error_filp) {
		fclose(conf_cycle->error_filp);
	}
}

static int fcache_load_conf(void)
{
#define CONF_CHECK(f) if (f(conf_cycle) != FCA_OK) goto rollback
	errno = 0;

	fca_conf_t *conf_cycle = conf_parse(conf_filename);
	if (conf_cycle == NULL) {
		return FCA_ERROR;
	}

	CONF_CHECK(fcache_global_conf_check);
	CONF_CHECK(device_conf_check);
	CONF_CHECK(server_conf_check);

	fcache_global_conf_load(conf_cycle);
	device_conf_load(conf_cycle);
	server_conf_load(conf_cycle);

	return FCA_OK;

rollback:
	fcache_global_conf_rollback(conf_cycle);
	device_conf_rollback(conf_cycle);
	server_conf_rollback(conf_cycle);
	return FCA_ERROR;
}

static void fcache_quit(void)
{
	if (quit_time != 0) { /* already in quiting */
		return;
	}

	quit_time = timer_now(&master_timer) + quit_timeout;
	server_stop_service();
	device_worker_quit(quit_time);
}

static void fcache_status(FILE *filp)
{
	device_status(filp);
	server_status(filp);
}

/* handler of admin port, print the current status */
static void fcache_admin_handler(int admin_fd)
{
	char buf[100];
	int sock_fd;
	ssize_t rc;

	sock_fd = tcp_accept(admin_fd, NULL);
	if (sock_fd == -1) {
		return;
	}

	rc = recv(sock_fd, buf, 99, 0);
	if (rc < 0) {
		return;
	}
	buf[rc] = '\0';

	admin_out_filp = fdopen(sock_fd, "w");
	if (admin_out_filp == NULL) {
		return;
	}

	if (strncmp(buf, "status", 6) == 0) {
		fcache_status(admin_out_filp);

	} else if (strncmp(buf, "reload", 6) == 0) {
		rc = fcache_load_conf();
		if (rc == FCA_OK) {
			fputs("Reload successfully!\n", admin_out_filp);
		}

	} else if (strncmp(buf, "quit", 4) == 0) {
		fcache_quit();
		fputs("Quiting...\n", admin_out_filp);

	} else if (strncmp(buf, "clear ", 6) == 0) {
		rc = server_clear((unsigned short)atoi(buf + 6));
		if (rc == FCA_OK) {
			fputs("Server cleared!\n", admin_out_filp);
		}

	} else {
		fputs("Invalid command!\n", admin_out_filp);
		fputs("Usage: status|reload|quit|clear SERVER\n", admin_out_filp);
	}

	fclose(admin_out_filp);
	admin_out_filp = NULL;
}

/* entry of the master thread */
static void fcache_master_entry(int admin_fd)
{
#define MAX_EVENTS 512
	struct epoll_event events[MAX_EVENTS];
	struct list_head *p, *expires, *safep;
	fca_timer_node_t *tnode;
	int rc, i, type;
	void *ptr;
	fca_request_t *r;
	time_t last, now;

	last = timer_now(&master_timer);

	device_format_load();

	while (quit_time == 0 || !request_check_quit(quit_time > last)) {

		rc = epoll_wait(master_epoll_fd, events, MAX_EVENTS, 1000);
		if (rc == -1 && errno != EINTR) {
			log_error_run(errno, "master epoll_wait");
		}

		/* ready events */
		timer_refresh(&master_timer);
		for (i = 0; i < rc; i++) {
			type = (uintptr_t)events[i].data.ptr & EVENT_TYPE_MASK;
			ptr = (void *)((uintptr_t)events[i].data.ptr & ~EVENT_TYPE_MASK);

			switch(type) {
			case EVENT_TYPE_LISTEN:
				if (ptr == NULL) {
					fcache_admin_handler(admin_fd);
				} else {
					server_listen_handler((fca_server_t *)ptr);
				}
				break;

			case EVENT_TYPE_PIPE:
				worker_request_recycle((fca_worker_t *)ptr);
				break;

			default: /* socket */
				r = ptr;
				r->event_handler(r);
			}
		}

		/* timeout requests */
		expires = timer_expire(&master_timer);
		list_for_each_safe(p, safep, expires) {
			tnode = list_entry(p, fca_timer_node_t, tnode_node);
			r = list_entry(tnode, fca_request_t, tnode);

			/* request_timeout_handler() will delete @p from @expires */
			request_timeout_handler(r);
		}

		/* routines */
		now = timer_now(&master_timer);
		if (now != last) {
			last = now;

			server_routine();
			device_routine();
			fflush(error_filp);
		}
	}

	device_format_store();
}

int main(int argc, char **argv)
{
	char *pid_filename = "fcache.pid";
	int admin_port = 5210;
	char *prefix = NULL;
	int daemon_mode = 1, ch;
	int admin_fd;

	char *help = "Usage: fcache [-hvb][-c conf_file][-p prefix][-a admin][-i pid]\n"
				"\th: print help\n"
				"\tv: print version\n"
				"\tb: block, non-daemon mode, for debug\n"
				"\tc: set configure file [fcache.conf]\n"
				"\tp: set prefix [.]\n"
				"\ta: set admin port [5210]\n"
				"\ti: set pid file [fcache.pid]\n";

	/* parse and process options */
	while ((ch = getopt(argc, argv, "hvbc:p:a:i:")) != -1) {
		switch(ch) {
		case 'h':
			printf("%s", help);
			return 0;
		case 'v':
			printf("%s\n", FCA_VERSION);
			return 0;
		case 'b':
			daemon_mode = 0;
			break;
		case 'c':
			conf_filename = optarg;
			break;
		case 'p':
			prefix = optarg;
			break;
		case 'a':
			admin_port = atoi(optarg);
			break;
		case 'i':
			pid_filename = optarg;
			break;
		default:
			printf("%s", help);
			return 1;
		}
	}

	if (prefix) {
		if (chdir(prefix) < 0) {
			perror("error in set prefix");
			return 1;
		}
	}

	/* get into daemon_mode */
	if (daemon_mode) {
		if (daemon(1, 1) < 0) {
			perror("error in get daemon");
			return 1;
		}
		/* stderr is set to error_filp, which will be
		 * closed in fcache_global_conf_load() later. */
		close(0);
		close(1);
	}
	error_log[0] = '\0';
	error_filp = stderr;

	/* admin_out_filp is init as stderr, so error message in
	 * fcache_load_conf() will print out;
	 * then stderr will be closed in fcache_global_conf_load(),
	 * and admin_out_filp will not be used, until be re-set in
	 * fcache_admin_handler() before calling fcache_load_conf(). */
	admin_out_filp = stderr;

	/* master timer */
	timer_init(&master_timer);

	/* master epoll */
	master_epoll_fd = epoll_create(100);
	if (master_epoll_fd < 0) {
		perror("error in open epoll");
		return 1;
	}

	/* admin port */
	admin_fd = tcp_bind(admin_port);
	if (admin_fd < 0) {
		perror("error in bind admin port");
		return 1;
	}
	tcp_listen(admin_fd);
	if (epoll_add_read(master_epoll_fd, admin_fd, (void *)EVENT_TYPE_LISTEN) < 0) {
		perror("error in add admin port in epoll");
		return 1;
	}

	/* signal */
	signal(SIGPIPE, SIG_IGN);

	/* load configure */
	if (fcache_load_conf() == FCA_ERROR) {
		return 1;
	}

	/* pid file */
	FILE *pid_filp = fopen(pid_filename, "w");
	if (pid_filp == NULL) {
		perror("error in open pid file");
		return 1;
	}
	fprintf(pid_filp, "%d\n", getpid());
	fclose(pid_filp);

	/* run fcache! */
	fcache_master_entry(admin_fd);

	/* quit */
	unlink(pid_filename);
	return 0;
}

void log_error(FILE *filp, const char *prefix, int errnum, const char *fmt, ...)
{
	if (prefix) {
		fputs(prefix, filp);
		fputs(" ", filp);
	}

	va_list args;
	va_start(args, fmt);
	vfprintf(filp, fmt, args);
	va_end(args);

	if (errnum) {
		fprintf(filp, " [%s]\n", strerror(errnum));
	} else {
		fputs("\n", filp);
	}
}
