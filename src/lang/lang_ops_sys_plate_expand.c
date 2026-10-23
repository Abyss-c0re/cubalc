/* CubalC lang — lang_ops_sys_plate_expand.c
 * SUBSTENV / FILLP / FILLPKEYS helpers.
 * Mesh vote A_split_lang_ops_core · modular extract from lang_ops_core.
 */
#include "lang/cubalc_lang_ops_sys_plate_expand.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <string.h>

/* Expand $NAME / ${NAME} from process env, then program vars.
 * $$ → literal $. Unset → empty. *miss_out = count of unset names.
 * Returns number of $ expansions attempted (hits including miss). */
long cubalc_expand_substenv(VM *vm, const char *tmpl, char *out, size_t outcap,
                                   long *miss_out) {
  const char *src = tmpl ? tmpl : "";
  size_t o = 0;
  long hits = 0, miss = 0;
  char name[128];
  if (!out || outcap == 0) {
    if (miss_out) *miss_out = 0;
    return 0;
  }
  out[0] = 0;
  while (*src && o + 1 < outcap) {
    if (*src == '$') {
      if (src[1] == '$') {
        out[o++] = '$';
        src += 2;
        continue;
      }
      name[0] = 0;
      if (src[1] == '{') {
        const char *p = src + 2;
        size_t ni = 0;
        while (*p && *p != '}' && ni + 1 < sizeof name)
          name[ni++] = *p++;
        name[ni] = 0;
        if (*p == '}')
          src = p + 1;
        else {
          out[o++] = *src++;
          continue;
        }
      } else if ((src[1] >= 'A' && src[1] <= 'Z') ||
                 (src[1] >= 'a' && src[1] <= 'z') ||
                 src[1] == '_') {
        const char *p = src + 1;
        size_t ni = 0;
        while (*p && ni + 1 < sizeof name &&
               ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
                (*p >= '0' && *p <= '9') || *p == '_'))
          name[ni++] = *p++;
        name[ni] = 0;
        src = p;
      } else {
        out[o++] = *src++;
        continue;
      }
      {
        const char *val = NULL;
        char vbuf[CUBALC_HOST_STR_MAX];
        cubalc_host_result er;
        vbuf[0] = 0;
        cubalc_host_env(name, &er);
        if (er.str[0] && er.n > 0) {
          val = er.str;
        } else {
          Var *vv = var_get(vm, name, 0);
          if (vv) {
            if (vv->is_str) {
              snprintf(vbuf, sizeof vbuf, "%s", vv->sval);
              val = vbuf;
            } else {
              snprintf(vbuf, sizeof vbuf, "%ld", vv->val);
              val = vbuf;
            }
          }
        }
        if (val) {
          size_t vn = strlen(val);
          if (o + vn >= outcap) vn = outcap - 1 - o;
          if (vn > 0) {
            memcpy(out + o, val, vn);
            o += vn;
          }
          out[o] = 0;
          hits++;
        } else {
          miss++;
          hits++;
        }
      }
      continue;
    }
    out[o++] = *src++;
  }
  out[o] = 0;
  if (miss_out) *miss_out = miss;
  return hits;
}


/* Expand {{key}} placeholders from a JSON plate object.
 * Unset keys → empty; hits = slots seen; miss = missing keys.
 * Incomplete {{ without }} is copied literally. */
/* Expand {{key}} from plate JSON. Optional miss_bag = newline bag of missing keys.
 * Returns slot count; *miss_out = missing key count. */
long cubalc_expand_fillp(const char *plate, const char *tmpl, char *out,
                                size_t outcap, long *miss_out,
                                char *miss_bag, size_t miss_cap) {
  const char *src = tmpl ? tmpl : "";
  const char *pl = (plate && plate[0]) ? plate : "{}";
  size_t o = 0, mo = 0;
  long hits = 0, miss = 0;
  char name[96];
  cubalc_host_result gr;
  if (miss_bag && miss_cap) miss_bag[0] = 0;
  if (!out || outcap == 0) {
    if (miss_out) *miss_out = 0;
    return 0;
  }
  out[0] = 0;
  while (*src && o + 1 < outcap) {
    if (src[0] == '{' && src[1] == '{') {
      const char *pp = src + 2;
      size_t ni = 0;
      name[0] = 0;
      while (*pp && !(pp[0] == '}' && pp[1] == '}') && ni + 1 < sizeof name) {
        char c = *pp;
        /* allow dotted/slash nest paths: {{freq.error}} {{meta/role}} */
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' ||
            c == '/')
          name[ni++] = c;
        else
          break;
        pp++;
      }
      name[ni] = 0;
      if (name[0] && pp[0] == '}' && pp[1] == '}') {
        src = pp + 2;
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
  if (miss_out) *miss_out = miss;
  return hits;
}

/* List unique {{key}} names in template order → newline bag. */
long cubalc_fillp_keys(const char *tmpl, char *out, size_t outcap) {
  const char *src = tmpl ? tmpl : "";
  size_t o = 0;
  long n = 0;
  char name[96];
  if (!out || outcap == 0) return 0;
  out[0] = 0;
  while (*src) {
    if (src[0] == '{' && src[1] == '{') {
      const char *pp = src + 2;
      size_t ni = 0;
      name[0] = 0;
      while (*pp && !(pp[0] == '}' && pp[1] == '}') && ni + 1 < sizeof name) {
        char c = *pp;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' ||
            c == '/')
          name[ni++] = c;
        else
          break;
        pp++;
      }
      name[ni] = 0;
      if (name[0] && pp[0] == '}' && pp[1] == '}') {
        size_t nl;
        int seen = 0;
        const char *q;
        src = pp + 2;
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
  return n;
}
