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

%option noyywrap

%{
#ifdef __linux__
#include <stdlib.h>
#else
#include <sys/malloc.h>
#endif /* __linux__ */
#include "url.h"

#define YYSTYPE URL
URL yylval;
%}

%% 
      
[a-z]{1,}:\/\/ { strncpy(yylval.scheme, yytext, MAX_URL_SCHEME-1); }
[a-z0-9]{1,}(:[a-z0-9]*)?@ { strncpy(yylval.userinfo, yytext, MAX_URL_USERINFO-1); }
[a-zA-Z0-9.-]{2,} { strncpy(yylval.host, yytext, MAX_URL_HOST-1); }
:[0-9]{1,5} { strncpy(yylval.port, yytext, MAX_URL_PORT-1); }
\/[^?]* { strncpy(yylval.path, yytext, MAX_URL_PATH-1); }
\?[a-zA-Z0-9=&]* { strncpy(yylval.query, yytext, MAX_URL_QUERY-1); }
#[a-zA-Z0-9]+ { strncpy(yylval.fragment, yytext, MAX_URL_FRAGMENT-1); }
  
%%

int lexurl(URL *url, char *buf) {
	if(url == NULL)
		return -1;

	yy_scan_string(buf);
	yylex();
	*url = yylval;

	return 1;	
}

URL *alexurl(char *buf) {
	URL *url;
	url = calloc(1, sizeof(*url));

	if(url == NULL)
		return NULL;

	yy_scan_string(buf);
	yylex();
	*url = yylval;

	return url;	
}
