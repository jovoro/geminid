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
#include "vhost.h"

int listen_port;
char server_root[MAXBUF];
char log_time_format[MAXBUF];
short log_local_time;
static volatile int keepRunning = 1;
VHOST *vhost;
unsigned int vhostcount = 0;

void initWorker(int client) {
	int i;
	char document_root[MAXBUF];
        SSL *ssl;
	VHOST *select_vhost;

	SSL_CTX_set_tlsext_servername_callback(vhost->ctx, sni_cb);
        ssl = SSL_new(vhost->ctx); /* Use first vhost as default context */
        SSL_set_fd(ssl, client);

        if (SSL_accept(ssl) <= 0) {
		ERR_print_errors_fp(stderr);
        } else {
		select_vhost = get_current_vhost();
		if(select_vhost == NULL) {
			fprintf(stderr, "Cannot get current vhost\n");
			return;
		}
		
		snprintf(document_root, MAXBUF-1, "%s/%s", server_root, select_vhost->docroot);
		handle_request(ssl, document_root, select_vhost->defaultdocument, select_vhost->accesslog, select_vhost->errorlog);
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

int main(int argc, char **argv) {
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
	GLOBALCONF *global;
	VHOSTLIST *vhostlist;
	VHOSTCONF *vhcp;
	VHOST *vhp;
	VHOST *tempvhp;
	config_t cfg;
	unsigned int i;

	while((opt = getopt(argc, argv, "c:t")) != -1) {
		switch(opt) {
			case 'c':
				if(strlen(optarg) < 1)
					usage(argv[0]);

				snprintf(configpath, MAXBUF-1, "%s", optarg);
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

	/* Prepare configuration */
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

	init_openssl();

	/* Global configuration */
	strncpy(server_root, global->serverroot, MAXBUF-1);
	listen_port = global->port;
	snprintf(log_time_format, MAXBUF-1, "%s", global->logtimeformat);
	if(strncmp(global->loglocaltime, "yes", 3) == 0)
		log_local_time = 1;
	else
		log_local_time = 0;

	/* Configuration per virtual host */
	vhostcount = vhostlist->count;
	vhost = calloc(vhostcount, sizeof(VHOST));
	if(vhost == NULL) {
		fprintf(stderr, "Error allocating vhost struct\n");
		exit(EXIT_FAILURE);
	}

	vhcp = vhostlist->vhost;
	vhp = vhost;
	for(i=0; i < vhostcount; i++) {
		snprintf(accesslog_path, MAXBUF-1, "%s/%s", global->logdir, vhcp->accesslog);
		snprintf(errorlog_path, MAXBUF-1, "%s/%s", global->logdir, vhcp->errorlog);
		tempvhp = create_vhost(vhcp->name, vhcp->docroot, vhcp->index, accesslog_path, errorlog_path, vhcp->cert, vhcp->key);
		if(tempvhp == NULL) {
			fprintf(stderr, "Error allocating vhost struct\n");
			exit(EXIT_FAILURE);
		}
		*vhp = *tempvhp;
		free(tempvhp);
			
		/* Print configuration settings */
		fprintf(stderr, "serverroot: %s\nlogdir: %s\nhostname: %s\naccess_log_path: %s\nerror_log_path: %s\ndefault_document: %s\nlisten_port: %d\ndocument_root: %s\npublic key: %s\nprivate key: %s\n\n\n", server_root, global->logdir, vhcp->name, accesslog_path, errorlog_path, vhcp->index, global->port, vhcp->docroot, vhcp->cert, vhcp->key);
		vhcp++;
		vhp++;
	}
	i = set_current_vhost(vhost);
	if(i < 0) {
		fprintf(stderr, "Cannot set current (default) vhost\n");
		exit(EXIT_FAILURE);
	}


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
			initWorker(client);
			exit(0);
		} else if (pid > 0) {
			// Parent
			close(client);
		} else {
			// Failed
			perror("Unable to fork");
		}
	}

	destroy_vhost(vhost, vhostcount);	
	close(sock);
	cleanup_openssl();
	
	free(global);
	free(vhostlist);
	config_destroy(&cfg);
}

