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
#include <stdlib.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "tls.h"
#include "vhost.h"

VHOST *current_vhost = NULL;

VHOST *create_vhost(const char *hostname, const char *docroot, const char *defaultdocument, const char *accesslog_path, const char *errorlog_path, const char *cert_public_path, const char *cert_private_path) {
	unsigned int i;
	VHOST *vhost;

	vhost = calloc(1, sizeof(VHOST));
	vhost->hostname = calloc(strlen(hostname)+1, sizeof(char));
	if(vhost->hostname == NULL)
		return NULL;

	strncpy(vhost->hostname, hostname, strlen(hostname)+1);

	vhost->docroot = calloc(strlen(docroot)+1, sizeof(char));
	if(vhost->docroot == NULL)
		return NULL;

	strncpy(vhost->docroot, docroot, strlen(docroot)+1);
	
	vhost->defaultdocument = calloc(strlen(defaultdocument)+1, sizeof(char));
	if(vhost->defaultdocument == NULL)
		return NULL;

	strncpy(vhost->defaultdocument, defaultdocument, strlen(defaultdocument)+1);

	vhost->accesslog = fopen(accesslog_path, "a");
	if(vhost->accesslog == NULL) {
		fprintf(stderr, "Cannot open access log: %s\n", accesslog_path);
		exit(EXIT_FAILURE);
        }


	vhost->errorlog = fopen(errorlog_path, "a");
	if(vhost->errorlog == NULL) {
		fprintf(stderr, "Cannot open error log: %s\n", errorlog_path);
		exit(EXIT_FAILURE);
	}

	vhost->ctx = create_context();
	if(vhost->ctx == NULL) {
		fprintf(stderr, "Error creating SSL context\n");
	}

	configure_context(vhost->ctx, cert_public_path, cert_private_path);
	return vhost;
}

void destroy_vhost(VHOST *vhost, unsigned int count) {
	unsigned int i;
	for(i=0; i < count; i++) {
		free((vhost+i)->hostname);
		free((vhost+i)->docroot);
		free((vhost+i)->defaultdocument);
		fclose((vhost+i)->accesslog);
		fclose((vhost+i)->errorlog);
		SSL_CTX_free((vhost+i)->ctx);
	}
	free(vhost);
}

int set_current_vhost(VHOST *vhost) {
	current_vhost = vhost;
	if(current_vhost == NULL)
		return -1;

	return 0;
}

VHOST *get_current_vhost() {
	return current_vhost;
}
