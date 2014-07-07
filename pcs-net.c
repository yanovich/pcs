/* pcs-net.c -- process control service networking
 * Copyright (C) 2014 Sergei Ianovich <ynvich@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "includes.h"

#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pcs-parser.h"

struct network_config {
	const char			*device;
	const char			*host;
	unsigned int			port;
	const char			*send_pipe;
	const char			*receive_file;
};

struct network_state {
	int			start;
	int			sent;
	int			paused;
	int			ready;
	int			fd;
	struct network_config	*conf;
};

static int
options_device_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	const char *from = (const char *) event->data.scalar.value;
	struct network_config *conf = node->state->data;

	if (YAML_SCALAR_EVENT != event->type)
		return pcs_parser_unexpected_event(node, event);

	debug(" %s\n", from);
	conf->device = strdup(from);
	pcs_parser_remove_node(node);
	return 1;
}

static int
options_host_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	const char *from = (const char *) event->data.scalar.value;
	struct network_config *conf = node->state->data;

	if (YAML_SCALAR_EVENT != event->type)
		return pcs_parser_unexpected_event(node, event);

	debug(" %s\n", from);
	conf->host = strdup(from);
	pcs_parser_remove_node(node);
	return 1;
}

static int
options_port_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	long l;
	struct network_config *conf = node->state->data;

	if (YAML_SCALAR_EVENT != event->type)
		return pcs_parser_unexpected_event(node, event);

	l = pcs_parser_long(node, event, NULL);
	debug(" %li\n", l);
	conf->port = (unsigned) l;
	pcs_parser_remove_node(node);
	return 1;
}

static int
options_send_pipe_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	const char *from = (const char *) event->data.scalar.value;
	struct network_config *conf = node->state->data;

	if (YAML_SCALAR_EVENT != event->type)
		return pcs_parser_unexpected_event(node, event);

	debug(" %s\n", from);
	conf->send_pipe = strdup(from);
	pcs_parser_remove_node(node);
	return 1;
}

static int
options_receive_file_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	const char *from = (const char *) event->data.scalar.value;
	struct network_config *conf = node->state->data;

	if (YAML_SCALAR_EVENT != event->type)
		return pcs_parser_unexpected_event(node, event);

	debug(" %s\n", from);
	conf->receive_file = strdup(from);
	pcs_parser_remove_node(node);
	return 1;
}

static struct pcs_parser_map top_map[] = {
	{
		.key			= "device",
		.handler		= options_device_event,
	}
	,{
		.key			= "host",
		.handler		= options_host_event,
	}
	,{
		.key			= "port",
		.handler		= options_port_event,
	}
	,{
		.key			= "send pipe",
		.handler		= options_send_pipe_event,
	}
	,{
		.key			= "receive file",
		.handler		= options_receive_file_event,
	}
	,{
	}
};

static struct pcs_parser_map document_map = {
	.handler		= pcs_parser_map,
	.data			= &top_map,
};

static struct pcs_parser_map stream_map = {
	.handler		= pcs_parser_one_document,
	.data			= &document_map,
};

static int
load_network_config(const char const *fn, struct network_config *c)
{
	int err = pcs_parse_yaml(fn, &stream_map, c);
	if (err)
		return 1;
	if (!c->device || !c->host || !c->send_pipe || !c->receive_file)
		return 1;
	return 0;
}

static int received_signal = 0;

static void
sigterm_handler(int sig)
{
	received_signal = sig;
}

static int start = 1;
static int paused = 0;

#define PCS_MAX_DEVICE_HDR	1024
char header[PCS_MAX_DEVICE_HDR] = "";

static size_t
reader(char *buffer, size_t size, size_t nitems, void *userdata)
{
	struct network_state *s = userdata;
	static size_t sent = 0;
	size_t max = size * nitems;
	size_t len;
	int *pfd = &s->fd;
	int res;
	struct stat st;

	//printf("sending start: %i, size: %li, nitems: %li\n", start, size, nitems);
	if (!header[0]) {
		res = snprintf(header, PCS_MAX_DEVICE_HDR, "{\"device\":\"%s\"}",
				s->conf->device);
		if (res < 0)
			header[0] = 0;
	}
	if (start) {
		len = strlen(&header[sent]);
		if (len > max) {
			memcpy(buffer, &header[sent], max);
			sent += max;
			return max;
		}
		memcpy(buffer, &header[sent], len);
		sent = 0;
		start = 0;
		return len;
	}

	res = stat(s->conf->send_pipe, &st);
	if (res < 0) {
		mkfifo(s->conf->send_pipe, 0600);
	}
	if (*pfd < 0) {
		*pfd = open(s->conf->send_pipe, O_RDONLY | O_NONBLOCK );
		paused = 1;
		return CURL_READFUNC_PAUSE;
	}
	len = read(*pfd, buffer, max);
	if (0 == len) {
		close(*pfd);
		*pfd = open(s->conf->send_pipe, O_RDONLY | O_NONBLOCK );
		paused = 1;
		return CURL_READFUNC_PAUSE;
	}
	if(-EAGAIN == len) {
		paused = 1;
		return CURL_READFUNC_PAUSE;
	}
	return len;
}

static size_t
writer(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	struct network_state *s = userdata;
	FILE *f;
	size_t res;

	debug("%s\n", ptr);

	if (!s->ready) {
		if ('o' == ptr[0] && 'k' == ptr[1]) {
			s->ready = 1;
			return size * nmemb;
		}
		return 0;
	}

	f = fopen(s->conf->receive_file, "w");
	if (!f)
		return size * nmemb;
	res = fwrite(ptr, size, nmemb, f);
	fclose(f);
	return res;
}

char url[PCS_MAX_DEVICE_HDR] = "";

static int
run(struct network_state *s)
{
	CURLcode ret;
	CURLMcode retm;
	CURL *hnd;
	CURLM *h;
	struct curl_slist *slist1;
	int r = 0;
	int res;
	fd_set rfds, wfds, efds;
	struct timeval t;
	struct CURLMsg *m;

	if (!url[0]) {
		if (s->conf->port)
			res = snprintf(url, PCS_MAX_DEVICE_HDR,
					"http://%s:%u/states",
					s->conf->host,
					s->conf->port);
		else
			res = snprintf(url, PCS_MAX_DEVICE_HDR,
					"http://%s/states",
					s->conf->host);
		if (res < 0)
			header[0] = 0;
	}
	slist1 = NULL;
	slist1 = curl_slist_append(slist1, "Content-Type: application/x-device-data");

	h = curl_multi_init();
	if (NULL == h) {
		ret = 1;
		goto headers;
	}

	hnd = curl_easy_init();
	if (NULL == hnd) {
		ret = 1;
		goto multi;
	}

	ret = curl_easy_setopt(hnd, CURLOPT_URL, "http://localhost:3000/states");
	if (CURLE_OK != ret) {
		ret = 1;
		goto easy;
	}
	ret = curl_easy_setopt(hnd, CURLOPT_UPLOAD, 1L);
	if (CURLE_OK != ret) {
		ret = 1;
		goto easy;
	}
	ret = curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.37.0");
	if (CURLE_OK != ret) {
		ret = 1;
		goto easy;
	}
	ret = curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);
	if (CURLE_OK != ret) {
		ret = 1;
		goto easy;
	}
	ret = curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
	if (CURLE_OK != ret) {
		ret = 1;
		goto easy;
	}
	ret = curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);
	if (CURLE_OK != ret) {
		ret = 1;
		goto easy;
	}

	ret = curl_easy_setopt(hnd, CURLOPT_READFUNCTION, reader);
	if (CURLE_OK != ret) {
		ret = 1;
		goto easy;
	}
	ret = curl_easy_setopt(hnd, CURLOPT_READDATA, s);
	if (CURLE_OK != ret) {
		ret = 1;
		goto easy;
	}

	ret = curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, writer);
	if (CURLE_OK != ret) {
		ret = 1;
		goto easy;
	}
	ret = curl_easy_setopt(hnd, CURLOPT_WRITEDATA, s);
	if (CURLE_OK != ret) {
		ret = 1;
		goto easy;
	}

	retm = curl_multi_add_handle(h, hnd);
	if (CURLM_OK != retm) {
		ret = 1;
		goto easy;
	}
	s->fd = open(s->conf->send_pipe, O_RDONLY | O_NONBLOCK );
	debug("init done\n");
	while (!received_signal) {
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&efds);
		curl_multi_fdset(h, &rfds, &wfds, &efds, &r);
		t.tv_sec = 0;
		t.tv_usec = 100000;
		select(r, &rfds, &wfds, &efds, &t);
		if (received_signal)
			break;
		curl_easy_pause(hnd, CURLPAUSE_CONT);
		paused = 0;
		while (CURLM_CALL_MULTI_PERFORM ==
				(retm = curl_multi_perform(h, &r)));
		if (received_signal)
			break;
		if (CURLM_OK != retm)
			break;
		while ((m = curl_multi_info_read(h, &r))) {
			if (m->easy_handle != hnd)
				continue;
			if (m->msg != CURLMSG_DONE)
				continue;
			curl_multi_remove_handle(h, hnd);
			start = 1;
			sleep(2);
			retm = curl_multi_add_handle(h, hnd);
			if (CURLM_OK != retm) {
				ret = 1;
				goto easy;
			}
		}
	}

	if (s->fd >= 0) {
		close(s->fd);
		s->fd = -1;
	}
	curl_multi_remove_handle(h, hnd);
easy:
	curl_easy_cleanup(hnd);
	hnd = NULL;
multi:
	curl_multi_cleanup(h);
	h = NULL;
headers:
	curl_slist_free_all(slist1);
	slist1 = NULL;
	return (int)ret;
}

int main(int argc, char **argv)
{
	const char *config_file_name = SYSCONFDIR "/pcs-net.conf";
	const char *pid_file = PKGRUNDIR "/pcs-net.pid";
	int log_level = LOG_NOTICE;
	int test_only = 0;
	struct network_config c = {};
	struct network_state s = {
		.conf = &c,
	};
	int opt;
	int no_detach = 0;
	FILE *f;

	while ((opt = getopt(argc, argv, "Ddf:t")) != -1) {
		switch (opt) {
		case 'D':
			no_detach = 1;
			break;
		case 'd':
			if (log_level >= LOG_DEBUG)
				log_level++;
			else
				log_level = LOG_DEBUG;
			break;
		case 'f':
			config_file_name = optarg;
			break;
		case 't':
			test_only = 1;
			break;
		default:
			break;
		}
	}

	log_init("pcs-net", log_level, LOG_DAEMON, 1);
	if (load_network_config(config_file_name, &c))
		fatal("Bad configuration\n");
	if (test_only)
		return 0;

	if (!no_detach)
		daemon(0, 0);

	log_init("pcs-net", log_level, LOG_DAEMON, no_detach);

	f = fopen(pid_file, "w");
	if (f != NULL) {
		fprintf(f, "%lu\n", (long unsigned) getpid());
		fclose(f);
	} else {
		error(PACKAGE ": failed to create %s\n", pid_file);
	}

	signal(SIGTERM, sigterm_handler);
	signal(SIGQUIT, sigterm_handler);
	signal(SIGINT, sigterm_handler);

	while (!received_signal) {
		s.start = 1;
		s.paused = 0;
		s.ready = 0;
		s.sent = 0;
		run(&s);
	}

	if (!no_detach)
		closelog();

	unlink(pid_file);

	return 0;
}
