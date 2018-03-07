#include "usbtemp.h"

static int ut_errno;

static int owReset(HANDLE fd)
{
  int rv;
  DWORD wbytes, rbytes;
  unsigned char wbuff, rbuff;
  DCB dcb;
  
  PurgeComm(fd, PURGE_RXCLEAR | PURGE_TXCLEAR);

  if (!GetCommState(fd, &dcb)) {
    return -1;
  }
  dcb.BaudRate = CBR_9600;
  dcb.ByteSize = 8;
  if (!SetCommState(fd, &dcb)) {
    return -1;
  }

  /* Send the reset pulse. */
  wbuff = 0xf0;
  WriteFile(fd, &wbuff, 1, &wbytes, NULL);
  if (wbytes != 1) {
    ut_errno = 9;
    return -1;
  }

  ReadFile(fd, &rbuff, 1, &rbytes, NULL);
  if (rbytes != 1) {
    rv = -1;
  } 
  else {
    switch (rbuff) {
      case 0:
        /* Ground. */
      case 0xf0:
        /* No response. */
        rv = -1; break;
      default:
        /* Got a response */
        rv = 0;
    }
  }

  dcb.BaudRate = CBR_115200;
  dcb.ByteSize = 6;
  if (!SetCommState(fd, &dcb)) {
    return -1;
  }

  return rv;
}

static unsigned char owWriteByte(HANDLE fd, unsigned char wbuff)
{
  char buf[8];
  DWORD wbytes, rbytes, remaining;
  unsigned char rbuff, i;
  
  PurgeComm(fd, PURGE_RXCLEAR | PURGE_TXCLEAR);

  for (i = 0; i < 8; i++)
    buf[i] = (wbuff & (1 << (i & 0x7))) ? 0xff : 0x00;
  WriteFile(fd, &buf, 8, &wbytes, NULL);
  if (wbytes != 8) {
    ut_errno = 9;
    return -1;
  }

  rbuff = 0;
  remaining = 8;
  while (remaining > 0) {
    ReadFile(fd, &buf, remaining, &rbytes, NULL);
    for (i = 0; i < rbytes; i++) {
      rbuff >>= 1;
      rbuff |= (buf[i] & 0x01) ? 0x80 : 0x00;
      remaining--;
    }
  }
  return rbuff;
}

unsigned char owRead(HANDLE fd)
{
  return owWriteByte(fd, 0xff);
}

int owWrite(HANDLE fd, unsigned char wbuff)
{
  if (owWriteByte(fd, wbuff) != wbuff) {
    return -1;
  }
  return 0;
}

HANDLE owOpen(const char *serial_port)
{
  HANDLE fd;
  DCB dcb = { 0 };
  COMMTIMEOUTS timeouts = { 0 };
  
  fd = CreateFile(serial_port, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (fd == INVALID_HANDLE_VALUE) {
    ut_errno = 3;
    return fd;
  }
  
  SecureZeroMemory(&dcb, sizeof(DCB));
  dcb.DCBlength = sizeof(DCB);

  if (!GetCommState(fd, &dcb)) {
    ut_errno = 1;
    CloseHandle(fd);
    return INVALID_HANDLE_VALUE;
  }
  
  dcb.BaudRate = CBR_115200;
  dcb.ByteSize = 6;
  dcb.StopBits = ONESTOPBIT;
  dcb.Parity = NOPARITY;

  if (!SetCommState(fd, &dcb)) {
    ut_errno = 2;
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
    ut_errno = 0;
    return INVALID_HANDLE_VALUE;
  }

  return fd;
}

void owClose(HANDLE fd)
{
  CloseHandle(fd);
}