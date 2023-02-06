# A HTTP/3 client demo based on [QUIC](https://quicwg.org) library [msquic](https://github.com/microsoft/msquic)

## Platforms

GNU/Linux

## Interoperability

With [nginx-quic](https://hg.nginx.org/nginx-quic).

### Note

Build [nginx-quic](https://hg.nginx.org/nginx-quic), please see [README](https://quic.nginx.org/README) for details.

## Prerequisites

* [QUIC](https://quicwg.org) library [msquic](https://github.com/microsoft/msquic).

* [cmake](https://cmake.org) or PowerShell (See **BUILD.md** in **docs** directory in [msquic](https://github.com/microsoft/msquic)).

* g++ for compilation on GNU/Linux.

## Build

Edit INC\_DIR and LIB\_DIR in Makefile according to your library directory and make.

## Usage

### Note

If library file **libmsquic.so** is not installed in the standard paths, _/usr/lib_ for example, the environment variable `LD_LIBRARY_PATH` must be set (change `path_to_your_library` to your library directory), or program can not load the library:

    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:path_to_your_library

In fact, **libmsquic.so** is a soft link to the real library file **libmsquic.so.2**, so make sure the real library file does exist and the soft link to it is correct. 

### Run

    ./H3Client[-h host -p port]
