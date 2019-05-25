#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "platform.h"
#include "usbtemp.h"

enum {
  HELP,
  ACQUIRE_TEMP,
  READ_ROM,
  SET
};

int main(int argc, char **argv)
{
  char c;
  HANDLE fd;
  char *serial_port = NULL;
  int verbose = 1;
  int rv = 0;
  int i;
  int action = ACQUIRE_TEMP;
  int precision = 12;
  float temperature;
  unsigned char rom[DS18X20_ROM_SIZE];
  char timebuf[40];
  time_t now;
  struct tm *timeptr;

  while ((c = getopt(argc, argv, "hp:qrs:")) != -1) {
    switch (c) {
      case 'h':
        action = HELP;
        break;
      case 'p':
        action = SET;
        precision = strtol(optarg, NULL, 10);
        break;
      case 'q':
        verbose = 0;
        break;
      case 'r':
        action = READ_ROM;
        break;
      case 's':
        serial_port = optarg;
        break;
      case '?':
        return -1;
    }
  }

  if (action == HELP) {
    verbose = 1;
  }

  if (verbose) {
    printf("USB Thermometer CLI v1.03 Copyright 2019 usbtemp.com Licensed under MIT licence.\n");
  }

  if (action == HELP) {
    printf("\t-p\tSet probe precision {9,10,11,12}\n");
    printf("\t-q\tQuiet mode\n");
    printf("\t-r\tGet probe serial number (ROM)\n");
    printf("\t-s\tSet serial port\n");
    return 0;
  }

  if (!serial_port) {
    serial_port = DEFAULT_SERIAL_PORT;
  }
  if (verbose) {
    printf("Using serial port: %s\n", serial_port);
  }

  fd = DS18B20_open(serial_port);
  if (!is_fd_valid(fd)) {
    fprintf(stderr, "%s\n", DS18B20_errmsg());
    return 1;
  }

  switch (action) {

    case ACQUIRE_TEMP:
      if (DS18B20_measure(fd) < 0) {
        fprintf(stderr, "%s\n", DS18B20_errmsg());
        rv = 1;
        break;
      }
      if (verbose) {
        printf("Waiting for response ...\n");
      }
      wait_1s();
      if (DS18B20_acquire(fd, &temperature) < 0) {
        fprintf(stderr, "%s\n", DS18B20_errmsg());
        rv = 1;
        break;
      }
      time(&now);
      timeptr = localtime(&now);
      strftime(timebuf, sizeof(timebuf), "%b %d %H:%M:%S", timeptr);

      printf("%s Sensor C: %.2f\n", timebuf, temperature);
      break;

    case READ_ROM:
      if (DS18B20_rom(fd, rom) < 0) {
        fprintf(stderr, "%s\n", DS18B20_errmsg());
        rv = 1;
        break;
      }
      printf("ROM: ");
      for (i = 0; i < DS18X20_ROM_SIZE; i++) {
        printf("%02x", rom[i]);
      }
      printf("\n");
      break;

    case SET:
      if (precision < 9 || 12 < precision) {
        if (verbose == 1) {
          fprintf(stderr, "Probe precision out of range!\n");
        }
        break;
      }
      rv = DS18B20_setprecision(fd, precision);
      break;

  }

  DS18B20_close(fd);
  return rv;
}
