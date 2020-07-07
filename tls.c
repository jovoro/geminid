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
#include "tls.h"
#include "gemini.h"
#include "vhost.h"
extern VHOST *vhost;
extern unsigned int vhostcount;

int create_socket(int port) {
	int s;
	struct sockaddr_in addr;
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		perror("Unable to create socket");
		exit(EXIT_FAILURE);
	}

	if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("Unable to bind");
		exit(EXIT_FAILURE);
	}
	
	if (listen(s, 1) < 0) {
		perror("Unable to listen");
		exit(EXIT_FAILURE);
	}
	
	return s;
}	
	
void init_openssl() {
	SSL_load_error_strings();	
	OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
	EVP_cleanup();
}

SSL_CTX *create_context() {
	const SSL_METHOD *method;
	SSL_CTX *ctx;
#ifdef TLS_USE_V1_2_METHOD
	method = TLSv1_2_server_method();
#else
	method = TLS_server_method();
#endif /*TLS_USE_V1_2_METHOD*/

	ctx = SSL_CTX_new(method);
	if (!ctx) {
		perror("Unable to create SSL context");
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	return ctx;
}

int sni_cb(SSL *ssl, int *ad, void *arg) {
	const char *servername;
	unsigned int i;
	int vr;
	SSL_CTX *r;

	if(ssl == NULL)
		return SSL_TLSEXT_ERR_NOACK;

	servername = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
	
	if (!servername || servername[0] == '\0')
		return SSL_TLSEXT_ERR_NOACK;

	for(i=0; i < vhostcount; i++) {
		if(strcmp((vhost+i)->hostname, servername) == 0) {
			r = SSL_set_SSL_CTX(ssl, (vhost+i)->ctx);
			if(r == NULL)
				SSL_TLSEXT_ERR_NOACK;

			vr = set_current_vhost((vhost+i));
			if(vr != 0)
				SSL_TLSEXT_ERR_NOACK;

			return SSL_TLSEXT_ERR_OK;
		}
	}

	return SSL_TLSEXT_ERR_NOACK;
}

void configure_context(SSL_CTX *ctx, const char *cert_public_path, const char *cert_private_path) {
	SSL_CTX_set_ecdh_auto(ctx, 1);

	if (SSL_CTX_use_certificate_chain_file(ctx, cert_public_path) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, cert_private_path, SSL_FILETYPE_PEM) <= 0 ) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
}
