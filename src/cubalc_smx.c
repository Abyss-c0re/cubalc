#define _POSIX_C_SOURCE 200809L
#include "cubalc_smx.h"
#include "cubalc_platform.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#if defined(CUBALC_OS_WINDOWS)
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
   static int cubalc_sock_init(void) {
     static int once;
     if (!once) { WSADATA w; WSAStartup(MAKEWORD(2,2), &w); once = 1; }
     return 0;
   }
#  define CUBALC_CLOSESOCK closesocket
#else
#  include <sys/socket.h>
#  include <sys/un.h>
#  include <sys/stat.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <netdb.h>
   static int cubalc_sock_init(void) { return 0; }
#  define CUBALC_CLOSESOCK close
#endif

/* --- minimal SHA-256 (public domain style compact) --- */
typedef struct {
  uint32_t s[8];
  uint64_t bits;
  uint8_t  buf[64];
  size_t   n;
} sha256_ctx;

static uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

static void sha256_init(sha256_ctx *c) {
  static const uint32_t iv[8] = {
    0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
    0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19
  };
  memcpy(c->s, iv, sizeof iv);
  c->bits = 0; c->n = 0;
}

static void sha256_block(sha256_ctx *c, const uint8_t *p) {
  static const uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
  };
  uint32_t w[64], a,b,c2,d,e,f,g,h, t1,t2;
  int i;
  for (i = 0; i < 16; i++)
    w[i] = ((uint32_t)p[i*4]<<24)|((uint32_t)p[i*4+1]<<16)|((uint32_t)p[i*4+2]<<8)|p[i*4+3];
  for (; i < 64; i++) {
    uint32_t s0 = rotr(w[i-15],7)^rotr(w[i-15],18)^(w[i-15]>>3);
    uint32_t s1 = rotr(w[i-2],17)^rotr(w[i-2],19)^(w[i-2]>>10);
    w[i] = w[i-16] + s0 + w[i-7] + s1;
  }
  a=c->s[0]; b=c->s[1]; c2=c->s[2]; d=c->s[3];
  e=c->s[4]; f=c->s[5]; g=c->s[6]; h=c->s[7];
  for (i = 0; i < 64; i++) {
    uint32_t S1 = rotr(e,6)^rotr(e,11)^rotr(e,25);
    uint32_t ch = (e & f) ^ ((~e) & g);
    t1 = h + S1 + ch + K[i] + w[i];
    uint32_t S0 = rotr(a,2)^rotr(a,13)^rotr(a,22);
    uint32_t maj = (a & b) ^ (a & c2) ^ (b & c2);
    t2 = S0 + maj;
    h=g; g=f; f=e; e=d+t1; d=c2; c2=b; b=a; a=t1+t2;
  }
  c->s[0]+=a; c->s[1]+=b; c->s[2]+=c2; c->s[3]+=d;
  c->s[4]+=e; c->s[5]+=f; c->s[6]+=g; c->s[7]+=h;
}

static void sha256_update(sha256_ctx *c, const void *data, size_t len) {
  const uint8_t *p = (const uint8_t *)data;
  c->bits += (uint64_t)len * 8;
  while (len > 0) {
    size_t take = 64 - c->n;
    if (take > len) take = len;
    memcpy(c->buf + c->n, p, take);
    c->n += take; p += take; len -= take;
    if (c->n == 64) { sha256_block(c, c->buf); c->n = 0; }
  }
}

static void sha256_final(sha256_ctx *c, uint8_t out[32]) {
  size_t i;
  c->buf[c->n++] = 0x80;
  if (c->n > 56) {
    while (c->n < 64) c->buf[c->n++] = 0;
    sha256_block(c, c->buf); c->n = 0;
  }
  while (c->n < 56) c->buf[c->n++] = 0;
  for (i = 0; i < 8; i++)
    c->buf[56+i] = (uint8_t)((c->bits >> (56 - 8*i)) & 0xff);
  sha256_block(c, c->buf);
  for (i = 0; i < 8; i++) {
    out[i*4]   = (uint8_t)(c->s[i] >> 24);
    out[i*4+1] = (uint8_t)(c->s[i] >> 16);
    out[i*4+2] = (uint8_t)(c->s[i] >> 8);
    out[i*4+3] = (uint8_t)(c->s[i]);
  }
}

static void sha256(const void *data, size_t len, uint8_t out[32]) {
  sha256_ctx c; sha256_init(&c); sha256_update(&c, data, len); sha256_final(&c, out);
}

static void hmac_sha256(const uint8_t *key, size_t key_len,
                        const uint8_t *msg, size_t msg_len, uint8_t out[32]) {
  uint8_t k[64], ipad[64], opad[64], tk[32], inner[32];
  size_t i;
  memset(k, 0, 64);
  if (key_len > 64) { sha256(key, key_len, tk); memcpy(k, tk, 32); }
  else memcpy(k, key, key_len);
  for (i = 0; i < 64; i++) { ipad[i] = k[i] ^ 0x36; opad[i] = k[i] ^ 0x5c; }
  {
    sha256_ctx c; sha256_init(&c);
    sha256_update(&c, ipad, 64); sha256_update(&c, msg, msg_len); sha256_final(&c, inner);
  }
  {
    sha256_ctx c; sha256_init(&c);
    sha256_update(&c, opad, 64); sha256_update(&c, inner, 32); sha256_final(&c, out);
  }
}

static int hex_nibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

void cubalc_smx_ctx_init(cubalc_smx_ctx *ctx) {
  if (!ctx) return;
  memset(ctx, 0, sizeof(*ctx));
  ctx->min_compat = 0.35f;
  ctx->hold_flash = CUBALC_HOLD_FLASH;
  /* auto-load: env hex, then key file, then peer token file (no log of secret) */
  {
    const char *hx = getenv("CUBALC_SMX_KEY");
    if (hx && cubalc_smx_load_key_hex(ctx, hx) == 0) return;
  }
  {
    const char *kf = getenv("CUBALC_SMX_KEY_FILE");
    if (kf && cubalc_smx_load_key_file(ctx, kf) == 0) return;
  }
  {
    const char *home = getenv("HOME");
    char path[512];
    if (home) {
      snprintf(path, sizeof path, "%s/.nanobot/peer_token", home);
      if (cubalc_smx_load_key_file(ctx, path) == 0) {
        /* file was hex or raw token — if raw, re-derive */
        if (!ctx->key_ok) {
          FILE *f = fopen(path, "r");
          char tok[128] = {0};
          if (f) {
            if (fgets(tok, sizeof tok, f)) {
              size_t n = strlen(tok);
              while (n && (tok[n-1]=='\n'||tok[n-1]=='\r')) tok[--n]=0;
              cubalc_smx_load_key_token(ctx, tok);
            }
            fclose(f);
          }
        }
        return;
      }
      /* raw peer token derive */
      FILE *f = fopen(path, "r");
      char tok[128] = {0};
      if (f) {
        if (fgets(tok, sizeof tok, f)) {
          size_t n = strlen(tok);
          while (n && (tok[n-1]=='\n'||tok[n-1]=='\r')) tok[--n]=0;
          if (n >= 16) cubalc_smx_load_key_token(ctx, tok);
        }
        fclose(f);
      }
    }
  }
  if (!ctx->key_ok)
    snprintf(ctx->last_err, sizeof ctx->last_err, "no_smx_key_fail_closed");
}

int cubalc_smx_load_key_hex(cubalc_smx_ctx *ctx, const char *hex64) {
  int i;
  if (!ctx || !hex64) return -1;
  if (strlen(hex64) < 64) return -1;
  for (i = 0; i < CUBALC_SMX_KEY_LEN; i++) {
    int hi = hex_nibble(hex64[i*2]), lo = hex_nibble(hex64[i*2+1]);
    if (hi < 0 || lo < 0) return -1;
    ctx->key[i] = (uint8_t)((hi << 4) | lo);
  }
  ctx->key_ok = 1;
  ctx->last_err[0] = 0;
  return 0;
}

int cubalc_smx_load_key_file(cubalc_smx_ctx *ctx, const char *path) {
  char buf[256];
  FILE *f;
  size_t n;
  if (!ctx || !path) return -1;
  f = fopen(path, "r");
  if (!f) return -1;
  if (!fgets(buf, sizeof buf, f)) { fclose(f); return -1; }
  fclose(f);
  n = strlen(buf);
  while (n && (buf[n-1]=='\n'||buf[n-1]=='\r'||buf[n-1]==' ')) buf[--n]=0;
  if (n == 64 && hex_nibble(buf[0]) >= 0)
    return cubalc_smx_load_key_hex(ctx, buf);
  if (n >= 16)
    return cubalc_smx_load_key_token(ctx, buf);
  return -1;
}

int cubalc_smx_load_key_token(cubalc_smx_ctx *ctx, const char *peer_token) {
  uint8_t dig[32];
  char mat[160];
  if (!ctx || !peer_token || !peer_token[0]) return -1;
  /* domain-separated derive — never store raw token in ctx beyond key */
  snprintf(mat, sizeof mat, "cubalc-smx2|smx|%s", peer_token);
  sha256(mat, strlen(mat), dig);
  memcpy(ctx->key, dig, CUBALC_SMX_KEY_LEN);
  ctx->key_ok = 1;
  ctx->last_err[0] = 0;
  /* wipe mat on stack */
  memset(mat, 0, sizeof mat);
  return 0;
}

static void fill_nonce(uint8_t n[CUBALC_SMX_NONCE_LEN]) {
  int fd = open("/dev/urandom", O_RDONLY);
  if (fd >= 0) {
    if (read(fd, n, CUBALC_SMX_NONCE_LEN) == CUBALC_SMX_NONCE_LEN) {
      close(fd); return;
    }
    close(fd);
  }
  /* weak fallback — still unique-ish with time+pid */
  {
    uint64_t t = (uint64_t)time(NULL) ^ ((uint64_t)getpid() << 16);
    memcpy(n, &t, CUBALC_SMX_NONCE_LEN <= 8 ? CUBALC_SMX_NONCE_LEN : 8);
  }
}

int cubalc_smx_seal(cubalc_smx_ctx *ctx, const cubalc_atom *atom,
                    const char *from, const char *to,
                    uint8_t *out, size_t cap, size_t *n_out) {
  cubalc_smx_hdr *h;
  size_t bits_n, need;
  uint8_t mac[CUBALC_SMX_MAC_LEN];
  if (!ctx || !atom || !out) return -1;
  if (!ctx->key_ok) {
    snprintf(ctx->last_err, sizeof ctx->last_err, "seal_fail_closed_no_key");
    return -2;
  }
  bits_n = (atom->matrix.n + 7) / 8;
  need = sizeof(cubalc_smx_hdr) + bits_n + CUBALC_SMX_MAC_LEN;
  if (cap < need) return -1;
  memset(out, 0, need);
  h = (cubalc_smx_hdr *)out;
  h->magic = CUBALC_MAGIC_BIN;
  h->proto = CUBALC_PROTO_SMX2;
  h->flags = CUBALC_SMX_F_HOLD_FLASH | CUBALC_SMX_F_REQUIRE_COMPAT;
  if (atom->proton) h->flags |= CUBALC_SMX_F_PROTON_CREATE;
  else h->flags |= CUBALC_SMX_F_PROTON_DESTROY;
  ctx->last_tx_seq++;
  if (ctx->last_tx_seq == 0) ctx->last_tx_seq = 1;
  h->seq = ctx->last_tx_seq;
  fill_nonce(h->nonce);
  if (from) strncpy(h->from_id, from, CUBALC_ID_LEN - 1);
  if (to) strncpy(h->to_id, to, CUBALC_ID_LEN - 1);
  h->n_bits = atom->matrix.n;
  h->set = atom->matrix.set;
  h->proton = atom->proton ? 1 : 0;
  h->digit = atom->digit;
  h->hold_flash = ctx->hold_flash ? 1 : 0;
  memcpy(out + sizeof(*h), atom->matrix.bits, bits_n);
  hmac_sha256(ctx->key, CUBALC_SMX_KEY_LEN, out, sizeof(*h) + bits_n, mac);
  memcpy(out + sizeof(*h) + bits_n, mac, CUBALC_SMX_MAC_LEN);
  if (n_out) *n_out = need;
  return 0;
}

int cubalc_smx_open(cubalc_smx_ctx *ctx, const uint8_t *in, size_t n,
                    cubalc_atom *atom_out, char *from, char *to,
                    const cubalc_matrix *local_for_compat) {
  const cubalc_smx_hdr *h;
  size_t bits_n, body;
  uint8_t mac[CUBALC_SMX_MAC_LEN], expect[CUBALC_SMX_MAC_LEN];
  if (!ctx || !in || !atom_out) return -1;
  if (!ctx->key_ok) {
    snprintf(ctx->last_err, sizeof ctx->last_err, "open_fail_closed_no_key");
    return -2;
  }
  if (n < sizeof(cubalc_smx_hdr) + CUBALC_SMX_MAC_LEN) return -1;
  h = (const cubalc_smx_hdr *)in;
  if (h->magic != CUBALC_MAGIC_BIN || h->proto != CUBALC_PROTO_SMX2) {
    snprintf(ctx->last_err, sizeof ctx->last_err, "bad_magic_or_proto");
    return -3;
  }
  /* LAW: hold_flash must be asserted on secure frames */
  if (!(h->flags & CUBALC_SMX_F_HOLD_FLASH) || !h->hold_flash) {
    snprintf(ctx->last_err, sizeof ctx->last_err, "hold_flash_required");
    return -4;
  }
  bits_n = (h->n_bits + 7) / 8;
  if (h->n_bits > CUBALC_ATOM_BITS) return -1;
  body = sizeof(*h) + bits_n;
  if (n < body + CUBALC_SMX_MAC_LEN) return -1;
  hmac_sha256(ctx->key, CUBALC_SMX_KEY_LEN, in, body, expect);
  memcpy(mac, in + body, CUBALC_SMX_MAC_LEN);
  if (memcmp(mac, expect, CUBALC_SMX_MAC_LEN) != 0) {
    snprintf(ctx->last_err, sizeof ctx->last_err, "mac_fail");
    return -5;
  }
  /* anti-replay: seq must strictly increase */
  if (h->seq <= ctx->last_rx_seq) {
    snprintf(ctx->last_err, sizeof ctx->last_err, "replay_seq");
    return -6;
  }
  memset(atom_out, 0, sizeof(*atom_out));
  atom_out->proton = h->proton ? 1 : 0;
  atom_out->digit = h->digit;
  atom_out->alive = h->proton ? 1 : 0;
  atom_out->matrix.n = h->n_bits;
  atom_out->matrix.set = h->set;
  memcpy(atom_out->matrix.bits, in + sizeof(*h), bits_n);
  if (from) { strncpy(from, h->from_id, CUBALC_ID_LEN - 1); from[CUBALC_ID_LEN-1]=0; }
  if (to) { strncpy(to, h->to_id, CUBALC_ID_LEN - 1); to[CUBALC_ID_LEN-1]=0; }

  if ((h->flags & CUBALC_SMX_F_REQUIRE_COMPAT) && local_for_compat) {
    float c = cubalc_matrix_compat(local_for_compat, &atom_out->matrix);
    if (c < ctx->min_compat) {
      snprintf(ctx->last_err, sizeof ctx->last_err, "compat_fail_%.2f", c);
      return -7;
    }
    atom_out->unity = c;
  }
  ctx->last_rx_seq = h->seq;
  ctx->last_err[0] = 0;
  return 0;
}

int cubalc_cube_talk_secure(cubalc_chain *ch, cubalc_smx_ctx *ctx, int from, int to) {
  uint8_t buf[512];
  size_t n = 0;
  cubalc_atom received;
  char f[CUBALC_ID_LEN], t[CUBALC_ID_LEN];
  cubalc_cube *A, *B;
  int rc;
  if (!ch || !ctx || from < 0 || to < 0 || from >= ch->n_cubes || to >= ch->n_cubes)
    return -1;
  A = &ch->cubes[from]; B = &ch->cubes[to];
  if (cubalc_smx_seal(ctx, &A->atom, A->id, B->id, buf, sizeof buf, &n) != 0)
    return -2;
  rc = cubalc_smx_open(ctx, buf, n, &received, f, t, &B->atom.matrix);
  if (rc != 0) return rc;
  /* apply sealed matrix transfer under proton law */
  if (received.proton) {
    for (int i = 0; i < received.matrix.n && i < CUBALC_ATOM_BITS; i++)
      if (cubalc_matrix_get(&received.matrix, i))
        cubalc_matrix_set(&B->atom.matrix, i, 1);
    B->atom.alive = 1;
  } else {
    for (int i = 0; i < received.matrix.n && i < CUBALC_ATOM_BITS; i++)
      if (cubalc_matrix_get(&received.matrix, i))
        cubalc_matrix_set(&B->atom.matrix, i, 0);
  }
  if (!B->atom.digit_lock)
    B->atom.digit = (uint8_t)cubalc_algocube_digit(&B->atom.matrix);
  B->atom.unity = cubalc_matrix_compat(&A->atom.matrix, &B->atom.matrix);
  if (ch->hold_flash) B->atom.proton = B->atom.proton; /* sticky chain hold — no flash path */
  return 0;
}

int cubalc_smx_write_frame(const char *path, const uint8_t *frame, size_t n) {
  FILE *f;
  if (!path || !frame || !n) return -1;
  f = fopen(path, "wb");
  if (!f) return -1;
  if (fwrite(frame, 1, n, f) != n) { fclose(f); return -1; }
  fclose(f);
  return 0;
}

int cubalc_smx_read_frame(const char *path, uint8_t *out, size_t cap, size_t *n_out) {
  FILE *f;
  size_t n;
  if (!path || !out) return -1;
  f = fopen(path, "rb");
  if (!f) return -1;
  n = fread(out, 1, cap, f);
  fclose(f);
  if (n_out) *n_out = n;
  return n > 0 ? 0 : -1;
}

/* ---- binary bus (no HTTP): u32le length + SMX2 frame ---- */
static int write_full(int fd, const void *buf, size_t n) {
  const uint8_t *p = (const uint8_t *)buf;
  size_t o = 0;
  while (o < n) {
#if defined(CUBALC_OS_WINDOWS)
    int w = send(fd, (const char *)(p + o), (int)(n - o), 0);
#else
    ssize_t w = write(fd, p + o, n - o);
#endif
    if (w < 0) {
      if (errno == EINTR) continue;
      return -1;
    }
    if (w == 0) return -1;
    o += (size_t)w;
  }
  return 0;
}

static int read_full(int fd, void *buf, size_t n) {
  uint8_t *p = (uint8_t *)buf;
  size_t o = 0;
  while (o < n) {
#if defined(CUBALC_OS_WINDOWS)
    int r = recv(fd, (char *)(p + o), (int)(n - o), 0);
#else
    ssize_t r = read(fd, p + o, n - o);
#endif
    if (r < 0) {
      if (errno == EINTR) continue;
      return -1;
    }
    if (r == 0) return -1;
    o += (size_t)r;
  }
  return 0;
}

int cubalc_smx_send_frame(int fd, const uint8_t *frame, size_t n) {
  uint8_t hdr[4];
  if (fd < 0 || !frame || n == 0 || n > 0xffffu) return -1;
  cubalc_sock_init();
  hdr[0] = (uint8_t)(n & 0xff);
  hdr[1] = (uint8_t)((n >> 8) & 0xff);
  hdr[2] = (uint8_t)((n >> 16) & 0xff);
  hdr[3] = (uint8_t)((n >> 24) & 0xff);
  if (write_full(fd, hdr, 4) != 0) return -1;
  return write_full(fd, frame, n);
}

int cubalc_smx_recv_frame(int fd, uint8_t *out, size_t cap, size_t *n_out) {
  uint8_t hdr[4];
  uint32_t n;
  if (fd < 0 || !out || cap < 16) return -1;
  cubalc_sock_init();
  if (read_full(fd, hdr, 4) != 0) return -1;
  n = (uint32_t)hdr[0] | ((uint32_t)hdr[1] << 8) |
      ((uint32_t)hdr[2] << 16) | ((uint32_t)hdr[3] << 24);
  if (n < 16 || n > cap || n > 0xffffu) return -1;
  if (read_full(fd, out, n) != 0) return -1;
  if (n_out) *n_out = n;
  return 0;
}

int cubalc_smx_unix_listen(const char *sock_path, int backlog) {
#if defined(CUBALC_OS_WINDOWS)
  (void)sock_path; (void)backlog;
  return -1; /* AF_UNIX not available on classic Winsock */
#else
  int fd;
  struct sockaddr_un addr;
  if (!sock_path || !sock_path[0] || strlen(sock_path) >= sizeof addr.sun_path)
    return -1;
  unlink(sock_path);
  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) return -1;
  memset(&addr, 0, sizeof addr);
  addr.sun_family = AF_UNIX;
  snprintf(addr.sun_path, sizeof addr.sun_path, "%s", sock_path);
  if (bind(fd, (struct sockaddr *)&addr, sizeof addr) != 0) {
    close(fd);
    return -1;
  }
  if (listen(fd, backlog > 0 ? backlog : 1) != 0) {
    close(fd);
    unlink(sock_path);
    return -1;
  }
  return fd;
#endif
}

int cubalc_smx_unix_connect(const char *sock_path) {
#if defined(CUBALC_OS_WINDOWS)
  (void)sock_path;
  return -1;
#else
  int fd;
  struct sockaddr_un addr;
  if (!sock_path || !sock_path[0] || strlen(sock_path) >= sizeof addr.sun_path)
    return -1;
  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) return -1;
  memset(&addr, 0, sizeof addr);
  addr.sun_family = AF_UNIX;
  snprintf(addr.sun_path, sizeof addr.sun_path, "%s", sock_path);
  if (connect(fd, (struct sockaddr *)&addr, sizeof addr) != 0) {
    close(fd);
    return -1;
  }
  return fd;
#endif
}

int cubalc_smx_tcp_listen(const char *host, int port, int backlog) {
  int fd, on = 1;
  struct sockaddr_in addr;
  if (port <= 0 || port > 65535) return -1;
  cubalc_sock_init();
  fd = (int)socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) return -1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof on);
  memset(&addr, 0, sizeof addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons((uint16_t)port);
  if (!host || !host[0] || strcmp(host, "0.0.0.0") == 0 ||
      strcmp(host, "*") == 0 || strcmp(host, "any") == 0) {
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
    CUBALC_CLOSESOCK(fd);
    return -1;
  }
  if (bind(fd, (struct sockaddr *)&addr, sizeof addr) != 0) {
    CUBALC_CLOSESOCK(fd);
    return -1;
  }
  if (listen(fd, backlog > 0 ? backlog : 4) != 0) {
    CUBALC_CLOSESOCK(fd);
    return -1;
  }
  return fd;
}

int cubalc_smx_tcp_connect(const char *host, int port) {
  int fd;
  struct sockaddr_in addr;
  struct addrinfo hints, *res = NULL, *rp;
  char portstr[16];
  if (!host || !host[0] || port <= 0 || port > 65535) return -1;
  cubalc_sock_init();
  /* fast path: dotted IPv4 */
  memset(&addr, 0, sizeof addr);
  if (inet_pton(AF_INET, host, &addr.sin_addr) == 1) {
    fd = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (connect(fd, (struct sockaddr *)&addr, sizeof addr) != 0) {
      CUBALC_CLOSESOCK(fd);
      return -1;
    }
    return fd;
  }
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  snprintf(portstr, sizeof portstr, "%d", port);
  if (getaddrinfo(host, portstr, &hints, &res) != 0) return -1;
  fd = -1;
  for (rp = res; rp; rp = rp->ai_next) {
    fd = (int)socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (fd < 0) continue;
    if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) break;
    CUBALC_CLOSESOCK(fd);
    fd = -1;
  }
  freeaddrinfo(res);
  return fd;
}
