# Copyright (c) 2020, 2021, 2022 J. von Rotz <jr@vrtz.ch>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
# 
# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived
# from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 

CC=/usr/bin/cc
LEX=/usr/bin/lex
LDFLAGS=-lconfig -lmagic -lssl -lcrypto 
CFLAGS=-Wall -Wextra -pedantic
DEBUGFLAGS=-g -DGEMINID_DEBUG
TLSFLAGS= # Use -DTLS_USE_V1_2_METHOD for older versions of OpenSSL, you can use Makefile.local for that
OBJ=main.o tls.o gemini.o util.o mime.o file.o log.o url.o lexurl.o config.o vhost.o
INCDIRS=
LIBDIRS=

-include Makefile.local

all: geminid

geminid: $(OBJ)
	$(CC) $(CFLAGS) $(LIBDIRS) $(DEBUGFLAGS) -o geminid $(LDFLAGS) $(OBJ)

parseurl: parseurl.c url.o lexurl.o
	$(CC) $(CFLAGS) $(LIBDIRS) $(DEBUGFLAGS) -o parseurl url.o lexurl.o parseurl.c

tls.o: tls.c
	$(CC) $(CFLAGS) $(INCDIRS) $(DEBUGFLAGS) $(TLSFLAGS) -c tls.c

%.o: %.c 
	$(CC) $(CFLAGS) $(INCDIRS) $(DEBUGFLAGS) -o $@ -c $<

lexurl.c: lexurl.l
	$(LEX) -o lexurl.c lexurl.l

cert:
	openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes # nodes: No DES, do not prompt for pass

clean:
	rm -f *.o *.core lexurl.c geminid parseurl

veryclean: clean
	rm -f *.pem *.key *.mgc
