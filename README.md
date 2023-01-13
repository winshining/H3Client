# A HTTP/3 client demo based on [QUIC](https://quicwg.org) library [msquic](https://github.com/microsoft/msquic)

## Platforms

* GNU/Linux.

## Interoperability

* With [nginx-quic](https://hg.nginx.org/nginx-quic).

## Prerequisites

* [QUIC](https://quicwg.org) library [msquic](https://github.com/microsoft/msquic).

* [cmake](https://cmake.org) or PowerShell (See BUILD steps in the above library).

* g++ for compilation on GNU/Linux.

## Build

* Edit INC\_DIR and LIB\_DIR in Makefile according to your library directory and make.

## Usage

* ./H3Client[-h host -p port]
