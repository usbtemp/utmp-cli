#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "platform.h"
#include "usbtemp.h"

#ifdef _XOPEN_VERSION
#if (_XOPEN_VERSION >= 4)
  #define SUPPORT_OPT_I
#endif
#endif

#define VERSION "1.071"

enum {
  NONE,
  HELP,
  ACQUIRE_TEMP,
  READ_ROM,
  SET_PRECISON,
};

int main(int argc, char **argv)
{
  const char *hex_upper = "%02X", *hex_lower = "%02x";
  int c;
  char units = 'C';
  HANDLE fd;
  char *serial_port = NULL;
  int json_mode = 0;
  int pseudo_iso_8601_mode = 0;
  int verbose = 1;
  int rv = 0;
  int i;
  int action = 0;
  int precision = 12;
  float temperature;
  unsigned char rom[DS18X20_ROM_SIZE];
  char timebuf[40];
  time_t now;
  struct tm *timeptr;
  const char *hex_format = NULL;

  while ((c = getopt(argc, argv, "fhiIjp:qRrs:")) != -1) {
    switch (c) {
      case 'f':
        units = 'F';
        break;
      case 'i':
        pseudo_iso_8601_mode = 1;
        break;
      case 'I':
        pseudo_iso_8601_mode = 2;
        break;
      case 'j':
        json_mode = 1;
        break;
      case 'h':
        action = HELP;
        break;
      case 'p':
        action = SET_PRECISON;
        precision = strtol(optarg, NULL, 10);
        if (precision < 9 || 12 < precision) {
          if (verbose) {
            fprintf(stderr, "Probe precision out of range!\n");
          }
          return -1;
        }
        break;
      case 'q':
        verbose = 0;
        break;
      case 'R':
        hex_format = hex_upper;
        break;
      case 'r':
        hex_format = hex_lower;
        break;
      case 's':
        serial_port = optarg;
        break;
      case '?':
        return -1;
    }
  }

  if (optind < argc) {
    serial_port = argv[optind];
  }

  if (hex_format) {
    action = READ_ROM;
  }

  if (!action) {
    action = ACQUIRE_TEMP;
  }

  if (json_mode > 0) {
    verbose = 0;
    if (action != ACQUIRE_TEMP) {
      fprintf(stderr, "JSON output only supported when displaying temperature.\n");
      return 1;
    }
  }

  if (action == HELP) {
    verbose = 1;
  }

  if (verbose) {
    printf("USB Thermometer CLI v" VERSION " Copyright 2026 usbtemp.com et al. Licensed under MIT licence.\n");
  }

  if (action == HELP) {
    printf("\t-f\tDisplay temperature using the Fahrenheit scale\n");
    printf("\t-i\tFormat date as UTC ISO 8601\n");
    printf("\t-j\tFormat date and temperature as JSON\n");
    printf("\t-p\tSet probe precision {9,10,11,12}\n");
    printf("\t-q\tQuiet mode\n");
    printf("\t-r\tGet probe serial number (ROM) in hexadecimal, or -R in uppercase hexadecimal\n");
    printf("\t-s\tSet serial port\n");
    return 0;
  }

#ifndef SUPPORT_OPT_I
  if (pseudo_iso_8601_mode > 1) {
    fprintf(stderr, "Option -I not supported!\n");
    return -2;
  }
#endif

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
      if (pseudo_iso_8601_mode > 0) {
        timeptr = gmtime(&now);
        if (!timeptr) {
          fprintf(stderr, "Cannot determine time!\n");
          rv = 1;
          break;
        }
        if (pseudo_iso_8601_mode == 1) {
          strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%SZ", timeptr);
        }
        else {
#ifdef SUPPORT_OPT_I
          strftime(timebuf, sizeof(timebuf), "%FT%T%Z", timeptr);
#endif
        }
      }
      else {
        timeptr = localtime(&now);
        if (!timeptr) {
          fprintf(stderr, "Cannot determine time!\n");
          rv = 1;
          break;
        }
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M:%S", timeptr);
      }

      if (units == 'F') {
        temperature = (9.0f * temperature) / 5.0f + 32.0f;
      }

      if (json_mode > 0) {
        printf("{ \"time\": \"%s\", \"temp_%c\": %.2f }\n", timebuf, tolower((unsigned char)units), temperature);
      }
      else {
        printf("%s Sensor %c: %.2f\n", timebuf, units, temperature);
      }

      break;

    case READ_ROM:
      if (DS18B20_rom(fd, rom) < 0) {
        fprintf(stderr, "%s\n", DS18B20_errmsg());
        rv = 1;
        break;
      }
      printf("ROM: ");
      for (i = 0; i < DS18X20_ROM_SIZE; i++) {
        printf(hex_format, rom[i]);
      }
      printf("\n");
      break;

    case SET_PRECISON:
      rv = DS18B20_setprecision(fd, precision);
      break;
  }

  DS18B20_close(fd);
  return rv;
}
