# geminid
A Gemini Server in C. Please note that this is not production-ready code.
The current state is a result of a few hours of hacking, barely able to even
serve content. But the goal is to have a compliant gemini server written in C.

## Prerequisites
A Unix-like or POSIX-compliant OS is required. OpenSSL 1.1.1 is recommended. If you build with an earlier version, I assume it is one which at least supports TLS 1.2. You need to define `TLS_USE_V1_2_METHOD` if you want to use TLS 1.2. If you want to use even older versions, you need to modify the source code in tls.c to use the appropriate version-specific method. OpenSSL 3.0 has been tested successfully.

I've added a more sophisticated URL parser which is based on lex/flex, so you need that, too. As of 2020-05-20 there is a new configuration file introduced which requires libconfig as a dependency.

## Building
Edit Makefile and gemini.h to your needs, do `make geminid`. There is a test program for the URL parser, which can be built with `make parseurl`. If you have non-standard include or library paths, you can use `INCDIRS` and `LIBDIRS`, respectively. You are advised to include local modifications in a separate file Makefile.local, which will be included if it exists.

So, if you are on BSD, you might want to do
```
cat > Makefile.local <<EOF
INCDIRS=-I/usr/local/include
LIBDIRS=-L/usr/local/lib
EOF
```
to let the preprocessor and linker know where to look for libraries, i.e. libconfig.

If you are running CentOS 7 you may want to install and use the separately packaged version of OpenSSL 1.1.x, as mentioned by Jaroslaw Zachwieja:

```
# yum install gcc openssl11-devel file-devel libconfig-devel flex
$ cat > Makefile.local <<EOF
INCDIRS=-I/usr/include/openssl11
LIBDIRS=-L/usr/lib64/openssl11
EOF
```

Depending on your compiler you might want to edit the CFLAGS, I've recently changed them to include all warnings, so if -Wall, -Wextra or -pedantic is not supported by your compiler, just remove those flags.

## Configuring
There's an example configuration file named `example.conf` in this repo. You can define multiple virtual hosts, of which the first definition is the default vhost. The default vhost is used if no servername is defined during the TLS handshake or if no vhost definition matches the provided hostname. Each vhost needs a separate TLS certificate.

The format of log times is described according to `strftime(3)`. The `docroot` directive is relative to the `serverroot`. Log files are relative to `logdir`. Certificates are absolute or relative to the cwd of the geminid process, since they tend to live anywhere on the filesystem. I don't know if that makes sense to you. If it doesn't, let me know - I'd love to hear your thoughts.

There is the possibility to use IPv6. Only one address will be bound, though. If you want to use both IPv4 and IPv6 use have to use 4in6. On some systems this needs to be enabled explicitly, for example on FreeBSD you'd have to change the following sysctl parameter: `net.inet6.ip6.v6only=0`

## Running
Just run the produced executable geminid. Some options are now configurable via command line parameters:
- `-c <config>`: Path to the configuration file
- `-t`: Test and print configuration

## Complaining
To vent your anger, you may reach me at jr at vrtz dot ch.

You can find a demonstration of it running at gemini://gemini.uxq.ch/ and some more information on how I run it at gemini://gemini.uxq.ch/running.gmi
