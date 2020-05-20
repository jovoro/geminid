# Copyright (c) 2020 J. von Rotz <jr@vrtz.ch>
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
CFLAGS=-lconfig -lmagic -lssl -lcrypto 
DEBUGFLAGS=-g -DGEMINID_DEBUG
TLSFLAGS= # Use -DTLS_USE_V1_2_METHOD for older versions of OpenSSL, you can use Makefile.local for that
OBJ=main.o tls.o gemini.o util.o mime.o file.o log.o url.o lexurl.o config.o

-include Makefile.local

geminid: $(OBJ)
	$(CC) -o geminid $(DEBUGLAGS) $(CFLAGS) $(OBJ)

parseurl: parseurl.c url.o lexurl.o
	$(CC) -o parseurl $(DEBUGFLAGS) url.o lexurl.o parseurl.c

main.o: main.c
	$(CC) $(DEBUGFLAGS) -c main.c

tls.o: tls.c
	$(CC) $(DEBUGFLAGS) $(TLSFLAGS) -c tls.c

gemini.o: gemini.c
	$(CC) $(DEBUGFLAGS) -c gemini.c

util.o: util.c
	$(CC) $(DEBUGFLAGS) -c util.c

mime.o: mime.c
	$(CC) $(DEBUGFLAGS) -c mime.c

file.o: file.c
	$(CC) $(DEBUGFLAGS) -c file.c

log.o: log.c
	$(CC) $(DEBUGFLAGS) -c log.c

url.o: url.c
	$(CC) $(DEBUGFLAGS) -c url.c

lexurl.o: lexurl.c
	$(CC) $(DEBUGFLAGS) -c lexurl.c

lexurl.c: lexurl.l
	$(LEX) -o lexurl.c lexurl.l

config.o: config.c
	$(CC) $(DEBUGFLAGS) -c config.c

cert:
	openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes # nodes: No DES, do not prompt for pass

clean:
	rm -f *.o *.core lexurl.c geminid parseurl

veryclean: clean
	rm -f *.pem *.key *.mgc
