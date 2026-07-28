#define _POSIX_C_SOURCE 200809L
#include "cubalc_translate.h"
#include "cubalc_law.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

/* =============================================================================
 * Anything → CubalC
 *
 * Inputs accepted:
 *   · native .cubalc (pass-through + law header if missing)
 *   · NEXUS_COORD plate lines
 *   · bitstrings (01…) / hex crumbs
 *   · JSON with "bits":"…"
 *   · English / pseudo verbs: plug, flow, impulse, decide, deconstruct…
 *   · raw text → hash into State Matrix SETBIT program + DECIDE
 *
 * Output always lawful: HOLD_FLASH, creed, braincube pipeline, DECIDE.
 * ============================================================================= */

int cubalc_looks_like_cubalc(const char *in, size_t n) {
  if (!in || !n) return 0;
  for (size_t i = 0; i < n; i++) if (in[i] == '[') return 1;
  if (strstr(in, "CREED") || strstr(in, "CUBE ") || strstr(in, "HOLD_FLASH") ||
      strstr(in, "GENESIS") || strstr(in, "DECIDE") || strstr(in, "LET ") ||
      strstr(in, "ASSERT") || strstr(in, "PLUG") || strstr(in, "IMPULSE"))
    return 1;
  return 0;
}

static int has_hold(const char *s) {
  return s && (strstr(s, "HOLD_FLASH") || strstr(s, "[hold]") || strstr(s, "hold_flash"));
}

static void append(char *out, size_t cap, size_t *o, const char *s) {
  if (!out || !o || !s) return;
  size_t n = strlen(s);
  if (*o + n + 1 >= cap) n = cap > *o + 1 ? cap - *o - 1 : 0;
  if (n) { memcpy(out + *o, s, n); *o += n; out[*o] = 0; }
}

static void appendf(char *out, size_t cap, size_t *o, const char *fmt, ...) {
  char buf[512];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  append(out, cap, o, buf);
}

static uint32_t hash_text(const char *s, size_t n) {
  uint32_t h = 0x811C9DC5u;
  for (size_t i = 0; i < n; i++) {
    h ^= (uint8_t)s[i];
    h *= 16777619u;
  }
  return h ? h : 1;
}

static void emit_header(char *out, size_t cap, size_t *o, const char *note) {
  (void)note;
  append(out, cap, o,
    "CREED \"C3\"\n"
    "HOLD_FLASH 1\n"
    "BUDGET 40\n"
    "SHARE smx\n\n");
}

static void emit_brain_pipeline(char *out, size_t cap, size_t *o) {
  append(out, cap, o,
    "CUBE titan ROLE host PROTON 1\n"
    "CUBE clanker ROLE body PROTON 1\n"
    "CUBE nanobot ROLE atom PROTON 1\n"
    "CUBE algo ROLE algocube PROTON 1\n"
    "CUBE brain ROLE braincube PROTON 1\n"
    "PLUG titan nanobot\n"
    "PLUG clanker nanobot\n"
    "PLUG nanobot algo\n"
    "PLUG algo brain\n\n");
}

static void emit_bits_from_hash(char *out, size_t cap, size_t *o,
                                uint32_t h, const char *cube, int nbits) {
  if (nbits < 1) nbits = 16;
  if (nbits > 64) nbits = 64;
  for (int i = 0; i < nbits; i++) {
    int on = (h >> (i % 32)) & 1;
    if (!on) on = ((h >> ((i * 3) % 32)) ^ (h >> 7)) & 1;
    appendf(out, cap, o, "SETBIT %s %d %d\n", cube, i, on ? 1 : 0);
    h = h * 1664525u + 1013904223u;
  }
  append(out, cap, o, "\n");
}

static void emit_decide_tail(char *out, size_t cap, size_t *o) {
  append(out, cap, o,
    "IMPULSE nanobot 1\n"
    "FLOW 4\n"
    "// energy must flow — if stuck, deconstruct → reconstruct\n"
    "IF ENERGY(nanobot) == 0 THEN\n"
    "  DECONSTRUCT nanobot\n"
    "  RECONSTRUCT nanobot\n"
    "  PLUG titan nanobot\n"
    "  PLUG clanker nanobot\n"
    "  IMPULSE nanobot 1\n"
    "  FLOW 4\n"
    "END\n"
    "IMPULSE algo 1\n"
    "IMPULSE brain 1\n"
    "DECIDE brain\n"
    "PRINT \"decide\" DECIDE DIGIT(brain) SET(brain) ENERGY(nanobot) UNITY CUBES\n"
    "ASSERT CUBES >= 5\n"
    "ASSERT DECIDE >= 0\n"
    "ASSERT DECIDE <= 9\n");
}

static int extract_bits01(const char *in, size_t n, char *bits, size_t cap) {
  size_t o = 0;
  const char *p = strstr(in, "\"bits\":\"");
  if (p) {
    p += 8;
    while (*p && *p != '"' && o + 1 < cap) {
      if (*p == '0' || *p == '1') bits[o++] = *p;
      p++;
    }
    bits[o] = 0;
    return (int)o;
  }
  /* pure 01 run */
  size_t ones = 0, digits = 0;
  for (size_t i = 0; i < n; i++) {
    if (in[i] == '0' || in[i] == '1') { digits++; if (in[i]=='1') ones++; }
    else if (!isspace((unsigned char)in[i]) && in[i] != '\n' && in[i] != '\r') {
      digits = 0; break;
    }
  }
  if (digits >= 8) {
    for (size_t i = 0; i < n && o + 1 < cap; i++)
      if (in[i] == '0' || in[i] == '1') bits[o++] = in[i];
    bits[o] = 0;
    return (int)o;
  }
  (void)ones;
  return 0;
}

static int is_plate(const char *in) {
  return in && (strstr(in, "NEXUS_COORD") || strstr(in, "hold_flash=") ||
                strstr(in, "HOLD_FLASH=") || strstr(in, "from="));
}

/* Map loose English / shell-ish lines into CubalC statements */
static void emit_pseudo_lines(char *out, size_t cap, size_t *o,
                              const char *in, size_t n) {
  const char *p = in;
  const char *end = in + n;
  while (p < end) {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\r')) p++;
    if (p >= end) break;
    const char *line = p;
    while (p < end && *p != '\n') p++;
    size_t ln = (size_t)(p - line);
    if (p < end && *p == '\n') p++;
    if (ln == 0 || line[0] == '#') continue;

    char buf[256];
    size_t cpy = ln < sizeof buf - 1 ? ln : sizeof buf - 1;
    memcpy(buf, line, cpy); buf[cpy] = 0;
    for (size_t i = 0; buf[i]; i++) buf[i] = (char)tolower((unsigned char)buf[i]);

    if (strstr(buf, "deconstruct") || strstr(buf, "destroy")) {
      const char *id = "hive";
      if (strstr(buf, "nanobot")) id = "nanobot";
      else if (strstr(buf, "brain")) id = "brain";
      appendf(out, cap, o, "DECONSTRUCT %s\n", id);
      continue;
    }
    if (strstr(buf, "reconstruct") || strstr(buf, "rebuild") || strstr(buf, "construct")) {
      const char *id = "hive";
      if (strstr(buf, "nanobot")) id = "nanobot";
      else if (strstr(buf, "brain")) id = "brain";
      appendf(out, cap, o, "RECONSTRUCT %s\n", id);
      continue;
    }
    if (strstr(buf, "decide") || strstr(buf, "algocube") || strstr(buf, "intent")) {
      append(out, cap, o, "DECIDE brain\n");
      continue;
    }
    if (strstr(buf, "flow") || strstr(buf, "tick")) {
      int nflow = 4;
      for (size_t i = 0; buf[i]; i++)
        if (isdigit((unsigned char)buf[i])) { nflow = atoi(buf + i); break; }
      if (nflow < 1) nflow = 1;
      if (nflow > 100) nflow = 100;
      appendf(out, cap, o, "FLOW %d\n", nflow);
      continue;
    }
    if (strstr(buf, "impulse") || strstr(buf, "pulse")) {
      int pr = strstr(buf, "destroy") || strstr(buf, "0") ? 0 : 1;
      const char *id = "nanobot";
      if (strstr(buf, "brain")) id = "brain";
      else if (strstr(buf, "algo")) id = "algo";
      else if (strstr(buf, "titan")) id = "titan";
      else if (strstr(buf, "clanker")) id = "clanker";
      appendf(out, cap, o, "IMPULSE %s %d\n", id, pr);
      continue;
    }
    if (strstr(buf, "plug") || strstr(buf, "wire") || strstr(buf, "connect")) {
      append(out, cap, o, "PLUG nanobot algo\nPLUG algo brain\n");
      continue;
    }
    if (strstr(buf, "sync") || strstr(buf, "hive")) {
      append(out, cap, o, "[sync]\n");
      continue;
    }
  }
}

int cubalc_translate(const char *in, size_t n,
                     char *out, size_t out_cap,
                     char *err, size_t err_cap) {
  if (!out || out_cap < 64) {
    if (err && err_cap) snprintf(err, err_cap, "out buffer too small");
    return 2;
  }
  out[0] = 0;
  size_t o = 0;
  if (!in) in = "";
  if (n == 0) n = strlen(in);

  /* strip UTF-8 BOM */
  if (n >= 3 && (unsigned char)in[0] == 0xEF) { in += 3; n -= 3; }

  /* --- already CubalC: ensure law header, ensure DECIDE if brain path missing --- */
  if (cubalc_looks_like_cubalc(in, n)) {
    if (!has_hold(in))
      emit_header(out, out_cap, &o, "pass-through+law");
    /* copy source */
    if (o + n + 8 < out_cap) {
      memcpy(out + o, in, n);
      o += n;
      out[o] = 0;
      if (n && in[n - 1] != '\n') append(out, out_cap, &o, "\n");
    } else {
      if (err) snprintf(err, err_cap, "source too large");
      return 2;
    }
    if (!strstr(out, "DECIDE") && !strstr(out, "[decide]") && !strstr(out, "decide ")) {
      append(out, out_cap, &o, "\n// auto-DECIDE (braincube law)\n");
      if (!strstr(out, "CUBE brain") && !strstr(out, "[brain")) {
        append(out, out_cap, &o, "CUBE brain ROLE braincube PROTON 1\n");
      }
      append(out, out_cap, &o, "DECIDE brain\nPRINT \"decide\" DECIDE\n");
    }
    return 0;
  }

  /* --- plate --- */
  if (is_plate(in)) {
    emit_header(out, out_cap, &o, "plate");
    /* escape plate into GENESIS string: take first line-ish */
    char plate[480];
    size_t pl = 0;
    for (size_t i = 0; i < n && pl + 1 < sizeof plate; i++) {
      char c = in[i];
      if (c == '\n' || c == '\r') break;
      if (c == '"') continue;
      plate[pl++] = c;
    }
    plate[pl] = 0;
    if (!strstr(plate, "hold_flash") && !strstr(plate, "HOLD_FLASH")) {
      size_t L = strlen(plate);
      if (L + 20 < sizeof plate) snprintf(plate + L, sizeof plate - L, " hold_flash=1 |");
    }
    appendf(out, out_cap, &o, "GENESIS \"%s\"\n\n", plate);
    emit_brain_pipeline(out, out_cap, &o);
    uint32_t h = hash_text(in, n);
    emit_bits_from_hash(out, out_cap, &o, h, "brain", 24);
    emit_bits_from_hash(out, out_cap, &o, h ^ 0xA5A5u, "titan", 16);
    emit_decide_tail(out, out_cap, &o);
    return 0;
  }

  /* --- bitstring / JSON bits --- */
  char bits[128];
  int nb = extract_bits01(in, n, bits, sizeof bits);
  if (nb >= 8) {
    emit_header(out, out_cap, &o, "bits");
    append(out, out_cap, &o,
      "GENESIS \"NEXUS_COORD v1 | from=translate | type=bits | hold_flash=1 |\"\n\n");
    emit_brain_pipeline(out, out_cap, &o);
    for (int i = 0; i < nb && i < 64; i++)
      appendf(out, out_cap, &o, "SETBIT brain %d %d\n", i, bits[i] == '1' ? 1 : 0);
    append(out, out_cap, &o, "\n");
    /* mirror raw to titan as IO matrix */
    for (int i = 0; i < nb && i < 32; i++)
      appendf(out, out_cap, &o, "SETBIT titan %d %d\n", i, bits[i] == '1' ? 1 : 0);
    append(out, out_cap, &o, "\n");
    emit_decide_tail(out, out_cap, &o);
    return 0;
  }

  /* --- pseudo English / mixed --- */
  int has_verb = 0;
  {
    char low[512];
    size_t cpy = n < sizeof low - 1 ? n : sizeof low - 1;
    for (size_t i = 0; i < cpy; i++) low[i] = (char)tolower((unsigned char)in[i]);
    low[cpy] = 0;
    if (strstr(low, "decide") || strstr(low, "flow") || strstr(low, "plug") ||
        strstr(low, "impulse") || strstr(low, "deconstruct") ||
        strstr(low, "reconstruct") || strstr(low, "matrix") ||
        strstr(low, "brain") || strstr(low, "algocube") || strstr(low, "nanobot"))
      has_verb = 1;
  }

  emit_header(out, out_cap, &o, has_verb ? "pseudo" : "hash-text");
  append(out, out_cap, &o,
    "GENESIS \"NEXUS_COORD v1 | from=translate | type=anything | hold_flash=1 |\"\n\n");
  emit_brain_pipeline(out, out_cap, &o);

  if (has_verb)
    emit_pseudo_lines(out, out_cap, &o, in, n);

  /* always fold text into matrix SoT (non-verbal: bits from hash, not prose on wire) */
  uint32_t h = hash_text(in, n);
  emit_bits_from_hash(out, out_cap, &o, h, "brain", 32);
  emit_bits_from_hash(out, out_cap, &o, h ^ 0xC0BEu, "titan", 16);
  emit_bits_from_hash(out, out_cap, &o, h ^ 0xC1A4u, "clanker", 16);
  emit_decide_tail(out, out_cap, &o);
  return 0;
}
