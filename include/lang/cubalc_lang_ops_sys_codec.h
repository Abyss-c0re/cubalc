/* CubalC lang — SYS codec plane (BASE64 HEX URL UUID JSONESC CBXF FNV CRC).
 * Split from lang_ops_core (mesh vote A_split_lang_ops_core).
 * Law: cube is SoT · modular · zero bloat · pure C.
 */
#ifndef CUBALC_LANG_OPS_SYS_CODEC_H
#define CUBALC_LANG_OPS_SYS_CODEC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int cubalc_b64_enc(const char *in, size_t in_len, char *out, size_t out_cap, size_t *n_out);
int cubalc_b64_dec(const char *in, size_t in_len, char *out, size_t out_cap, size_t *n_out);
int cubalc_hex_enc(const char *in, size_t in_len, char *out, size_t out_cap, size_t *n_out);
int cubalc_hex_dec(const char *in, size_t in_len, char *out, size_t out_cap, size_t *n_out);
int cubalc_url_enc(const char *in, size_t in_len, char *out, size_t out_cap, size_t *n_out);
int cubalc_url_dec(const char *in, size_t in_len, char *out, size_t out_cap, size_t *n_out);
int cubalc_uuid_fmt(char *out, size_t out_cap, int compact);
int cubalc_json_esc(const char *in, size_t in_len, char *out, size_t out_cap, size_t *n_out);
int cubalc_json_unesc(const char *in, size_t in_len, char *out, size_t out_cap, size_t *n_out);
int cubalc_cbxf_send(int fd, const char *payload, size_t plen);
int cubalc_cbxf_recv(int fd, char *out, size_t cap, size_t *n_out, int timeout_ms);
int cubalc_parse_host_port_simple(const char *ep, char *host, size_t hsz, int *port);

uint32_t cubalc_fnv1a32(const char *s, size_t n);
void cubalc_hash_hex8(uint32_t h, char out[9]);
uint32_t cubalc_crc32(const char *s, size_t n);

#ifdef __cplusplus
}
#endif
#endif /* CUBALC_LANG_OPS_SYS_CODEC_H */
