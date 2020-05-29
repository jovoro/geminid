typedef struct {
	char *hostname;
	char *docroot;
	char *defaultdocument;
	SSL_CTX *ctx;
	FILE *accesslog;
	FILE *errorlog;
} VHOST;

VHOST *create_vhost(char *hostname, char *docroot, char *defaultdocument, char *accesslog_path, char *errorlog_path, char *cert_public_path, char *cert_private_path);
void destroy_vhost(VHOST *vhost, unsigned int count);
int set_current_vhost(VHOST *vhost);
VHOST *get_current_vhost();
