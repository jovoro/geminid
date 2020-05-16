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
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "tls.h"
#include "gemini.h"

int listen_port;
char document_root[MAXBUF];
char default_document[MAXBUF];

void initWorker(int client, SSL_CTX *ctx) {
        SSL *ssl;

        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        }
        else {
		handle_request(ssl);
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client);
}

void usage(char *progname) {
	fprintf(stderr, "Usage: %s [-d document] [-p port] [-r directory]\n", progname);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	int sock;
	int pid;
	int client;
	int true = 1;
	int opt;
	uint len;
	struct sockaddr_in addr;
	SSL_CTX *ctx;

	listen_port = LISTEN_PORT;
	strncpy(default_document, DEFAULT_DOCUMENT, MAXBUF);
	strncpy(document_root, DOCUMENT_ROOT, MAXBUF);

	while((opt = getopt(argc, argv, "d:p:r:")) != -1) {
		switch(opt) {
			case 'd':
				if(strlen(optarg) < 1)
					usage(argv[0]);

				strncpy(default_document, optarg, MAXBUF);
				break;

			case 'p':
				if(strlen(optarg) < 1)
					usage(argv[0]);

				sscanf(optarg, "%d", &listen_port);
				break;

			case 'r':
				if(strlen(optarg) < 1)
					usage(argv[0]);

				strncpy(document_root, optarg, MAXBUF);
				break;

			default:
				usage(argv[0]);
		}
	}

	fprintf(stderr, "default_document: %s\nlisten_port: %d\ndocument_root: %s\n\n", default_document, listen_port, document_root);
	
	init_openssl();
	ctx = create_context();
	configure_context(ctx);
	sock = create_socket(listen_port);
	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int));
	
	while(1) {
		len = sizeof(addr);
		client = accept(sock, (struct sockaddr*)&addr, &len);
		if (client < 0) {
			perror("Unable to accept");
			exit(EXIT_FAILURE);
		}
		if((pid = fork()) == 0) {
			// Child
			initWorker(client, ctx);
			exit(0);
		} else if (pid > 0) {
			// Parent
			fprintf(stderr, "Started child process %d\n", pid);
			close(client);
		} else {
			// Failed
			perror("Unable to fork");
		}
	}

	close(sock);
	SSL_CTX_free(ctx);
	cleanup_openssl();
}

