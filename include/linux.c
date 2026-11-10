#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "usbtemp.h"

#define TIMEOUT 1

int ut_errno;

int owReset(int fd)
{
  int rv = -1;
  ssize_t wbytes, rbytes;
  unsigned char wbuff, rbuff;
  fd_set readset;
  struct timeval timeout_tv;
  struct termios term;

  tcflush(fd, TCIOFLUSH);

  if (tcgetattr(fd, &term) < 0) {
    ut_errno = UT_ERR_GET_BAUD;
    return -1;
  }
  term.c_cflag &= ~CSIZE;
  term.c_cflag |= CS8;
  cfsetispeed(&term, B9600);
  cfsetospeed(&term, B9600);
  if (tcsetattr(fd, TCSANOW, &term) < 0) {
    ut_errno = UT_ERR_SET_BAUD;
    return -1;
  }

  /* Send the reset pulse. */
  wbuff = 0xf0;
  wbytes = write(fd, &wbuff, 1);
  if (wbytes != 1) {
    ut_errno = UT_ERR_SEND;
  }
  else {
    timeout_tv.tv_usec = 0;
    timeout_tv.tv_sec = TIMEOUT;

    FD_ZERO(&readset);
    FD_SET(fd, &readset);

    if (select(fd + 1, &readset, NULL, NULL, &timeout_tv) > 0 && FD_ISSET(fd, &readset)) {
      rbytes = read(fd, &rbuff, 1);
      if (rbytes != 1) {
        ut_errno = UT_ERR_RECEIVE;
      }
      else if (rbuff == 0 || rbuff == 0xf0) {
        /* Ground, or no response. */
        ut_errno = UT_ERR_NO_SENSOR;
      }
      else {
        /* Got a response */
        rv = 0;
      }
    }
    else {
      /* Timed out or interrupt. */
      ut_errno = UT_ERR_NO_SENSOR;
    }
  }

  /* Always restore the data speed, even after a failed reset. */
  term.c_cflag &= ~CSIZE;
  term.c_cflag |= CS6;
  cfsetispeed(&term, B115200);
  cfsetospeed(&term, B115200);
  if (tcsetattr(fd, TCSANOW, &term) < 0) {
    ut_errno = UT_ERR_SET_BAUD;
    return -1;
  }

  return rv;
}

/* Returns the byte read back (0..255), or -1 on error with ut_errno set. */
static int owWriteByte(int fd, unsigned char wbuff)
{
  unsigned char buf[8];
  unsigned char rbuff = 0;
  ssize_t wbytes, rbytes, i;
  size_t remaining = 8;
  fd_set readset;
  struct timeval timeout_tv;

  tcflush(fd, TCIOFLUSH);

  for (i = 0; i < 8; i++) {
    buf[i] = (wbuff & (1 << i)) ? 0xff : 0x00;
  }
  wbytes = write(fd, buf, 8);
  if (wbytes != 8) {
    ut_errno = UT_ERR_SEND;
    return -1;
  }

  timeout_tv.tv_usec = 0;
  timeout_tv.tv_sec = TIMEOUT;

  while (remaining > 0) {
    FD_ZERO(&readset);
    FD_SET(fd, &readset);

    if (select(fd + 1, &readset, NULL, NULL, &timeout_tv) <= 0 || !FD_ISSET(fd, &readset)) {
      ut_errno = UT_ERR_RECEIVE;
      return -1;
    }

    rbytes = read(fd, buf, remaining);
    if (rbytes <= 0) {
      ut_errno = UT_ERR_RECEIVE;
      return -1;
    }
    for (i = 0; i < rbytes; i++) {
      rbuff >>= 1;
      rbuff |= (buf[i] & 0x01) ? 0x80 : 0x00;
    }
    remaining -= (size_t)rbytes;
  }
  return rbuff;
}

int owRead(int fd)
{
  return owWriteByte(fd, 0xff);
}

int owWrite(int fd, unsigned char wbuff)
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

static int file_exists(const char *filename)
{
  struct stat st;

  return (stat(filename, &st) == 0);
}

int owOpen(const char *serial_port)
{
  int fd;
  struct termios term;

  if (!file_exists(serial_port)) {
    ut_errno = UT_ERR_NO_PORT;
    return -1;
  }

  if (access(serial_port, R_OK | W_OK) < 0) {
    ut_errno = UT_ERR_PERMISSION;
    return -1;
  }

  fd = open(serial_port, O_RDWR);
  if (fd < 0) {
    ut_errno = UT_ERR_OPEN;
    return -1;
  }

  memset(&term, 0, sizeof(term));

  term.c_cc[VMIN] = 1;
  term.c_cc[VTIME] = 0;
  term.c_cflag |= CS6 | CREAD | HUPCL | CLOCAL;

  cfsetispeed(&term, B115200);
  cfsetospeed(&term, B115200);

  if (tcsetattr(fd, TCSANOW, &term) < 0) {
    close(fd);
    ut_errno = UT_ERR_SET_BAUD;
    return -1;
  }

  tcflush(fd, TCIOFLUSH);

  return fd;
}

void owClose(int fd)
{
  close(fd);
}
