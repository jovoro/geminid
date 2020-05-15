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
CFLAGS=-lmagic -lssl -lcrypto
DEBUGFLAGS=-g -DGEMINID_DEBUG
OBJ=main.o tls.o gemini.o util.o mime.o

geminid: $(OBJ)
	$(CC) -o geminid $(DEBUGLAGS) $(CFLAGS) $(OBJ)

main.o: main.c
	$(CC) $(DEBUGFLAGS) -c main.c

tls.o: tls.c
	$(CC) $(DEBUGFLAGS) -c tls.c

gemini.o: gemini.c
	$(CC) $(DEBUGFLAGS) -c gemini.c

util.o: util.c
	$(CC) $(DEBUGFLAGS) -c util.c

mime.o: mime.c
	$(CC) $(DEBUGFLAGS) -c mime.c

cert:
	openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes # nodes: No DES, do not prompt for pass

clean:
	rm -f *.o *.core geminid

veryclean: clean
	rm -f *.pem *.key *.mgc
