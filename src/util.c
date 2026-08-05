/* See LICENSE.dwm file for copyright and license details. */
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

void die(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
    fputc(' ', stderr);
    perror(NULL);
  } else {
    fputc('\n', stderr);
  }

  exit(1);
}

void *ecalloc(size_t nmemb, size_t size) {
  void *p;

  if (!(p = calloc(nmemb, size)))
    die("calloc:");
  return p;
}

int fd_set_nonblock(int fd) {
  int flags = fcntl(fd, F_GETFL);
  if (flags < 0) {
    perror("fcntl(F_GETFL):");
    return -1;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    perror("fcntl(F_SETFL):");
    return -1;
  }

  return 0;
}

static int parse_hex2(const char *p) {
  int hi = p[0], lo = p[1];
  if (!isxdigit(hi) || !isxdigit(lo))
    return -1;
  if (isdigit(hi))
    hi -= '0';
  else
    hi = (hi | 0x20) - 'a' + 10;
  if (isdigit(lo))
    lo -= '0';
  else
    lo = (lo | 0x20) - 'a' + 10;
  return (hi << 4) | lo;
}

int hex_to_rgba(const char *s, float out[4]) {
  if (!s)
    return -1;
  if (s[0] == '#')
    s++;
  size_t len = strlen(s);
  if (len != 6 && len != 8)
    return -1;

  int r = parse_hex2(s);
  int g = parse_hex2(s + 2);
  int b = parse_hex2(s + 4);
  int a = (len == 8) ? parse_hex2(s + 6) : 255;
  if (r < 0 || g < 0 || b < 0 || a < 0)
    return -1;

  out[0] = r / 255.0f;
  out[1] = g / 255.0f;
  out[2] = b / 255.0f;
  out[3] = a / 255.0f;
  return 0;
}
