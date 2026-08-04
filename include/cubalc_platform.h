/* CubalC platform shims — pure C, multiplatform (law: devices free). */
#ifndef CUBALC_PLATFORM_H
#define CUBALC_PLATFORM_H

#if defined(_WIN32) || defined(_WIN64)
#  ifndef CUBALC_OS_WINDOWS
#  define CUBALC_OS_WINDOWS 1
#  endif
#  ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
#  endif
#elif defined(__APPLE__)
#  define CUBALC_OS_DARWIN 1
#  ifndef _DARWIN_C_SOURCE
#  define _DARWIN_C_SOURCE 1
#  endif
#else
#  define CUBALC_OS_POSIX 1
#  ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 200809L
#  endif
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <errno.h>

#if defined(CUBALC_OS_WINDOWS)
#  include <io.h>
#  include <process.h>
#  ifndef strcasecmp
#    define strcasecmp _stricmp
#  endif
#  ifndef strncasecmp
#    define strncasecmp _strnicmp
#  endif
#else
#  include <strings.h>
#  include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CUBALC_OS_WINDOWS)
#  define CUBALC_PATH_SEP '\\'
#  define CUBALC_PATH_SEP_STR "\\"
#else
#  define CUBALC_PATH_SEP '/'
#  define CUBALC_PATH_SEP_STR "/"
#endif

static inline const char *cubalc_path_slash(const char *p) {
  if (!p) return NULL;
  const char *a = strrchr(p, '/');
#if defined(CUBALC_OS_WINDOWS)
  const char *b = strrchr(p, '\\');
  if (!a || (b && b > a)) a = b;
#endif
  return a;
}

#ifdef __cplusplus
}
#endif
#endif /* CUBALC_PLATFORM_H */
