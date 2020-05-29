typedef struct {
	char *hostname;
	char *docroot;
	char *defaultdocument;
	SSL_CTX *ctx;
	FILE *accesslog;
	FILE *errorlog;
} VHOST;

VHOST *create_vhost(const char *hostname, const char *docroot, const char *defaultdocument, const char *accesslog_path, const char *errorlog_path, const char *cert_public_path, const char *cert_private_path);
void destroy_vhost(VHOST *vhost, unsigned int count);
int set_current_vhost(VHOST *vhost);
VHOST *get_current_vhost();
