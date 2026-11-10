# utmp-cli (usbtemp-cli)
Read temperature from [usbtemp.com USB thermometer](https://usbtemp.com/) and DS9097E compatible 1-wire adapter with one DS18B20 digital probe attached through command line interface.

### How to compile from source
1. Clone (or download) this repository (and extract files)
2. Run `make` to compile the binary
3. Execute the binary `./utmp-cli`

By default it outputs date, time and the temperature in degrees Celsius, like:
```
May 13 17:05:02 Sensor C: 22.62
```
By using `-f` switch the temperature unit will be changed to Fahrenheit. Date/time formatting is `%b %d %H:%M:%S`, `%Y-%m-%dT%H:%M:%SZ` (`-i`, ISO 8601 in UTC) or `%Y-%m-%dT%H:%M:%S+hh:mm` (`-I`, ISO 8601 in local time with UTC offset, not available on Windows).

Instead of compiling from the source, already compiled executable could be downloaded from [Releases tab](https://github.com/usbtemp/utmp-cli/releases/latest).

### Windows
This application could be also compiled on Windows with a MinGW compiler, see `COMPILING.md`

### OpenWRT/LEDE
Package (with digitemp) is available at [opkg-packages](https://github.com/usbtemp/opkg-packages).

## Usage
```
$ ./usbtemp-cli -h
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

### Troubleshooting

User, running binary, must have permissions to write to `/dev/ttyUSB0` or similar character device.
Usually, `adduser` to `dialout` group or `chmod o+rw` the character device helps.
