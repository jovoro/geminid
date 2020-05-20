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

I've added a more sophisticated URL parser which is based on lex/flex, so you need that, too. As of 2020-05-20 there is a new configuration file introduced which requires libconfig as a dependency.

## Building
Edit Makefile and gemini.h to your needs, do `make geminid`. You can include local modifications in Makefile.local, which will be included if it exists. There is a test program for the URL parser, which can be built with `make parseurl`

## Running
Just run the produced executable geminid. Some options are now configurable via command line parameters:
- `-c <config>`: Path to the configuration file
- `-t`: Test and print configuration

You can find a demonstration of it running at gemini://gemini.uxq.ch/
