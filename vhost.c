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
