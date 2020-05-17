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
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "tls.h"
#include "gemini.h"
#include "log.h"

int listen_port;
char document_root[MAXBUF];
char default_document[MAXBUF];
char ssl_private_path[MAXBUF];
char ssl_public_path[MAXBUF];
static volatile int keepRunning = 1;

void initWorker(int client, SSL_CTX *ctx, FILE *access_log, FILE *error_log) {
        SSL *ssl;

        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        }
        else {
		handle_request(ssl, access_log, error_log);
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client);
}

void usage(char *progname) {
	fprintf(stderr, "Usage: %s [-a path] [-c path] [-e path]  [-d document] [-p port] [-r directory]\n\n", progname);
	fprintf(stderr, "\t-a path\n\t\tPath to the access log file\n\n");
	fprintf(stderr, "\t-c path\n\t\tPath to the SSL public key\n\n");
	fprintf(stderr, "\t-e path\n\t\tPath to the error log file\n\n");
	fprintf(stderr, "\t-d document\n\t\tName of the default document\n\n");
	fprintf(stderr, "\t-p port\n\t\tTCP port to listen on\n\n");
	fprintf(stderr, "\t-r directory\n\t\tDocument root directory\n\n");
	exit(EXIT_FAILURE);
}

void intHandler(int signal) {
	keepRunning = 0;
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
	char access_log_path[MAXBUF];
	char error_log_path[MAXBUF];
	char tmpbuf[MAXBUF];
	FILE *access_log;
	FILE *error_log;

	listen_port = LISTEN_PORT;
	strncpy(default_document, DEFAULT_DOCUMENT, MAXBUF);
	strncpy(document_root, DOCUMENT_ROOT, MAXBUF);
	strncpy(ssl_public_path, "cert.pem", MAXBUF);
	strncpy(ssl_private_path, "key.pem", MAXBUF);
	strncpy(access_log_path, "-", MAXBUF);
	strncpy(error_log_path, "-", MAXBUF);

	while((opt = getopt(argc, argv, "a:c:e:d:l:p:r:")) != -1) {
		switch(opt) {
			case 'a':
				if(strlen(optarg) < 1)
					usage(argv[0]);

				strncpy(access_log_path, optarg, MAXBUF);
				break;

			case 'c':
				if(strlen(optarg) < 1)
					usage(argv[0]);

				strncpy(ssl_public_path, optarg, MAXBUF);
				break;
				
			case 'e':
				if(strlen(optarg) < 1)
					usage(argv[0]);

				strncpy(error_log_path, optarg, MAXBUF);
				break;

			case 'd':
				if(strlen(optarg) < 1)
					usage(argv[0]);

				strncpy(default_document, optarg, MAXBUF);
				break;

			case 'l':
				if(strlen(optarg) < 1)
					usage(argv[0]);

				sscanf(optarg, "%d", &listen_port);
				break;

			case 'p':
				if(strlen(optarg) < 1)
					usage(argv[0]);

				strncpy(ssl_private_path, optarg, MAXBUF);
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

/*	signal(SIGINT, intHandler); */

	fprintf(stderr, "access_log_path: %s\nerror_log_path: %s\ndefault_document: %s\nlisten_port: %d\ndocument_root: %s\npublic key: %s\nprivate key: %s\n\n\n", access_log_path, error_log_path, default_document, listen_port, document_root, ssl_public_path, ssl_private_path);

	access_log = open_log(access_log_path);
	error_log = open_log(error_log_path);

	if(access_log == NULL) {
		fprintf(stderr, "Cannot open access log\n");
		exit(EXIT_FAILURE);
	}
	
	if(error_log == NULL) {
		fprintf(stderr, "Cannot open error log\n");
		exit(EXIT_FAILURE);
	}
	
	init_openssl();
	ctx = create_context();
	configure_context(ctx);
	sock = create_socket(listen_port);
	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int));
	
	while(keepRunning) {
		len = sizeof(addr);
		client = accept(sock, (struct sockaddr*)&addr, &len);
		if (client < 0) {
			perror("Unable to accept");
			exit(EXIT_FAILURE);
		}
		if((pid = fork()) == 0) {
			// Child
			initWorker(client, ctx, access_log, error_log);
			exit(0);
		} else if (pid > 0) {
			// Parent
			snprintf(tmpbuf, MAXBUF, "Started child process %d", pid);
			log_error(error_log, tmpbuf);
			close(client);
		} else {
			// Failed
			perror("Unable to fork");
		}
	}
	
	close(sock);
	SSL_CTX_free(ctx);
	cleanup_openssl();
	close_log(access_log);
	close_log(error_log);
}

