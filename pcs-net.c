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
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>

static int start = 1;
static int paused = 0;

static size_t
reader(char *buffer, size_t size, size_t nitems, void *instream)
{
	const char header[] = "{\"device\":\"53856461c015057143f63f25\"}";
	static size_t sent = 0;
	size_t max = size * nitems;
	size_t len;
	int *pfd = instream;
	int res;
	struct stat st;

	//printf("sending start: %i, size: %li, nitems: %li\n", start, size, nitems);
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

	res = stat("/tmp/t0006.output", &st);
	if (res < 0) {
		mkfifo("/tmp/t0006.output", 0600);
	}
	if (*pfd < 0) {
		*pfd = open("/tmp/t0006.output", O_RDONLY | O_NONBLOCK );
		paused = 1;
		return CURL_READFUNC_PAUSE;
	}
	len = read(*pfd, buffer, max);
	if (0 == len) {
		close(*pfd);
		*pfd = open("/tmp/t0006.output", O_RDONLY | O_NONBLOCK );
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
	FILE *f = fopen("/tmp/pcs.input", "w");
	size_t res;
	printf("%s", ptr);
	if (!f)
		return size * nmemb;
	res = fwrite(ptr, size, nmemb, f);
	fclose(f);
	return res;
}

int main(int argc, char *argv[])
{
  CURLcode ret;
  CURLMcode retm;
  CURL *hnd;
  CURLM *h;
  struct curl_slist *slist1;
  char *b = xzalloc(16384);
  int fd = -1;
  int r = 0;
  fd_set rfds, wfds;
  struct timeval t;
  struct CURLMsg *m;

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
  ret = curl_easy_setopt(hnd, CURLOPT_READDATA, &fd);
  if (CURLE_OK != ret) {
	  ret = 1;
	  goto easy;
  }

  ret = curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, writer);
  if (CURLE_OK != ret) {
	  ret = 1;
	  goto easy;
  }
  ret = curl_easy_setopt(hnd, CURLOPT_WRITEDATA, NULL);
  if (CURLE_OK != ret) {
	  ret = 1;
	  goto easy;
  }

  ret = curl_multi_add_handle(h, hnd);
  if (CURLE_OK != ret) {
	  ret = 1;
	  goto easy;
  }
  fd = open("/tmp/t0006.output", O_RDONLY | O_NONBLOCK );
  while (1) {
		  FD_ZERO(&rfds);
		  FD_ZERO(&wfds);
		  t.tv_sec = 0;
		  t.tv_usec = 100000;
		  select(1, &rfds, &wfds, NULL, &t);
		  curl_easy_pause(hnd, CURLPAUSE_CONT);
		  paused = 0;
		  while (CURLM_CALL_MULTI_PERFORM ==
				  (retm = curl_multi_perform(h, &r)));
		  if (CURLM_OK != retm)
			  break;
		  while ((m = curl_multi_info_read(h, &r))) {
			  if (m->easy_handle != hnd)
				  continue;
			  if (m->msg != CURLMSG_DONE)
				  continue;
			  curl_multi_remove_handle(h, hnd);
			  start = 1;
			  curl_multi_add_handle(h, hnd);
		  }
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
  xfree(b);
  b = NULL;
  return (int)ret;
}
