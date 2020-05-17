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
#include <string.h>
#include <time.h>
#include "gemini.h"
#include "log.h"

int log_access(FILE *lf, char *reqbuf, char *host, char *path, int status_major, int status_minor, long bytes, char *cc_issuer, char *cc_subject) {
	char timebuf[32];
	struct tm *sTm;
	time_t now;

	now = time(0);
	sTm = gmtime (&now);
	strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%S%z", sTm);
	return fprintf(lf, "%s %s %s %s %d%d %ld %s %s\n", timebuf, reqbuf, host, path, status_major, status_minor, bytes, cc_issuer, cc_subject);
}

int log_error(FILE *lf, char *logbuf) {
	char timebuf[32];
	struct tm *sTm;
	time_t now;

	now = time(0);
	sTm = gmtime (&now);
	strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%S%z", sTm);
	return fprintf(lf, "%s %s\n", timebuf, logbuf);
}

FILE *open_log(char *path) {
	FILE *lf;

	if(strncmp(path, "-", 1) == 0) 
		return stderr;

	return fopen(path, "a");
}

int close_log(FILE *lf) {
	if(lf == stderr)
		return 0;

	return fclose(lf);
}
