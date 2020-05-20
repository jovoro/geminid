#define MAX_VHOST 1024

typedef struct {
	const char *serverroot;
	const char *logdir;
	int port;
	const char *loglocaltime;
	const char *logtimeformat;
} GLOBALCONF;

typedef struct {
	const char *name;
	const char *docroot;
	const char *accesslog;
	const char *errorlog;
	const char *cert;
	const char *key;
	const char *index;
} VHOSTCONF;

typedef struct {
	unsigned int count;
	VHOSTCONF *vhost;
} VHOSTLIST;

int init_geminid_config(char *configpath, config_t *cfg, GLOBALCONF **global, VHOSTLIST **vhostlist);
int testprintconfig(char *configpath);
