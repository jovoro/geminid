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

#include "log.h"

static struct {
	const char *time_format;
	struct tm * (*broken_down)(const time_t *, struct tm *);
} g_logconfig = {
	.time_format = "[%d/%b/%Y:%H:%M:%S %z]",
	.broken_down = gmtime_r,
};

int log_setup(const LOGCONFIG *logconfig) {
	g_logconfig.time_format = logconfig->time_format;
	g_logconfig.broken_down = logconfig->use_local_time
							? localtime_r
							: gmtime_r
							;
	return 0;
}

static const char *current_time(char *buffer, size_t buflen) {
	struct tm tm;
	time_t t;

	if (time(&t) == -1) {
		perror("time");
		return "???";
	}

	if (g_logconfig.broken_down(&t, &tm) == NULL) {
		perror("gmtime_r / localtime_r");
		return "???";
	}

	if (strftime(buffer, buflen, g_logconfig.time_format, &tm) == 0) {
		fprintf(stderr, "stftime failed to format time");
		return "???";
	}

	return buffer;
}

void log_access(FILE *lf, const LOG_ACCESS_ENTRY *entry) {
	char timebuf[32];

	// NOTE 2021-03-29 dacav@fastmail.com:
	//  The last two fields are used to be the expansion of two
	//  Ttring variables named `cc_issuer` and `cc_subject`.  All
	//  invocations of `log_access` in the original code passed the "-"
	//  string for both.
	fprintf(lf, "%s %s %s %s %d%d %ld - -\n",
		current_time(timebuf, sizeof(timebuf)),
		entry->request ?: "-",
		entry->host ?: "",
		entry->path ?: "",
		entry->status.major,
		entry->status.minor,
		entry->response_length
	);
}

int log_error(FILE *lf, char *logbuf) {
	char timebuf[32];

	return fprintf(lf, "%s %s\n", current_time(timebuf, sizeof(timebuf)), logbuf);
}

FILE *open_log(const char *path) {
	if(strncmp(path, "-", 1) == 0) 
		return stderr;

	return fopen(path, "a");
}

int close_log(FILE *lf) {
	if(lf == stderr)
		return 0;

	return fclose(lf);
}
