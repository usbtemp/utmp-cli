#include "usbtemp.h"

#ifdef _WIN32
#include "windows.c"
void wait_1s(void)
{
  Sleep(1000);
}

int is_fd_valid(HANDLE fd)
{
  return (fd != INVALID_HANDLE_VALUE);
}
#else
/* Generic POSIX implementation (Linux, macOS, BSD). */
#include "linux.c"
void wait_1s(void)
{
  struct timeval wait_tv;

  wait_tv.tv_usec = 0;
  wait_tv.tv_sec = 1;
  select(0, NULL, NULL, NULL, &wait_tv);
}

int is_fd_valid(HANDLE fd)
{
  return (fd >= 0);
}
#endif

#define DS18B20_CONFIG_MASK 0x9F
#define DS18B20_CONFIG_FIXED 0x1F

extern int ut_errno;

static const char *const ut_msgs[] = {
  [UT_OK] = "",
  [UT_ERR_GET_BAUD] = "Error, could not get baudrate!",
  [UT_ERR_SET_BAUD] = "Error, could not set baudrate!",
  [UT_ERR_NO_PORT] = "Error, serial port does not exist!",
  [UT_ERR_PERMISSION] = "Error, you don't have rw permission to access serial port!",
  [UT_ERR_OPEN] = "Error, failed to open serial port device!",
  [UT_ERR_NO_SENSOR] = "Error, sensor not found!",
  [UT_ERR_CRC] = "Error, sensor CRC mismatch!",
  [UT_ERR_RESPONSE] = "Warning, not expected sensor response!",
  [UT_ERR_SEND] = "Error, could not send data!",
  [UT_ERR_RECEIVE] = "Error, could not receive data!"
};

static unsigned char lsb_crc8(unsigned char *data_in, unsigned int len, const unsigned char generator)
{
  unsigned char i, bit_counter;
  unsigned char crc = 0;

  for (i = 0; i < len; i++) {
    crc ^= data_in[i];
    bit_counter = 8;
    do {
      if (crc & 0x01) {
        crc = (crc >> 1) ^ generator;
      }
      else {
        crc >>= 1;
      }
      bit_counter--;
    } while (bit_counter > 0);
  }
  return crc;
}

const char *DS18B20_errmsg(void)
{
  return ut_msgs[ut_errno];
}

static int DS18B20_start(HANDLE fd)
{
  if (owReset(fd) < 0) {
    return -1;
  }
  if (owWrite(fd, DS18B20_SKIP_ROM) < 0) {
    return -1;
  }
  return 0;
}

static int DS18B20_sp(HANDLE fd, unsigned char *sp)
{
  unsigned char i, crc;
  int byte;

  if (DS18B20_start(fd) < 0) {
    return -1;
  }
  if (owWrite(fd, DS18B20_READ_SCRATCHPAD) < 0) {
    return -1;
  }
  for (i = 0; i < DS18X20_SP_SIZE; i++) {
    byte = owRead(fd);
    if (byte < 0) {
      /* Report a failed read as a CRC mismatch, as before. */
      ut_errno = UT_ERR_CRC;
      return -1;
    }
    sp[i] = (unsigned char)byte;
  }

  if ((sp[4] & DS18B20_CONFIG_MASK) != DS18B20_CONFIG_FIXED) {
    ut_errno = UT_ERR_NO_SENSOR;
    return -1;
  }

  crc = lsb_crc8(sp, DS18X20_SP_SIZE - 1, DS18X20_GENERATOR);
  if (sp[DS18X20_SP_SIZE - 1] != crc) {
    ut_errno = UT_ERR_CRC;
    return -1;
  }

  return 0;
}

int DS18B20_measure(HANDLE fd)
{
  if (DS18B20_start(fd) < 0) {
    return -1;
  }
  if (owWrite(fd, DS18B20_CONVERT_T) < 0) {
    return -1;
  }
  return 0;
}

int DS18B20_set_precision(HANDLE fd, int precision)
{
  int i, rv;
  unsigned char cfg_new, sp_sensor[DS18X20_SP_SIZE];
  unsigned char cmd[4];

  /* Resolution bits R1:R0 are config bits 6:5 (9 bit = 00 ... 12 bit = 11). */
  cfg_new = 0x1f | ((unsigned char)(precision - 9) << 5);

  rv = DS18B20_sp(fd, sp_sensor);
  if (rv < 0) {
    return rv;
  }

  if (sp_sensor[DS18B20_SP_CONFIG] == cfg_new) {
    return 0;
  }

  /* Write Scratchpad: command, then TH, TL and config (TH/TL kept as they are). */
  cmd[0] = DS18B20_SP_WRITE;
  cmd[1] = sp_sensor[DS18B20_SP_TH];
  cmd[2] = sp_sensor[DS18B20_SP_TL];
  cmd[3] = cfg_new;

  if (DS18B20_start(fd) < 0) {
    return -1;
  }
  for (i = 0; i < 4; i++) {
    if (owWrite(fd, cmd[i]) < 0) {
      return -1;
    }
  }

  if (DS18B20_start(fd) < 0) {
    return -1;
  }
  if (owWrite(fd, DS18B20_SP_SAVE) < 0) {
    return -1;
  }

  return 0;
}

int DS18B20_acquire(HANDLE fd, float *temperature)
{
  int rv;
  short T;
  unsigned short uT;
  unsigned char sp_sensor[DS18X20_SP_SIZE];

  if (!temperature) {
    return -1;
  }

  rv = DS18B20_sp(fd, sp_sensor);
  if (rv < 0) {
    return rv;
  }

  uT = (sp_sensor[1] << 8) | (sp_sensor[0] & 0xff);
  T = (short)uT; /* Force using leading bit as a sign. */
  *temperature = (float)T / 16;

  return 0;
}

int DS18B20_rom(HANDLE fd, unsigned char *rom)
{
  unsigned char i, crc;
  int byte;

  if (!rom) {
    return -1;
  }

  if (owReset(fd) < 0) {
    return -1;
  }
  if (owWrite(fd, DS18B20_READ_ROM) < 0) {
    return -1;
  }

  for (i = 0; i < DS18X20_ROM_SIZE; i++) {
    byte = owRead(fd);
    if (byte < 0) {
      /* Report a failed read as a CRC mismatch, as before. */
      ut_errno = UT_ERR_CRC;
      return -1;
    }
    rom[i] = (unsigned char)byte;
  }

  crc = lsb_crc8(rom, DS18X20_ROM_SIZE - 1, DS18X20_GENERATOR);
  if (rom[DS18X20_ROM_SIZE - 1] != crc) {
    ut_errno = UT_ERR_CRC;
    return -1;
  }

  return 0;
}

HANDLE DS18B20_open(const char *serial_port)
{
  return owOpen(serial_port);
}

void DS18B20_close(HANDLE fd)
{
  owClose(fd);
}
