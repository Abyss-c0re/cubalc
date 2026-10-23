/* CubalC lang — lang_ops_core_sys.c
 * SYS/HOST megablock dispatch (mesh vote A_split_lang_ops_core).
 * Peel of ~33k-line SYS plane from lang_ops_core.
 */
#include "lang/cubalc_lang_ops_sys_string.h"
#include "lang/cubalc_lang_ops_sys_codec.h"
#include "lang/cubalc_lang_ops_sys_time_walk.h"
#include "lang/cubalc_lang_ops_sys_plate_expand.h"
#include "lang/cubalc_lang_ops_sys_cli.h"
#include "lang/cubalc_lang_ops_core_help.h"
#include "cubalc_hostops.h"
#include "cubalc_async.h"
#include "cubalc_smx.h"
#include "cubalc_law.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#  include <dirent.h>
#  include <sys/stat.h>
#  include <sys/statvfs.h>
#  include <fcntl.h>
#  include <poll.h>
#  include <pwd.h>
#  include <grp.h>
#  include <fnmatch.h>
#  include <sys/utsname.h>
#  include <sys/resource.h>
#  include <sys/socket.h>
#  include <sys/wait.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
/* getloadavg is BSD/POSIX extension; declare if libc omits under strict feature macros. */
int getloadavg(double loadavg[], int nelem);
#endif

int cubalc_lang_ops_sys_try_string(VM *vm, Lex *L) {
  /* String/codec SYS plane — EXIST · HASH/CRC · BASE64/HEX/URL/UUID · JSONESC · SHQUOTE
   * A2 theme peel. L at subcommand after plate/path_fs/env miss.
   */
    if (kw(&L->cur,"EXIST") || kw(&L->cur,"EXISTS")){
      lex_next(L);
      char path[512];
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm,"SYS EXIST \"path\"|LAST"); return -1;
      }
      int e = cubalc_host_exists(path);
      var_set_num(vm, "LAST_N", e);
      var_set_num(vm, "EXIST", e);
      bump(vm); return 1;
    }
    /* SYS HASH|FINGERPRINT|FNV [str]
     * — FNV-1a 32-bit hex fingerprint of string (default prior LAST).
     * LAST = 8-char lowercase hex; LAST_N / HASH_N = hash as unsigned long.
     * SYS HASHFILE|FILEHASH|FINGERPRINTFILE path — same for file bytes.
     * Soft path miss → OK=0. Empty string → still OK with FNV seed digest.
     * Usability: plate/cache change stamps without shell md5sum/sha. */
    if (kw(&L->cur,"HASH") || kw(&L->cur,"FINGERPRINT") || kw(&L->cur,"FNV") ||
        kw(&L->cur,"FNV1A") || kw(&L->cur,"DIGEST") || kw(&L->cur,"HASHSTR") ||
        kw(&L->cur,"STRHASH") || kw(&L->cur,"HASH32") ||
        kw(&L->cur,"HASHFILE") || kw(&L->cur,"FILEHASH") ||
        kw(&L->cur,"FINGERPRINTFILE") || kw(&L->cur,"FNVFILE") ||
        kw(&L->cur,"DIGESTFILE") || kw(&L->cur,"FILEFNV") ||
        kw(&L->cur,"HASHF") || kw(&L->cur,"CONTENTHASH")){
      char src[CUBALC_HOST_STR_MAX], path[512], hex[9], a[CUBALC_HOST_STR_MAX];
      uint32_t h;
      long n_as_long;
      int is_file = (kw(&L->cur,"HASHFILE") || kw(&L->cur,"FILEHASH") ||
                    kw(&L->cur,"FINGERPRINTFILE") || kw(&L->cur,"FNVFILE") ||
                    kw(&L->cur,"DIGESTFILE") || kw(&L->cur,"FILEFNV") ||
                    kw(&L->cur,"HASHF") || kw(&L->cur,"CONTENTHASH"));
      lex_next(L);
      src[0] = 0; path[0] = 0; hex[0] = 0; a[0] = 0;
      if (is_file) {
        cubalc_host_result hr;
        if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
          fail(vm, "SYS HASHFILE path");
          return -1;
        }
        snprintf(path, sizeof path, "%s", a);
        if (!path[0] || cubalc_host_read(path, &hr) != 0) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "HASH_N", 0);
          var_set_num(vm, "HASHFILE_N", 0);
          var_set_num(vm, "HASH_BYTES", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", path[0] ? "HASHFILE: read fail" : "HASHFILE: empty path");
          var_set_str(vm, "ERR", path[0] ? "HASHFILE: read fail" : "HASHFILE: empty path");
          bump(vm); return 1;
        }
        h = cubalc_fnv1a32(hr.str, strlen(hr.str));
        cubalc_hash_hex8(h, hex);
        n_as_long = (long)(uint32_t)h;
        var_set_str(vm, "LAST", hex);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", hex);
        vm->last_n = n_as_long;
        var_set_num(vm, "LAST_N", n_as_long);
        var_set_str(vm, "HASH", hex);
        var_set_str(vm, "HASHFILE", hex);
        var_set_str(vm, "FINGERPRINT", hex);
        var_set_num(vm, "HASH_N", n_as_long);
        var_set_num(vm, "HASHFILE_N", n_as_long);
        var_set_num(vm, "HASH_BYTES", (long)strlen(hr.str));
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      /* string hash */
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      h = cubalc_fnv1a32(src, strlen(src));
      cubalc_hash_hex8(h, hex);
      n_as_long = (long)(uint32_t)h;
      var_set_str(vm, "LAST", hex);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hex);
      vm->last_n = n_as_long;
      var_set_num(vm, "LAST_N", n_as_long);
      var_set_str(vm, "HASH", hex);
      var_set_str(vm, "FINGERPRINT", hex);
      var_set_str(vm, "FNV", hex);
      var_set_num(vm, "HASH_N", n_as_long);
      var_set_num(vm, "HASH_BYTES", (long)strlen(src));
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CRC32|CRC|CHECKSUM32 [str]
     * — IEEE CRC-32 hex stamp of string (default prior LAST).
     * LAST = 8-char lowercase hex; LAST_N / CRC32_N = value as unsigned long.
     * SYS CRC32FILE|FILECRC32|CRCFILE path — same for file bytes.
     * Soft path miss → OK=0. Empty string still OK (known digest).
     * Usability: interoperable integrity vs tools/zip without shell cksum. */
    if (kw(&L->cur,"CRC32") || kw(&L->cur,"CRC") || kw(&L->cur,"CHECKSUM32") ||
        kw(&L->cur,"CRC32SUM") || kw(&L->cur,"ISOCRC") || kw(&L->cur,"PKZIPCRC") ||
        kw(&L->cur,"CRC32FILE") || kw(&L->cur,"FILECRC32") || kw(&L->cur,"CRCFILE") ||
        kw(&L->cur,"FILECRC") || kw(&L->cur,"CRC32F") || kw(&L->cur,"CHECKSUMFILE") ||
        kw(&L->cur,"CRC32PATH")){
      char src[CUBALC_HOST_STR_MAX], path[512], hex[9], a[CUBALC_HOST_STR_MAX];
      uint32_t h;
      long n_as_long;
      size_t nbytes = 0;
      int is_file = (kw(&L->cur,"CRC32FILE") || kw(&L->cur,"FILECRC32") ||
                    kw(&L->cur,"CRCFILE") || kw(&L->cur,"FILECRC") ||
                    kw(&L->cur,"CRC32F") || kw(&L->cur,"CHECKSUMFILE") ||
                    kw(&L->cur,"CRC32PATH"));
      lex_next(L);
      src[0] = 0; path[0] = 0; hex[0] = 0; a[0] = 0;
      if (is_file) {
        cubalc_host_result hr;
        if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
          fail(vm, "SYS CRC32FILE path");
          return -1;
        }
        snprintf(path, sizeof path, "%s", a);
        if (!path[0] || cubalc_host_read(path, &hr) != 0) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "CRC32_N", 0);
          var_set_num(vm, "CRC32FILE_N", 0);
          var_set_num(vm, "CRC32_BYTES", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", path[0] ? "CRC32FILE: read fail" : "CRC32FILE: empty path");
          var_set_str(vm, "ERR", path[0] ? "CRC32FILE: read fail" : "CRC32FILE: empty path");
          bump(vm); return 1;
        }
        nbytes = strlen(hr.str);
        h = cubalc_crc32(hr.str, nbytes);
        cubalc_hash_hex8(h, hex);
        n_as_long = (long)(uint32_t)h;
        var_set_str(vm, "LAST", hex);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", hex);
        vm->last_n = n_as_long;
        var_set_num(vm, "LAST_N", n_as_long);
        var_set_str(vm, "CRC32", hex);
        var_set_str(vm, "CRC32FILE", hex);
        var_set_str(vm, "CRC", hex);
        var_set_num(vm, "CRC32_N", n_as_long);
        var_set_num(vm, "CRC32FILE_N", n_as_long);
        var_set_num(vm, "CRC32_BYTES", (long)nbytes);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      nbytes = strlen(src);
      h = cubalc_crc32(src, nbytes);
      cubalc_hash_hex8(h, hex);
      n_as_long = (long)(uint32_t)h;
      var_set_str(vm, "LAST", hex);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hex);
      vm->last_n = n_as_long;
      var_set_num(vm, "LAST_N", n_as_long);
      var_set_str(vm, "CRC32", hex);
      var_set_str(vm, "CRC", hex);
      var_set_str(vm, "CHECKSUM32", hex);
      var_set_num(vm, "CRC32_N", n_as_long);
      var_set_num(vm, "CRC32_BYTES", (long)nbytes);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS BASE64|B64ENC|ENCODE64 [str]
     * SYS BASE64D|B64DEC|DECODE64 [str]
     * — standard base64 encode/decode (default prior LAST).
     * LAST = result; LAST_N / BASE64_N = output byte length; soft fail OK=0.
     * Usability: plate/payload transport stamps without shell base64. */
    if (kw(&L->cur,"BASE64") || kw(&L->cur,"B64ENC") || kw(&L->cur,"ENCODE64") ||
        kw(&L->cur,"B64ENCODE") || kw(&L->cur,"TOBASE64") || kw(&L->cur,"B64") ||
        kw(&L->cur,"BASE64ENC") || kw(&L->cur,"ENC64") ||
        kw(&L->cur,"BASE64D") || kw(&L->cur,"B64DEC") || kw(&L->cur,"DECODE64") ||
        kw(&L->cur,"B64DECODE") || kw(&L->cur,"FROMBASE64") || kw(&L->cur,"BASE64DEC") ||
        kw(&L->cur,"DEC64") || kw(&L->cur,"UNBASE64")){
      char src[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      size_t n_out = 0;
      int is_dec = (kw(&L->cur,"BASE64D") || kw(&L->cur,"B64DEC") ||
                   kw(&L->cur,"DECODE64") || kw(&L->cur,"B64DECODE") ||
                   kw(&L->cur,"FROMBASE64") || kw(&L->cur,"BASE64DEC") ||
                   kw(&L->cur,"DEC64") || kw(&L->cur,"UNBASE64"));
      int rc;
      lex_next(L);
      src[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      if (is_dec)
        rc = cubalc_b64_dec(src, strlen(src), out, sizeof out, &n_out);
      else
        rc = cubalc_b64_enc(src, strlen(src), out, sizeof out, &n_out);
      if (rc != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "BASE64_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", is_dec ? "BASE64D: decode fail" : "BASE64: encode fail");
        var_set_str(vm, "ERR", is_dec ? "BASE64D: decode fail" : "BASE64: encode fail");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)n_out;
      var_set_num(vm, "LAST_N", (long)n_out);
      var_set_str(vm, "BASE64", out);
      if (is_dec) {
        var_set_str(vm, "BASE64D", out);
        var_set_str(vm, "B64DEC", out);
      } else {
        var_set_str(vm, "B64ENC", out);
        var_set_str(vm, "ENCODE64", out);
      }
      var_set_num(vm, "BASE64_N", (long)n_out);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS HEX|HEXENC|TOHEX|ENCODEHEX [str]
     * SYS HEXD|HEXDEC|FROMHEX|DECODEHEX [str]
     * — lowercase hex encode/decode (default prior LAST).
     * LAST = result; LAST_N / HEX_N = output length; soft fail OK=0.
     * Usability: binary-safe plate fields / fingerprints without shell xxd. */
    if (kw(&L->cur,"HEX") || kw(&L->cur,"HEXENC") || kw(&L->cur,"TOHEX") ||
        kw(&L->cur,"ENCODEHEX") || kw(&L->cur,"HEXENCODE") || kw(&L->cur,"BINTOHEX") ||
        kw(&L->cur,"ASHEX") || kw(&L->cur,"HEXIFY") ||
        kw(&L->cur,"HEXD") || kw(&L->cur,"HEXDEC") || kw(&L->cur,"FROMHEX") ||
        kw(&L->cur,"DECODEHEX") || kw(&L->cur,"HEXDECODE") || kw(&L->cur,"UNHEX") ||
        kw(&L->cur,"HEXTOBIN") || kw(&L->cur,"FROM_HEX")){
      char src[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      size_t n_out = 0;
      int is_dec = (kw(&L->cur,"HEXD") || kw(&L->cur,"HEXDEC") ||
                   kw(&L->cur,"FROMHEX") || kw(&L->cur,"DECODEHEX") ||
                   kw(&L->cur,"HEXDECODE") || kw(&L->cur,"UNHEX") ||
                   kw(&L->cur,"HEXTOBIN") || kw(&L->cur,"FROM_HEX"));
      int rc;
      lex_next(L);
      src[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      if (is_dec)
        rc = cubalc_hex_dec(src, strlen(src), out, sizeof out, &n_out);
      else
        rc = cubalc_hex_enc(src, strlen(src), out, sizeof out, &n_out);
      if (rc != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "HEX_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", is_dec ? "HEXD: decode fail" : "HEX: encode fail");
        var_set_str(vm, "ERR", is_dec ? "HEXD: decode fail" : "HEX: encode fail");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)n_out;
      var_set_num(vm, "LAST_N", (long)n_out);
      var_set_str(vm, "HEX", out);
      if (is_dec) {
        var_set_str(vm, "HEXD", out);
        var_set_str(vm, "HEXDEC", out);
      } else {
        var_set_str(vm, "HEXENC", out);
        var_set_str(vm, "TOHEX", out);
      }
      var_set_num(vm, "HEX_N", (long)n_out);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS URLENC|URLENCODE|PERCENTENC|PCTENC [str]
     * SYS URLDEC|URLDECODE|PERCENTDEC|PCTDEC [str]
     * — RFC 3986 percent-encode/decode (default prior LAST).
     * LAST = result; LAST_N / URLENC_N = output length; soft fail OK=0.
     * Usability: path/query-safe plate fields without shell urlencode. */
    if (kw(&L->cur,"URLENC") || kw(&L->cur,"URLENCODE") || kw(&L->cur,"PERCENTENC") ||
        kw(&L->cur,"PCTENC") || kw(&L->cur,"URIENC") || kw(&L->cur,"URIENCODE") ||
        kw(&L->cur,"PERCENTENCODE") || kw(&L->cur,"URL_ENCODE") ||
        kw(&L->cur,"URLDEC") || kw(&L->cur,"URLDECODE") || kw(&L->cur,"PERCENTDEC") ||
        kw(&L->cur,"PCTDEC") || kw(&L->cur,"URIDEC") || kw(&L->cur,"URLDECODE") ||
        kw(&L->cur,"PERCENTDECODE") || kw(&L->cur,"URL_DECODE") ||
        kw(&L->cur,"UNURL") || kw(&L->cur,"DEPERCENT")){
      char src[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      size_t n_out = 0;
      int is_dec = (kw(&L->cur,"URLDEC") || kw(&L->cur,"URLDECODE") ||
                   kw(&L->cur,"PERCENTDEC") || kw(&L->cur,"PCTDEC") ||
                   kw(&L->cur,"URIDEC") || kw(&L->cur,"PERCENTDECODE") ||
                   kw(&L->cur,"URL_DECODE") || kw(&L->cur,"UNURL") ||
                   kw(&L->cur,"DEPERCENT"));
      int rc;
      lex_next(L);
      src[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      if (is_dec)
        rc = cubalc_url_dec(src, strlen(src), out, sizeof out, &n_out);
      else
        rc = cubalc_url_enc(src, strlen(src), out, sizeof out, &n_out);
      if (rc != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "URLENC_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", is_dec ? "URLDEC: decode fail" : "URLENC: encode fail");
        var_set_str(vm, "ERR", is_dec ? "URLDEC: decode fail" : "URLENC: encode fail");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)n_out;
      var_set_num(vm, "LAST_N", (long)n_out);
      var_set_str(vm, "URLENC", out);
      if (is_dec) {
        var_set_str(vm, "URLDEC", out);
        var_set_str(vm, "URLDECODE", out);
      } else {
        var_set_str(vm, "URLENCODE", out);
        var_set_str(vm, "PERCENTENC", out);
      }
      var_set_num(vm, "URLENC_N", (long)n_out);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS UUID|GUID|GENUUID|NEWUUID
     * SYS UUID0|UUIDNODASH|GUID0|COMPACTUUID
     * — RFC 4122 v4 UUID (dashed or 32-hex compact).
     * LAST = id; LAST_N / UUID_N = length (36|32); OK=1.
     * Usability: unique agent work/claim ids without shell uuidgen. */
    if (kw(&L->cur,"UUID") || kw(&L->cur,"GUID") || kw(&L->cur,"GENUUID") ||
        kw(&L->cur,"NEWUUID") || kw(&L->cur,"GENID") || kw(&L->cur,"MAKEUUID") ||
        kw(&L->cur,"UUIDV4") || kw(&L->cur,"UUID4") ||
        kw(&L->cur,"UUID0") || kw(&L->cur,"UUIDNODASH") || kw(&L->cur,"GUID0") ||
        kw(&L->cur,"COMPACTUUID") || kw(&L->cur,"UUIDCOMPACT") ||
        kw(&L->cur,"UUIDN") || kw(&L->cur,"NODASHUUID")){
      char out[48];
      int compact = (kw(&L->cur,"UUID0") || kw(&L->cur,"UUIDNODASH") ||
                    kw(&L->cur,"GUID0") || kw(&L->cur,"COMPACTUUID") ||
                    kw(&L->cur,"UUIDCOMPACT") || kw(&L->cur,"UUIDN") ||
                    kw(&L->cur,"NODASHUUID"));
      int n;
      lex_next(L);
      out[0] = 0;
      n = cubalc_uuid_fmt(out, sizeof out, compact);
      if (n < 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "UUID_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "UUID: format fail");
        var_set_str(vm, "ERR", "UUID: format fail");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)n;
      var_set_num(vm, "LAST_N", (long)n);
      var_set_str(vm, "UUID", out);
      var_set_str(vm, "GUID", out);
      if (compact) {
        var_set_str(vm, "UUID0", out);
        var_set_str(vm, "UUIDNODASH", out);
      } else {
        var_set_str(vm, "GENUUID", out);
        var_set_str(vm, "NEWUUID", out);
      }
      var_set_num(vm, "UUID_N", (long)n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ENTROPY|URANDOM|RANDHEX [n] — n random bytes as lowercase hex → LAST.
     * Default n=16 (32 hex chars); cap 256 bytes. LAST_N = hex length.
     * Usability: agent nonces/tokens/salts beyond UUID (fixed) / RAND (int). */
    if (kw(&L->cur,"ENTROPY") || kw(&L->cur,"URANDOM") || kw(&L->cur,"RANDHEX") ||
        kw(&L->cur,"RANDOMHEX") || kw(&L->cur,"HEXRAND") || kw(&L->cur,"RAND_BYTES") ||
        kw(&L->cur,"RANDBYTES") || kw(&L->cur,"SECURE_RAND") || kw(&L->cur,"NONCE")){
      long nbytes = 16;
      unsigned char raw[256];
      char out[256 * 2 + 4];
      size_t o = 0;
      long i;
      lex_next(L);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          (L->cur.kind == TK_IDENT && var_get(vm, L->cur.text, 0) &&
           !var_get(vm, L->cur.text, 0)->is_str)) {
        if (L->cur.kind == TK_IDENT) {
          Var *gv = var_get(vm, L->cur.text, 0);
          nbytes = gv->val;
          lex_next(L);
        } else {
          nbytes = parse_expr(vm, L);
        }
      }
      if (nbytes < 1) nbytes = 1;
      if (nbytes > 256) nbytes = 256;
      memset(raw, 0, (size_t)nbytes);
#if !defined(CUBALC_OS_WINDOWS)
      {
        FILE *rf = fopen("/dev/urandom", "rb");
        if (!rf) {
          /* soft fail */
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "ENTROPY_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", "ENTROPY: urandom open fail");
          var_set_str(vm, "ERR", "ENTROPY: urandom open fail");
          bump(vm); return 1;
        }
        if (fread(raw, 1, (size_t)nbytes, rf) != (size_t)nbytes) {
          fclose(rf);
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", "ENTROPY: short read");
          var_set_str(vm, "ERR", "ENTROPY: short read");
          bump(vm); return 1;
        }
        fclose(rf);
      }
#else
      /* weak fallback */
      for (i = 0; i < nbytes; i++)
        raw[i] = (unsigned char)(rand() & 0xff);
#endif
      for (i = 0; i < nbytes; i++) {
        static const char *hx = "0123456789abcdef";
        out[o++] = hx[(raw[i] >> 4) & 0xf];
        out[o++] = hx[raw[i] & 0xf];
      }
      out[o] = 0;
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)o;
      var_set_num(vm, "LAST_N", (long)o);
      var_set_str(vm, "ENTROPY", out);
      var_set_str(vm, "URANDOM", out);
      var_set_str(vm, "RANDHEX", out);
      var_set_num(vm, "ENTROPY_N", (long)o);
      var_set_num(vm, "ENTROPY_BYTES", nbytes);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS JSONESC|JESC|ESCJSON|JSON_ESCAPE [str]
     * SYS JSONUNESC|JUNESC|UNJSON|JSON_UNESCAPE [str]
     * — JSON string-body escape/unescape (no outer quotes; default prior LAST).
     * LAST = result; LAST_N / JSONESC_N = length; soft fail OK=0.
     * Usability: embed agent text in cubalc.*.v1 plates without hand-escaping. */
    if (kw(&L->cur,"JSONESC") || kw(&L->cur,"JESC") || kw(&L->cur,"ESCJSON") ||
        kw(&L->cur,"JSON_ESCAPE") || kw(&L->cur,"JSONESCAPE") || kw(&L->cur,"TOJSONSTR") ||
        kw(&L->cur,"JSONQUOTEBODY") ||
        kw(&L->cur,"JSONUNESC") || kw(&L->cur,"JUNESC") || kw(&L->cur,"UNJSON") ||
        kw(&L->cur,"JSON_UNESCAPE") || kw(&L->cur,"JSONUNESCAPE") ||
        kw(&L->cur,"FROMJSONSTR") || kw(&L->cur,"UNESCJSON")){
      char src[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      size_t n_out = 0;
      int is_dec = (kw(&L->cur,"JSONUNESC") || kw(&L->cur,"JUNESC") ||
                   kw(&L->cur,"UNJSON") || kw(&L->cur,"JSON_UNESCAPE") ||
                   kw(&L->cur,"JSONUNESCAPE") || kw(&L->cur,"FROMJSONSTR") ||
                   kw(&L->cur,"UNESCJSON"));
      int rc;
      lex_next(L);
      src[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      if (is_dec)
        rc = cubalc_json_unesc(src, strlen(src), out, sizeof out, &n_out);
      else
        rc = cubalc_json_esc(src, strlen(src), out, sizeof out, &n_out);
      if (rc != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONESC_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", is_dec ? "JSONUNESC: decode fail" : "JSONESC: encode fail");
        var_set_str(vm, "ERR", is_dec ? "JSONUNESC: decode fail" : "JSONESC: encode fail");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)n_out;
      var_set_num(vm, "LAST_N", (long)n_out);
      var_set_str(vm, "JSONESC", out);
      if (is_dec) {
        var_set_str(vm, "JSONUNESC", out);
        var_set_str(vm, "JUNESC", out);
      } else {
        var_set_str(vm, "JESC", out);
        var_set_str(vm, "ESCJSON", out);
      }
      var_set_num(vm, "JSONESC_N", (long)n_out);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SHQUOTE|SHELLQUOTE|SQUOTE [str] — shell-safe single-quote wrap.
     * 'foo' + internal ' → '\''  (POSIX shell-safe for SPAWN/logs).
     * Missing arg uses LAST. LAST_N = length. Soft fail if overflow.
     * Usability: quote plate/user strings for shell without hand escaping. */
    if (kw(&L->cur,"SHQUOTE") || kw(&L->cur,"SHELLQUOTE") || kw(&L->cur,"SQUOTE") ||
        kw(&L->cur,"SHELL_QUOTE") || kw(&L->cur,"QUOTE_SHELL") || kw(&L->cur,"SQ") ||
        kw(&L->cur,"POSIXQUOTE") || kw(&L->cur,"SH_QUOTE")){
      char src[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      size_t i, o = 0;
      lex_next(L);
      src[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      /* wrap in single quotes; escape ' as '\'' (quote, backslash, quote, quote) */
      if (o + 1 >= sizeof out) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "SHQUOTE: overflow");
        var_set_str(vm, "ERR", "SHQUOTE: overflow");
        bump(vm); return 1;
      }
      out[o++] = (char)39; /* ' */
      for (i = 0; src[i]; i++) {
        if (src[i] == (char)39) {
          if (o + 4 >= sizeof out) {
            var_set_str(vm, "LAST", "");
            vm->last_str[0] = 0;
            vm->last_n = 0;
            var_set_num(vm, "LAST_N", 0);
            var_set_num(vm, "OK", 0);
            var_set_str(vm, "LAST_ERR", "SHQUOTE: overflow");
            var_set_str(vm, "ERR", "SHQUOTE: overflow");
            bump(vm); return 1;
          }
          out[o++] = (char)39;  /* end quote */
          out[o++] = (char)92;  /* backslash */
          out[o++] = (char)39;  /* quote */
          out[o++] = (char)39;  /* reopen quote */
        } else {
          if (o + 1 >= sizeof out) {
            var_set_str(vm, "LAST", "");
            vm->last_str[0] = 0;
            vm->last_n = 0;
            var_set_num(vm, "LAST_N", 0);
            var_set_num(vm, "OK", 0);
            var_set_str(vm, "LAST_ERR", "SHQUOTE: overflow");
            var_set_str(vm, "ERR", "SHQUOTE: overflow");
            bump(vm); return 1;
          }
          out[o++] = src[i];
        }
      }
      if (o + 1 >= sizeof out) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "SHQUOTE: overflow");
        var_set_str(vm, "ERR", "SHQUOTE: overflow");
        bump(vm); return 1;
      }
      out[o++] = (char)39; /* closing ' */
      out[o] = 0;
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)o;
      var_set_num(vm, "LAST_N", (long)o);
      var_set_str(vm, "SHQUOTE", out);
      var_set_str(vm, "SHELLQUOTE", out);
      var_set_str(vm, "SQUOTE", out);
      var_set_num(vm, "SHQUOTE_N", (long)o);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SHUNQUOTE|UNQUOTE|SHELLUNQUOTE [str] — peel POSIX single-quote wrap.
     * Inverse of SHQUOTE: strip outer quotes; map '\'' → ' inside.
     * Unquoted input returned as-is (OK=1). Missing arg uses LAST.
     * Usability: recover raw plate text from shell-quoted fields without shell eval. */
    if (kw(&L->cur,"SHUNQUOTE") || kw(&L->cur,"UNQUOTE") || kw(&L->cur,"SHELLUNQUOTE") ||
        kw(&L->cur,"UNSHQUOTE") || kw(&L->cur,"DEQUOTE") || kw(&L->cur,"UNSQUOTE") ||
        kw(&L->cur,"POSIXUNQUOTE") || kw(&L->cur,"SH_UNQUOTE")){
      char src[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      size_t i = 0, o = 0, n;
      lex_next(L);
      src[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      n = strlen(src);
      /* if starts and ends with single-quote and len>=2, peel */
      if (n >= 2 && src[0] == (char)39 && src[n - 1] == (char)39) {
        i = 1;
        while (i + 1 < n) { /* stop before final closing quote */
          if (src[i] == (char)39 && i + 3 < n &&
              src[i + 1] == (char)92 && src[i + 2] == (char)39 && src[i + 3] == (char)39) {
            /* '\'' sequence → one ' */
            if (o + 1 >= sizeof out) break;
            out[o++] = (char)39;
            i += 4;
          } else if (src[i] == (char)39 && i + 1 == n - 1) {
            /* closing quote reached */
            break;
          } else {
            if (o + 1 >= sizeof out) break;
            out[o++] = src[i];
            i++;
          }
        }
        out[o] = 0;
      } else {
        /* not single-quoted — pass through */
        snprintf(out, sizeof out, "%s", src);
        o = strlen(out);
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)o;
      var_set_num(vm, "LAST_N", (long)o);
      var_set_str(vm, "SHUNQUOTE", out);
      var_set_str(vm, "UNQUOTE", out);
      var_set_num(vm, "SHUNQUOTE_N", (long)o);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
  return 0;
}
