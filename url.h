/* Copyright (c) 2020 J. von Rotz <jr@vrtz.ch>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define MAX_URL_SCHEME 15
#define MAX_URL_USERINFO 31
#define MAX_URL_HOST 511
#define MAX_URL_PORT 7
#define MAX_URL_PATH 1023
#define MAX_URL_QUERY 1023
#define MAX_URL_FRAGMENT 127

typedef struct {
	char scheme[MAX_URL_SCHEME+1];
	char userinfo[MAX_URL_USERINFO+1];
	char host[MAX_URL_HOST+1];
	char port[MAX_URL_PORT+1];
	char path[MAX_URL_PATH+1];
	char query[MAX_URL_QUERY+1];
	char fragment[MAX_URL_FRAGMENT+1];
} URL;

int lexurl(URL *url, char *buf);
URL *alexurl(char *buf);
int build_request_string(char *buf, int bufsiz, URL *url);
char *new_url_encoder_table();
int url_encode(char *table, unsigned char *inbuf, char *outbuf, int outbufsiz);
int url_decode(char *dst, const char *src, int bufsiz);
