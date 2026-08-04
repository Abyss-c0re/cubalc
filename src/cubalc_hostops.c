#define _POSIX_C_SOURCE 200809L
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

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

int cubalc_host_exists(const char *path) {
  struct stat st;
  if (!path || !path[0]) return 0;
  return stat(path, &st) == 0 ? 1 : 0;
}

int cubalc_host_read(const char *path, cubalc_host_result *r) {
  r_clear(r);
  if (!path || !path[0]) { snprintf(r->err, sizeof r->err, "read: empty path"); return -1; }
  FILE *f = fopen(path, "rb");
  if (!f) { snprintf(r->err, sizeof r->err, "read: %s", strerror(errno)); return -1; }
  size_t n = fread(r->str, 1, sizeof r->str - 1, f);
  r->str[n] = 0;
  fclose(f);
  r->n = (long)n;
  r->ok = 1;
  r->code = 0;
  return 0;
}

int cubalc_host_write(const char *path, const char *data, cubalc_host_result *r) {
  r_clear(r);
  if (!path || !path[0]) { snprintf(r->err, sizeof r->err, "write: empty path"); return -1; }
  FILE *f = fopen(path, "wb");
  if (!f) { snprintf(r->err, sizeof r->err, "write: %s", strerror(errno)); return -1; }
  const char *d = data ? data : "";
  size_t n = fwrite(d, 1, strlen(d), f);
  fclose(f);
  r->n = (long)n;
  r->ok = 1;
  return 0;
}

int cubalc_host_env(const char *name, cubalc_host_result *r) {
  r_clear(r);
  if (!name) { snprintf(r->err, sizeof r->err, "env: empty"); return -1; }
  const char *v = getenv(name);
  if (!v) { r->ok = 1; r->str[0] = 0; r->n = 0; return 0; }
  snprintf(r->str, sizeof r->str, "%s", v);
  r->n = (long)strlen(r->str);
  r->ok = 1;
  return 0;
}

int cubalc_host_which(const char *name, cubalc_host_result *r) {
  r_clear(r);
  if (!name) return -1;
  char try[512];
  const char *home = getenv("HOME");
  const char *cbin = getenv("CUBALC_BIN");
  if (cbin && strstr(name, "cubalc") && access(cbin, X_OK) == 0) {
    snprintf(r->str, sizeof r->str, "%s", cbin); r->ok=1; r->n=1; return 0;
  }
  if (name[0]=='/' && access(name, X_OK)==0) {
    snprintf(r->str, sizeof r->str, "%s", name); r->ok=1; r->n=1; return 0;
  }
  snprintf(try, sizeof try, "./out/%s", name);
  if (access(try, X_OK) == 0) { snprintf(r->str, sizeof r->str, "%s", try); r->ok=1; r->n=1; return 0; }
  if (home) {
    snprintf(try, sizeof try, "%s/.local/bin/%s", home, name);
    if (access(try, X_OK) == 0) { snprintf(r->str, sizeof r->str, "%s", try); r->ok=1; r->n=1; return 0; }
  }
  /* PATH search */
  const char *path = getenv("PATH");
  if (path) {
    char pbuf[4096]; snprintf(pbuf, sizeof pbuf, "%s", path);
    for (char *tok = strtok(pbuf, ":"); tok; tok = strtok(NULL, ":")) {
      snprintf(try, sizeof try, "%s/%s", tok, name);
      if (access(try, X_OK) == 0) {
        snprintf(r->str, sizeof r->str, "%s", try); r->ok=1; r->n=1; return 0;
      }
    }
  }
  snprintf(r->err, sizeof r->err, "which: not found %s", name);
  return -1;
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

/* ---- Grokium CubalC surface: join / json / chat ---- */

int cubalc_host_join(const char *a, const char *b, cubalc_host_result *r) {
  r_clear(r);
  if (!a) a = "";
  if (!b) b = "";
  size_t la = strlen(a), lb = strlen(b);
  int need_slash = 0;
  if (la > 0 && lb > 0 && a[la - 1] != '/' && b[0] != '/') need_slash = 1;
  if (la + lb + need_slash + 1 >= sizeof r->str) {
    snprintf(r->err, sizeof r->err, "join: too long");
    return -1;
  }
  if (need_slash)
    snprintf(r->str, sizeof r->str, "%s/%s", a, b);
  else
    snprintf(r->str, sizeof r->str, "%s%s", a, b);
  r->n = (long)strlen(r->str);
  r->ok = 1;
  return 0;
}

int cubalc_host_json_get(const char *json, const char *key, cubalc_host_result *r) {
  r_clear(r);
  if (!json || !key || !key[0]) {
    snprintf(r->err, sizeof r->err, "json: bad args");
    return -1;
  }
  char pat[96];
  snprintf(pat, sizeof pat, "\"%s\"", key);
  const char *p = json;
  int any = 0;
  char best[CUBALC_HOST_STR_MAX];
  best[0] = 0;
  while ((p = strstr(p, pat)) != NULL) {
    p += strlen(pat);
    while (*p == ' ' || *p == '\t') p++;
    if (*p != ':') continue;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    char tmp[CUBALC_HOST_STR_MAX];
    size_t k = 0;
    tmp[0] = 0;
    if (*p == '"') {
      /* string value */
      p++;
      while (*p && k + 1 < sizeof tmp) {
        if (*p == '"') break;
        if (*p == '\\' && p[1]) {
          p++;
          if (*p == 'n') { tmp[k++] = '\n'; p++; continue; }
          if (*p == 't') { tmp[k++] = '\t'; p++; continue; }
          if (*p == 'r') { p++; continue; }
          if (*p == 'u') {
            p++;
            for (int i = 0; i < 4 && isxdigit((unsigned char)*p); i++) p++;
            tmp[k++] = '?';
            continue;
          }
          tmp[k++] = *p++;
          continue;
        }
        tmp[k++] = *p++;
      }
      tmp[k] = 0;
      if (*p == '"') p++;
    } else if (*p == '-' || isdigit((unsigned char)*p)) {
      /* number → string so SYS NUM / SETDIGIT can fold peer digits */
      if (*p == '-') tmp[k++] = *p++;
      while (*p && k + 1 < sizeof tmp &&
             (isdigit((unsigned char)*p) || *p == '.' || *p == 'e' || *p == 'E' ||
              *p == '+' || *p == '-')) {
        tmp[k++] = *p++;
      }
      tmp[k] = 0;
    } else if (strncmp(p, "true", 4) == 0) {
      snprintf(tmp, sizeof tmp, "1");
      k = 1;
      p += 4;
    } else if (strncmp(p, "false", 5) == 0) {
      snprintf(tmp, sizeof tmp, "0");
      k = 1;
      p += 5;
    } else if (strncmp(p, "null", 4) == 0) {
      tmp[0] = 0;
      k = 0;
      p += 4;
    } else {
      continue;
    }
    size_t nz = 0;
    for (size_t i = 0; i < k; i++) if ((unsigned char)tmp[i] > ' ') nz++;
    if (nz || k == 0) {
      /* last match wins when key repeats */
      snprintf(best, sizeof best, "%s", tmp);
      any = 1;
    }
  }
  if (!any) {
    snprintf(r->err, sizeof r->err, "json: no field %s", key);
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", best);
  r->n = (long)strlen(r->str);
  r->ok = 1;
  return 0;
}

static int load_token(char *out, size_t outn) {
  out[0] = 0;
  const char *e = getenv("XAI_API_KEY");
  if (!e || !e[0]) e = getenv("GROK_API_KEY");
  if (!e || !e[0]) e = getenv("CUBALC_HTTP_AUTH");
  if (e && e[0]) { snprintf(out, outn, "%s", e); return 0; }
  const char *home = getenv("HOME");
  if (!home) return -1;
  char path[512];
  snprintf(path, sizeof path, "%s/.grok/auth.json", home);
  FILE *f = fopen(path, "r");
  if (!f) return -1;
  char *buf = malloc(512 * 1024);
  if (!buf) { fclose(f); return -1; }
  size_t n = fread(buf, 1, 512 * 1024 - 1, f);
  buf[n] = 0;
  fclose(f);
  cubalc_host_result jr;
  if (cubalc_host_json_get(buf, "key", &jr) == 0 && strncmp(jr.str, "eyJ", 3) == 0) {
    snprintf(out, outn, "%s", jr.str);
    free(buf);
    return 0;
  }
  if (cubalc_host_json_get(buf, "access_token", &jr) == 0) {
    snprintf(out, outn, "%s", jr.str);
    free(buf);
    return 0;
  }
  free(buf);
  return -1;
}

static void json_esc(FILE *f, const char *s) {
  if (!s) return;
  for (const char *p = s; *p; p++) {
    if (*p == '"' || *p == '\\') fputc('\\', f);
    else if (*p == '\n') fputs("\\n", f);
    else if ((unsigned char)*p < 32) continue;
    else fputc(*p, f);
  }
}

static int resolve_local_model_id(char *model, size_t n) {
  if (model[0] && strcmp(model, "local") != 0)
    return 0;
  FILE *mf = popen("curl -sS -m 3 http://127.0.0.1:1212/v1/models 2>/dev/null", "r");
  if (!mf) { if (!model[0]) snprintf(model, n, "local"); return 0; }
  char buf[8192];
  size_t nr = fread(buf, 1, sizeof buf - 1, mf);
  buf[nr] = 0;
  pclose(mf);
  cubalc_host_result jr;
  if (cubalc_host_json_get(buf, "id", &jr) == 0 && jr.str[0]) {
    snprintf(model, n, "%s", jr.str);
    return 0;
  }
  if (!model[0]) snprintf(model, n, "local");
  return 0;
}

int cubalc_host_chat(const char *backend, const char *model_in, const char *msg,
                     const char *state_dir, cubalc_host_result *r) {
  r_clear(r);
  if (!msg || !msg[0]) {
    snprintf(r->err, sizeof r->err, "chat: empty msg");
    return -1;
  }
  int is_grok = backend && (strcmp(backend, "grok") == 0 || strcmp(backend, "xai") == 0);
  char token[8192];
  token[0] = 0;
  if (is_grok && load_token(token, sizeof token) != 0) {
    snprintf(r->err, sizeof r->err, "chat: no token");
    return -1;
  }
  char model[512];
  snprintf(model, sizeof model, "%s",
           model_in && model_in[0] ? model_in : (is_grok ? "grok-4" : "local"));
  if (!is_grok) {
    if (strncmp(model, "grok", 4) == 0) snprintf(model, sizeof model, "local");
    resolve_local_model_id(model, sizeof model);
  } else if (strcmp(model, "local") == 0) {
    snprintf(model, sizeof model, "grok-4");
  }

  const char *url = is_grok
    ? "https://api.x.ai/v1/chat/completions"
    : "http://127.0.0.1:1212/v1/chat/completions";
  if (is_grok) {
    const char *u = getenv("GROKIUM_GROK_URL");
    if (u && u[0]) url = u;
  } else {
    const char *u = getenv("GROKIUM_LOCAL_URL");
    if (u && u[0]) url = u;
  }

  char bodypath[512];
  const char *sd = state_dir && state_dir[0] ? state_dir : "/tmp";
  snprintf(bodypath, sizeof bodypath, "%s/_cblc_chat_body.json", sd);
  mkdir(sd, 0755);
  FILE *bf = fopen(bodypath, "w");
  if (!bf) {
    snprintf(r->err, sizeof r->err, "chat: body write");
    return -1;
  }
  fputs("{\"model\":\"", bf);
  json_esc(bf, model);
  fputs("\",\"messages\":[{\"role\":\"system\",\"content\":\"", bf);
  if (is_grok)
    json_esc(bf, "Concise helpful assistant. Use tools via host when offered.");
  else
    json_esc(bf, "Reply one short plain sentence. No chain-of-thought.");
  fputs("\"},{\"role\":\"user\",\"content\":\"", bf);
  json_esc(bf, msg);
  if (is_grok)
    fputs("\"}],\"max_tokens\":512,\"temperature\":0.7,\"stream\":false}", bf);
  else
    fputs("\"}],\"max_tokens\":256,\"temperature\":0.7,\"stream\":false,"
          "\"chat_template_kwargs\":{\"enable_thinking\":false}}", bf);
  fclose(bf);

  /* read body back for http */
  cubalc_host_result br;
  if (cubalc_host_read(bodypath, &br) != 0) {
    snprintf(r->err, sizeof r->err, "chat: body read");
    return -1;
  }

  /* HTTP with optional bearer — inline curl for auth */
  char tmpout[] = "/tmp/cubalc_chat_XXXXXX";
  int fd = mkstemp(tmpout);
  if (fd < 0) { snprintf(r->err, sizeof r->err, "chat: tmp"); return -1; }
  close(fd);
  pid_t pid = fork();
  if (pid < 0) { unlink(tmpout); return -1; }
  if (pid == 0) {
    int out = open(tmpout, O_WRONLY | O_TRUNC);
    if (out >= 0) { dup2(out, 1); dup2(out, 2); close(out); }
    char dflag[600];
    snprintf(dflag, sizeof dflag, "@%s", bodypath);
    if (is_grok) {
      char auth[8500];
      snprintf(auth, sizeof auth, "Authorization: Bearer %s", token);
      execlp("curl", "curl", "-sS", "-m", "120",
             "-H", "Content-Type: application/json",
             "-H", "User-Agent: grokium/c3 (cubalc)",
             "-H", auth,
             "-d", dflag,
             "-w", "\n__HTTP_CODE__%{http_code}",
             url, (char *)NULL);
    } else {
      execlp("curl", "curl", "-sS", "-m", "90",
             "-H", "Content-Type: application/json",
             "-d", dflag,
             "-w", "\n__HTTP_CODE__%{http_code}",
             url, (char *)NULL);
    }
    _exit(127);
  }
  int st = 0;
  waitpid(pid, &st, 0);
  FILE *f = fopen(tmpout, "rb");
  if (!f) { unlink(tmpout); snprintf(r->err, sizeof r->err, "chat: no out"); return -1; }
  size_t n = fread(r->str, 1, sizeof r->str - 1, f);
  r->str[n] = 0;
  fclose(f);
  unlink(tmpout);
  char *p = strstr(r->str, "__HTTP_CODE__");
  if (p) {
    r->code = atoi(p + 13);
    *p = 0;
  } else {
    r->code = WIFEXITED(st) && WEXITSTATUS(st) == 0 ? 200 : 0;
  }
  /* cache raw */
  {
    char lastp[512];
    snprintf(lastp, sizeof lastp, "%s/_chat_last.json", sd);
    FILE *lf = fopen(lastp, "w");
    if (lf) { fputs(r->str, lf); fclose(lf); }
  }
  cubalc_host_result jr;
  if (cubalc_host_json_get(r->str, "content", &jr) != 0 || !jr.str[0]) {
    if (cubalc_host_json_get(r->str, "reasoning_content", &jr) != 0) {
      snprintf(r->err, sizeof r->err, "chat: no content http=%d", r->code);
      r->ok = 0;
      return -1;
    }
  }
  /* squash ws lightly */
  {
    char buf[CUBALC_HOST_STR_MAX];
    size_t o = 0;
    int sp = 1;
    for (char *q = jr.str; *q && o + 1 < sizeof buf; q++) {
      unsigned char c = (unsigned char)*q;
      if (c == '\n' || c == '\r' || c == '\t' || c == ' ') {
        if (!sp) { buf[o++] = ' '; sp = 1; }
        continue;
      }
      sp = 0;
      buf[o++] = (char)c;
    }
    while (o > 0 && buf[o - 1] == ' ') o--;
    buf[o] = 0;
    snprintf(r->str, sizeof r->str, "%s", buf);
  }
  r->n = (long)strlen(r->str);
  r->ok = 1;
  return 0;
}
