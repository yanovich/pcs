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

#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static size_t
reader(char *buffer, size_t size, size_t nitems, void *instream)
{
	static int start = 1;
	const char header[] = "{\"device\":\"53856461c015057143f63f25\"}";
	const char message[] = "{\"MV4\":1}";
	static size_t sent = 0;
	size_t max = size * nitems;
	size_t len;
	printf("sending size: %li, nitems: %li\n", size, nitems);
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

	if (0 == sent)
		sleep(10);

	len = strlen(&message[sent]);
	if (len > max) {
		memcpy(buffer, &message[sent], max);
		sent += max;
		return max;
	}
	memcpy(buffer, &message[sent], len);
	sent = 0;
	return len;
}

static size_t
writer(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	printf("size: %li, nmemb: %li\n%s\n", size, nmemb, ptr);
	return size * nmemb;
}

int main(int argc, char *argv[])
{
  CURLcode ret;
  CURL *hnd;
  struct curl_slist *slist1;

  slist1 = NULL;
  slist1 = curl_slist_append(slist1, "Content-Type: application/x-device-data");

  hnd = curl_easy_init();
  curl_easy_setopt(hnd, CURLOPT_URL, "http://localhost:3000/states");
  curl_easy_setopt(hnd, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.37.0");
  curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);
  curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
  /* curl_easy_setopt(hnd, CURLOPT_SSH_KNOWNHOSTS, "/home/sergei/.ssh/known_hosts") */;
  curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);

  /* Here is a list of options the curl code used that cannot get generated
     as source easily. You may select to either not use them or implement
     them yourself.

  CURLOPT_WRITEDATA set to a objectpointer
  CURLOPT_WRITEFUNCTION set to a functionpointer
  CURLOPT_READDATA set to a objectpointer
  CURLOPT_READFUNCTION set to a functionpointer
  CURLOPT_SEEKDATA set to a objectpointer
  CURLOPT_SEEKFUNCTION set to a functionpointer
  CURLOPT_ERRORBUFFER set to a objectpointer
  CURLOPT_STDERR set to a objectpointer
  CURLOPT_HEADERFUNCTION set to a functionpointer
  CURLOPT_HEADERDATA set to a objectpointer

  */
  curl_easy_setopt(hnd, CURLOPT_READFUNCTION, reader);
  curl_easy_setopt(hnd, CURLOPT_READDATA, NULL);

  curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, writer);
  curl_easy_setopt(hnd, CURLOPT_WRITEDATA, NULL);

  ret = curl_easy_perform(hnd);

  curl_easy_cleanup(hnd);
  hnd = NULL;
  curl_slist_free_all(slist1);
  slist1 = NULL;

  return (int)ret;
}
