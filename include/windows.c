#include "usbtemp.h"
#include <stdio.h>

static int ut_errno;

static int is_digit(char c)
{
  return '0' <= c && c <= '9';
}

static int get_new_serial_port(char *buf, const unsigned int buflen, const char *s)
{
  const char *fmt_x = "COM%u", *fmt_xx = "\\\\.\\COM%u";
  const char *fmt, *p;
  unsigned int num = 0;

  for (p = s; *p && !is_digit(*p); p++);

  if (!is_digit(*p)) {
    return -1;
  }

  while (is_digit(*p)) {
    num *= 10;
    num += *p - '0';
    p++;
  }

  fmt = num > 9 ? fmt_xx : fmt_x;

  return snprintf(buf, buflen, fmt, num);
}

static int owReset(HANDLE fd)
{
  int rv = -1;
  DWORD wbytes, rbytes;
  unsigned char wbuff, rbuff;
  DCB dcb;

  PurgeComm(fd, PURGE_RXCLEAR | PURGE_TXCLEAR);

  SecureZeroMemory(&dcb, sizeof(DCB));
  dcb.DCBlength = sizeof(DCB);
  if (!GetCommState(fd, &dcb)) {
    ut_errno = UT_ERR_GET_BAUD;
    return -1;
  }
  dcb.BaudRate = CBR_9600;
  dcb.ByteSize = 8;
  if (!SetCommState(fd, &dcb)) {
    ut_errno = UT_ERR_SET_BAUD;
    return -1;
  }

  /* Send the reset pulse. */
  wbuff = 0xf0;
  if (!WriteFile(fd, &wbuff, 1, &wbytes, NULL) || wbytes != 1) {
    ut_errno = UT_ERR_SEND;
  }
  else if (!ReadFile(fd, &rbuff, 1, &rbytes, NULL)) {
    ut_errno = UT_ERR_RECEIVE;
  }
  else if (rbytes != 1 || rbuff == 0 || rbuff == 0xf0) {
    /* Timed out, ground, or no response. */
    ut_errno = UT_ERR_NO_SENSOR;
  }
  else {
    /* Got a response */
    rv = 0;
  }

  /* Always restore the data speed, even after a failed reset. */
  dcb.BaudRate = CBR_115200;
  dcb.ByteSize = 6;
  if (!SetCommState(fd, &dcb)) {
    ut_errno = UT_ERR_SET_BAUD;
    return -1;
  }

  return rv;
}

/* Returns the byte read back (0..255), or -1 on error with ut_errno set. */
static int owWriteByte(HANDLE fd, unsigned char wbuff)
{
  unsigned char buf[8];
  unsigned char rbuff = 0;
  DWORD wbytes, rbytes, remaining, i;

  PurgeComm(fd, PURGE_RXCLEAR | PURGE_TXCLEAR);

  for (i = 0; i < 8; i++) {
    buf[i] = (wbuff & (1 << i)) ? 0xff : 0x00;
  }
  if (!WriteFile(fd, buf, 8, &wbytes, NULL) || wbytes != 8) {
    ut_errno = UT_ERR_SEND;
    return -1;
  }

  remaining = 8;
  while (remaining > 0) {
    /* Zero bytes read means the read timed out. */
    if (!ReadFile(fd, buf, remaining, &rbytes, NULL) || rbytes == 0) {
      ut_errno = UT_ERR_RECEIVE;
      return -1;
    }
    for (i = 0; i < rbytes; i++) {
      rbuff >>= 1;
      rbuff |= (buf[i] & 0x01) ? 0x80 : 0x00;
    }
    remaining -= rbytes;
  }
  return rbuff;
}

int owRead(HANDLE fd)
{
  return owWriteByte(fd, 0xff);
}

int owWrite(HANDLE fd, unsigned char wbuff)
{
  int rv = owWriteByte(fd, wbuff);

  if (rv < 0) {
    return -1;
  }
  if (rv != wbuff) {
    ut_errno = UT_ERR_RESPONSE;
    return -1;
  }
  return 0;
}

HANDLE owOpen(const char *serial_port)
{
  HANDLE fd;
  DCB dcb = {0};
  COMMTIMEOUTS timeouts = {0};
  char new_serial_port[16];

  fd = CreateFile(serial_port, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (fd == INVALID_HANDLE_VALUE) {
    if (get_new_serial_port(new_serial_port, sizeof(new_serial_port) - 1, serial_port) < 0) {
      ut_errno = UT_ERR_NO_PORT;
      return fd;
    }
    fd = CreateFile(new_serial_port, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fd == INVALID_HANDLE_VALUE) {
      ut_errno = UT_ERR_NO_PORT;
      return fd;
    }
  }

  SecureZeroMemory(&dcb, sizeof(DCB));
  dcb.DCBlength = sizeof(DCB);

  if (!GetCommState(fd, &dcb)) {
    ut_errno = UT_ERR_GET_BAUD;
    CloseHandle(fd);
    return INVALID_HANDLE_VALUE;
  }

  dcb.BaudRate = CBR_115200;
  dcb.ByteSize = 6;
  dcb.StopBits = ONESTOPBIT;
  dcb.Parity = NOPARITY;

  if (!SetCommState(fd, &dcb)) {
    ut_errno = UT_ERR_SET_BAUD;
    CloseHandle(fd);
    return INVALID_HANDLE_VALUE;
  }

  timeouts.ReadIntervalTimeout = 50;
  timeouts.ReadTotalTimeoutConstant = 50;
  timeouts.ReadTotalTimeoutMultiplier = 10;
  timeouts.WriteTotalTimeoutConstant = 50;
  timeouts.WriteTotalTimeoutMultiplier = 10;

  if (!SetCommTimeouts(fd, &timeouts)) {
    CloseHandle(fd);
    ut_errno = UT_ERR_OPEN;
    return INVALID_HANDLE_VALUE;
  }

  return fd;
}

void owClose(HANDLE fd)
{
  CloseHandle(fd);
}
