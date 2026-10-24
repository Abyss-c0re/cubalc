/* CubalC — hostops groups/ingroup/copy peel (B_hostops_margin).
 * SYS GROUPS · INGROUP · COPY|CP. Mesh vote · zero bloat · cube is SoT.
 */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#  include <grp.h>
#endif

static void r_clear(cubalc_host_result *r) {
  if (!r) return;
  memset(r, 0, sizeof *r);
}

/* Usability: SYS GROUPS — process group names (egid + supplementary) without shell id -Gn. */
int cubalc_host_groups(cubalc_host_result *r) {
  gid_t list[64];
  int n = 0, i, j, count = 0;
  size_t o = 0;
  gid_t eg;
  r_clear(r);
  r->str[0] = 0;
#if defined(CUBALC_OS_WINDOWS)
  snprintf(r->str, sizeof r->str, "%s", "group");
  r->n = 1;
  r->ok = 1;
  return 0;
#else
  eg = getegid();
  n = getgroups((int)(sizeof list / sizeof list[0]), list);
  if (n < 0) n = 0;
  /* ensure egid present first */
  {
    int has_eg = 0;
    for (i = 0; i < n; i++)
      if (list[i] == eg) { has_eg = 1; break; }
    if (!has_eg && n < (int)(sizeof list / sizeof list[0])) {
      /* shift and insert eg at front for stable primary-first order */
      for (i = n; i > 0; i--) list[i] = list[i - 1];
      list[0] = eg;
      n++;
    }
  }
  for (i = 0; i < n; i++) {
    /* dedupe */
    int dup = 0;
    char name[128];
    size_t L;
    for (j = 0; j < i; j++)
      if (list[j] == list[i]) { dup = 1; break; }
    if (dup) continue;
    name[0] = 0;
    {
      struct group *gr = getgrgid(list[i]);
      if (gr && gr->gr_name && gr->gr_name[0])
        snprintf(name, sizeof name, "%s", gr->gr_name);
      else
        snprintf(name, sizeof name, "%ld", (long)list[i]);
    }
    L = strlen(name);
    if (o + L + 2 >= sizeof r->str) break;
    if (o > 0) r->str[o++] = '\n';
    memcpy(r->str + o, name, L);
    o += L;
    r->str[o] = 0;
    count++;
  }
  r->n = (long)count;
  r->ok = 1;
  return 0;
#endif
}

/* Usability: SYS INGROUP name|gid — process membership probe without shell id -nG. */
int cubalc_host_ingroup(const char *name_or_gid, cubalc_host_result *r) {
  gid_t target = (gid_t)-1;
  gid_t list[64];
  int n, i;
  char *end = NULL;
  unsigned long v;
  r_clear(r);
  if (!name_or_gid || !name_or_gid[0]) {
    snprintf(r->err, sizeof r->err, "ingroup: empty name");
    return -1;
  }
#if defined(CUBALC_OS_WINDOWS)
  r->n = 0;
  snprintf(r->str, sizeof r->str, "0");
  r->ok = 1;
  return 0;
#else
  /* decimal gid? */
  v = strtoul(name_or_gid, &end, 10);
  if (end && end != name_or_gid && *end == 0)
    target = (gid_t)v;
  else {
    struct group *gr = getgrnam(name_or_gid);
    if (!gr) {
      r->n = 0;
      snprintf(r->str, sizeof r->str, "0");
      r->ok = 1;
      return 0;
    }
    target = gr->gr_gid;
  }
  if (getegid() == target || getgid() == target) {
    r->n = 1;
    snprintf(r->str, sizeof r->str, "1");
    r->ok = 1;
    return 0;
  }
  n = getgroups((int)(sizeof list / sizeof list[0]), list);
  if (n < 0) n = 0;
  for (i = 0; i < n; i++) {
    if (list[i] == target) {
      r->n = 1;
      snprintf(r->str, sizeof r->str, "1");
      r->ok = 1;
      return 0;
    }
  }
  r->n = 0;
  snprintf(r->str, sizeof r->str, "0");
  r->ok = 1;
  return 0;
#endif
}

/* Usability: SYS COPY|CP src dst — duplicate plate without shell. */
int cubalc_host_copy(const char *src, const char *dst, cubalc_host_result *r) {
  FILE *in = NULL, *out = NULL;
  char buf[4096];
  size_t nr, nw;
  long total = 0;
  struct stat st;
  r_clear(r);
  if (!src || !src[0] || !dst || !dst[0]) {
    snprintf(r->err, sizeof r->err, "copy: empty path");
    return -1;
  }
  if (stat(src, &st) != 0) {
    snprintf(r->err, sizeof r->err, "copy: missing source");
    return -1;
  }
  if (S_ISDIR(st.st_mode)) {
    snprintf(r->err, sizeof r->err, "copy: source is a directory");
    return -1;
  }
  /* refuse copy onto self */
  if (strcmp(src, dst) == 0) {
    snprintf(r->str, sizeof r->str, "%s", dst);
    r->n = (long)st.st_size;
    r->ok = 1;
    return 0;
  }
  in = fopen(src, "rb");
  if (!in) {
    snprintf(r->err, sizeof r->err, "copy: open src %s", strerror(errno));
    return -1;
  }
  out = fopen(dst, "wb");
  if (!out) {
    fclose(in);
    snprintf(r->err, sizeof r->err, "copy: open dst %s", strerror(errno));
    return -1;
  }
  while ((nr = fread(buf, 1, sizeof buf, in)) > 0) {
    nw = fwrite(buf, 1, nr, out);
    if (nw != nr) {
      fclose(in);
      fclose(out);
      snprintf(r->err, sizeof r->err, "copy: write short");
      return -1;
    }
    total += (long)nw;
  }
  if (ferror(in)) {
    fclose(in);
    fclose(out);
    snprintf(r->err, sizeof r->err, "copy: read error");
    return -1;
  }
  fclose(in);
  if (fclose(out) != 0) {
    snprintf(r->err, sizeof r->err, "copy: close dst %s", strerror(errno));
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", dst);
  r->n = total;
  r->ok = 1;
  return 0;
}
