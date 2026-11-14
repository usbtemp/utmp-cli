# utmp-cli (usbtemp-cli)
Read temperature from [usbtemp.com USB thermometer](https://usbtemp.com/) and DS9097E compatible 1-wire adapter with one DS18B20 digital probe attached through command line interface.

### Download
Ready-built executables are on the [Releases page](https://github.com/usbtemp/utmp-cli/releases/latest):

| File | Platform |
|---|---|
| `utmp-cli` | Linux x86-64 |
| `utmp-cli-arm64` | Linux ARM 64-bit (e.g. Raspberry Pi OS 64-bit) |
| `utmp-cli-arm32` | Linux ARM 32-bit, ARMv7 or newer, hard-float (e.g. Raspberry Pi 2 and later on 32-bit Raspberry Pi OS) |
| `utmp-cli-w64.exe` | Windows 64-bit |
| `utmp-cli.exe` | Windows 32-bit |

The Linux executables need glibc 2.17 or newer, so they run on practically any current distribution (not on musl-based ones such as Alpine or OpenWrt). After downloading, make the file executable with `chmod +x utmp-cli`.

Each release also has a `SHA256SUMS` file. To check the downloaded files, put it next to them and run:
```
sha256sum -c SHA256SUMS --ignore-missing
```

### How to compile from source
1. Clone (or download) this repository (and extract files)
2. Run `make` to compile the binary
3. Execute the binary `./utmp-cli`

By default it outputs date, time and the temperature in degrees Celsius, like:
```
May 13 17:05:02 Sensor C: 22.62
```
By using `-f` switch the temperature unit will be changed to Fahrenheit. Date/time formatting is `%b %d %H:%M:%S`, `%Y-%m-%dT%H:%M:%SZ` (`-i`, ISO 8601 in UTC) or `%Y-%m-%dT%H:%M:%S+hh:mm` (`-I`, ISO 8601 in local time with UTC offset, not available on Windows).


### Windows
This application could be also compiled on Windows with a MinGW compiler, or cross-compiled for Windows and ARM on Linux, see [`COMPILING.md`](COMPILING.md).

### OpenWRT/LEDE
Package (with digitemp) is available at [opkg-packages](https://github.com/usbtemp/opkg-packages).

## Usage
```
$ ./utmp-cli -h
	-f	Display temperature using the Fahrenheit scale
	-i	Format date as UTC ISO 8601
	-I	Format date as local time ISO 8601 with UTC offset
	-j	Format date and temperature as JSON
	-p	Set probe precision {9,10,11,12}
	-q	Quiet mode
	-r	Get probe serial number (ROM) in hexadecimal, or -R in uppercase hexadecimal
	-s	Set serial port
```
Serial port could be anything like `/dev/ttyUSB0`, `COM6` or similar.

### Exit codes
The exit code tells scripts what went wrong:

| Code | Meaning |
|---|---|
| 0 | Success |
| 1 | Other error |
| 2 | Invalid option or argument |
| 3 | Serial port does not exist |
| 4 | No read/write permission for the serial port |
| 5 | Could not open or configure the serial port |
| 6 | Serial port is in use by another program |
| 7 | Sensor not found |
| 8 | Communication error (send, receive or unexpected response) |
| 9 | Sensor data CRC mismatch |

Only one utmp-cli can use the serial port at a time. If the port is busy, utmp-cli waits up to 2 seconds for it before giving up with exit code 6, so overlapping runs (e.g. from cron) queue up instead of corrupting each other's readings.

### Troubleshooting

User, running binary, must have permissions to write to `/dev/ttyUSB0` or similar character device.
Usually, `adduser` to `dialout` group or `chmod o+rw` the character device helps.
