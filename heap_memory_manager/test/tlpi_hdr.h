/* Minimal stand-in for the TLPI book's tlpi_hdr.h (only what free_and_sbrk.c needs). */
#ifndef TLPI_HDR_H
#define TLPI_HDR_H

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#define GN_NONNEG   01   /* value must be >= 0 */
#define GN_GT_0     02   /* value must be > 0 */
#define GN_ANY_BASE 0100 /* accept any base (like strtol with base 0) */
#define GN_BASE_8   0200
#define GN_BASE_16  0400

static inline void errExit(const char *msg)
{
  perror(msg);
  exit(EXIT_FAILURE);
}

static inline void usageErr(const char *fmt, ...)
{
  va_list ap;
  fprintf(stderr, "Usage: ");
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  exit(EXIT_FAILURE);
}

static inline void cmdLineErr(const char *fmt, ...)
{
  va_list ap;
  fprintf(stderr, "Command-line usage error: ");
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  exit(EXIT_FAILURE);
}

static inline int getInt(const char *arg, int flags, const char *name)
{
  char *end;
  int base = (flags & GN_ANY_BASE) ? 0 : (flags & GN_BASE_8) ? 8 : (flags & GN_BASE_16) ? 16 : 10;
  errno = 0;
  long v = strtol(arg, &end, base);
  if (errno != 0 || *end != '\0' || end == arg)
  {
    fprintf(stderr, "invalid number for %s: %s\n", name, arg);
    exit(EXIT_FAILURE);
  }
  if ((flags & GN_NONNEG) && v < 0)
  {
    fprintf(stderr, "%s must be >= 0\n", name);
    exit(EXIT_FAILURE);
  }
  if ((flags & GN_GT_0) && v <= 0)
  {
    fprintf(stderr, "%s must be > 0\n", name);
    exit(EXIT_FAILURE);
  }
  return (int)v;
}

#endif /* TLPI_HDR_H */
