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
#include <libconfig.h>
#include "tls.h"
#include "gemini.h"
#include "log.h"
#include "config.h"

int listen_port;
char server_root[MAXBUF];
char document_root[MAXBUF];
char default_document[MAXBUF];
char ssl_private_path[MAXBUF];
char ssl_public_path[MAXBUF];
char log_time_format[MAXBUF];
short log_local_time;
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
	fprintf(stderr, "Usage: %s -c config [-t]\n\n", progname);
	fprintf(stderr, "\t-c config\n\t\tPath to configuration file\n\n");
	fprintf(stderr, "\t-t\n\t\tTest and print configuration\n\n");
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
	char tmpbuf[MAXBUF];
	char configpath[MAXBUF];
	char accesslog_path[MAXBUF];
	char errorlog_path[MAXBUF];
	FILE *access_log;
	FILE *error_log;
	GLOBALCONF *global;
	VHOSTLIST *vhostlist;
	config_t cfg;
	unsigned int i;

	while((opt = getopt(argc, argv, "c:t")) != -1) {
		switch(opt) {
			case 'c':
				if(strlen(optarg) < 1)
					usage(argv[0]);

				snprintf(configpath, MAXBUF, "%s", optarg);
				break;

			case 't':
				if(strncmp(configpath, "", MAXBUF) == 0)
					usage(argv[0]);

				testprintconfig(configpath);
				exit(EXIT_SUCCESS);

			default:
				usage(argv[0]);
		}
	}

	if(strncmp(configpath, "", MAXBUF) == 0)
		usage(argv[0]);

/*	signal(SIGINT, intHandler); */

	/* Prepare configuration;
	 * We currently only honor the first vhost, since we haven't implemented
	 * SNI yet.
	 */
	if(init_geminid_config(configpath, &cfg, &global, &vhostlist) < 1) {
		config_destroy(&cfg);
		fprintf(stderr, "Cannot parse config.\n");
		exit(EXIT_FAILURE);
	}

	if(global == NULL || global == NULL) {
		config_destroy(&cfg);
		fprintf(stderr, "Cannot parse config.\n");
		exit(EXIT_FAILURE);
	}

	if(vhostlist->count < 1) {
		config_destroy(&cfg);
		fprintf(stderr, "No vhosts defined.\n");
		exit(EXIT_FAILURE);
	}

	if(vhostlist->count > 1) {
		fprintf(stderr, "Please note that currently only the first vhost entry will be honored, since we haven't implemented SNI yet.\n");
	}

	/* Copy configuration to global variables;
	 * This is hopefully not necessary in the future
	 * as we will pass the configuration to each vhost and
	 * worker eventually
	 */
	strncpy(server_root, global->serverroot, MAXBUF-1);
	strncpy(ssl_public_path, vhostlist->vhost->cert, MAXBUF-1);
	strncpy(ssl_private_path, vhostlist->vhost->key, MAXBUF-1);
	strncpy(default_document, vhostlist->vhost->index, MAXBUF-1);
	snprintf(document_root, MAXBUF-1, "%s/%s", server_root, vhostlist->vhost->docroot);
	snprintf(accesslog_path, MAXBUF-1, "%s/%s", global->logdir, vhostlist->vhost->accesslog);
	snprintf(errorlog_path, MAXBUF-1, "%s/%s", global->logdir, vhostlist->vhost->errorlog);
	listen_port = global->port;
	snprintf(log_time_format, MAXBUF-1, "%s", global->logtimeformat);
	if(strncmp(global->loglocaltime, "yes", 3) == 0)
		log_local_time = 1;
	else
		log_local_time = 0;

	/* Print configuration settings */
	fprintf(stderr, "serverroot: %s\nlogdir: %s\nhostname: %s\naccess_log_path: %s\nerror_log_path: %s\ndefault_document: %s\nlisten_port: %d\ndocument_root: %s\npublic key: %s\nprivate key: %s\n\n\n", server_root, global->logdir, vhostlist->vhost->name, vhostlist->vhost->accesslog, vhostlist->vhost->errorlog, default_document, global->port, vhostlist->vhost->docroot, vhostlist->vhost->cert, vhostlist->vhost->key);

	access_log = open_log(accesslog_path);
	error_log = open_log(errorlog_path);

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

	/* Auto-reap zombies for now.
	 * Maybe refine that with a real signal handler later 
	 * to get exit codes from children.
	 */
	signal(SIGCHLD, SIG_IGN);
	
	while(keepRunning) {
		len = sizeof(addr);
		client = accept(sock, (struct sockaddr*)&addr, &len);
		if (client < 0) {
			perror("Unable to accept");
			exit(EXIT_FAILURE);
		}
		if((pid = fork()) == 0) {
			// Child
			close(sock);
			initWorker(client, ctx, access_log, error_log);
			exit(0);
		} else if (pid > 0) {
			// Parent
			close(client);
			snprintf(tmpbuf, MAXBUF, "Started child process %d", pid);
			log_error(error_log, tmpbuf);
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
	
	free(global);
	free(vhostlist);
	config_destroy(&cfg);
}

