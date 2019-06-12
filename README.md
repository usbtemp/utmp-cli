# usbtemp-cli
Read temperature from [usbtemp.com USB thermometer](https://usbtemp.com/) and DS9097E compatible 1-wire adapter with one DS18B20 digital probe attached.

### How to run it
1. Clone (or download) this repository (and extract files)
2. Run `make` to compile the binary
3. Execute the binary `./usbtemp-cli`

The default output is time with the temperature in degrees Celsius and looks like:
```
May 13 17:05:02 Sensor C: 22.62
```
By using `-f` switch the temperature unit will be switched to Fahrenheit. Date/time formatting is `%b %d %H:%M:%S`.

Instead of compiling from the source, already compiled executable could be downloaded from [Releases tab](https://github.com/usbtemp/usbtemp-cli/releases/latest).

### Windows
This application could be also compiled on Windows with a MinGW compiler.

### OpenWRT/LEDE
Package (with digitemp) is available at [opkg-packages](https://github.com/jaka/opkg-packages).

## Usage
```
$ ./usbtemp-cli -h
	-f	Display temperature using the Fahrenheit scale
	-p	Set probe precision
	-q	Quiet mode
	-r	Get probe serial number (ROM)
	-s	Set serial port
```
Serial port could be anything like `/dev/ttyUSB0`, `COM6` or similar.

### Troubleshooting

User, running binary, must have permissions to write to `/dev/ttyUSB0` or similar character device.
Usually, `adduser` to `dialout` group or `chmod o+rw` the character device helps.
