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
Easiest method is to install gcc and make using [MSYS2](https://www.msys2.org/), which provides a Unix-like shell and the `pacman` package manager on Windows.

1. Download and install MSYS2 from [msys2.org](https://www.msys2.org/).
2. From the Start menu, open the **MSYS2 MINGW64** terminal (`pacman` and the commands below run there, not in the Windows command prompt or PowerShell).
3. Install make and gcc by running:
   ```
   pacman -S make mingw-w64-x86_64-gcc
   ```
4. Change to the source directory (Windows drives are under `/c/`, `/d/`, ...) and build the executable:
   ```
   cd /c/Users/me/utmp-cli
   make CC=gcc
   ```

The result is the 64-bit `utmp-cli.exe`. For a 32-bit executable, open the **MSYS2 MINGW32** terminal instead and install `mingw-w64-i686-gcc` in place of `mingw-w64-x86_64-gcc`.

Running `make CC=gcc` from the Windows command prompt also works, if `make` and `gcc` are on the `PATH`, e.g.
```
PATH=%PATH%;C:\msys64\mingw64\bin;C:\msys64\usr\bin
```
(use `mingw32` instead of `mingw64` for the 32-bit compiler).

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
ARM Linux executables could be built on a x86-64 Debian GNU/Linux. Install the cross-compilers by running:
```
apt-get install gcc-arm-linux-gnueabihf libc6-dev-armhf-cross gcc-aarch64-linux-gnu libc6-dev-arm64-cross make
```

Then build the 32-bit (ARMv7 hard-float, e.g. Raspberry Pi OS 32-bit) or the 64-bit (e.g. Raspberry Pi OS 64-bit) executable:
```
make clean
make CROSS_COMPILE=arm-linux-gnueabihf- CC=arm-linux-gnueabihf-gcc
```
```
make clean
make CROSS_COMPILE=aarch64-linux-gnu- CC=aarch64-linux-gnu-gcc
```

The result is `utmp-cli`. Notes:
* `CROSS_COMPILE` selects the matching `strip` and `CC` the compiler, as for Windows.
* As with Windows, run `make clean` before switching between native and ARM builds.
* The executable needs at least the glibc version it was built against, so build on a system as old as the oldest one it should run on, or use `zig cc` as below.

## Building for older glibc with zig
The released Linux executables are built with [`zig cc`](https://ziglang.org/), which links against an older glibc (2.17) than the build machine has, so they run on older distributions too. With zig installed, and `binutils-arm-linux-gnueabihf` and `binutils-aarch64-linux-gnu` for the matching `strip`, run:
```
make clean
CFLAGS=-fPIE LDFLAGS=-pie make CC="zig cc -target x86_64-linux-gnu.2.17"
```
```
make clean
CFLAGS=-fPIE LDFLAGS=-pie make CROSS_COMPILE=arm-linux-gnueabihf- CC="zig cc -target arm-linux-gnueabihf.2.17"
```
```
make clean
CFLAGS=-fPIE LDFLAGS=-pie make CROSS_COMPILE=aarch64-linux-gnu- CC="zig cc -target aarch64-linux-gnu.2.17"
```

`CFLAGS` and `LDFLAGS` are added to the Makefile's own flags; `-fPIE`/`-pie` build a position-independent executable, as gcc does by default. See `.github/workflows/release.yml` for the exact release build.
