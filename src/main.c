#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "platform.h"
#include "usbtemp.h"

#ifdef _XOPEN_VERSION
#if (_XOPEN_VERSION >= 4)
  #define SUPPORT_OPT_I
#endif
#endif

#ifndef SUPPORT_OPT_I
static const char opt_I_unsupported[] = "Option -I not supported!\n";
#endif

#define VERSION "1.091"

#define MIN_PRECISION 9
#define MAX_PRECISION 12

enum {
  NONE,
  HELP,
  ACQUIRE_TEMP,
  READ_ROM,
  SET_PRECISION,
};

enum {
  TIME_LOCAL,
  TIME_ISO8601,
  TIME_ISO8601_OFFSET
};

/* Exit codes, documented in README.md. */
enum {
  RC_OK = 0,
  RC_ERROR = 1,      /* Any other error. */
  RC_USAGE = 2,      /* Invalid option or argument. */
  RC_NO_PORT = 3,    /* Serial port does not exist. */
  RC_PERMISSION = 4, /* No read/write permission for the serial port. */
  RC_PORT = 5,       /* Could not open or configure the serial port. */
  RC_BUSY = 6,       /* Serial port is in use by another program. */
  RC_NO_SENSOR = 7,  /* No sensor found on the adapter. */
  RC_COMM = 8,       /* Sending, receiving or unexpected response. */
  RC_CRC = 9         /* Sensor data CRC mismatch. */
};

/* Print the last device error and return the matching exit code. */
static int fail(void)
{
  fprintf(stderr, "%s\n", DS18B20_errmsg());

  switch (DS18B20_errcode()) {
    case UT_ERR_NO_PORT:
      return RC_NO_PORT;
    case UT_ERR_PERMISSION:
      return RC_PERMISSION;
    case UT_ERR_OPEN:
    case UT_ERR_GET_BAUD:
    case UT_ERR_SET_BAUD:
      return RC_PORT;
    case UT_ERR_NO_SENSOR:
      return RC_NO_SENSOR;
    case UT_ERR_SEND:
    case UT_ERR_RECEIVE:
    case UT_ERR_RESPONSE:
      return RC_COMM;
    case UT_ERR_CRC:
      return RC_CRC;
    case UT_ERR_BUSY:
      return RC_BUSY;
    default:
      return RC_ERROR;
  }
}

static int format_timestamp(char *timebuf, const size_t len, const int pseudo_iso_8601_mode)
{
  struct tm *timeptr;
  time_t now;
  const char *fmt;
  size_t n;

  time(&now);
  if (pseudo_iso_8601_mode == TIME_ISO8601) {
    timeptr = gmtime(&now);
  }
  else {
    timeptr = localtime(&now);
  }
  if (!timeptr) {
    fprintf(stderr, "Cannot determine time!\n");
    return 1;
  }

  switch (pseudo_iso_8601_mode) {
    case TIME_ISO8601:
      fmt = "%Y-%m-%dT%H:%M:%SZ";
      break;
    case TIME_ISO8601_OFFSET:
#ifdef SUPPORT_OPT_I
      fmt = "%Y-%m-%dT%H:%M:%S%z";
      break;
#else
      /* Older C libraries print a zone name instead of a numeric offset for %z. */
      fputs(opt_I_unsupported, stderr);
      return 1;
#endif
    default:
      fmt = "%b %d %H:%M:%S";
  }

  n = strftime(timebuf, len, fmt, timeptr);
  if (!n) {
    return 1;
  }

  /* Turn the +hhmm offset from %z into +hh:mm, as ISO 8601 extended format needs. */
  if (pseudo_iso_8601_mode == TIME_ISO8601_OFFSET && n >= 5 && n + 1 < len) {
    memmove(timebuf + n - 1, timebuf + n - 2, 3);
    timebuf[n - 2] = ':';
  }

  return 0;
}

int main(int argc, char **argv)
{
  const char hex_upper[] = "%02X", hex_lower[] = "%02x";
  int c;
  char units = 'C';
  HANDLE fd;
  char *serial_port = NULL;
  int json_mode = 0;
  int pseudo_iso_8601_mode = TIME_LOCAL;
  int verbose = 1;
  int rv = RC_OK;
  int i;
  int action = NONE;
  int precision = 12;
  long precision_arg;
  char *endptr;
  float temperature;
  unsigned char rom[DS18X20_ROM_SIZE];
  char timebuf[40];
  const char *hex_format = NULL;

  while ((c = getopt(argc, argv, "fhiIjp:qRrs:")) != -1) {
    switch (c) {
      case 'f':
        units = 'F';
        break;
      case 'i':
        pseudo_iso_8601_mode = TIME_ISO8601;
        break;
      case 'I':
        pseudo_iso_8601_mode = TIME_ISO8601_OFFSET;
        break;
      case 'j':
        json_mode = 1;
        break;
      case 'h':
        action = HELP;
        break;
      case 'p':
        action = SET_PRECISION;
        precision_arg = strtol(optarg, &endptr, 10);
        if (endptr == optarg || *endptr != '\0') {
          if (verbose) {
            fprintf(stderr, "Probe precision is not a number!\n");
          }
          return RC_USAGE;
        }
        if (precision_arg < MIN_PRECISION || MAX_PRECISION < precision_arg) {
          if (verbose) {
            fprintf(stderr, "Probe precision out of range!\n");
          }
          return RC_USAGE;
        }
        precision = (int)precision_arg;
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
        return RC_USAGE;
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
      return RC_USAGE;
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
#ifdef SUPPORT_OPT_I
    printf("\t-I\tFormat date as local time ISO 8601 with UTC offset\n");
#endif
    printf("\t-j\tFormat date and temperature as JSON\n");
    printf("\t-p\tSet probe precision {9,10,11,12}\n");
    printf("\t-q\tQuiet mode\n");
    printf("\t-r\tGet probe serial number (ROM) in hexadecimal, or -R in uppercase hexadecimal\n");
    printf("\t-s\tSet serial port\n");
    return RC_OK;
  }

#ifndef SUPPORT_OPT_I
  if (pseudo_iso_8601_mode > TIME_ISO8601) {
    fputs(opt_I_unsupported, stderr);
    return RC_USAGE;
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
    return fail();
  }

  switch (action) {

    case ACQUIRE_TEMP:
      if (DS18B20_measure(fd) < 0) {
        rv = fail();
        break;
      }
      if (verbose) {
        printf("Waiting for response ...\n");
      }
      wait_s(1);
      if (DS18B20_acquire(fd, &temperature) < 0) {
        rv = fail();
        break;
      }
      if (format_timestamp(timebuf, sizeof(timebuf), pseudo_iso_8601_mode)) {
        rv = RC_ERROR;
        break;
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
        rv = fail();
        break;
      }
      printf("ROM: ");
      for (i = 0; i < DS18X20_ROM_SIZE; i++) {
        printf(hex_format, rom[i]);
      }
      printf("\n");
      break;

    case SET_PRECISION:
      if (DS18B20_set_precision(fd, precision) < 0) {
        rv = fail();
      }
      break;
  }

  DS18B20_close(fd);
  return rv;
}
