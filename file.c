/*
 * Copyright (c) 2020, J. von Rotz
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <magic.h>
#include <dirent.h>
#include <sys/stat.h>
#include "gemini.h"
#include "file.h"

int read_file_meta(char *path, char *buffer) {
	int i;
	int pathlen;
	int pathpos;
	char tmpbuf[MAXBUF];
	magic_t magic;
	const char *magicstr;

	/* Assume text/gemini for *.gmi files */
	i = 0;
	pathlen = strlen(path);
	pathpos = pathlen-4;
	while(i < pathlen) {
		tmpbuf[i] = path[pathpos+i];
		i++;
	}
	tmpbuf[i] = '\0';
	
	if(strncmp(tmpbuf, ".gmi", 4) != 0) {
		magic = magic_open(MAGIC_MIME_TYPE);
		magic_load(magic, NULL);
		magic_compile(magic, NULL);
		magicstr = magic_file(magic, path);
	
		if(magicstr == NULL)
			strncpy(buffer, "text/gemini; charset=utf-8", 26);
		else
			strncpy(buffer, magicstr, MAXBUF);

		magic_close(magic);
	} else {
		strncpy(buffer, "text/gemini", 11);
	}
	return strlen(buffer);
}

int read_file(char *path, void *buffer) {
	FILE *fh;
	long fsize;
	size_t result;

	fh = fopen(path, "r");
	if(fh == NULL)
		return -1;

	fseek(fh, 0, SEEK_END);
	fsize = ftell(fh);
	fseek(fh, 0, SEEK_SET);
	if(buffer == NULL)
		return -1;

	result = fread(buffer, sizeof(char), fsize, fh);
	fclose(fh);

	return result;
}

int read_directory(char *path, char *requesturl, void *buffer) {
	DIR *dp;
	struct stat statbuf;
	struct dirent *ep;
	int chars_avail = MAXBUF-1;
	int i;
	char localpath[MAXBUF];
	char tmpbuf[MAXBUF];

	snprintf(localpath, MAXBUF, "%s/%s", document_root, path);

	dp = opendir(localpath);
	if(dp == NULL)
		return -1;

	memset(buffer, 0, MAXBUF);

	strncat(buffer, "# Directory listing of ", 23);

	chars_avail -= 23;
	strncat(buffer, path, chars_avail);
	chars_avail -= strlen(path);
	if(chars_avail < 1)
		return -1;
	
	strncat(buffer, "\r\n", 2);
	chars_avail -= 2;	

	while ((ep = readdir(dp)) != NULL) {
		if(strncmp(ep->d_name, ".", 1) == 0 || strncmp(ep->d_name, "..", 2) == 0)
			continue;

		if(chars_avail < 1)
			return -1;

		snprintf(tmpbuf, MAXBUF, "%s/%s", localpath, ep->d_name);
		if((i = stat(tmpbuf, &statbuf)) == 0) {
			strncat(buffer, "=> ", 3);
			chars_avail -= 3;
			strncat(buffer, requesturl, strlen(requesturl));
			chars_avail -= strlen(requesturl);
			strncat(buffer, ep->d_name, strlen(ep->d_name));
			chars_avail -= strlen(ep->d_name);
			if(S_ISDIR(statbuf.st_mode)) {
				strncat(buffer, "/", 1);
				chars_avail -= 1;
			}
			strncat(buffer, " ", 1);
			chars_avail -= 1;
			strncat(buffer, ep->d_name, strlen(ep->d_name));
			chars_avail -= strlen(ep->d_name);
			if(S_ISDIR(statbuf.st_mode)) {
				strncat(buffer, "/", 1);
				chars_avail -= 1;
			}
			strncat(buffer, "\r\n", 2);
			chars_avail -= 2;
		}
	}
	closedir(dp);

	return MAXBUF - chars_avail - 1;
 
}
