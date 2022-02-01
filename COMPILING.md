# compile tools

To compile utmp-cli from source, you need the following set of software:
* C compiler, e.g. gcc
* C Library, e.g. libc
* make 

## Debian GNU/Linux
All needed tools could be installed by running:
```
apt-get install gcc libc6-dev make
```

## Windows
Easiest method is to install gcc and make using MSYS2. Since Windows code uses snprintf, in Makefile update C standard to `c99` and append `-mwin32`. So that compiling options are
```
OPTS := -O2 -s -std=c99 -I./include -mwin32
```