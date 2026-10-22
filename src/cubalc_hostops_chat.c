/* CubalC — hostops chat + FILLP peel (G_split_hostops).
 * Mesh vote · zero bloat · cube is SoT.
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
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

static void r_clear(cubalc_host_result *r) {
  if (!r) return;
  memset(r, 0, sizeof *r);
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


/* Expand {{key}} from plate object — CLI + agents without .cubalc FILLP. */
int cubalc_host_expand_fillp(const char *plate, const char *tmpl,
                             char *out, size_t outcap,
                             long *hits_out, long *miss_out,
                             char *miss_bag, size_t miss_cap) {
  const char *src = tmpl ? tmpl : "";
  const char *pl = (plate && plate[0]) ? plate : "{}";
  size_t o = 0, mo = 0;
  long hits = 0, miss = 0;
  char name[96];
  cubalc_host_result gr;
  if (miss_bag && miss_cap) miss_bag[0] = 0;
  if (!out || outcap == 0) {
    if (hits_out) *hits_out = 0;
    if (miss_out) *miss_out = 0;
    return -1;
  }
  out[0] = 0;
  while (*src && o + 1 < outcap) {
    if (src[0] == '{' && src[1] == '{') {
      const char *p = src + 2;
      size_t ni = 0;
      name[0] = 0;
      while (*p && !(p[0] == '}' && p[1] == '}') && ni + 1 < sizeof name) {
        char ch = *p;
        /* dotted/slash nest paths: {{freq.error}} {{meta/role}} */
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') || ch == '_' || ch == '-' || ch == '.' ||
            ch == '/')
          name[ni++] = ch;
        else
          break;
        p++;
      }
      name[ni] = 0;
      if (name[0] && p[0] == '}' && p[1] == '}') {
        src = p + 2;
        hits++;
        memset(&gr, 0, sizeof gr);
        if (cubalc_host_json_path_get(pl, name, &gr) == 0) {
          size_t vn = strlen(gr.str);
          if (o + vn >= outcap) vn = outcap - 1 - o;
          if (vn > 0) {
            memcpy(out + o, gr.str, vn);
            o += vn;
          }
          out[o] = 0;
        } else {
          miss++;
          if (miss_bag && miss_cap > 1) {
            size_t nl = strlen(name);
            if (mo > 0 && mo + 1 < miss_cap) miss_bag[mo++] = '\n';
            if (mo + nl < miss_cap) {
              memcpy(miss_bag + mo, name, nl);
              mo += nl;
              miss_bag[mo] = 0;
            }
          }
        }
        continue;
      }
      out[o++] = *src++;
      continue;
    }
    out[o++] = *src++;
  }
  out[o] = 0;
  if (hits_out) *hits_out = hits;
  if (miss_out) *miss_out = miss;
  return 0;
}

int cubalc_host_fillp_keys(const char *tmpl, char *out, size_t outcap, long *nkeys) {
  const char *src = tmpl ? tmpl : "";
  size_t o = 0;
  long n = 0;
  char name[96];
  if (!out || outcap == 0) {
    if (nkeys) *nkeys = 0;
    return -1;
  }
  out[0] = 0;
  while (*src) {
    if (src[0] == '{' && src[1] == '{') {
      const char *p = src + 2;
      size_t ni = 0;
      name[0] = 0;
      while (*p && !(p[0] == '}' && p[1] == '}') && ni + 1 < sizeof name) {
        char ch = *p;
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') || ch == '_' || ch == '-' || ch == '.' ||
            ch == '/')
          name[ni++] = ch;
        else
          break;
        p++;
      }
      name[ni] = 0;
      if (name[0] && p[0] == '}' && p[1] == '}') {
        size_t nl;
        int seen = 0;
        const char *q;
        src = p + 2;
        nl = strlen(name);
        q = out;
        while (*q) {
          size_t ll = 0;
          while (q[ll] && q[ll] != '\n') ll++;
          if (ll == nl && memcmp(q, name, nl) == 0) { seen = 1; break; }
          q += ll;
          if (*q == '\n') q++;
        }
        if (!seen) {
          if (o > 0 && o + 1 < outcap) out[o++] = '\n';
          if (o + nl < outcap) {
            memcpy(out + o, name, nl);
            o += nl;
            out[o] = 0;
            n++;
          }
        }
        continue;
      }
      src++;
      continue;
    }
    src++;
  }
  if (nkeys) *nkeys = n;
  return 0;
}

