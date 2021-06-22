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
#include <dirent.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "vhost.h"
#include "tls.h"
#include "gemini.h"

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

int create_socket6(int port) {
	int s;
	struct sockaddr_in6 addr;
	
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(port);
	addr.sin6_addr = in6addr_any;
	
	s = socket(AF_INET6, SOCK_STREAM, 0);
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
 
int load_trusted_client_certs(SSL_CTX *ctx, const char *client_certificate_location) {
	DIR *dp;
	FILE *fp;
	int i = 0;
	struct dirent *ep;
	struct stat statbuf;
	char certpath[MAXBUF];
	X509 *cert;
	
	dp = opendir(client_certificate_location);
	if(dp == NULL) {
		fprintf(stderr, "Error: Cannot open client certificate location: %s\n", client_certificate_location);
		exit(1);
	}
	
	while ((ep = readdir(dp)) != NULL) {
		snprintf(certpath, MAXBUF, "%s/%s", client_certificate_location, ep->d_name);
		if(stat(certpath, &statbuf) == 0) {
			if(S_ISDIR(statbuf.st_mode))
				continue;
			
			fp = fopen(certpath, "r");
			if(!fp)
				continue;
			
			cert = PEM_read_X509(fp, NULL, NULL, NULL);
			if(!cert) {
				fclose(fp);
				continue;
			}
			
			SSL_CTX_add_client_CA(ctx, cert);
			SSL_CTX_load_verify_locations(ctx, certpath, NULL);
			X509_free(cert);
			fclose(fp);
			i++;
		}
	}
	
	return i;
}

void configure_context(SSL_CTX *ctx, const char *cert_public_path, const char *cert_private_path, const char *client_certificate_location) {
	SSL_CTX_set_ecdh_auto(ctx, 1);
/*	if(load_trusted_client_certs(ctx, client_certificate_location) > 0) */
		SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verify_cb);
/*	else
		fprintf(stderr, "Warning: No trusted client certs loaded");
*/
	if (SSL_CTX_use_certificate_file(ctx, cert_public_path, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, cert_private_path, SSL_FILETYPE_PEM) <= 0 ) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
}

static int verify_cb(int preverify_ok, X509_STORE_CTX *ctx) {
	char buf[256];
	unsigned char fp[EVP_MAX_MD_SIZE];
	int fpsiz;
	int err;
	int depth;
	int j;
	X509 *err_cert;
	SSL *ssl;
	const EVP_MD *digest = EVP_get_digestbyname("sha1");

	return 1;

/* We might make use of the following code if we'd want to
 * add extra security by only allowing certificates that
 * we added to our store of trusted/known certs instead
 * of just relying on the fingerprint.
 * Note that it would be necessary to load all known
 * certs into the context by using load_trusted_client_certs()
 * in configure_context().
 */

/*
	err_cert = X509_STORE_CTX_get_current_cert(ctx);
	err = X509_STORE_CTX_get_error(ctx);
	depth = X509_STORE_CTX_get_error_depth(ctx);
	
	if (depth > 1) {
		preverify_ok = 0;
		err = X509_V_ERR_CERT_CHAIN_TOO_LONG;
		X509_STORE_CTX_set_error(ctx, err);
	}
	if(X509_digest(err_cert, digest, fp, &fpsiz)) {
		for (j=0; j<fpsiz; j++) {
			fprintf(stderr,"%02X%c", fp[j], (j+1 == fpsiz) ?'\n':':');
		}
	} else {
		fprintf(stderr, "Failed to calculate digest\n");
	}

	ssl = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
	X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);
	fprintf(stderr, "Cert Name: %s\n", buf);

	if (depth > 1) {
		preverify_ok = 0;
		err = X509_V_ERR_CERT_CHAIN_TOO_LONG;
		X509_STORE_CTX_set_error(ctx, err);
	}
	if(!preverify_ok)
		SSL_write(ssl, "62 Certificate not valid\r\n", 26);
	if (!preverify_ok) {
		printf("verify error:num=%d:%s:depth=%d:%s\n", err,
				 X509_verify_cert_error_string(err), depth, buf);
	}
	
	if (!preverify_ok && (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT)) {
	  X509_NAME_oneline(X509_get_issuer_name(X509_STORE_CTX_get_current_cert(ctx)), buf, 256);
	  printf("issuer= %s\n", buf);
	}
	
	return preverify_ok;
	*/
 }
