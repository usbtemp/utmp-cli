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
Easiest method is to install gcc and make using MSYS2. In the Makefile, append `-mwin32` to the compiling options, so that they are
```
OPTS := -O2 -std=c99 -I./include -mwin32
```
In case terminal returns `make` is not recognized as an internal or external command, adjust `PATH` variable, e.g.
```
PATH=%PATH%;C:\msys64\mingw32\bin;C:\msys64\usr\bin
```

## Cross-compiling for Windows on Debian GNU/Linux
Windows executables could be also built on Linux with the MinGW-w64 cross-compilers. Install them by running:
```
apt-get install gcc-mingw-w64-x86-64 gcc-mingw-w64-i686 make
```

Then build the 64-bit (`gcc-mingw-w64-x86-64`) or the 32-bit (`gcc-mingw-w64-i686`) executable:
```
make clean
make OS=Windows_NT CROSS_COMPILE=x86_64-w64-mingw32- CC=x86_64-w64-mingw32-gcc
```
```
make clean
make OS=Windows_NT CROSS_COMPILE=i686-w64-mingw32- CC=i686-w64-mingw32-gcc
```

The result is `utmp-cli.exe`. Notes:
* `OS=Windows_NT` adds the `.exe` extension, `CROSS_COMPILE` selects the matching `strip` and `CC` the compiler (MinGW-w64 provides no `*-cc` alias, so it must be set).
* Always run `make clean` before switching between Linux and Windows builds, otherwise object files built by the other compiler are reused and linking fails.
* Use `make clean OS=Windows_NT` to also remove `utmp-cli.exe`.


## Cross-compiling for ARM on Debian GNU/Linux
32-bit ARM Linux executables (hard-float, e.g. Raspberry Pi OS 32-bit or other ARMv7 boards) could be built on a x86-64 Debian GNU/Linux. Install the cross-compiler by running:
```
apt-get install gcc-arm-linux-gnueabihf libc6-dev-armhf-cross make
```

Then build the executable:
```
make clean
CFLAGS=-fno-pie LDFLAGS=-no-pie make CROSS_COMPILE=arm-linux-gnueabihf- CC=arm-linux-gnueabihf-gcc
```

The result is `utmp-cli`. Notes:
* `CROSS_COMPILE` selects the matching `strip` and `CC` the compiler, as for Windows.
* `CFLAGS` and `LDFLAGS` are added to the Makefile's own flags; `-fno-pie`/`-no-pie` build a non-position-independent executable, the same as the released `utmp-cli-arm32`.
* As with Windows, run `make clean` before switching between native and ARM builds.
