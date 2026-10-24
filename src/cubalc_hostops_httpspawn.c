/* CubalC — hostops http/spawn peel (B_hostops_margin).
 * SYS HTTP · SYS SPAWN · allowlists. Mesh vote · zero bloat · cube is SoT.
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
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

static void r_clear(cubalc_host_result *r) {
  if (!r) return;
  memset(r, 0, sizeof *r);
}

/* Core protect: CUBALC_PROTECT=1 tightens host edge (fail-closed for Core stability). */
static int protect_mode(void) {
  const char *p = getenv("CUBALC_PROTECT");
  return (p && p[0] && p[0] != '0') ? 1 : 0;
}

static int url_allowed(const char *url) {
  if (!url) return 0;
  if (strncmp(url, "http://127.0.0.1", 16) == 0) return 1;
  if (strncmp(url, "http://localhost", 16) == 0) return 1;
  if (strncmp(url, "http://[::1]", 12) == 0) return 1;
  /* protect mode: loopback only unless explicit protect-http override */
  if (protect_mode()) {
    const char *ph = getenv("CUBALC_PROTECT_HTTP");
    if (!(ph && ph[0] && ph[0] != '0')) return 0;
  }
  if (strncmp(url, "https://api.x.ai/", 17) == 0) return 1;
  const char *extra = getenv("CUBALC_HTTP_ALLOW");
  if (extra && extra[0] && strstr(url, extra)) return 1;
  return 0;
}

static int base_in_allowlist(const char *base, const char *list) {
  /* colon-separated basenames, e.g. "nanobot:extra:tool" */
  if (!base || !list || !list[0]) return 0;
  const char *p = list;
  size_t bl = strlen(base);
  while (*p) {
    while (*p == ':' || *p == ',' || *p == ' ') p++;
    if (!*p) break;
    const char *start = p;
    while (*p && *p != ':' && *p != ',' && *p != ' ') p++;
    size_t n = (size_t)(p - start);
    if (n == bl && strncmp(start, base, bl) == 0) return 1;
  }
  return 0;
}

static int bin_allowed(const char *bin) {
  if (!bin || !bin[0]) return 0;
  const char *base = strrchr(bin, '/');
  base = base ? base + 1 : bin;
  /* core hive tools only — extend via CUBALC_SPAWN_ALLOW (no device hardcode) */
  /* protect mode: nanobot + cubalc only (curl needs CUBALC_PROTECT_HTTP=1) */
  if (strcmp(base, "nanobot") == 0) return 1;
  if (strcmp(base, "cubalc") == 0) return 1;
  if (strcmp(base, "curl") == 0) {
    if (protect_mode()) {
      const char *ph = getenv("CUBALC_PROTECT_HTTP");
      if (!(ph && ph[0] && ph[0] != '0')) return 0;
    }
    return 1; /* internal HTTP helper */
  }
  if (protect_mode()) {
    /* fail-closed: no extra SPAWN basenames under Core protect */
    return 0;
  }
  const char *extra = getenv("CUBALC_SPAWN_ALLOW");
  if (base_in_allowlist(base, extra)) return 1;
  /* absolute path only if basename already allowed */
  if (bin[0] == '/' &&
      (strcmp(base, "nanobot") == 0 || strcmp(base, "cubalc") == 0 ||
       strcmp(base, "curl") == 0 || base_in_allowlist(base, extra)))
    return 1;
  return 0;
}

/* minimal HTTP/1.0 via TCP using /dev/tcp equivalent: use curl if allowlisted path */
int cubalc_host_http(const char *method, const char *url, const char *body,
                     cubalc_host_result *r) {
  r_clear(r);
  if (!url_allowed(url)) {
    snprintf(r->err, sizeof r->err, "http: url not allowlisted");
    return -1;
  }
  const char *m = method && method[0] ? method : "GET";
  char tmpout[] = "/tmp/cubalc_http_XXXXXX";
  int fd = mkstemp(tmpout);
  if (fd < 0) { snprintf(r->err, sizeof r->err, "http: tmp"); return -1; }
  close(fd);

  pid_t pid = fork();
  if (pid < 0) { unlink(tmpout); return -1; }
  if (pid == 0) {
    int out = open(tmpout, O_WRONLY | O_TRUNC);
    if (out >= 0) { dup2(out, 1); dup2(out, 2); close(out); }
    {
      /* POST chat can be long; override with CUBALC_HTTP_TIMEOUT seconds */
      const char *tmax =
        (body && body[0] && (strcmp(m, "POST") == 0 || strcmp(m, "PUT") == 0))
          ? "120" : "15";
      const char *envt = getenv("CUBALC_HTTP_TIMEOUT");
      if (envt && envt[0]) tmax = envt;
      if (body && body[0] && (strcmp(m, "POST") == 0 || strcmp(m, "PUT") == 0)) {
        execlp("curl", "curl", "-sS", "-m", tmax, "-X", m,
               "-H", "Content-Type: application/json",
               "-d", body, "-w", "\n__HTTP_CODE__%{http_code}",
               url, (char *)NULL);
      } else {
        execlp("curl", "curl", "-sS", "-m", tmax, "-X", m,
               "-w", "\n__HTTP_CODE__%{http_code}",
               url, (char *)NULL);
      }
    }
    _exit(127);
  }
  int st = 0;
  waitpid(pid, &st, 0);
  FILE *f = fopen(tmpout, "rb");
  if (!f) { unlink(tmpout); snprintf(r->err, sizeof r->err, "http: no out"); return -1; }
  size_t n = fread(r->str, 1, sizeof r->str - 1, f);
  r->str[n] = 0;
  fclose(f);
  unlink(tmpout);
  /* parse code trailer */
  char *p = strstr(r->str, "__HTTP_CODE__");
  if (p) {
    r->code = atoi(p + 13);
    *p = 0;
    r->n = (long)strlen(r->str);
  } else {
    r->code = WIFEXITED(st) && WEXITSTATUS(st) == 0 ? 200 : 0;
    r->n = (long)n;
  }
  r->ok = (r->code >= 200 && r->code < 400) ? 1 : 0;
  if (!r->ok && !r->err[0])
    snprintf(r->err, sizeof r->err, "http: code %d", r->code);
  return r->ok ? 0 : -1;
}

int cubalc_host_spawn(const char *bin, char *const argv[], cubalc_host_result *r) {
  r_clear(r);
  if (!bin_allowed(bin)) {
    snprintf(r->err, sizeof r->err, "spawn: binary not allowlisted");
    return -1;
  }
  if (access(bin, X_OK) != 0) {
    snprintf(r->err, sizeof r->err, "spawn: not executable %s", bin);
    return -1;
  }
  int pipefd[2];
  if (pipe(pipefd) != 0) return -1;
  pid_t pid = fork();
  if (pid < 0) { close(pipefd[0]); close(pipefd[1]); return -1; }
  if (pid == 0) {
    close(pipefd[0]);
    dup2(pipefd[1], 1);
    dup2(pipefd[1], 2);
    close(pipefd[1]);
    execv(bin, argv);
    _exit(127);
  }
  close(pipefd[1]);
  size_t o = 0;
  char buf[512];
  ssize_t nr;
  while ((nr = read(pipefd[0], buf, sizeof buf)) > 0) {
    if (o + (size_t)nr >= sizeof r->str) nr = (ssize_t)(sizeof r->str - 1 - o);
    if (nr <= 0) break;
    memcpy(r->str + o, buf, (size_t)nr);
    o += (size_t)nr;
  }
  r->str[o] = 0;
  close(pipefd[0]);
  int st = 0;
  waitpid(pid, &st, 0);
  r->code = WIFEXITED(st) ? WEXITSTATUS(st) : 1;
  r->n = (long)o;
  r->ok = (r->code == 0) ? 1 : 0;
  if (!r->ok) snprintf(r->err, sizeof r->err, "spawn exit %d", r->code);
  return r->ok ? 0 : -1;
}
