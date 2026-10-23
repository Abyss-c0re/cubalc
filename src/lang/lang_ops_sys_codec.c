/* CubalC lang — lang_ops_sys_codec.c
 * SYS BASE64/HEX/URL/UUID/JSONESC/CBXF helpers (modular extract from lang_ops_core).
 * Mesh vote A_split_lang_ops_core · law: zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_codec.h"
#include "cubalc_platform.h"
#include "cubalc_hostops.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#  include <errno.h>
#  include <fcntl.h>
#  include <poll.h>
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#endif

/* SYS BASE64 / BASE64D — standard base64 encode/decode for agent plate payloads.
 * Returns 0 on success; fills out with NUL-terminated result; *n_out = byte length. */
static const char cubalc_b64_tab[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
int cubalc_b64_enc(const char *in, size_t in_len, char *out, size_t out_cap,
                          size_t *n_out) {
  size_t i = 0, o = 0;
  if (!out || out_cap < 1) return -1;
  if (!in) in = "";
  if (((in_len + 2) / 3) * 4 + 1 > out_cap) return -1;
  while (i + 2 < in_len) {
    unsigned v = ((unsigned char)in[i] << 16) | ((unsigned char)in[i + 1] << 8) |
                 (unsigned char)in[i + 2];
    out[o++] = cubalc_b64_tab[(v >> 18) & 63];
    out[o++] = cubalc_b64_tab[(v >> 12) & 63];
    out[o++] = cubalc_b64_tab[(v >> 6) & 63];
    out[o++] = cubalc_b64_tab[v & 63];
    i += 3;
  }
  if (i < in_len) {
    unsigned v = (unsigned char)in[i] << 16;
    out[o++] = cubalc_b64_tab[(v >> 18) & 63];
    if (i + 1 < in_len) {
      v |= (unsigned char)in[i + 1] << 8;
      out[o++] = cubalc_b64_tab[(v >> 12) & 63];
      out[o++] = cubalc_b64_tab[(v >> 6) & 63];
      out[o++] = '=';
    } else {
      out[o++] = cubalc_b64_tab[(v >> 12) & 63];
      out[o++] = '=';
      out[o++] = '=';
    }
  }
  out[o] = 0;
  if (n_out) *n_out = o;
  return 0;
}
static int cubalc_b64_val(int c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}
int cubalc_b64_dec(const char *in, size_t in_len, char *out, size_t out_cap,
                          size_t *n_out) {
  size_t i = 0, o = 0;
  if (!out || out_cap < 1) return -1;
  if (!in) in = "";
  /* strip trailing whitespace from logical length */
  while (in_len > 0 && (in[in_len - 1] == '\n' || in[in_len - 1] == '\r' ||
                        in[in_len - 1] == ' ' || in[in_len - 1] == '\t'))
    in_len--;
  if (in_len % 4 != 0) return -1;
  if ((in_len / 4) * 3 + 1 > out_cap) return -1;
  while (i < in_len) {
    int a = cubalc_b64_val((unsigned char)in[i]);
    int b = cubalc_b64_val((unsigned char)in[i + 1]);
    int c = (in[i + 2] == '=') ? -2 : cubalc_b64_val((unsigned char)in[i + 2]);
    int d = (in[i + 3] == '=') ? -2 : cubalc_b64_val((unsigned char)in[i + 3]);
    unsigned v;
    if (a < 0 || b < 0 || c == -1 || d == -1) return -1;
    if (c == -2 && d != -2) return -1;
    v = ((unsigned)a << 18) | ((unsigned)b << 12);
    if (c >= 0) v |= (unsigned)c << 6;
    if (d >= 0) v |= (unsigned)d;
    if (o + 1 >= out_cap) return -1;
    out[o++] = (char)((v >> 16) & 0xff);
    if (c >= 0) {
      if (o + 1 >= out_cap) return -1;
      out[o++] = (char)((v >> 8) & 0xff);
    }
    if (d >= 0) {
      if (o + 1 >= out_cap) return -1;
      out[o++] = (char)(v & 0xff);
    }
    i += 4;
  }
  out[o] = 0;
  if (n_out) *n_out = o;
  return 0;
}

/* SYS HEX / HEXD — lowercase hex encode/decode for agent plate bytes. */
int cubalc_hex_enc(const char *in, size_t in_len, char *out, size_t out_cap,
                          size_t *n_out) {
  static const char *hx = "0123456789abcdef";
  size_t i, o = 0;
  if (!out || out_cap < 1) return -1;
  if (!in) in = "";
  if (in_len * 2 + 1 > out_cap) return -1;
  for (i = 0; i < in_len; i++) {
    unsigned char c = (unsigned char)in[i];
    out[o++] = hx[c >> 4];
    out[o++] = hx[c & 0xf];
  }
  out[o] = 0;
  if (n_out) *n_out = o;
  return 0;
}
static int cubalc_hex_nibble(int c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
int cubalc_hex_dec(const char *in, size_t in_len, char *out, size_t out_cap,
                          size_t *n_out) {
  size_t i = 0, o = 0;
  if (!out || out_cap < 1) return -1;
  if (!in) in = "";
  while (in_len > 0 && (in[in_len - 1] == '\n' || in[in_len - 1] == '\r' ||
                        in[in_len - 1] == ' ' || in[in_len - 1] == '\t'))
    in_len--;
  if (in_len % 2 != 0) return -1;
  if (in_len / 2 + 1 > out_cap) return -1;
  for (i = 0; i + 1 < in_len; i += 2) {
    int hi = cubalc_hex_nibble((unsigned char)in[i]);
    int lo = cubalc_hex_nibble((unsigned char)in[i + 1]);
    if (hi < 0 || lo < 0) return -1;
    out[o++] = (char)((hi << 4) | lo);
  }
  out[o] = 0;
  if (n_out) *n_out = o;
  return 0;
}

/* SYS URLENC / URLDEC — RFC 3986 percent-encode/decode for plate paths/queries.
 * Unreserved A-Za-z0-9-._~ pass through; space → %20; others → %XX (upper hex).
 * Decode accepts %XX (any hex case) and '+' as space (form-urlencoded). Soft fail. */
static int cubalc_url_unreserved(unsigned char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
         (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~';
}
int cubalc_url_enc(const char *in, size_t in_len, char *out, size_t out_cap,
                          size_t *n_out) {
  static const char *hx = "0123456789ABCDEF";
  size_t i, o = 0;
  if (!out || out_cap < 1) return -1;
  if (!in) in = "";
  for (i = 0; i < in_len; i++) {
    unsigned char c = (unsigned char)in[i];
    if (cubalc_url_unreserved(c)) {
      if (o + 1 >= out_cap) return -1;
      out[o++] = (char)c;
    } else {
      if (o + 3 >= out_cap) return -1;
      out[o++] = '%';
      out[o++] = hx[c >> 4];
      out[o++] = hx[c & 0xf];
    }
  }
  if (o >= out_cap) return -1;
  out[o] = 0;
  if (n_out) *n_out = o;
  return 0;
}
int cubalc_url_dec(const char *in, size_t in_len, char *out, size_t out_cap,
                          size_t *n_out) {
  size_t i = 0, o = 0;
  if (!out || out_cap < 1) return -1;
  if (!in) in = "";
  while (i < in_len) {
    unsigned char c = (unsigned char)in[i];
    if (c == '%') {
      int hi, lo;
      if (i + 2 >= in_len) return -1;
      hi = cubalc_hex_nibble((unsigned char)in[i + 1]);
      lo = cubalc_hex_nibble((unsigned char)in[i + 2]);
      if (hi < 0 || lo < 0) return -1;
      if (o + 1 >= out_cap) return -1;
      out[o++] = (char)((hi << 4) | lo);
      i += 3;
    } else if (c == '+') {
      if (o + 1 >= out_cap) return -1;
      out[o++] = ' ';
      i++;
    } else {
      if (o + 1 >= out_cap) return -1;
      out[o++] = (char)c;
      i++;
    }
  }
  if (o >= out_cap) return -1;
  out[o] = 0;
  if (n_out) *n_out = o;
  return 0;
}

/* SYS UUID / UUID0 — RFC 4122 version-4 UUID for agent work/claim ids.
 * dashed: 8-4-4-4-12 lowercase; compact (nodash): 32 hex. Entropy: /dev/urandom
 * then time+pid+rand fallback. */
static void cubalc_uuid_rand16(unsigned char b[16]) {
  size_t got = 0;
#if !defined(CUBALC_OS_WINDOWS)
  {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
      while (got < 16) {
        ssize_t n = read(fd, b + got, 16 - got);
        if (n <= 0) break;
        got += (size_t)n;
      }
      close(fd);
    }
  }
#endif
  if (got < 16) {
    static unsigned long seed;
    unsigned long s;
    size_t i;
    if (!seed) {
      seed = (unsigned long)time(NULL);
#if !defined(CUBALC_OS_WINDOWS)
      seed ^= (unsigned long)getpid() << 16;
#endif
      seed ^= (unsigned long)(size_t)&seed;
      if (!seed) seed = 0xC0B1CUL;
    }
    s = seed;
    for (i = 0; i < 16; i++) {
      s = s * 1103515245UL + 12345UL;
      b[i] = (unsigned char)((s >> 16) & 0xff);
    }
    seed = s;
  }
  /* RFC 4122: version 4, variant 10xx */
  b[6] = (unsigned char)((b[6] & 0x0f) | 0x40);
  b[8] = (unsigned char)((b[8] & 0x3f) | 0x80);
}
int cubalc_uuid_fmt(char *out, size_t out_cap, int compact) {
  static const char *hx = "0123456789abcdef";
  unsigned char b[16];
  size_t need = compact ? 32 : 36;
  size_t i, o = 0;
  if (!out || out_cap < need + 1) return -1;
  cubalc_uuid_rand16(b);
  for (i = 0; i < 16; i++) {
    if (!compact && (i == 4 || i == 6 || i == 8 || i == 10))
      out[o++] = '-';
    out[o++] = hx[b[i] >> 4];
    out[o++] = hx[b[i] & 0xf];
  }
  out[o] = 0;
  return (int)o;
}

/* SYS JSONESC / JSONUNESC — JSON string body escape/unescape for agent plates.
 * Escapes " \ / and controls (\b \f \n \r \t · \u00XX for other C0). No outer quotes.
 * Unescape accepts those + \u00XX hex. Soft fail on truncated/bad sequences. */
int cubalc_json_esc(const char *in, size_t in_len, char *out, size_t out_cap,
                           size_t *n_out) {
  static const char *hx = "0123456789abcdef";
  size_t i, o = 0;
  if (!out || out_cap < 1) return -1;
  if (!in) in = "";
  for (i = 0; i < in_len; i++) {
    unsigned char c = (unsigned char)in[i];
    const char *rep = NULL;
    char tmp[8];
    size_t rlen = 0;
    if (c == '"') rep = "\\\"";
    else if (c == '\\') rep = "\\\\";
    else if (c == '/') rep = "\\/";
    else if (c == '\b') rep = "\\b";
    else if (c == '\f') rep = "\\f";
    else if (c == '\n') rep = "\\n";
    else if (c == '\r') rep = "\\r";
    else if (c == '\t') rep = "\\t";
    else if (c < 0x20) {
      tmp[0] = '\\'; tmp[1] = 'u'; tmp[2] = '0'; tmp[3] = '0';
      tmp[4] = hx[c >> 4]; tmp[5] = hx[c & 0xf]; tmp[6] = 0;
      rep = tmp;
    }
    if (rep) {
      rlen = strlen(rep);
      if (o + rlen >= out_cap) return -1;
      memcpy(out + o, rep, rlen);
      o += rlen;
    } else {
      if (o + 1 >= out_cap) return -1;
      out[o++] = (char)c;
    }
  }
  if (o >= out_cap) return -1;
  out[o] = 0;
  if (n_out) *n_out = o;
  return 0;
}
int cubalc_json_unesc(const char *in, size_t in_len, char *out, size_t out_cap,
                             size_t *n_out) {
  size_t i = 0, o = 0;
  if (!out || out_cap < 1) return -1;
  if (!in) in = "";
  while (i < in_len) {
    unsigned char c = (unsigned char)in[i];
    if (c != '\\') {
      if (o + 1 >= out_cap) return -1;
      out[o++] = (char)c;
      i++;
      continue;
    }
    if (i + 1 >= in_len) return -1;
    c = (unsigned char)in[i + 1];
    if (c == '"' || c == '\\' || c == '/') {
      if (o + 1 >= out_cap) return -1;
      out[o++] = (char)c;
      i += 2;
    } else if (c == 'b') {
      if (o + 1 >= out_cap) return -1;
      out[o++] = '\b';
      i += 2;
    } else if (c == 'f') {
      if (o + 1 >= out_cap) return -1;
      out[o++] = '\f';
      i += 2;
    } else if (c == 'n') {
      if (o + 1 >= out_cap) return -1;
      out[o++] = '\n';
      i += 2;
    } else if (c == 'r') {
      if (o + 1 >= out_cap) return -1;
      out[o++] = '\r';
      i += 2;
    } else if (c == 't') {
      if (o + 1 >= out_cap) return -1;
      out[o++] = '\t';
      i += 2;
    } else if (c == 'u') {
      int h0, h1, h2, h3, code;
      if (i + 5 >= in_len) return -1;
      h0 = cubalc_hex_nibble((unsigned char)in[i + 2]);
      h1 = cubalc_hex_nibble((unsigned char)in[i + 3]);
      h2 = cubalc_hex_nibble((unsigned char)in[i + 4]);
      h3 = cubalc_hex_nibble((unsigned char)in[i + 5]);
      if (h0 < 0 || h1 < 0 || h2 < 0 || h3 < 0) return -1;
      code = (h0 << 12) | (h1 << 8) | (h2 << 4) | h3;
      /* Agent plate plane is byte-oriented; only emit U+0000..U+00FF. */
      if (code > 0xff) return -1;
      if (o + 1 >= out_cap) return -1;
      out[o++] = (char)code;
      i += 6;
    } else {
      return -1;
    }
  }
  if (o >= out_cap) return -1;
  out[o] = 0;
  if (n_out) *n_out = o;
  return 0;
}

#if !defined(CUBALC_OS_WINDOWS)
/* CBXF — CubalC Bidirectional XFer framing (any payload ≤ CUBALC_HOST_STR_MAX-1).
 * wire: magic "CBXF" + uint32 BE length + payload bytes. */
int cubalc_cbxf_send(int fd, const char *payload, size_t plen) {
  uint8_t hdr[8];
  uint32_t be;
  size_t off = 0;
  ssize_t n;
  if (fd < 0 || plen > (size_t)(CUBALC_HOST_STR_MAX - 1)) return -1;
  hdr[0] = 'C'; hdr[1] = 'B'; hdr[2] = 'X'; hdr[3] = 'F';
  be = htonl((uint32_t)plen);
  memcpy(hdr + 4, &be, 4);
  while (off < 8) {
    n = send(fd, hdr + off, 8 - off, 0);
    if (n <= 0) return -1;
    off += (size_t)n;
  }
  off = 0;
  while (off < plen) {
    n = send(fd, payload + off, plen - off, 0);
    if (n <= 0) return -1;
    off += (size_t)n;
  }
  return 0;
}
int cubalc_cbxf_recv(int fd, char *out, size_t cap, size_t *n_out, int timeout_ms) {
  uint8_t hdr[8];
  size_t off = 0;
  uint32_t be, plen;
  ssize_t n;
  if (n_out) *n_out = 0;
  if (fd < 0 || !out || cap < 2) return -1;
  if (timeout_ms > 0) {
    struct pollfd pfd;
    pfd.fd = fd; pfd.events = POLLIN; pfd.revents = 0;
    if (poll(&pfd, 1, timeout_ms) <= 0) return -1;
  }
  while (off < 8) {
    n = recv(fd, hdr + off, 8 - off, 0);
    if (n <= 0) return -1;
    off += (size_t)n;
  }
  if (hdr[0] != 'C' || hdr[1] != 'B' || hdr[2] != 'X' || hdr[3] != 'F') return -1;
  memcpy(&be, hdr + 4, 4);
  plen = ntohl(be);
  if (plen >= cap) return -1;
  off = 0;
  while (off < plen) {
    n = recv(fd, out + off, plen - off, 0);
    if (n <= 0) return -1;
    off += (size_t)n;
  }
  out[plen] = 0;
  if (n_out) *n_out = plen;
  return 0;
}
int cubalc_parse_host_port_simple(const char *ep, char *host, size_t hsz, int *port) {
  const char *colon;
  size_t hl;
  if (!ep || !host || !port || hsz < 2) return -1;
  colon = strrchr(ep, ':');
  if (!colon || colon == ep || !colon[1]) return -1;
  hl = (size_t)(colon - ep);
  if (hl >= hsz) hl = hsz - 1;
  memcpy(host, ep, hl); host[hl] = 0;
  *port = atoi(colon + 1);
  if (*port <= 0 || *port > 65535) return -1;
  return 0;
}
#endif /* !WINDOWS */

/* FNV-1a 32-bit — fast non-crypto fingerprint for plate/cache stamps. */
uint32_t cubalc_fnv1a32(const char *s, size_t n) {
  uint32_t h = 2166136261u;
  size_t i;
  if (!s) return h;
  for (i = 0; i < n; i++) {
    h ^= (uint8_t)s[i];
    h *= 16777619u;
  }
  return h;
}

void cubalc_hash_hex8(uint32_t h, char out[9]) {
  static const char *hex = "0123456789abcdef";
  int i;
  for (i = 7; i >= 0; i--) {
    out[i] = hex[h & 0xfu];
    h >>= 4;
  }
  out[8] = 0;
}

/* IEEE/ISO 3309 CRC-32 (poly 0xEDB88320) — interoperable plate integrity stamp. */
uint32_t cubalc_crc32(const char *s, size_t n) {
  static uint32_t table[256];
  static int ready = 0;
  uint32_t c;
  size_t i;
  if (!ready) {
    uint32_t k;
    int j;
    for (k = 0; k < 256; k++) {
      uint32_t crc = k;
      for (j = 0; j < 8; j++)
        crc = (crc & 1u) ? (0xEDB88320u ^ (crc >> 1)) : (crc >> 1);
      table[k] = crc;
    }
    ready = 1;
  }
  c = 0xFFFFFFFFu;
  if (!s) s = "";
  for (i = 0; i < n; i++)
    c = table[(c ^ (uint8_t)s[i]) & 0xffu] ^ (c >> 8);
  return c ^ 0xFFFFFFFFu;
}

