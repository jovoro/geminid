# geminid
A Gemini Server in C. Please note that this is not production-ready code.
The current state is a result of a few hours of hacking, barely able to even
serve content. But the goal is to have a compliant gemini server written in C.

## Configuring

There is no configuration file yet. Some tunable parameters are avaible in
gemini.h. For convenience you can use `make cert` to generate a self-signed
certificate.

## Prerequisites
A Unix-like or POSIX-compliant OS is required. OpenSSL 1.1.1 is recommended. If you build with an earlier version, I assume it is one which at least supports TLS 1.2. You need to define `TLS_USE_V1_2_METHOD` if you want to use TLS 1.2. If you want to use even older versions, you need to modify the source code in tls.c to use the appropriate version-specific method.

I've added a more sophisticated URL parser which is based on lex/flex, so you need that, too. 

## Building
Edit Makefile and gemini.h to your needs, do `make geminid`. There is a test program for the URL parser, which can be built with `make parseurl`

## Running
Just run the produced executable geminid. Some options are now configurable via command line parameters:
- `-a <path>`: Path to the access log file. Default: stderr
- `-c <path>`: Path to the SSL public key. Default: cert.pem
- `-d <file>`: Filename of the default document, which is loaded when a directory is requested. Default: index.gmi
- `-e <path>`: Path to the error log file. Default: stderr
- `-l <port>`: Use different listen port than 1965.
- `-p <path>`: Path to the SSL private key. Default: key.pem
- `-r <path>`: Path of the document root directory. Default is `docroot` (local to the process)

You can find a demonstration of it running at gemini://gemini.uxq.ch/
