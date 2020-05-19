#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef __linux__
#include <sys/malloc.h>
#endif /* ! __linux__ */
#include "url.h"

int main(int argc, char **argv) {
	URL *test;

	if(argc < 2) {
		fprintf(stderr, "Usage: %s <URL>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	printf("input: %s\n", argv[1]);
	test = alexurl(argv[1]);

	printf("scheme: %s\n", test->scheme);
	printf("userinfo: %s\n", test->userinfo);
	printf("host: %s\n", test->host);
	printf("port: %s\n", test->port);
	printf("path: %s\n", test->path);
	printf("query: %s\n", test->query);
	printf("fragment: %s\n", test->fragment);

	free(test);
	exit(EXIT_SUCCESS);
}


