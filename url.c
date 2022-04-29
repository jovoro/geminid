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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "url.h"

int build_request_string(char *buf, int bufsiz, URL *url) {
	if(url == NULL)
		return -1;

	bufsiz--; /* Save one for \0 */
	if(bufsiz < 1)
		return -1;

	return snprintf(buf, bufsiz, "%s%s%s%s%s%s", url->scheme, url->host, url->port, url->path, url->query, url->fragment);
}

/* URL encoding function, stolen from 
 * https://stackoverflow.com/questions/5842471/c-url-encoding
 */

unsigned char *new_url_encoder_table() {
	int i;
	unsigned char *table;

	table = malloc(256);
	if(!table)
		return NULL;

	for(i = 0; i < 256; i++) {
		table[i] = isalnum(i) || i == '~' || i == '*' || i == '-' || i == '.' || i == '_' || i == '/' || i == ':' ? i : 0;
	}

	return table;
}

int url_encode(unsigned char *table, unsigned char *inbuf, char *outbuf, int outbufsiz) {
	int outlen = 0;

	for (; *inbuf; inbuf++) {
		if(outlen + 3 > outbufsiz)
			return outbufsiz;

		if(table[*inbuf])
			outlen += sprintf(outbuf, "%c", table[*inbuf]);
		else
			outlen += sprintf(outbuf, "%%%02X", *inbuf);

		while(*++outbuf);
	}

	return outlen;
}

/* URL decoding function, stolen from
 * https://stackoverflow.com/questions/2673207/c-c-url-decode-library
 */
int url_decode(unsigned char *dst, const char *src, int bufsiz) {
        char a, b;
	int outlen = 0;
        while (*src) {
		if(outlen > bufsiz)
			return -1;

                if ((*src == '%') &&
                    ((a = src[1]) && (b = src[2])) &&
                    (isxdigit(a) && isxdigit(b))) {
                        if (a >= 'a')
                                a -= 'a'-'A';
                        if (a >= 'A')
                                a -= ('A' - 10);
                        else
                                a -= '0';
                        if (b >= 'a')
                                b -= 'a'-'A';
                        if (b >= 'A')
                                b -= ('A' - 10);
                        else
                                b -= '0';
                        *dst = 16*a+b;
                        src+=3;
			dst++;
                } else {
                        *dst = *src;
			dst++;
			src++;
                }
		outlen++;	
        }

        *dst++ = '\0';
	return outlen;
}
