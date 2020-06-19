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
#include <limits.h>
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
#include "vhost.h"
#include "gemini.h"
#include "util.h"
#include "mime.h"
#include "file.h"
#include "log.h"
#include "url.h"


int read_request(SSL *ssl, char *buffer) {
	return SSL_read(ssl, buffer, MAXREQSIZ);
}

int write_gemini_response(SSL *ssl, int status_major, int status_minor, char *meta, int metalen, char *buffer, int buflen) {
	char *header;
	int headerlen = MAXSTATUSSIZ+MAXWHITESIZ+metalen+1;
	header = malloc(headerlen);
	if(header == NULL)
		return -1;

	snprintf(header, headerlen, "%d%d %s\r\n", status_major, status_minor, meta);
	SSL_write(ssl, header, strlen(header));
	SSL_write(ssl, buffer, buflen);
	free(header);
	return 1;
}

int parse_request(char *buffer, int reqlen, URL *urlp) {
	int i;
	char tmpbuf[MAXBUF];
	
	*(buffer+reqlen) = '\0';
	trim(buffer);
	i = lexurl(urlp, buffer);

	if(i < 0) {
		fprintf(stderr, "Error: Failed to parse URL: %s\n", buffer);
		return -1;
	}
	
	if(strcmp(urlp->userinfo, "") != 0)
		return -2;


	if(strncmp(urlp->scheme, "", MAX_URL_SCHEME) == 0)
		strncpy(urlp->scheme, "gemini://", MAX_URL_SCHEME);

	i = url_decode(tmpbuf, urlp->path, MAXBUF);
	if(i < 0)
		return -1;

	memcpy(urlp->path, tmpbuf, MAX_URL_PATH);
	urlp->path[MAX_URL_PATH] = 0;

	return 1;
}

int handle_request(SSL *ssl, char *document_root, VHOST *vhost) {
	int reqlen = 0;
	int reslen = 0;
	int mimelen = 0;
	int i;
	char tmpbuf[MAXBUF];
	char reqbuf[MAXBUF];
	char *resbuf;
	char *pathbuf;
	char host[MAXBUF];
	char path[MAXBUF];
	char defdocpath[MAXBUF];
	char localpath[MAXBUF];
	char mime[MAXBUF];
	struct stat statbuf;
	struct stat defdocstatbuf;
	URL requrl;
	X509 *pcert;

	memset(reqbuf, 0, MAXBUF);
	memset(host, 0, MAXBUF);
	memset(path, 0, MAXBUF);
	memset(localpath, 0, MAXBUF);
	
	reqlen = read_request(ssl, reqbuf);
	i = parse_request(reqbuf, reqlen, &requrl);
	
	if(i < 0) {
		write_gemini_response(ssl, STATUS_PERMFAIL, 9, "Bad request", 11, "", 0);
		log_access(vhost->accesslog, reqbuf, "", "", STATUS_PERMFAIL, 9, 0, "-", "-");
		if(i == -2)
			snprintf(tmpbuf, MAXBUF, "Error: Userinfo detected in URL request\n");
		else
			snprintf(tmpbuf, MAXBUF, "Error: Could not parse request %s\n", reqbuf);

		log_error(vhost->errorlog, tmpbuf);
		return -1;
	}

	i = build_request_string(reqbuf, MAXBUF, &requrl);
	
	if(i < 0) { 
		write_gemini_response(ssl, STATUS_TEMPFAIL, 1, "Processing Error", 9, "", 0);
		log_access(vhost->accesslog, reqbuf, "", "", STATUS_TEMPFAIL, 1, 0, "-", "-");
		snprintf(tmpbuf, MAXBUF, "Error: Could not handle request for %s\n", reqbuf);
		log_error(vhost->errorlog, tmpbuf);
		return -1;
	}
	
	strncpy(host, requrl.host, MAXBUF);
	strncpy(path, requrl.path, MAXBUF);
	snprintf(localpath, MAXBUF, "%s/%s", document_root, path);
	pathbuf = realpath(localpath, NULL);
	
	if(pathbuf == NULL) {
		write_gemini_response(ssl, STATUS_PERMFAIL, 1, "File not found", 14, "", 0);
		log_access(vhost->accesslog, reqbuf, host, path, STATUS_PERMFAIL, 1, 0, "-", "-");
		snprintf(tmpbuf, MAXBUF, "Error: Could not get realpath for %s\n", reqbuf);
		log_error(vhost->errorlog, tmpbuf);
		return -1;
	}
	
	if(strncmp(document_root, pathbuf, strlen(document_root)) != 0) {
		memcpy(localpath, document_root, strlen(document_root)+1);
		localpath[strlen(document_root)+1] = 0;
		strcat(localpath, "/");
			
		requrl.path[0] = path[0] = '/';
		requrl.path[1] = path[1] = 0;
		i = build_request_string(reqbuf, MAXBUF, &requrl);
		if(i < 0) { 
			write_gemini_response(ssl, STATUS_TEMPFAIL, 1, "Processing Error", 9, "", 0);
			log_access(vhost->accesslog, reqbuf, "", "", STATUS_TEMPFAIL, 1, 0, "-", "-");
			snprintf(tmpbuf, MAXBUF, "Error: Could not handle request for %s\n", reqbuf);
			log_error(vhost->errorlog, tmpbuf);
			return -1;
		}
	}
	
	free(pathbuf);
	
	/* temporary playground for client cert testing */
	if(strncmp(path, "/secure", 7) == 0) {
		/* Request client certificates */
		pcert = SSL_get_peer_certificate(ssl);
		if(pcert != NULL) {
			fprintf(stderr, "Client certificate:\nSubject: %s\n", X509_NAME_oneline(X509_get_subject_name(pcert), 0, 0));
			free(pcert);
		} else {
			write_gemini_response(ssl, STATUS_CERT, 0, "Client certificate required", 27, "", 0);
			log_access(vhost->accesslog, reqbuf, host, path, STATUS_CERT, 0, 0, "-", "-");
			return -1;
		}
	}

	if(access(localpath, R_OK) != -1) {
		/* Local path is readable; Find out, if file or directory */
		/* FIXME: What happens if it is neither of both..? Hm... */
		if((i = stat(localpath, &statbuf)) != 0) {
			write_gemini_response(ssl, STATUS_TEMPFAIL, 1, "I/O Error", 9, "", 0);
			log_access(vhost->accesslog, reqbuf, host, path, STATUS_TEMPFAIL, 1, 0, "-", "-");
	
			return -1;
		}
		if(S_ISDIR(statbuf.st_mode)) {
			/* path is directory */

			if(path[strlen(path)-1] != '/') {
				/* redirect to correct location with trailing slash (status 31) */
				snprintf(mime, MAXBUF, "%s/", reqbuf);
				write_gemini_response(ssl, STATUS_REDIRECT, 1, mime, strlen(mime), "", 0);
				log_access(vhost->accesslog, reqbuf, host, path, STATUS_REDIRECT, 1, 0, "-", "-");
			
				return 0;
			}

			snprintf(defdocpath, MAXBUF, "%s/%s", localpath, vhost->defaultdocument);
			if(access(defdocpath, R_OK) != -1) {
				if((i = stat(defdocpath, &defdocstatbuf)) == 0) {
					/* We have a default document, read it */
					resbuf = malloc(defdocstatbuf.st_size + 1);
					mimelen = read_file_meta(defdocpath, mime);
					reslen = read_file(defdocpath, resbuf);
				} else {
					write_gemini_response(ssl, STATUS_TEMPFAIL, 1, "I/O Error", 9, "", 0);
					log_access(vhost->accesslog, reqbuf, host, path, STATUS_TEMPFAIL, 1, 0, "-", "-");
					
					return -1;
				}
			} else {
				/* No default document, list directory */
				strncpy(mime, "text/gemini", 12);
				mimelen = 12;
				reslen = read_directory(path, document_root, reqbuf, &resbuf); /* document_root will be prepended again */
				reslen--; /* Dont write terminating NULL byte */
				if(reslen < 0) {
					write_gemini_response(ssl, STATUS_TEMPFAIL, 1, "I/O Error", 9, "", 0);
					log_access(vhost->accesslog, reqbuf, host, path, STATUS_TEMPFAIL, 1, 0, "-", "-");
					
					return -1;
				}
			}
		} else {
			/* path is file, maybe... */
			resbuf = malloc(statbuf.st_size + 1);
			mimelen = read_file_meta(localpath, mime);
			reslen = read_file(localpath, resbuf);
		}
		if(reslen < 1) {
			write_gemini_response(ssl, STATUS_TEMPFAIL, 1, "I/O Error", 9, "", 0);
			log_access(vhost->accesslog, reqbuf, host, path, STATUS_TEMPFAIL, 1, 0, "-", "-");
			
			return -1;
		}
		write_gemini_response(ssl, STATUS_SUCCESS, 0, mime, mimelen, resbuf, reslen);
		log_access(vhost->accesslog, reqbuf, host, path, STATUS_SUCCESS, 0, reslen, "-", "-");
		free(resbuf);
	} else {
		/* Local path is not readable. Currently, this simply means, that the file
		 * does not exist and we send the according response. But in the future
		 * this could mean more than that.
		 */
		write_gemini_response(ssl, STATUS_PERMFAIL, 1, "File not found", 14, "", 0);
		log_access(vhost->accesslog, reqbuf, host, path, STATUS_PERMFAIL, 1, 0, "-", "-");
	}

	
	return 1;
}

