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
#include <stdlib.h>
#ifndef __linux__
#include <sys/malloc.h>
#endif /* ! __linux__ */
#include <libconfig.h>
#include "config.h"

GLOBALCONF *new_globalconf(config_t *cfg) {
	config_setting_t *setting;
	GLOBALCONF *global;

	global = calloc(1, sizeof(*global));

	if(global == NULL)
		return NULL;

	
	setting = config_lookup(cfg, "global");
	if(setting != NULL) {
		if(!(config_setting_lookup_string(setting, "serverroot", &(global->serverroot))
		  && config_setting_lookup_string(setting, "logdir", &(global->logdir))
		  && config_setting_lookup_string(setting, "loglocaltime", &(global->loglocaltime))
		  && config_setting_lookup_string(setting, "logtimeformat", &(global->logtimeformat))
		  && config_setting_lookup_int(setting, "port", &(global->port)))) {
			fprintf(stderr, "Failed parsing config\n");
			return NULL;
		}
		if(!config_setting_lookup_bool(setting, "ipv6_enable", &(global->ipv6_enable)))
			global->ipv6_enable = 0;
	}
	return global;
}

VHOSTLIST *new_vhostlist(config_t *cfg) {
	config_setting_t *setting;
	VHOSTLIST *vhostlist;
	VHOSTCONF *firstvhost;
	unsigned int count;
	unsigned int i;

	vhostlist = calloc(1, sizeof(*vhostlist));
	setting = config_lookup(cfg, "vhost");
	if(setting != NULL) {
		vhostlist->count = config_setting_length(setting);
		vhostlist->vhost = calloc(vhostlist->count, sizeof(VHOSTCONF));
		firstvhost = vhostlist->vhost;

		for(i = 0; i < vhostlist->count; ++i) {
			config_setting_t *vhost = config_setting_get_elem(setting, i);

			if(!(config_setting_lookup_string(vhost, "name", &(vhostlist->vhost->name))
		           && config_setting_lookup_string(vhost, "docroot", &(vhostlist->vhost->docroot))
		           && config_setting_lookup_string(vhost, "accesslog", &(vhostlist->vhost->accesslog))
		           && config_setting_lookup_string(vhost, "errorlog", &(vhostlist->vhost->errorlog))
		           && config_setting_lookup_string(vhost, "cert", &(vhostlist->vhost->cert))
		           && config_setting_lookup_string(vhost, "key", &(vhostlist->vhost->key))
		           && config_setting_lookup_string(vhost, "index", &(vhostlist->vhost->index)))) {
				fprintf(stderr, "Failed parsing config\n");
				return NULL;
			}
			
			vhostlist->vhost++;
		}
		vhostlist->vhost = firstvhost;
	} else {
		fprintf(stderr, "no vhost definitions found.\n");
		return NULL;
	}

	
	return vhostlist;
}

int init_geminid_config(char *configpath, config_t *cfg, GLOBALCONF **global, VHOSTLIST **vhostlist) {
	config_init(cfg);
	if(! config_read_file(cfg, configpath)) {
		fprintf(stderr, "%s:%d - %s\n", config_error_file(cfg),
		config_error_line(cfg), config_error_text(cfg));
		return -1;
	}

	*global = new_globalconf(cfg);
	*vhostlist = new_vhostlist(cfg);
	if(global == NULL)
		return -1;

	return 1;
}


int testprintconfig(char *configpath) {
	GLOBALCONF *global;
	VHOSTLIST *vhostlist;
	VHOSTCONF *firstvhost;
	config_t cfg;
	unsigned int i;

	if(init_geminid_config(configpath, &cfg, &global, &vhostlist) < 1) {
		config_destroy(&cfg);
		exit(EXIT_FAILURE);
	}

	firstvhost = vhostlist->vhost;	
	fprintf(stderr, "Global config:\n-------------\nserverroot: %s\nlogdir: %s\nport: %d\n\n", global->serverroot, global->logdir, global->port);
	for(i=0; i<vhostlist->count; i++) {
		fprintf(stderr, "vHost %d:\n------\nname: %s\ndocroot: %s\naccesslog: %s\nerrorlog: %s\ncert: %s\nkey: %s\nindex: %s\n\n", i, vhostlist->vhost->name, vhostlist->vhost->docroot, vhostlist->vhost->accesslog, vhostlist->vhost->errorlog, vhostlist->vhost->cert, vhostlist->vhost->key, vhostlist->vhost->index);
		vhostlist->vhost++;
	}

	free(global);
	free(firstvhost);
	free(vhostlist);
	config_destroy(&cfg);
	return(EXIT_SUCCESS);
}
