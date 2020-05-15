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
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/types.h>
#ifdef __linux__
#include <stdlib.h>
#else
#include <sys/malloc.h>
#endif /* __linux__ */
#include <sys/stat.h>
#include <magic.h>
#include "gemini.h"
#include "util.h"
#include "mime.h"

int read_request(SSL *ssl, char *buffer) {
	return SSL_read(ssl, buffer, MAXREQSIZ);
}

int write_gemini_response(SSL *ssl, int status_major, int status_minor, char *meta, int metalen, char *buffer, int buflen) {
	char *header;
	int headerlen = MAXSTATUSSIZ+MAXWHITESIZ+metalen;
	header = malloc(headerlen);

	snprintf(header, headerlen, "%d%d\t%s\r\n", status_major, status_minor, meta);
	SSL_write(ssl, header, strlen(header));
	SSL_write(ssl, buffer, buflen);
	return 1;
}

int parse_request(char *buffer, int reqlen, char *host, char *path) {
	/*
	 * This is very basic, we incorporate a more sophisticated URL parser later.
	 * Also: Is maximum request of 1024 bytes considered with or without scheme?
	 */

	char *scheme = "gemini://";
	char *tmpbuf;
	int len;
	int i;

/*	memset(buffer + MAXREQSIZ, '\0', MAXBUF-MAXREQSIZ); */
	*(buffer+reqlen) = '\0';
	trim(buffer);
	len = strlen(buffer);

	if(strncmp(buffer, scheme, strlen(scheme)) != 0) {
		/* request has apparently no scheme, assume gemini:// */
		tmpbuf = malloc(MAXBUF);
		memset(tmpbuf, 0, MAXBUF);
		strncat(tmpbuf, scheme, 9);
		strncat(tmpbuf, buffer, len);
		tmpbuf[len+10] = '\0';
		strncpy(buffer, tmpbuf, len+10);
		free(tmpbuf);
	} 
	sscanf(buffer, "gemini://%99[^/]/%99[^\n]", host, path);
	if(strlen(path) == 0) {
		strncpy(path, DEFAULT_DOCUMENT, strlen(DEFAULT_DOCUMENT));
	}
	
	fprintf(stderr, "%s\t%s %s\n", buffer, host, path);

	return 1;
}

int handle_request(SSL *ssl) {
	int reqlen = 0;
	int reslen = 0;
	int i;
	int status; 
	char reqbuf[MAXBUF];
	char resbuf[MAXBUF];
	char host[MAXBUF];
	char path[MAXBUF];
	char localpath[MAXBUF];
	void *fb;
	FILE *fh;
	long fsize;
	magic_t magic;
	const char *magicstr;
	char mime[MAXBUF];
	char tmpbuf[MAXBUF];
	int pathlen;
	int pathpos;

	memset(reqbuf, 0, MAXBUF);
	memset(resbuf, 0, MAXBUF);
	memset(host, 0, MAXBUF);
	memset(path, 0, MAXBUF);
	memset(localpath, 0, MAXBUF);

	reqlen = read_request(ssl, reqbuf);
	i = parse_request(reqbuf, reqlen, host, path);
	snprintf(localpath, MAXBUF, "%s/%s", DOCUMENT_ROOT, path);

	if(access(localpath, R_OK) != -1) {
		/* Assume text/gemini for *.gmi files */
		i = 0;
		pathlen = strlen(path);
		pathpos = pathlen-4;
		while(i < pathlen) {
			tmpbuf[i] = path[pathpos+i];
			i++;
		}
		tmpbuf[i] = '\0';
		
		if(strncmp(tmpbuf, ".gmi", 4) != 0) {
			magic = magic_open(MAGIC_MIME_TYPE);
			magic_load(magic, NULL);
			magic_compile(magic, NULL);
			magicstr = magic_file(magic, localpath);
		
			if(magicstr == NULL)
				strncpy(mime, "text/gemini; charset=utf-8", 26);
			else
				strncpy(mime, magicstr, MAXBUF);
	
			magic_close(magic);
		} else {
			/*strncpy(mime, "text/gemini; charset=utf-8", 26);*/
			strncpy(mime, "text/gemini", 11);
		}

		/* Read file */
		fh = fopen(localpath, "r");
		if(fh == NULL)
			return -1;

		fseek(fh, 0, SEEK_END);
		fsize = ftell(fh);
		fseek(fh, 0, SEEK_SET);
		fb = malloc(fsize + 1);
		if(fb == NULL)
			return -1;

		fread(fb, sizeof(char), fsize, fh);
		fclose(fh);

		/* Finally write response */
		write_gemini_response(ssl, STATUS_SUCCESS, 0, mime, strlen(mime), fb, fsize);
		free(fb);
	} else {
		memset(resbuf, 0, MAXBUF);
		reslen = 0;
		write_gemini_response(ssl, STATUS_TEMPFAIL, 1, "File not found", 14, resbuf, reslen);
	}

	return 1;
}

