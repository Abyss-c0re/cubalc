/* CubalC lang — lang_ops_dual_clip.c
 * DUAL clip/sext/rotate plane: DCLIP32…DROL32/DROR32 (G peel from dual).
 * Mesh vote G_split_dual_cell · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_dual_clip.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

int cubalc_lang_ops_dual_try_clip(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-9 dual-stack data-path 32-bit: DCLIP32 · DSEXT32 · DZEXT32 */
  if (kw(&L->cur,"DCLIP32")||kw(&L->cur,"2CLIP32")||kw(&L->cur,"S2CLIP32")||
      kw(&L->cur,"STACK2CLIP32")||kw(&L->cur,"PAIRCLIP32")||
      kw(&L->cur,"DSEXT32")||kw(&L->cur,"2SEXT32")||kw(&L->cur,"S2SEXT32")||
      kw(&L->cur,"STACK2SEXT32")||kw(&L->cur,"PAIRSEXT32")||kw(&L->cur,"DSEXTD")||
      kw(&L->cur,"2SEXTD")||
      kw(&L->cur,"DZEXT32")||kw(&L->cur,"2ZEXT32")||kw(&L->cur,"S2ZEXT32")||
      kw(&L->cur,"STACK2ZEXT32")||kw(&L->cur,"PAIRZEXT32")||kw(&L->cur,"DZEXTD")||
      kw(&L->cur,"2ZEXTD")||kw(&L->cur,"DZEROEXT32")){
    /* a b → f(a) f(b)
     * CLIP32: clamp to unsigned 32-bit [0, 2^32-1]
     * SEXT32: sign-extend low 32 bits to 64
     * ZEXT32: zero-extend low 32 bits */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_clip = (strcmp(op,"DCLIP32")==0 || strcmp(op,"2CLIP32")==0 ||
                   strcmp(op,"S2CLIP32")==0 || strcmp(op,"STACK2CLIP32")==0 ||
                   strcmp(op,"PAIRCLIP32")==0);
    int is_sext = (strcmp(op,"DSEXT32")==0 || strcmp(op,"2SEXT32")==0 ||
                   strcmp(op,"S2SEXT32")==0 || strcmp(op,"STACK2SEXT32")==0 ||
                   strcmp(op,"PAIRSEXT32")==0 || strcmp(op,"DSEXTD")==0 ||
                   strcmp(op,"2SEXTD")==0);
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x, y;
    const long u32max = 4294967295L; /* 2^32-1 */
    if (is_clip){
      x = a < 0 ? 0 : (a > u32max ? u32max : a);
      y = b < 0 ? 0 : (b > u32max ? u32max : b);
    } else if (is_sext){
      x = a & 0xFFFFFFFFL; if (x & 0x80000000L) x |= ~0xFFFFFFFFL;
      y = b & 0xFFFFFFFFL; if (y & 0x80000000L) y |= ~0xFFFFFFFFL;
    } else {
      x = a & 0xFFFFFFFFL;
      y = b & 0xFFFFFFFFL;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack signed clip: DCLIPS4 · DCLIPS8 · DCLIPS16
   * (signed dual of DCLIP4/8/16; pair of SCLIPS4/8/16 stack forms) */
  if (kw(&L->cur,"DCLIPS4")||kw(&L->cur,"2CLIPS4")||kw(&L->cur,"S2CLIPS4")||
      kw(&L->cur,"STACK2CLIPS4")||kw(&L->cur,"PAIRCLIPS4")||kw(&L->cur,"DCLIPSN")||
      kw(&L->cur,"2CLIPSN")||
      kw(&L->cur,"DCLIPS8")||kw(&L->cur,"2CLIPS8")||kw(&L->cur,"S2CLIPS8")||
      kw(&L->cur,"STACK2CLIPS8")||kw(&L->cur,"PAIRCLIPS8")||kw(&L->cur,"DCLIPSB")||
      kw(&L->cur,"2CLIPSB")||
      kw(&L->cur,"DCLIPS16")||kw(&L->cur,"2CLIPS16")||kw(&L->cur,"S2CLIPS16")||
      kw(&L->cur,"STACK2CLIPS16")||kw(&L->cur,"PAIRCLIPS16")||kw(&L->cur,"DCLIPSW")||
      kw(&L->cur,"2CLIPSW")){
    /* a b → signed-clamp pair at 4/8/16-bit bounds */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *q=op;*q;q++) if (*q>='a'&&*q<='z') *q=(char)(*q-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is4 = (strcmp(op,"DCLIPS4")==0 || strcmp(op,"2CLIPS4")==0 ||
               strcmp(op,"S2CLIPS4")==0 || strcmp(op,"STACK2CLIPS4")==0 ||
               strcmp(op,"PAIRCLIPS4")==0 || strcmp(op,"DCLIPSN")==0 ||
               strcmp(op,"2CLIPSN")==0);
    int is8 = (strcmp(op,"DCLIPS8")==0 || strcmp(op,"2CLIPS8")==0 ||
               strcmp(op,"S2CLIPS8")==0 || strcmp(op,"STACK2CLIPS8")==0 ||
               strcmp(op,"PAIRCLIPS8")==0 || strcmp(op,"DCLIPSB")==0 ||
               strcmp(op,"2CLIPSB")==0);
    long lo, hi;
    if (is4){ lo = -8L; hi = 7L; }
    else if (is8){ lo = -128L; hi = 127L; }
    else { lo = -32768L; hi = 32767L; }
    long x = a, y = b;
    if (x < lo) x = lo; if (x > hi) x = hi;
    if (y < lo) y = lo; if (y > hi) y = hi;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack signed clip32 + fixed rotate32: DCLIPS32 · DROL32 · DROR32
   * (complete dual-stack signed clip 4/8/16/32; complete fixed rotate 4/8/16/32) */
  if (kw(&L->cur,"DCLIPS32")||kw(&L->cur,"2CLIPS32")||kw(&L->cur,"S2CLIPS32")||
      kw(&L->cur,"STACK2CLIPS32")||kw(&L->cur,"PAIRCLIPS32")||kw(&L->cur,"DCLIPSL")||
      kw(&L->cur,"2CLIPSL")||kw(&L->cur,"DCLIPSD")||kw(&L->cur,"2CLIPSD")){
    /* a b → signed-clamp pair to INT32 bounds */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long lo = (-2147483647L - 1);
    long hi = 2147483647L;
    long x = a, y = b;
    if (x < lo) x = lo; if (x > hi) x = hi;
    if (y < lo) y = lo; if (y > hi) y = hi;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROL32")||kw(&L->cur,"2ROL32")||kw(&L->cur,"S2ROL32")||
      kw(&L->cur,"STACK2ROL32")||kw(&L->cur,"PAIRROL32")||kw(&L->cur,"DROTL32")||
      kw(&L->cur,"2ROTL32")||
      kw(&L->cur,"DROR32")||kw(&L->cur,"2ROR32")||kw(&L->cur,"S2ROR32")||
      kw(&L->cur,"STACK2ROR32")||kw(&L->cur,"PAIRROR32")||kw(&L->cur,"DROTR32")||
      kw(&L->cur,"2ROTR32")){
    /* a b c d → rot32(a,c) rot32(b,d); amount mod 32; result low 32 bits */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *q=op;*q;q++) if (*q>='a'&&*q<='z') *q=(char)(*q-'a'+'A');
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (c < 0) c = 0;
    if (d < 0) d = 0;
    int is_rol = (strstr(op,"ROL") != NULL || strstr(op,"ROTL") != NULL);
    unsigned mask = 0xFFFFFFFFu;
    unsigned bits = 32u;
    unsigned uc = (unsigned)c & (bits - 1u);
    unsigned ud = (unsigned)d & (bits - 1u);
    unsigned wa = (unsigned)a & mask, wb = (unsigned)b & mask;
    long x, y;
    if (is_rol){
      x = (uc == 0) ? (long)wa : (long)(((wa << uc) | (wa >> (bits - uc))) & mask);
      y = (ud == 0) ? (long)wb : (long)(((wb << ud) | (wb >> (bits - ud))) & mask);
    } else {
      x = (uc == 0) ? (long)wa : (long)(((wa >> uc) | (wa << (bits - uc))) & mask);
      y = (ud == 0) ? (long)wb : (long)(((wb >> ud) | (wb << (bits - ud))) & mask);
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-7 dual-stack pair bitwise + unary: DAND · DOR · DXOR · DNEG · DABS */
  if (kw(&L->cur,"DAND")||kw(&L->cur,"2AND")||kw(&L->cur,"S2AND")||
      kw(&L->cur,"STACK2AND")||kw(&L->cur,"PAIRAND")){
    /* a b c d → (a&c) (b&d) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = a & c;
    long y = b & d;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DOR")||kw(&L->cur,"2OR")||kw(&L->cur,"S2OR")||
      kw(&L->cur,"STACK2OR")||kw(&L->cur,"PAIROR")){
    /* a b c d → (a|c) (b|d) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = a | c;
    long y = b | d;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXOR")||kw(&L->cur,"2XOR")||kw(&L->cur,"S2XOR")||
      kw(&L->cur,"STACK2XOR")||kw(&L->cur,"PAIRXOR")){
    /* a b c d → (a^c) (b^d) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = a ^ c;
    long y = b ^ d;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNEG")||kw(&L->cur,"2NEG")||kw(&L->cur,"S2NEG")||
      kw(&L->cur,"STACK2NEG")||kw(&L->cur,"PAIRNEG")){
    /* a b → (-a) (-b) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp - 2] = -vm->stack[vm->sp - 2];
    vm->stack[vm->sp - 1] = -vm->stack[vm->sp - 1];
    long y = vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DABS")||kw(&L->cur,"2ABS")||kw(&L->cur,"S2ABS")||
      kw(&L->cur,"STACK2ABS")||kw(&L->cur,"PAIRABS")){
    /* a b → |a| |b| */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    vm->stack[vm->sp - 2] = a;
    vm->stack[vm->sp - 1] = b;
    var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-7 dual-stack ALU unary ext: DDBL · DHALF · DBSWAP (pair scale + endian) */
  if (kw(&L->cur,"DDBL")||kw(&L->cur,"2DBL")||kw(&L->cur,"S2DBL")||
      kw(&L->cur,"STACK2DBL")||kw(&L->cur,"PAIRDBL")||kw(&L->cur,"DDOUBLE")||
      kw(&L->cur,"2DOUBLE")||
      kw(&L->cur,"DHALF")||kw(&L->cur,"2HALF")||kw(&L->cur,"S2HALF")||
      kw(&L->cur,"STACK2HALF")||kw(&L->cur,"PAIRHALF")||kw(&L->cur,"DHALVE")||
      kw(&L->cur,"2HALVE")||
      kw(&L->cur,"DBSWAP")||kw(&L->cur,"2BSWAP")||kw(&L->cur,"S2BSWAP")||
      kw(&L->cur,"STACK2BSWAP")||kw(&L->cur,"PAIRBSWAP")||kw(&L->cur,"DBSWAP32")||
      kw(&L->cur,"2BSWAP32")){
    /* a b → f(a) f(b); DBL=*2; HALF=toward-zero /2; BSWAP=32-bit byte reverse */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_dbl = (strcmp(op,"DDBL")==0 || strcmp(op,"2DBL")==0 || strcmp(op,"S2DBL")==0 ||
                  strcmp(op,"STACK2DBL")==0 || strcmp(op,"PAIRDBL")==0 ||
                  strcmp(op,"DDOUBLE")==0 || strcmp(op,"2DOUBLE")==0);
    int is_half = (strcmp(op,"DHALF")==0 || strcmp(op,"2HALF")==0 || strcmp(op,"S2HALF")==0 ||
                   strcmp(op,"STACK2HALF")==0 || strcmp(op,"PAIRHALF")==0 ||
                   strcmp(op,"DHALVE")==0 || strcmp(op,"2HALVE")==0);
    long x, y;
    if (is_dbl){ x = a * 2; y = b * 2; }
    else if (is_half){ x = a / 2; y = b / 2; }
    else {
      unsigned int wa = (unsigned int)a, wb = (unsigned int)b;
      wa = ((wa & 0x000000FFu) << 24) | ((wa & 0x0000FF00u) << 8) |
           ((wa & 0x00FF0000u) >> 8) | ((wa & 0xFF000000u) >> 24);
      wb = ((wb & 0x000000FFu) << 24) | ((wb & 0x0000FF00u) << 8) |
           ((wb & 0x00FF0000u) >> 8) | ((wb & 0xFF000000u) >> 24);
      x = (long)wa; y = (long)wb;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-7 dual-stack ALU fused/high: DMADD · DMULHI */
  if (kw(&L->cur,"DMADD")||kw(&L->cur,"2MADD")||kw(&L->cur,"S2MADD")||
      kw(&L->cur,"STACK2MADD")||kw(&L->cur,"PAIRMADD")||kw(&L->cur,"DFMA")||
      kw(&L->cur,"2FMA")||kw(&L->cur,"DMULADD")||kw(&L->cur,"2MULADD")){
    /* a b c d e f → a*c+e  b*d+f  (pairwise fused multiply-add) */
    lex_next(L);
    if (vm->sp < 6){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long f = vm->stack[--vm->sp];
    long e = vm->stack[--vm->sp];
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = a * c + e;
    long y = b * d + f;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMULHI")||kw(&L->cur,"2MULHI")||kw(&L->cur,"S2MULHI")||
      kw(&L->cur,"STACK2MULHI")||kw(&L->cur,"PAIRMULHI")||kw(&L->cur,"DMULH")||
      kw(&L->cur,"2MULH")||kw(&L->cur,"DHMUL")||kw(&L->cur,"2HMUL")){
    /* a b c d → high64(a*c) high64(b*d) signed */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    __int128 px = (__int128)a * (__int128)c;
    __int128 py = (__int128)b * (__int128)d;
    long x = (long)(px >> 64);
    long y = (long)(py >> 64);
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-0 dual-stack unsigned mulhi + endian: DUMULHI · DBSWAP16 · DBSWAP64 */
  if (kw(&L->cur,"DUMULHI")||kw(&L->cur,"2UMULHI")||kw(&L->cur,"S2UMULHI")||
      kw(&L->cur,"STACK2UMULHI")||kw(&L->cur,"PAIRUMULHI")||kw(&L->cur,"DUMULH")||
      kw(&L->cur,"2UMULH")||kw(&L->cur,"S2UMULH")||kw(&L->cur,"PAIRUMULH")){
    /* a b c d → high64(ua*uc) high64(ub*ud) unsigned */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    unsigned __int128 px =
        (unsigned __int128)(unsigned long)a *
        (unsigned __int128)(unsigned long)c;
    unsigned __int128 py =
        (unsigned __int128)(unsigned long)b *
        (unsigned __int128)(unsigned long)d;
    long x = (long)(px >> 64);
    long y = (long)(py >> 64);
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DBSWAP16")||kw(&L->cur,"2BSWAP16")||kw(&L->cur,"S2BSWAP16")||
      kw(&L->cur,"STACK2BSWAP16")||kw(&L->cur,"PAIRBSWAP16")||
      kw(&L->cur,"DBSWAP64")||kw(&L->cur,"2BSWAP64")||kw(&L->cur,"S2BSWAP64")||
      kw(&L->cur,"STACK2BSWAP64")||kw(&L->cur,"PAIRBSWAP64")){
    /* a b → bswap16/64(a) bswap16/64(b) */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is16 = (strcmp(op,"DBSWAP16")==0 || strcmp(op,"2BSWAP16")==0 ||
                strcmp(op,"S2BSWAP16")==0 || strcmp(op,"STACK2BSWAP16")==0 ||
                strcmp(op,"PAIRBSWAP16")==0);
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x, y;
    if (is16){
      unsigned int wa = (unsigned int)a & 0xFFFFu;
      unsigned int wb = (unsigned int)b & 0xFFFFu;
      wa = ((wa & 0x00FFu) << 8) | ((wa & 0xFF00u) >> 8);
      wb = ((wb & 0x00FFu) << 8) | ((wb & 0xFF00u) >> 8);
      x = (long)wa; y = (long)wb;
    } else {
      unsigned long wa = (unsigned long)a, wb = (unsigned long)b;
      wa = ((wa & 0x00000000000000FFul) << 56) | ((wa & 0x000000000000FF00ul) << 40) |
           ((wa & 0x0000000000FF0000ul) << 24) | ((wa & 0x00000000FF000000ul) << 8) |
           ((wa & 0x000000FF00000000ul) >> 8) | ((wa & 0x0000FF0000000000ul) >> 24) |
           ((wa & 0x00FF000000000000ul) >> 40) | ((wa & 0xFF00000000000000ul) >> 56);
      wb = ((wb & 0x00000000000000FFul) << 56) | ((wb & 0x000000000000FF00ul) << 40) |
           ((wb & 0x0000000000FF0000ul) << 24) | ((wb & 0x00000000FF000000ul) << 8) |
           ((wb & 0x000000FF00000000ul) >> 8) | ((wb & 0x0000FF0000000000ul) >> 24) |
           ((wb & 0x00FF000000000000ul) >> 40) | ((wb & 0xFF00000000000000ul) >> 56);
      x = (long)wa; y = (long)wb;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-0 dual-stack overflow predicates: DADDOVF · DSUBOVF · DMULOVF (0/1) */
  if (kw(&L->cur,"DADDOVF")||kw(&L->cur,"2ADDOVF")||kw(&L->cur,"S2ADDOVF")||
      kw(&L->cur,"STACK2ADDOVF")||kw(&L->cur,"PAIRADDOVF")||kw(&L->cur,"DADDOVER")||
      kw(&L->cur,"2ADDOVER")||
      kw(&L->cur,"DSUBOVF")||kw(&L->cur,"2SUBOVF")||kw(&L->cur,"S2SUBOVF")||
      kw(&L->cur,"STACK2SUBOVF")||kw(&L->cur,"PAIRSUBOVF")||kw(&L->cur,"DSUBOVER")||
      kw(&L->cur,"2SUBOVER")||
      kw(&L->cur,"DMULOVF")||kw(&L->cur,"2MULOVF")||kw(&L->cur,"S2MULOVF")||
      kw(&L->cur,"STACK2MULOVF")||kw(&L->cur,"PAIRMULOVF")||kw(&L->cur,"DMULOVER")||
      kw(&L->cur,"2MULOVER")){
    /* a b c d → ovf(a⋆c) ovf(b⋆d) as 0/1 signed long overflow */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_add = (strcmp(op,"DADDOVF")==0 || strcmp(op,"2ADDOVF")==0 ||
                  strcmp(op,"S2ADDOVF")==0 || strcmp(op,"STACK2ADDOVF")==0 ||
                  strcmp(op,"PAIRADDOVF")==0 || strcmp(op,"DADDOVER")==0 ||
                  strcmp(op,"2ADDOVER")==0);
    int is_sub = (strcmp(op,"DSUBOVF")==0 || strcmp(op,"2SUBOVF")==0 ||
                  strcmp(op,"S2SUBOVF")==0 || strcmp(op,"STACK2SUBOVF")==0 ||
                  strcmp(op,"PAIRSUBOVF")==0 || strcmp(op,"DSUBOVER")==0 ||
                  strcmp(op,"2SUBOVER")==0);
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = 0, y = 0;
    if (is_add){
      if (c > 0 && a > LONG_MAX - c) x = 1;
      else if (c < 0 && a < LONG_MIN - c) x = 1;
      if (d > 0 && b > LONG_MAX - d) y = 1;
      else if (d < 0 && b < LONG_MIN - d) y = 1;
    } else if (is_sub){
      if (c > 0 && a < LONG_MIN + c) x = 1;
      else if (c < 0 && a > LONG_MAX + c) x = 1;
      if (d > 0 && b < LONG_MIN + d) y = 1;
      else if (d < 0 && b > LONG_MAX + d) y = 1;
    } else {
      if (a != 0 && c != 0){
        __int128 p = (__int128)a * (__int128)c;
        if (p > (__int128)LONG_MAX || p < (__int128)LONG_MIN) x = 1;
      }
      if (b != 0 && d != 0){
        __int128 p = (__int128)b * (__int128)d;
        if (p > (__int128)LONG_MAX || p < (__int128)LONG_MIN) y = 1;
      }
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-3 dual-stack imm overflow predicates: DADDOVFN · DSUBOVFN · DMULOVFN (dual of SADDOVFN) */
  if (kw(&L->cur,"DADDOVFN")||kw(&L->cur,"S2ADDOVFN")||kw(&L->cur,"STACK2ADDOVFN")||
      kw(&L->cur,"PAIRADDOVFN")||kw(&L->cur,"DADDOVERIMM")||kw(&L->cur,"PAIRADDOVERIMM")){
    /* a b + n → ovf(a+n) ovf(b+n) as 0/1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (n > 0 && a > LONG_MAX - n) x = 1;
    else if (n < 0 && a < LONG_MIN - n) x = 1;
    if (n > 0 && b > LONG_MAX - n) y = 1;
    else if (n < 0 && b < LONG_MIN - n) y = 1;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSUBOVFN")||kw(&L->cur,"S2SUBOVFN")||kw(&L->cur,"STACK2SUBOVFN")||
      kw(&L->cur,"PAIRSUBOVFN")||kw(&L->cur,"DSUBOVERIMM")||kw(&L->cur,"PAIRSUBOVERIMM")){
    /* a b + n → ovf(a-n) ovf(b-n) as 0/1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (n > 0 && a < LONG_MIN + n) x = 1;
    else if (n < 0 && a > LONG_MAX + n) x = 1;
    if (n > 0 && b < LONG_MIN + n) y = 1;
    else if (n < 0 && b > LONG_MAX + n) y = 1;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMULOVFN")||kw(&L->cur,"S2MULOVFN")||kw(&L->cur,"STACK2MULOVFN")||
      kw(&L->cur,"PAIRMULOVFN")||kw(&L->cur,"DMULOVERIMM")||kw(&L->cur,"PAIRMULOVERIMM")){
    /* a b + n → ovf(a*n) ovf(b*n) as 0/1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (a != 0 && n != 0){
      __int128 p = (__int128)a * (__int128)n;
      if (p > (__int128)LONG_MAX || p < (__int128)LONG_MIN) x = 1;
    }
    if (b != 0 && n != 0){
      __int128 p = (__int128)b * (__int128)n;
      if (p > (__int128)LONG_MAX || p < (__int128)LONG_MIN) y = 1;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-0 dual-stack unsigned overflow: DUADDOVF · DUSUBOVF · DUMULOVF (0/1) */
  if (kw(&L->cur,"DUADDOVF")||kw(&L->cur,"2UADDOVF")||kw(&L->cur,"S2UADDOVF")||
      kw(&L->cur,"STACK2UADDOVF")||kw(&L->cur,"PAIRUADDOVF")||kw(&L->cur,"DUADDOVER")||
      kw(&L->cur,"2UADDOVER")||
      kw(&L->cur,"DUSUBOVF")||kw(&L->cur,"2USUBOVF")||kw(&L->cur,"S2USUBOVF")||
      kw(&L->cur,"STACK2USUBOVF")||kw(&L->cur,"PAIRUSUBOVF")||kw(&L->cur,"DUSUBOVER")||
      kw(&L->cur,"2USUBOVER")||
      kw(&L->cur,"DUMULOVF")||kw(&L->cur,"2UMULOVF")||kw(&L->cur,"S2UMULOVF")||
      kw(&L->cur,"STACK2UMULOVF")||kw(&L->cur,"PAIRUMULOVF")||kw(&L->cur,"DUMULOVER")||
      kw(&L->cur,"2UMULOVER")){
    /* a b c d → uovf(a⋆c) uovf(b⋆d) as 0/1 (unsigned long wrap) */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_add = (strcmp(op,"DUADDOVF")==0 || strcmp(op,"2UADDOVF")==0 ||
                  strcmp(op,"S2UADDOVF")==0 || strcmp(op,"STACK2UADDOVF")==0 ||
                  strcmp(op,"PAIRUADDOVF")==0 || strcmp(op,"DUADDOVER")==0 ||
                  strcmp(op,"2UADDOVER")==0);
    int is_sub = (strcmp(op,"DUSUBOVF")==0 || strcmp(op,"2USUBOVF")==0 ||
                  strcmp(op,"S2USUBOVF")==0 || strcmp(op,"STACK2USUBOVF")==0 ||
                  strcmp(op,"PAIRUSUBOVF")==0 || strcmp(op,"DUSUBOVER")==0 ||
                  strcmp(op,"2USUBOVER")==0);
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
    unsigned long uc = (unsigned long)c, ud = (unsigned long)d;
    long x = 0, y = 0;
    if (is_add){
      /* carry out: sum wraps past ULONG_MAX */
      x = (ua + uc) < ua ? 1 : 0;
      y = (ub + ud) < ub ? 1 : 0;
    } else if (is_sub){
      /* borrow: minuend < subtrahend */
      x = (ua < uc) ? 1 : 0;
      y = (ub < ud) ? 1 : 0;
    } else {
      /* unsigned mul: product exceeds ULONG_MAX */
      if (ua != 0 && uc != 0){
        unsigned __int128 p = (unsigned __int128)ua * (unsigned __int128)uc;
        if (p > (unsigned __int128)ULONG_MAX) x = 1;
      }
      if (ub != 0 && ud != 0){
        unsigned __int128 p = (unsigned __int128)ub * (unsigned __int128)ud;
        if (p > (unsigned __int128)ULONG_MAX) y = 1;
      }
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-7 dual-stack unsigned div/mod: DUDIV · DUMOD */
  if (kw(&L->cur,"DUDIV")||kw(&L->cur,"2UDIV")||kw(&L->cur,"S2UDIV")||
      kw(&L->cur,"STACK2UDIV")||kw(&L->cur,"PAIRUDIV")||kw(&L->cur,"DUDIVIDE")||
      kw(&L->cur,"2UDIVIDE")||
      kw(&L->cur,"DUMOD")||kw(&L->cur,"2UMOD")||kw(&L->cur,"S2UMOD")||
      kw(&L->cur,"STACK2UMOD")||kw(&L->cur,"PAIRUMOD")||kw(&L->cur,"DUREM")||
      kw(&L->cur,"2UREM")){
    /* a b c d → unsigned a⋆c , b⋆d ; /0 or %0 → 0 */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_div = (strcmp(op,"DUDIV")==0 || strcmp(op,"2UDIV")==0 ||
                  strcmp(op,"S2UDIV")==0 || strcmp(op,"STACK2UDIV")==0 ||
                  strcmp(op,"PAIRUDIV")==0 || strcmp(op,"DUDIVIDE")==0 ||
                  strcmp(op,"2UDIVIDE")==0);
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
    unsigned long uc = (unsigned long)c, ud = (unsigned long)d;
    long x = 0, y = 0;
    if (is_div){
      if (uc != 0) x = (long)(ua / uc);
      if (ud != 0) y = (long)(ub / ud);
    } else {
      if (uc != 0) x = (long)(ua % uc);
      if (ud != 0) y = (long)(ub % ud);
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-0 dual-stack imm unsigned div/mod: DUDIVN · DUMODN (imm dual of DUDIV/DUMOD; of SUDIVN) */
  if (kw(&L->cur,"DUDIVN")||kw(&L->cur,"2UDIVN")||kw(&L->cur,"S2UDIVN")||
      kw(&L->cur,"STACK2UDIVN")||kw(&L->cur,"PAIRUDIVN")||kw(&L->cur,"DUDIVIMM")||
      kw(&L->cur,"2UDIVIMM")||kw(&L->cur,"PAIRUDIVIMM")||kw(&L->cur,"DUQUOTN")){
    /* a b + n → unsigned (a/n) (b/n); n==0 → 0,0 soft */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (n != 0){
      x = (long)((unsigned long)a / (unsigned long)n);
      y = (long)((unsigned long)b / (unsigned long)n);
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DUMODN")||kw(&L->cur,"2UMODN")||kw(&L->cur,"S2UMODN")||
      kw(&L->cur,"STACK2UMODN")||kw(&L->cur,"PAIRUMODN")||kw(&L->cur,"DUMODIMM")||
      kw(&L->cur,"2UMODIMM")||kw(&L->cur,"PAIRUMODIMM")||kw(&L->cur,"DUREMN")){
    /* a b + n → unsigned (a%n) (b%n); n==0 → 0,0 soft */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (n != 0){
      x = (long)((unsigned long)a % (unsigned long)n);
      y = (long)((unsigned long)b % (unsigned long)n);
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-7 dual-stack unsigned min/max: DUMIN · DUMAX */
  if (kw(&L->cur,"DUMIN")||kw(&L->cur,"2UMIN")||kw(&L->cur,"S2UMIN")||
      kw(&L->cur,"STACK2UMIN")||kw(&L->cur,"PAIRUMIN")||kw(&L->cur,"DUMINU")||
      kw(&L->cur,"DUMAX")||kw(&L->cur,"2UMAX")||kw(&L->cur,"S2UMAX")||
      kw(&L->cur,"STACK2UMAX")||kw(&L->cur,"PAIRUMAX")||kw(&L->cur,"DUMAXU")){
    /* a b c d → unsigned min/max(a,c) min/max(b,d) */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_min = (strcmp(op,"DUMIN")==0 || strcmp(op,"2UMIN")==0 ||
                  strcmp(op,"S2UMIN")==0 || strcmp(op,"STACK2UMIN")==0 ||
                  strcmp(op,"PAIRUMIN")==0 || strcmp(op,"DUMINU")==0);
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
    unsigned long uc = (unsigned long)c, ud = (unsigned long)d;
    long x, y;
    if (is_min){
      x = (long)(ua < uc ? ua : uc);
      y = (long)(ub < ud ? ub : ud);
    } else {
      x = (long)(ua > uc ? ua : uc);
      y = (long)(ub > ud ? ub : ud);
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-7 dual-stack unsigned compare: DULT · DULE · DUGT · DUGE (0/1) */
  if (kw(&L->cur,"DULT")||kw(&L->cur,"2ULT")||kw(&L->cur,"S2ULT")||
      kw(&L->cur,"STACK2ULT")||kw(&L->cur,"PAIRULT")||
      kw(&L->cur,"DULE")||kw(&L->cur,"2ULE")||kw(&L->cur,"S2ULE")||
      kw(&L->cur,"STACK2ULE")||kw(&L->cur,"PAIRULE")||
      kw(&L->cur,"DUGT")||kw(&L->cur,"2UGT")||kw(&L->cur,"S2UGT")||
      kw(&L->cur,"STACK2UGT")||kw(&L->cur,"PAIRUGT")||
      kw(&L->cur,"DUGE")||kw(&L->cur,"2UGE")||kw(&L->cur,"S2UGE")||
      kw(&L->cur,"STACK2UGE")||kw(&L->cur,"PAIRUGE")){
    /* a b c d → unsigned (a ? c) (b ? d) as 0/1 */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
    unsigned long uc = (unsigned long)c, ud = (unsigned long)d;
    long x = 0, y = 0;
    int is_lt = (strcmp(op,"DULT")==0 || strcmp(op,"2ULT")==0 || strcmp(op,"S2ULT")==0 ||
                 strcmp(op,"STACK2ULT")==0 || strcmp(op,"PAIRULT")==0);
    int is_le = (strcmp(op,"DULE")==0 || strcmp(op,"2ULE")==0 || strcmp(op,"S2ULE")==0 ||
                 strcmp(op,"STACK2ULE")==0 || strcmp(op,"PAIRULE")==0);
    int is_gt = (strcmp(op,"DUGT")==0 || strcmp(op,"2UGT")==0 || strcmp(op,"S2UGT")==0 ||
                 strcmp(op,"STACK2UGT")==0 || strcmp(op,"PAIRUGT")==0);
    /* else DUGE family */
    if (is_lt){ x = (ua < uc) ? 1 : 0; y = (ub < ud) ? 1 : 0; }
    else if (is_le){ x = (ua <= uc) ? 1 : 0; y = (ub <= ud) ? 1 : 0; }
    else if (is_gt){ x = (ua > uc) ? 1 : 0; y = (ub > ud) ? 1 : 0; }
    else { x = (ua >= uc) ? 1 : 0; y = (ub >= ud) ? 1 : 0; }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-7 dual-stack multiword: DADDC · DSUBB (pair + cin/bin from CARRY/BORROW) */
  if (kw(&L->cur,"DADDC")||kw(&L->cur,"2ADDC")||kw(&L->cur,"S2ADDC")||
      kw(&L->cur,"STACK2ADDC")||kw(&L->cur,"PAIRADDC")||kw(&L->cur,"DADC")||
      kw(&L->cur,"2ADC")||
      kw(&L->cur,"DSUBB")||kw(&L->cur,"2SUBB")||kw(&L->cur,"S2SUBB")||
      kw(&L->cur,"STACK2SUBB")||kw(&L->cur,"PAIRSUBB")||kw(&L->cur,"DSBB")||
      kw(&L->cur,"2SBB")){
    /* a b c d → f(a,c) f(b,d)
     * DADDC: unsigned wrap add with cin from CARRY (both lanes); CARRY = cout_x|cout_y
     * DSUBB: unsigned wrap sub with bin from BORROW/CARRY; BORROW=CARRY = bout_x|bout_y */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_sub = (strcmp(op,"DSUBB")==0 || strcmp(op,"2SUBB")==0 ||
                  strcmp(op,"S2SUBB")==0 || strcmp(op,"STACK2SUBB")==0 ||
                  strcmp(op,"PAIRSUBB")==0 || strcmp(op,"DSBB")==0 ||
                  strcmp(op,"2SBB")==0);
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long cin = 0;
    {
      Var *vc = var_get(vm, is_sub ? "BORROW" : "CARRY", 0);
      if (!vc && is_sub) vc = var_get(vm, "CARRY", 0);
      if (vc && vc->val) cin = 1;
    }
    unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
    unsigned long uc = (unsigned long)c, ud = (unsigned long)d;
    unsigned long uin = (unsigned long)cin;
    long x, y;
    int flag = 0;
    if (!is_sub){
      unsigned long s0 = ua + uc;
      int c1 = (s0 < ua) ? 1 : 0;
      unsigned long sum0 = s0 + uin;
      int c2 = (sum0 < s0) ? 1 : 0;
      unsigned long s1 = ub + ud;
      int c3 = (s1 < ub) ? 1 : 0;
      unsigned long sum1 = s1 + uin;
      int c4 = (sum1 < s1) ? 1 : 0;
      flag = c1 | c2 | c3 | c4;
      x = (long)sum0; y = (long)sum1;
      var_set_num(vm,"CARRY",flag); var_set_num(vm,"CY",flag);
    } else {
      int b1 = (ua < uc) ? 1 : 0;
      unsigned long d0 = ua - uc;
      int b2 = (d0 < uin) ? 1 : 0;
      unsigned long diff0 = d0 - uin;
      int b3 = (ub < ud) ? 1 : 0;
      unsigned long d1 = ub - ud;
      int b4 = (d1 < uin) ? 1 : 0;
      unsigned long diff1 = d1 - uin;
      flag = b1 | b2 | b3 | b4;
      x = (long)diff0; y = (long)diff1;
      var_set_num(vm,"BORROW",flag); var_set_num(vm,"BW",flag);
      var_set_num(vm,"CARRY",flag);
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 dual-stack multiword imm: DADDCN · DSUBBN (imm dual of DADDC/DSUBB) */
  if (kw(&L->cur,"DADDCN")||kw(&L->cur,"2ADDCN")||kw(&L->cur,"S2ADDCN")||
      kw(&L->cur,"STACK2ADDCN")||kw(&L->cur,"PAIRADDCN")||kw(&L->cur,"DADCN")||
      kw(&L->cur,"2ADCN")||kw(&L->cur,"DADDCIMM")||kw(&L->cur,"PAIRADDCIMM")){
    /* a b + n → (a+n+cin) (b+n+cin); CARRY = any cout */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long cin = 0;
    {
      Var *vc = var_get(vm, "CARRY", 0);
      if (vc && vc->val) cin = 1;
    }
    unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
    unsigned long un = (unsigned long)n, uin = (unsigned long)cin;
    unsigned long s0 = ua + un;
    int c1 = (s0 < ua) ? 1 : 0;
    unsigned long sum0 = s0 + uin;
    int c2 = (sum0 < s0) ? 1 : 0;
    unsigned long s1 = ub + un;
    int c3 = (s1 < ub) ? 1 : 0;
    unsigned long sum1 = s1 + uin;
    int c4 = (sum1 < s1) ? 1 : 0;
    int flag = c1 | c2 | c3 | c4;
    long x = (long)sum0, y = (long)sum1;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"CARRY",flag); var_set_num(vm,"CY",flag);
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSUBBN")||kw(&L->cur,"2SUBBN")||kw(&L->cur,"S2SUBBN")||
      kw(&L->cur,"STACK2SUBBN")||kw(&L->cur,"PAIRSUBBN")||kw(&L->cur,"DSBBN")||
      kw(&L->cur,"2SBBN")||kw(&L->cur,"DSUBBIMM")||kw(&L->cur,"PAIRSUBBIMM")){
    /* a b + n → (a−n−bin) (b−n−bin); BORROW=CARRY = any bout */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long cin = 0;
    {
      Var *vc = var_get(vm, "BORROW", 0);
      if (!vc) vc = var_get(vm, "CARRY", 0);
      if (vc && vc->val) cin = 1;
    }
    unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
    unsigned long un = (unsigned long)n, uin = (unsigned long)cin;
    int b1 = (ua < un) ? 1 : 0;
    unsigned long d0 = ua - un;
    int b2 = (d0 < uin) ? 1 : 0;
    unsigned long diff0 = d0 - uin;
    int b3 = (ub < un) ? 1 : 0;
    unsigned long d1 = ub - un;
    int b4 = (d1 < uin) ? 1 : 0;
    unsigned long diff1 = d1 - uin;
    int flag = b1 | b2 | b3 | b4;
    long x = (long)diff0, y = (long)diff1;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"BORROW",flag); var_set_num(vm,"BW",flag);
    var_set_num(vm,"CARRY",flag);
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-7 dual-stack multiword stack-cin: DADDC2 · DSUBB2 (per-lane cin/bin) */
  if (kw(&L->cur,"DADDC2")||kw(&L->cur,"2ADDC2")||kw(&L->cur,"S2ADDC2")||
      kw(&L->cur,"STACK2ADDC2")||kw(&L->cur,"PAIRADDC2")||kw(&L->cur,"DADC2")||
      kw(&L->cur,"2ADC2")||kw(&L->cur,"DADDCST")||kw(&L->cur,"2ADDCST")||
      kw(&L->cur,"DSUBB2")||kw(&L->cur,"2SUBB2")||kw(&L->cur,"S2SUBB2")||
      kw(&L->cur,"STACK2SUBB2")||kw(&L->cur,"PAIRSUBB2")||kw(&L->cur,"DSBB2")||
      kw(&L->cur,"2SBB2")||kw(&L->cur,"DSUBBST")||kw(&L->cur,"2SUBBST")){
    /* a b c d ca cb → f(a,c,ca) f(b,d,cb)
     * DADDC2: unsigned wrap add with per-lane cin (0/1 from ca/cb); CARRY = cout_x|cout_y
     * DSUBB2: unsigned wrap sub with per-lane bin; BORROW=CARRY = bout_x|bout_y */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_sub = (strcmp(op,"DSUBB2")==0 || strcmp(op,"2SUBB2")==0 ||
                  strcmp(op,"S2SUBB2")==0 || strcmp(op,"STACK2SUBB2")==0 ||
                  strcmp(op,"PAIRSUBB2")==0 || strcmp(op,"DSBB2")==0 ||
                  strcmp(op,"2SBB2")==0 || strcmp(op,"DSUBBST")==0 ||
                  strcmp(op,"2SUBBST")==0);
    lex_next(L);
    if (vm->sp < 6){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long cb = vm->stack[--vm->sp];
    long ca = vm->stack[--vm->sp];
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
    unsigned long uc = (unsigned long)c, ud = (unsigned long)d;
    unsigned long uina = ca ? 1ul : 0ul;
    unsigned long uinb = cb ? 1ul : 0ul;
    long x, y;
    int flag = 0;
    if (!is_sub){
      unsigned long s0 = ua + uc;
      int c1 = (s0 < ua) ? 1 : 0;
      unsigned long sum0 = s0 + uina;
      int c2 = (sum0 < s0) ? 1 : 0;
      unsigned long s1 = ub + ud;
      int c3 = (s1 < ub) ? 1 : 0;
      unsigned long sum1 = s1 + uinb;
      int c4 = (sum1 < s1) ? 1 : 0;
      flag = c1 | c2 | c3 | c4;
      x = (long)sum0; y = (long)sum1;
      var_set_num(vm,"CARRY",flag); var_set_num(vm,"CY",flag);
    } else {
      int b1 = (ua < uc) ? 1 : 0;
      unsigned long d0 = ua - uc;
      int b2 = (d0 < uina) ? 1 : 0;
      unsigned long diff0 = d0 - uina;
      int b3 = (ub < ud) ? 1 : 0;
      unsigned long d1 = ub - ud;
      int b4 = (d1 < uinb) ? 1 : 0;
      unsigned long diff1 = d1 - uinb;
      flag = b1 | b2 | b3 | b4;
      x = (long)diff0; y = (long)diff1;
      var_set_num(vm,"BORROW",flag); var_set_num(vm,"BW",flag);
      var_set_num(vm,"CARRY",flag);
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-7 dual-stack multiword shift-through-carry: DSHLC · DSHRC */
  if (kw(&L->cur,"DSHLC")||kw(&L->cur,"2SHLC")||kw(&L->cur,"S2SHLC")||
      kw(&L->cur,"STACK2SHLC")||kw(&L->cur,"PAIRSHLC")||kw(&L->cur,"DSHLCY")||
      kw(&L->cur,"2SHLCY")||
      kw(&L->cur,"DSHRC")||kw(&L->cur,"2SHRC")||kw(&L->cur,"S2SHRC")||
      kw(&L->cur,"STACK2SHRC")||kw(&L->cur,"PAIRSHRC")||kw(&L->cur,"DSHRCY")||
      kw(&L->cur,"2SHRCY")){
    /* a b ca cb → shift-1 with per-lane cin bit; CARRY = any cout
     * DSHLC: x=(a<<1)|cin_lsb; cout=old MSB
     * DSHRC: x=(a>>1)|(cin_msb<<MSB); cout=old LSB */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_left = (strcmp(op,"DSHLC")==0 || strcmp(op,"2SHLC")==0 ||
                   strcmp(op,"S2SHLC")==0 || strcmp(op,"STACK2SHLC")==0 ||
                   strcmp(op,"PAIRSHLC")==0 || strcmp(op,"DSHLCY")==0 ||
                   strcmp(op,"2SHLCY")==0);
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long cb = vm->stack[--vm->sp];
    long ca = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
    unsigned long msb = 1ul << (sizeof(unsigned long) * 8 - 1);
    long x, y;
    int flag = 0;
    if (is_left){
      int c0 = (ua & msb) ? 1 : 0;
      int c1 = (ub & msb) ? 1 : 0;
      flag = c0 | c1;
      x = (long)((ua << 1) | (ca ? 1ul : 0ul));
      y = (long)((ub << 1) | (cb ? 1ul : 0ul));
    } else {
      int c0 = (ua & 1ul) ? 1 : 0;
      int c1 = (ub & 1ul) ? 1 : 0;
      flag = c0 | c1;
      x = (long)((ua >> 1) | (ca ? msb : 0ul));
      y = (long)((ub >> 1) | (cb ? msb : 0ul));
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"CARRY",flag); var_set_num(vm,"CY",flag);
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-5 dual-stack shift-through-CARRY flag: DSHLCC · DSHRCC (cin from CARRY; dual of SSHLC) */
  if (kw(&L->cur,"DSHLCC")||kw(&L->cur,"2SHLCC")||kw(&L->cur,"S2SHLCC")||
      kw(&L->cur,"STACK2SHLCC")||kw(&L->cur,"PAIRSHLCC")||kw(&L->cur,"DSHLCF")||
      kw(&L->cur,"2SHLCF")||kw(&L->cur,"PAIRSHLCF")){
    /* a b → (a<<1)|cin  (b<<1)|cin; cin=CARRY; CARRY = msb_a|msb_b */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long cin = 0;
    {
      Var *vc = var_get(vm, "CARRY", 0);
      if (vc && vc->val) cin = 1;
    }
    unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
    unsigned long msb = 1ul << (sizeof(unsigned long) * 8 - 1);
    int flag = ((ua & msb) ? 1 : 0) | ((ub & msb) ? 1 : 0);
    long x = (long)((ua << 1) | (cin ? 1ul : 0ul));
    long y = (long)((ub << 1) | (cin ? 1ul : 0ul));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"CARRY",flag); var_set_num(vm,"CY",flag);
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSHRCC")||kw(&L->cur,"2SHRCC")||kw(&L->cur,"S2SHRCC")||
      kw(&L->cur,"STACK2SHRCC")||kw(&L->cur,"PAIRSHRCC")||kw(&L->cur,"DSHRCF")||
      kw(&L->cur,"2SHRCF")||kw(&L->cur,"PAIRSHRCF")){
    /* a b → (a>>1)|(cin<<MSB)  (b>>1)|(cin<<MSB); cin=CARRY; CARRY = lsb_a|lsb_b */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long cin = 0;
    {
      Var *vc = var_get(vm, "CARRY", 0);
      if (vc && vc->val) cin = 1;
    }
    unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
    unsigned long msb = 1ul << (sizeof(unsigned long) * 8 - 1);
    int flag = ((ua & 1ul) ? 1 : 0) | ((ub & 1ul) ? 1 : 0);
    long x = (long)((ua >> 1) | (cin ? msb : 0ul));
    long y = (long)((ub >> 1) | (cin ? msb : 0ul));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"CARRY",flag); var_set_num(vm,"CY",flag);
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-5 dual-stack imm rotate-through-CARRY: DSHLCCN · DSHRCCN (dual of SSHLCN/SSHRCN) */
  if (kw(&L->cur,"DSHLCCN")||kw(&L->cur,"2SHLCCN")||kw(&L->cur,"S2SHLCCN")||
      kw(&L->cur,"STACK2SHLCCN")||kw(&L->cur,"PAIRSHLCCN")||kw(&L->cur,"DSHLCFN")||
      kw(&L->cur,"2SHLCFN")||kw(&L->cur,"PAIRSHLCFN")||kw(&L->cur,"DRCLN")||
      kw(&L->cur,"2RCLN")||kw(&L->cur,"PAIRRCLN")){
    /* a b + n → RCL(a,n) RCL(b,n); shared cin, CARRY = OR of final couts */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long ub = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long msb = 1ul << (sizeof(unsigned long) * 8 - 1);
    long cin = 0;
    {
      Var *vc = var_get(vm, "CARRY", 0);
      if (vc && vc->val) cin = 1;
    }
    long cina = cin, cinb = cin;
    for (long i = 0; i < n; i++){
      int ca = (ua & msb) ? 1 : 0;
      int cb = (ub & msb) ? 1 : 0;
      ua = (ua << 1) | (cina ? 1ul : 0ul);
      ub = (ub << 1) | (cinb ? 1ul : 0ul);
      cina = ca; cinb = cb;
    }
    int flag = (cina ? 1 : 0) | (cinb ? 1 : 0);
    long x = (long)ua, y = (long)ub;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"CARRY",flag); var_set_num(vm,"CY",flag);
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSHRCCN")||kw(&L->cur,"2SHRCCN")||kw(&L->cur,"S2SHRCCN")||
      kw(&L->cur,"STACK2SHRCCN")||kw(&L->cur,"PAIRSHRCCN")||kw(&L->cur,"DSHRCFN")||
      kw(&L->cur,"2SHRCFN")||kw(&L->cur,"PAIRSHRCFN")||kw(&L->cur,"DRCRN")||
      kw(&L->cur,"2RCRN")||kw(&L->cur,"PAIRRCRN")){
    /* a b + n → RCR(a,n) RCR(b,n); shared cin, CARRY = OR of final couts */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long ub = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long msb = 1ul << (sizeof(unsigned long) * 8 - 1);
    long cin = 0;
    {
      Var *vc = var_get(vm, "CARRY", 0);
      if (vc && vc->val) cin = 1;
    }
    long cina = cin, cinb = cin;
    for (long i = 0; i < n; i++){
      int ca = (ua & 1ul) ? 1 : 0;
      int cb = (ub & 1ul) ? 1 : 0;
      ua = (ua >> 1) | (cina ? msb : 0ul);
      ub = (ub >> 1) | (cinb ? msb : 0ul);
      cina = ca; cinb = cb;
    }
    int flag = (cina ? 1 : 0) | (cinb ? 1 : 0);
    long x = (long)ua, y = (long)ub;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"CARRY",flag); var_set_num(vm,"CY",flag);
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-7 dual-stack multiword negate-via-complement+cin: DNEGC2 */
  if (kw(&L->cur,"DNEGC2")||kw(&L->cur,"2NEGC2")||kw(&L->cur,"S2NEGC2")||
      kw(&L->cur,"STACK2NEGC2")||kw(&L->cur,"PAIRNEGC2")||kw(&L->cur,"DNEGC")||
      kw(&L->cur,"2NEGC")||kw(&L->cur,"S2NEGC")||kw(&L->cur,"PAIRNEGC")||
      kw(&L->cur,"DCOMADC")||kw(&L->cur,"2COMADC")||kw(&L->cur,"DNEGADC")||
      kw(&L->cur,"2NEGADC")){
    /* a b ca cb → (~a)+ca  (~b)+cb  (unsigned wrap); CARRY = any cout
     * multiword two's-complement: first limb ca=1, next limbs ca=prev cout */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long cb = vm->stack[--vm->sp];
    long ca = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
    unsigned long ta = ~ua, tb = ~ub;
    unsigned long uina = ca ? 1ul : 0ul;
    unsigned long uinb = cb ? 1ul : 0ul;
    unsigned long sa = ta + uina;
    unsigned long sb = tb + uinb;
    int c0 = (uina && sa < ta) ? 1 : 0;
    int c1 = (uinb && sb < tb) ? 1 : 0;
    int flag = c0 | c1;
    vm->stack[vm->sp++] = (long)sa;
    vm->stack[vm->sp++] = (long)sb;
    var_set_num(vm,"CARRY",flag); var_set_num(vm,"CY",flag);
    var_set_num(vm,"LAST_N",(long)sb); vm->last_n=(long)sb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-7 dual-stack shared-CARRY negate: DNEGCC (pair of SNEGC; cin from CARRY) */
  if (kw(&L->cur,"DNEGCC")||kw(&L->cur,"2NEGCC")||kw(&L->cur,"S2NEGCC")||
      kw(&L->cur,"STACK2NEGCC")||kw(&L->cur,"PAIRNEGCC")||kw(&L->cur,"DCOMADCC")||
      kw(&L->cur,"2COMADCC")||kw(&L->cur,"PAIRCOMADC")||kw(&L->cur,"DNEGCF")){
    /* a b → (~a)+cin  (~b)+cin; cin=CARRY; CARRY = cout_a|cout_b */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long cin = 0;
    {
      Var *vc = var_get(vm, "CARRY", 0);
      if (vc && vc->val) cin = 1;
    }
    unsigned long uin = cin ? 1ul : 0ul;
    unsigned long ta = ~(unsigned long)a, tb = ~(unsigned long)b;
    unsigned long sa = ta + uin, sb = tb + uin;
    int c0 = (uin && sa < ta) ? 1 : 0;
    int c1 = (uin && sb < tb) ? 1 : 0;
    int flag = c0 | c1;
    vm->stack[vm->sp - 2] = (long)sa;
    vm->stack[vm->sp - 1] = (long)sb;
    var_set_num(vm,"CARRY",flag); var_set_num(vm,"CY",flag);
    var_set_num(vm,"LAST_N",(long)sb); vm->last_n=(long)sb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-0 dual-stack imm unsigned min/max: DUMINN · DUMAXN (dual of SUMINN/SUMAXN; imm of DUMIN/DUMAX) */
  if (kw(&L->cur,"DUMINN")||kw(&L->cur,"2UMINN")||kw(&L->cur,"S2UMINN")||
      kw(&L->cur,"STACK2UMINN")||kw(&L->cur,"PAIRUMINN")||kw(&L->cur,"DUMINIMM")||
      kw(&L->cur,"2UMINIMM")||kw(&L->cur,"PAIRUMINIMM")){
    /* a b + n → unsigned min(a,n) min(b,n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long un = (unsigned long)n;
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long ub = (unsigned long)vm->stack[vm->sp - 1];
    long x = (long)(ua < un ? ua : un);
    long y = (long)(ub < un ? ub : un);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DUMAXN")||kw(&L->cur,"2UMAXN")||kw(&L->cur,"S2UMAXN")||
      kw(&L->cur,"STACK2UMAXN")||kw(&L->cur,"PAIRUMAXN")||kw(&L->cur,"DUMAXIMM")||
      kw(&L->cur,"2UMAXIMM")||kw(&L->cur,"PAIRUMAXIMM")){
    /* a b + n → unsigned max(a,n) max(b,n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long un = (unsigned long)n;
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long ub = (unsigned long)vm->stack[vm->sp - 1];
    long x = (long)(ua > un ? ua : un);
    long y = (long)(ub > un ? ub : un);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-7 dual-stack imm cin negate: DNEGCN n (shared cin from n 0/1) */
  if (kw(&L->cur,"DNEGCN")||kw(&L->cur,"2NEGCN")||kw(&L->cur,"S2NEGCN")||
      kw(&L->cur,"STACK2NEGCN")||kw(&L->cur,"PAIRNEGCN")||kw(&L->cur,"DCOMADCN")||
      kw(&L->cur,"2COMADCN")||kw(&L->cur,"DNEGCIMM")||kw(&L->cur,"PAIRNEGCIMM")){
    /* a b + n → (~a)+(n?1:0) (~b)+(n?1:0); CARRY = OR cout */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    unsigned long uin = n ? 1ul : 0ul;
    unsigned long ta = ~(unsigned long)a, tb = ~(unsigned long)b;
    unsigned long sa = ta + uin, sb = tb + uin;
    int c0 = (uin && sa < ta) ? 1 : 0;
    int c1 = (uin && sb < tb) ? 1 : 0;
    int flag = c0 | c1;
    vm->stack[vm->sp - 2] = (long)sa;
    vm->stack[vm->sp - 1] = (long)sb;
    var_set_num(vm,"CARRY",flag); var_set_num(vm,"CY",flag);
    var_set_num(vm,"LAST_N",(long)sb); vm->last_n=(long)sb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-3 dual-stack imm unsigned compare: DULTN · DUGTN · DULEN · DUGEN (dual of SULTN) */
  if (kw(&L->cur,"DULTN")||kw(&L->cur,"2ULTN")||kw(&L->cur,"S2ULTN")||
      kw(&L->cur,"STACK2ULTN")||kw(&L->cur,"PAIRULTN")||kw(&L->cur,"DULTIMM")||
      kw(&L->cur,"2ULTIMM")||kw(&L->cur,"PAIRULTIMM")){
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long un = (unsigned long)n;
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long ub = (unsigned long)vm->stack[vm->sp - 1];
    long x = (ua < un) ? 1 : 0;
    long y = (ub < un) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DUGTN")||kw(&L->cur,"2UGTN")||kw(&L->cur,"S2UGTN")||
      kw(&L->cur,"STACK2UGTN")||kw(&L->cur,"PAIRUGTN")||kw(&L->cur,"DUGTIMM")||
      kw(&L->cur,"2UGTIMM")||kw(&L->cur,"PAIRUGTIMM")){
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long un = (unsigned long)n;
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long ub = (unsigned long)vm->stack[vm->sp - 1];
    long x = (ua > un) ? 1 : 0;
    long y = (ub > un) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DULEN")||kw(&L->cur,"2ULEN")||kw(&L->cur,"S2ULEN")||
      kw(&L->cur,"STACK2ULEN")||kw(&L->cur,"PAIRULEN")||kw(&L->cur,"DULEQN")||
      kw(&L->cur,"2ULEQN")||kw(&L->cur,"DULEIMM")||kw(&L->cur,"PAIRULEQN")){
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long un = (unsigned long)n;
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long ub = (unsigned long)vm->stack[vm->sp - 1];
    long x = (ua <= un) ? 1 : 0;
    long y = (ub <= un) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DUGEN")||kw(&L->cur,"2UGEN")||kw(&L->cur,"S2UGEN")||
      kw(&L->cur,"STACK2UGEN")||kw(&L->cur,"PAIRUGEN")||kw(&L->cur,"DUGEQN")||
      kw(&L->cur,"2UGEQN")||kw(&L->cur,"DUGEIMM")||kw(&L->cur,"PAIRUGEQN")){
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long un = (unsigned long)n;
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long ub = (unsigned long)vm->stack[vm->sp - 1];
    long x = (ua >= un) ? 1 : 0;
    long y = (ub >= un) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-1 dual-stack pair compare: DEQ · DNE · DLT · DLE · DGT · DGE (0/1 predicates) */
  if (kw(&L->cur,"DEQ")||kw(&L->cur,"2EQ")||kw(&L->cur,"S2EQ")||
      kw(&L->cur,"STACK2EQ")||kw(&L->cur,"PAIREQ")||
      kw(&L->cur,"DNE")||kw(&L->cur,"2NE")||kw(&L->cur,"S2NE")||
      kw(&L->cur,"STACK2NE")||kw(&L->cur,"PAIRNE")||
      kw(&L->cur,"DLT")||kw(&L->cur,"2LT")||kw(&L->cur,"S2LT")||
      kw(&L->cur,"STACK2LT")||kw(&L->cur,"PAIRLT")||
      kw(&L->cur,"DLE")||kw(&L->cur,"2LE")||kw(&L->cur,"S2LE")||
      kw(&L->cur,"STACK2LE")||kw(&L->cur,"PAIRLE")||
      kw(&L->cur,"DGT")||kw(&L->cur,"2GT")||kw(&L->cur,"S2GT")||
      kw(&L->cur,"STACK2GT")||kw(&L->cur,"PAIRGT")||
      kw(&L->cur,"DGE")||kw(&L->cur,"2GE")||kw(&L->cur,"S2GE")||
      kw(&L->cur,"STACK2GE")||kw(&L->cur,"PAIRGE")){
    /* a b c d → (a ? c) (b ? d) as 0/1 */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = 0, y = 0;
    int is_eq = (strcmp(op,"DEQ")==0 || strcmp(op,"2EQ")==0 || strcmp(op,"S2EQ")==0 ||
                 strcmp(op,"STACK2EQ")==0 || strcmp(op,"PAIREQ")==0);
    int is_ne = (strcmp(op,"DNE")==0 || strcmp(op,"2NE")==0 || strcmp(op,"S2NE")==0 ||
                 strcmp(op,"STACK2NE")==0 || strcmp(op,"PAIRNE")==0);
    int is_lt = (strcmp(op,"DLT")==0 || strcmp(op,"2LT")==0 || strcmp(op,"S2LT")==0 ||
                 strcmp(op,"STACK2LT")==0 || strcmp(op,"PAIRLT")==0);
    int is_le = (strcmp(op,"DLE")==0 || strcmp(op,"2LE")==0 || strcmp(op,"S2LE")==0 ||
                 strcmp(op,"STACK2LE")==0 || strcmp(op,"PAIRLE")==0);
    int is_gt = (strcmp(op,"DGT")==0 || strcmp(op,"2GT")==0 || strcmp(op,"S2GT")==0 ||
                 strcmp(op,"STACK2GT")==0 || strcmp(op,"PAIRGT")==0);
    /* else DGE family */
    if (is_eq){ x = (a == c) ? 1 : 0; y = (b == d) ? 1 : 0; }
    else if (is_ne){ x = (a != c) ? 1 : 0; y = (b != d) ? 1 : 0; }
    else if (is_lt){ x = (a < c) ? 1 : 0; y = (b < d) ? 1 : 0; }
    else if (is_le){ x = (a <= c) ? 1 : 0; y = (b <= d) ? 1 : 0; }
    else if (is_gt){ x = (a > c) ? 1 : 0; y = (b > d) ? 1 : 0; }
    else { x = (a >= c) ? 1 : 0; y = (b >= d) ? 1 : 0; }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-1 dual-stack boolean control: DLAND · DLOR · DLXOR · DIMP (dual of SLAND/SLOR/SLXOR/SIMP) */
  if (kw(&L->cur,"DLAND")||kw(&L->cur,"2LAND")||kw(&L->cur,"S2LAND")||
      kw(&L->cur,"STACK2LAND")||kw(&L->cur,"PAIRLAND")||
      kw(&L->cur,"DLOR")||kw(&L->cur,"2LOR")||kw(&L->cur,"S2LOR")||
      kw(&L->cur,"STACK2LOR")||kw(&L->cur,"PAIRLOR")||
      kw(&L->cur,"DLXOR")||kw(&L->cur,"2LXOR")||kw(&L->cur,"S2LXOR")||
      kw(&L->cur,"STACK2LXOR")||kw(&L->cur,"PAIRLXOR")||
      kw(&L->cur,"DIMP")||kw(&L->cur,"2IMP")||kw(&L->cur,"S2IMP")||
      kw(&L->cur,"STACK2IMP")||kw(&L->cur,"PAIRIMP")||kw(&L->cur,"DIMPLY")||
      kw(&L->cur,"2IMPLY")||kw(&L->cur,"PAIRIMPLY")||kw(&L->cur,"DIMPLIES")||
      kw(&L->cur,"2IMPLIES")){
    /* a b c d → land/lor/lxor/imp(a,c) land/lor/lxor/imp(b,d) as 0/1 */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    int is_and = (strcmp(op,"DLAND")==0 || strcmp(op,"2LAND")==0 || strcmp(op,"S2LAND")==0 ||
                  strcmp(op,"STACK2LAND")==0 || strcmp(op,"PAIRLAND")==0);
    int is_or = (strcmp(op,"DLOR")==0 || strcmp(op,"2LOR")==0 || strcmp(op,"S2LOR")==0 ||
                 strcmp(op,"STACK2LOR")==0 || strcmp(op,"PAIRLOR")==0);
    int is_xor = (strcmp(op,"DLXOR")==0 || strcmp(op,"2LXOR")==0 || strcmp(op,"S2LXOR")==0 ||
                  strcmp(op,"STACK2LXOR")==0 || strcmp(op,"PAIRLXOR")==0);
    /* else DIMP / imply */
    long x, y;
    if (is_and){
      x = (a && c) ? 1 : 0;
      y = (b && d) ? 1 : 0;
    } else if (is_or){
      x = (a || c) ? 1 : 0;
      y = (b || d) ? 1 : 0;
    } else if (is_xor){
      x = ((a != 0) ^ (c != 0)) ? 1 : 0;
      y = ((b != 0) ^ (d != 0)) ? 1 : 0;
    } else {
      x = (!a || c) ? 1 : 0;
      y = (!b || d) ? 1 : 0;
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-2 dual-stack pair numthy: DGCD · DLCM · DPOW (vector pairs a b ⋆ c d) */
  if (kw(&L->cur,"DGCD")||kw(&L->cur,"2GCD")||kw(&L->cur,"S2GCD")||
      kw(&L->cur,"STACK2GCD")||kw(&L->cur,"PAIRGCD")||
      kw(&L->cur,"DLCM")||kw(&L->cur,"2LCM")||kw(&L->cur,"S2LCM")||
      kw(&L->cur,"STACK2LCM")||kw(&L->cur,"PAIRLCM")||
      kw(&L->cur,"DPOW")||kw(&L->cur,"2POW")||kw(&L->cur,"S2POW")||
      kw(&L->cur,"STACK2POW")||kw(&L->cur,"PAIRPOW")){
    /* a b c d → f(a,c) f(b,d) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    int is_gcd = (strcmp(op,"DGCD")==0 || strcmp(op,"2GCD")==0 || strcmp(op,"S2GCD")==0 ||
                  strcmp(op,"STACK2GCD")==0 || strcmp(op,"PAIRGCD")==0);
    int is_lcm = (strcmp(op,"DLCM")==0 || strcmp(op,"2LCM")==0 || strcmp(op,"S2LCM")==0 ||
                  strcmp(op,"STACK2LCM")==0 || strcmp(op,"PAIRLCM")==0);
    long x = 0, y = 0;
    if (is_gcd || is_lcm){
      long ax = a < 0 ? -a : a, cx = c < 0 ? -c : c;
      long bx = b < 0 ? -b : b, dx = d < 0 ? -d : d;
      long gx = ax, hy = cx;
      while (hy){ long t = gx % hy; gx = hy; hy = t; }
      long gy = bx, hz = dx;
      while (hz){ long t = gy % hz; gy = hz; hz = t; }
      if (is_gcd){ x = gx; y = gy; }
      else {
        /* LCM — 0 if either side 0 */
        x = (!ax || !cx) ? 0 : (ax / gx) * cx;
        y = (!bx || !dx) ? 0 : (bx / gy) * dx;
      }
    } else {
      /* DPOW — a^c, b^d; neg exp → 0 */
      if (c < 0) x = 0;
      else { x = 1; long e = c; while (e-- > 0) x *= a; }
      if (d < 0) y = 0;
      else { y = 1; long e = d; while (e-- > 0) y *= b; }
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-2 dual-stack imm numthy: DGCDN · DLCMN (shared-n dual of SGCDN/SLCMN) */
  if (kw(&L->cur,"DGCDN")||kw(&L->cur,"2GCDN")||kw(&L->cur,"S2GCDN")||
      kw(&L->cur,"STACK2GCDN")||kw(&L->cur,"PAIRGCDN")||kw(&L->cur,"DGCDIMM")||
      kw(&L->cur,"2GCDIMM")||kw(&L->cur,"PAIRGCDIMM")){
    /* a b + n → gcd(a,n) gcd(b,n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long nn = n < 0 ? -n : n;
    long ax = a < 0 ? -a : a, bx = b < 0 ? -b : b;
    long gx = ax, hy = nn;
    while (hy){ long t = gx % hy; gx = hy; hy = t; }
    long gy = bx, hz = nn;
    while (hz){ long t = gy % hz; gy = hz; hz = t; }
    vm->stack[vm->sp - 2] = gx;
    vm->stack[vm->sp - 1] = gy;
    var_set_num(vm,"LAST_N",gy); vm->last_n=gy;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DLCMN")||kw(&L->cur,"2LCMN")||kw(&L->cur,"S2LCMN")||
      kw(&L->cur,"STACK2LCMN")||kw(&L->cur,"PAIRLCMN")||kw(&L->cur,"DLCMIMM")||
      kw(&L->cur,"2LCMIMM")||kw(&L->cur,"PAIRLCMIMM")){
    /* a b + n → lcm(a,n) lcm(b,n); 0 if either side 0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long nn = n < 0 ? -n : n;
    long ax = a < 0 ? -a : a, bx = b < 0 ? -b : b;
    long x = 0, y = 0;
    if (ax && nn){
      long g = ax, h = nn;
      while (h){ long t = g % h; g = h; h = t; }
      x = (ax / g) * nn;
    }
    if (bx && nn){
      long g = bx, h = nn;
      while (h){ long t = g % h; g = h; h = t; }
      y = (bx / g) * nn;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-2 dual-stack imm numthy ext: DCOPRIMEN · DPOWN (dual of SCOPRIMEN/SPOWN) */
  if (kw(&L->cur,"DCOPRIMEN")||kw(&L->cur,"2COPRIMEN")||kw(&L->cur,"S2COPRIMEN")||
      kw(&L->cur,"STACK2COPRIMEN")||kw(&L->cur,"PAIRCOPRIMEN")||kw(&L->cur,"DISCOPRIMEN")||
      kw(&L->cur,"2ISCOPRIMEN")||kw(&L->cur,"DCOPRIMEIMM")||kw(&L->cur,"PAIRCOPRIMEIMM")){
    /* a b + n → (gcd(a,n)==1) (gcd(b,n)==1) as 0/1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long nn = n < 0 ? -n : n;
    long ax = a < 0 ? -a : a, bx = b < 0 ? -b : b;
    long gx = ax, hy = nn;
    long x = 0, y = 0;
    if (!(ax == 0 && nn == 0)){
      while (hy){ long t = gx % hy; gx = hy; hy = t; }
      x = (gx == 1) ? 1 : 0;
    }
    long gy = bx, hz = nn;
    if (!(bx == 0 && nn == 0)){
      while (hz){ long t = gy % hz; gy = hz; hz = t; }
      y = (gy == 1) ? 1 : 0;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPOWN")||kw(&L->cur,"2POWN")||kw(&L->cur,"S2POWN")||
      kw(&L->cur,"STACK2POWN")||kw(&L->cur,"PAIRPOWN")||kw(&L->cur,"DPOWIMM")||
      kw(&L->cur,"2POWIMM")||kw(&L->cur,"PAIRPOWIMM")||kw(&L->cur,"DPOWERN")){
    /* a b + n → a^n b^n; n<0 → 0,0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (n >= 0){
      x = 1; long e = n; while (e-- > 0) x *= a;
      y = 1; e = n; while (e-- > 0) y *= b;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-8 dual-stack pair shifts: DSHL · DSHR · DSAR (a b c d → a≪c b≪d etc) */
  if (kw(&L->cur,"DSHL")||kw(&L->cur,"2SHL")||kw(&L->cur,"S2SHL")||
      kw(&L->cur,"STACK2SHL")||kw(&L->cur,"PAIRSHL")||
      kw(&L->cur,"DSHR")||kw(&L->cur,"2SHR")||kw(&L->cur,"S2SHR")||
      kw(&L->cur,"STACK2SHR")||kw(&L->cur,"PAIRSHR")||
      kw(&L->cur,"DSAR")||kw(&L->cur,"2SAR")||kw(&L->cur,"S2SAR")||
      kw(&L->cur,"STACK2SAR")||kw(&L->cur,"PAIRSAR")){
    /* a b c d → shift(a,c) shift(b,d); amounts clamped 0..63 */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long kc = c < 0 ? 0 : (c > 63 ? 63 : c);
    long kd = d < 0 ? 0 : (d > 63 ? 63 : d);
    int is_shl = (strcmp(op,"DSHL")==0 || strcmp(op,"2SHL")==0 || strcmp(op,"S2SHL")==0 ||
                  strcmp(op,"STACK2SHL")==0 || strcmp(op,"PAIRSHL")==0);
    int is_shr = (strcmp(op,"DSHR")==0 || strcmp(op,"2SHR")==0 || strcmp(op,"S2SHR")==0 ||
                  strcmp(op,"STACK2SHR")==0 || strcmp(op,"PAIRSHR")==0);
    long x, y;
    if (is_shl){
      x = (long)((unsigned long)a << (unsigned)kc);
      y = (long)((unsigned long)b << (unsigned)kd);
    } else if (is_shr){
      x = (long)((unsigned long)a >> (unsigned)kc);
      y = (long)((unsigned long)b >> (unsigned)kd);
    } else {
      /* arithmetic right */
      x = a >> kc;
      y = b >> kd;
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* dual-stack fixed-width shift: DSHL4/8/16 · DSHR4/8/16 · DSAR4/8/16 (digit-3/5 nibble+byte) */
  if (kw(&L->cur,"DSHL4")||kw(&L->cur,"2SHL4")||kw(&L->cur,"S2SHL4")||
      kw(&L->cur,"STACK2SHL4")||kw(&L->cur,"PAIRSHL4")||
      kw(&L->cur,"DSHR4")||kw(&L->cur,"2SHR4")||kw(&L->cur,"S2SHR4")||
      kw(&L->cur,"STACK2SHR4")||kw(&L->cur,"PAIRSHR4")||
      kw(&L->cur,"DSAR4")||kw(&L->cur,"2SAR4")||kw(&L->cur,"S2SAR4")||
      kw(&L->cur,"STACK2SAR4")||kw(&L->cur,"PAIRSAR4")||kw(&L->cur,"DASHR4")||
      kw(&L->cur,"DSHL8")||kw(&L->cur,"2SHL8")||kw(&L->cur,"S2SHL8")||
      kw(&L->cur,"STACK2SHL8")||kw(&L->cur,"PAIRSHL8")||
      kw(&L->cur,"DSHR8")||kw(&L->cur,"2SHR8")||kw(&L->cur,"S2SHR8")||
      kw(&L->cur,"STACK2SHR8")||kw(&L->cur,"PAIRSHR8")||
      kw(&L->cur,"DSAR8")||kw(&L->cur,"2SAR8")||kw(&L->cur,"S2SAR8")||
      kw(&L->cur,"STACK2SAR8")||kw(&L->cur,"PAIRSAR8")||kw(&L->cur,"DASHR8")||
      kw(&L->cur,"DSHL16")||kw(&L->cur,"2SHL16")||kw(&L->cur,"S2SHL16")||
      kw(&L->cur,"STACK2SHL16")||kw(&L->cur,"PAIRSHL16")||
      kw(&L->cur,"DSHR16")||kw(&L->cur,"2SHR16")||kw(&L->cur,"S2SHR16")||
      kw(&L->cur,"STACK2SHR16")||kw(&L->cur,"PAIRSHR16")||
      kw(&L->cur,"DSAR16")||kw(&L->cur,"2SAR16")||kw(&L->cur,"S2SAR16")||
      kw(&L->cur,"STACK2SAR16")||kw(&L->cur,"PAIRSAR16")||kw(&L->cur,"DASHR16")){
    /* a b c d → shift_w(a,c) shift_w(b,d); w∈{4,8,16}; amounts clamped; result width-masked (SAR sign-ext) */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    int is_w16 = (strstr(op,"16") != NULL);
    int is_w8 = (!is_w16 && strstr(op,"8") != NULL);
    /* else width 4 */
    int is_shl = (strstr(op,"SHL") != NULL);
    /* ASHR contains "SHR" — detect SAR/ASHR before plain SHR */
    int is_sar = (!is_shl && (strstr(op,"SAR") != NULL || strstr(op,"ASHR") != NULL));
    int is_shr = (!is_shl && !is_sar && strstr(op,"SHR") != NULL);
    /* else SAR/ASHR (is_sar) */
    int bits = is_w16 ? 16 : (is_w8 ? 8 : 4);
    unsigned long mask = is_w16 ? 0xFFFFul : (is_w8 ? 0xFFul : 0xFul);
    long signb = is_w16 ? 0x8000L : (is_w8 ? 0x80L : 0x8L);
    long kc = c < 0 ? 0 : c;
    long kd = d < 0 ? 0 : d;
    long x, y;
    if (is_shl){
      if (kc >= bits) x = 0;
      else x = (long)((((unsigned long)a & mask) << (unsigned)kc) & mask);
      if (kd >= bits) y = 0;
      else y = (long)((((unsigned long)b & mask) << (unsigned)kd) & mask);
    } else if (is_shr){
      if (kc >= bits) x = 0;
      else x = (long)(((unsigned long)a & mask) >> (unsigned)kc);
      if (kd >= bits) y = 0;
      else y = (long)(((unsigned long)b & mask) >> (unsigned)kd);
    } else {
      /* arithmetic right: sign-extend low w bits, then >> k (k>=bits → all sign) */
      long va = (long)((unsigned long)a & mask);
      long vb = (long)((unsigned long)b & mask);
      if (va & signb) va |= (long)~mask;
      if (vb & signb) vb |= (long)~mask;
      if (kc >= bits) x = (va < 0) ? -1L : 0L;
      else x = va >> kc;
      if (kd >= bits) y = (vb < 0) ? -1L : 0L;
      else y = vb >> kd;
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-5 dual-stack fixed-width shift32: DSHL32 · DSHR32 · DSAR32
   * (complete dual-stack fixed shift 4/8/16/32 plane after DROL32/DROR32) */
  if (kw(&L->cur,"DSHL32")||kw(&L->cur,"2SHL32")||kw(&L->cur,"S2SHL32")||
      kw(&L->cur,"STACK2SHL32")||kw(&L->cur,"PAIRSHL32")||
      kw(&L->cur,"DSHR32")||kw(&L->cur,"2SHR32")||kw(&L->cur,"S2SHR32")||
      kw(&L->cur,"STACK2SHR32")||kw(&L->cur,"PAIRSHR32")||
      kw(&L->cur,"DSAR32")||kw(&L->cur,"2SAR32")||kw(&L->cur,"S2SAR32")||
      kw(&L->cur,"STACK2SAR32")||kw(&L->cur,"PAIRSAR32")||kw(&L->cur,"DASHR32")||
      kw(&L->cur,"2ASHR32")){
    /* a b c d → shift32(a,c) shift32(b,d); result low 32; SAR sign-extends bit31 */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *q=op;*q;q++) if (*q>='a'&&*q<='z') *q=(char)(*q-'a'+'A');
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    int is_shl = (strstr(op,"SHL") != NULL);
    int is_sar = (!is_shl && (strstr(op,"SAR") != NULL || strstr(op,"ASHR") != NULL));
    int bits = 32;
    unsigned long mask = 0xFFFFFFFFul;
    long signb = 0x80000000L;
    long kc = c < 0 ? 0 : c;
    long kd = d < 0 ? 0 : d;
    long x, y;
    if (is_shl){
      if (kc >= bits) x = 0;
      else x = (long)((((unsigned long)a & mask) << (unsigned)kc) & mask);
      if (kd >= bits) y = 0;
      else y = (long)((((unsigned long)b & mask) << (unsigned)kd) & mask);
    } else if (!is_sar){
      /* logical SHR */
      if (kc >= bits) x = 0;
      else x = (long)(((unsigned long)a & mask) >> (unsigned)kc);
      if (kd >= bits) y = 0;
      else y = (long)(((unsigned long)b & mask) >> (unsigned)kd);
    } else {
      long va = (long)((unsigned long)a & mask);
      long vb = (long)((unsigned long)b & mask);
      if (va & signb) va |= (long)~mask;
      if (vb & signb) vb |= (long)~mask;
      if (kc >= bits) x = (va < 0) ? -1L : 0L;
      else x = va >> kc;
      if (kd >= bits) y = (vb < 0) ? -1L : 0L;
      else y = vb >> kd;
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-2 dual-stack numthy ext: DSQR · DISQRT (unary pair) · DCOPRIME (pairwise) */
  if (kw(&L->cur,"DSQR")||kw(&L->cur,"2SQR")||kw(&L->cur,"S2SQR")||
      kw(&L->cur,"STACK2SQR")||kw(&L->cur,"PAIRSQR")||kw(&L->cur,"DSQUARE")||
      kw(&L->cur,"DISQRT")||kw(&L->cur,"2ISQRT")||kw(&L->cur,"S2ISQRT")||
      kw(&L->cur,"STACK2ISQRT")||kw(&L->cur,"PAIRISQRT")||kw(&L->cur,"2SQRT")||
      kw(&L->cur,"DSQRT")){
    /* a b → f(a) f(b) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_sqr = (strcmp(op,"DSQR")==0 || strcmp(op,"2SQR")==0 || strcmp(op,"S2SQR")==0 ||
                  strcmp(op,"STACK2SQR")==0 || strcmp(op,"PAIRSQR")==0 ||
                  strcmp(op,"DSQUARE")==0);
    long x, y;
    if (is_sqr){
      x = a * a;
      y = b * b;
    } else {
      /* integer sqrt; neg → 0 */
      if (a < 0) x = 0;
      else { long t = 0; while ((t + 1) * (t + 1) <= a) t++; x = t; }
      if (b < 0) y = 0;
      else { long t = 0; while ((t + 1) * (t + 1) <= b) t++; y = t; }
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DCOPRIME")||kw(&L->cur,"2COPRIME")||kw(&L->cur,"S2COPRIME")||
      kw(&L->cur,"STACK2COPRIME")||kw(&L->cur,"PAIRCOPRIME")||kw(&L->cur,"DISCOPRIME")){
    /* a b c d → (gcd(a,c)==1) (gcd(b,d)==1) as 0/1 */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long ax = a < 0 ? -a : a, cx = c < 0 ? -c : c;
    long bx = b < 0 ? -b : b, dx = d < 0 ? -d : d;
    long gx = ax, hy = cx;
    while (hy){ long t = gx % hy; gx = hy; hy = t; }
    long gy = bx, hz = dx;
    while (hz){ long t = gy % hz; gy = hz; hz = t; }
    long x = (gx == 1) ? 1 : 0;
    long y = (gy == 1) ? 1 : 0;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-2 dual-stack numthy unary: DLOG2 · DPHI · DISPRIME */
  if (kw(&L->cur,"DLOG2")||kw(&L->cur,"2LOG2")||kw(&L->cur,"S2LOG2")||
      kw(&L->cur,"STACK2LOG2")||kw(&L->cur,"PAIRLOG2")||kw(&L->cur,"DILOG2")||
      kw(&L->cur,"2ILOG2")||
      kw(&L->cur,"DPHI")||kw(&L->cur,"2PHI")||kw(&L->cur,"S2PHI")||
      kw(&L->cur,"STACK2PHI")||kw(&L->cur,"PAIRPHI")||kw(&L->cur,"DTOTIENT")||
      kw(&L->cur,"2TOTIENT")||
      kw(&L->cur,"DISPRIME")||kw(&L->cur,"2ISPRIME")||kw(&L->cur,"S2ISPRIME")||
      kw(&L->cur,"STACK2ISPRIME")||kw(&L->cur,"PAIRISPRIME")||kw(&L->cur,"DPRIMEP")||
      kw(&L->cur,"2PRIMEP")){
    /* a b → f(a) f(b)
     * LOG2: floor(log2); a<=0 → -1
     * PHI: Euler totient; a<=0 → 0
     * ISPRIME: 1 if prime else 0 */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_log = (strcmp(op,"DLOG2")==0 || strcmp(op,"2LOG2")==0 || strcmp(op,"S2LOG2")==0 ||
                  strcmp(op,"STACK2LOG2")==0 || strcmp(op,"PAIRLOG2")==0 ||
                  strcmp(op,"DILOG2")==0 || strcmp(op,"2ILOG2")==0);
    int is_phi = (strcmp(op,"DPHI")==0 || strcmp(op,"2PHI")==0 || strcmp(op,"S2PHI")==0 ||
                  strcmp(op,"STACK2PHI")==0 || strcmp(op,"PAIRPHI")==0 ||
                  strcmp(op,"DTOTIENT")==0 || strcmp(op,"2TOTIENT")==0);
    long x = 0, y = 0;
    if (is_log){
      if (a <= 0) x = -1;
      else { unsigned long u = (unsigned long)a; x = -1; while (u){ x++; u >>= 1; } }
      if (b <= 0) y = -1;
      else { unsigned long u = (unsigned long)b; y = -1; while (u){ y++; u >>= 1; } }
    } else if (is_phi){
      if (a > 0){
        if (a == 1) x = 1;
        else {
          long n = a, r = n;
          if ((n % 2) == 0){ while ((n % 2) == 0) n /= 2; r -= r / 2; }
          for (long p = 3; p * p <= n; p += 2){
            if ((n % p) == 0){ while ((n % p) == 0) n /= p; r -= r / p; }
          }
          if (n > 1) r -= r / n;
          x = r;
        }
      }
      if (b > 0){
        if (b == 1) y = 1;
        else {
          long n = b, r = n;
          if ((n % 2) == 0){ while ((n % 2) == 0) n /= 2; r -= r / 2; }
          for (long p = 3; p * p <= n; p += 2){
            if ((n % p) == 0){ while ((n % p) == 0) n /= p; r -= r / p; }
          }
          if (n > 1) r -= r / n;
          y = r;
        }
      }
    } else {
      /* ISPRIME */
      long va = a, vb = b;
      if (va > 1){
        if (va <= 3) x = 1;
        else if ((va % 2) && (va % 3)){
          x = 1;
          for (long i = 5; i * i <= va; i += 6){
            if ((va % i) == 0 || (va % (i + 2)) == 0){ x = 0; break; }
          }
        }
      }
      if (vb > 1){
        if (vb <= 3) y = 1;
        else if ((vb % 2) && (vb % 3)){
          y = 1;
          for (long i = 5; i * i <= vb; i += 6){
            if ((vb % i) == 0 || (vb % (i + 2)) == 0){ y = 0; break; }
          }
        }
      }
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-2 dual-stack numthy unary ext: DFIB · DFACT · DLOG10 · DPOW10 */
  if (kw(&L->cur,"DFIB")||kw(&L->cur,"2FIB")||kw(&L->cur,"S2FIB")||
      kw(&L->cur,"STACK2FIB")||kw(&L->cur,"PAIRFIB")||kw(&L->cur,"DFIBONACCI")||
      kw(&L->cur,"2FIBONACCI")||
      kw(&L->cur,"DFACT")||kw(&L->cur,"2FACT")||kw(&L->cur,"S2FACT")||
      kw(&L->cur,"STACK2FACT")||kw(&L->cur,"PAIRFACT")||kw(&L->cur,"DFACTORIAL")||
      kw(&L->cur,"2FACTORIAL")||
      kw(&L->cur,"DLOG10")||kw(&L->cur,"2LOG10")||kw(&L->cur,"S2LOG10")||
      kw(&L->cur,"STACK2LOG10")||kw(&L->cur,"PAIRLOG10")||kw(&L->cur,"DILOG10")||
      kw(&L->cur,"2ILOG10")||
      kw(&L->cur,"DPOW10")||kw(&L->cur,"2POW10")||kw(&L->cur,"S2POW10")||
      kw(&L->cur,"STACK2POW10")||kw(&L->cur,"PAIRPOW10")||kw(&L->cur,"DTENPOW")||
      kw(&L->cur,"2TENPOW")){
    /* a b → f(a) f(b)
     * FIB: Fib(n); n<=0 → 0; n>92 clamped
     * FACT: n!; n<0 → 0; n>20 clamped
     * LOG10: floor(log10); a<=0 → -1
     * POW10: 10^n; n in 0..18 else 0 */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_fib = (strcmp(op,"DFIB")==0 || strcmp(op,"2FIB")==0 || strcmp(op,"S2FIB")==0 ||
                  strcmp(op,"STACK2FIB")==0 || strcmp(op,"PAIRFIB")==0 ||
                  strcmp(op,"DFIBONACCI")==0 || strcmp(op,"2FIBONACCI")==0);
    int is_fact = (strcmp(op,"DFACT")==0 || strcmp(op,"2FACT")==0 || strcmp(op,"S2FACT")==0 ||
                   strcmp(op,"STACK2FACT")==0 || strcmp(op,"PAIRFACT")==0 ||
                   strcmp(op,"DFACTORIAL")==0 || strcmp(op,"2FACTORIAL")==0);
    int is_log10 = (strcmp(op,"DLOG10")==0 || strcmp(op,"2LOG10")==0 || strcmp(op,"S2LOG10")==0 ||
                    strcmp(op,"STACK2LOG10")==0 || strcmp(op,"PAIRLOG10")==0 ||
                    strcmp(op,"DILOG10")==0 || strcmp(op,"2ILOG10")==0);
    long x = 0, y = 0;
    if (is_fib){
      long na = a, nb = b;
      if (na > 0){
        if (na == 1 || na == 2) x = 1;
        else {
          if (na > 92) na = 92;
          long f0 = 0, f1 = 1;
          for (long i = 2; i <= na; i++){ long f2 = f0 + f1; f0 = f1; f1 = f2; }
          x = f1;
        }
      }
      if (nb > 0){
        if (nb == 1 || nb == 2) y = 1;
        else {
          if (nb > 92) nb = 92;
          long f0 = 0, f1 = 1;
          for (long i = 2; i <= nb; i++){ long f2 = f0 + f1; f0 = f1; f1 = f2; }
          y = f1;
        }
      }
    } else if (is_fact){
      if (a >= 0){
        long n = a > 20 ? 20 : a;
        x = 1;
        for (long i = 2; i <= n; i++) x *= i;
      }
      if (b >= 0){
        long n = b > 20 ? 20 : b;
        y = 1;
        for (long i = 2; i <= n; i++) y *= i;
      }
    } else if (is_log10){
      if (a <= 0) x = -1;
      else { x = 0; long t = a; while (t >= 10){ x++; t /= 10; } }
      if (b <= 0) y = -1;
      else { y = 0; long t = b; while (t >= 10){ y++; t /= 10; } }
    } else {
      /* POW10 / TENPOW */
      if (a >= 0 && a <= 18){
        x = 1;
        for (long i = 0; i < a; i++) x *= 10;
      }
      if (b >= 0 && b <= 18){
        y = 1;
        for (long i = 0; i < b; i++) y *= 10;
      }
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-2 dual-stack arithmetic numthy: DMOBIUS · DRAD · DSQFREE */
  if (kw(&L->cur,"DMOBIUS")||kw(&L->cur,"2MOBIUS")||kw(&L->cur,"S2MOBIUS")||
      kw(&L->cur,"STACK2MOBIUS")||kw(&L->cur,"PAIRMOBIUS")||kw(&L->cur,"DMU")||
      kw(&L->cur,"2MU")||
      kw(&L->cur,"DRAD")||kw(&L->cur,"2RAD")||kw(&L->cur,"S2RAD")||
      kw(&L->cur,"STACK2RAD")||kw(&L->cur,"PAIRRAD")||kw(&L->cur,"DRADICAL")||
      kw(&L->cur,"2RADICAL")||
      kw(&L->cur,"DSQFREE")||kw(&L->cur,"2SQFREE")||kw(&L->cur,"S2SQFREE")||
      kw(&L->cur,"STACK2SQFREE")||kw(&L->cur,"PAIRSQFREE")||kw(&L->cur,"DISSQFREE")||
      kw(&L->cur,"2ISSQFREE")||kw(&L->cur,"DISSQUAREFREE")||kw(&L->cur,"2ISSQUAREFREE")){
    /* a b → f(a) f(b)
     * MOBIUS/MU: μ(n); n<=0 → 0; square factor → 0; else (−1)^ω
     * RAD/RADICAL: product of distinct primes; n<=0 → 0
     * SQFREE: 1 if square-free (and n>0), else 0 */
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_mu = (strcmp(op,"DMOBIUS")==0 || strcmp(op,"2MOBIUS")==0 || strcmp(op,"S2MOBIUS")==0 ||
                 strcmp(op,"STACK2MOBIUS")==0 || strcmp(op,"PAIRMOBIUS")==0 ||
                 strcmp(op,"DMU")==0 || strcmp(op,"2MU")==0);
    int is_rad = (strcmp(op,"DRAD")==0 || strcmp(op,"2RAD")==0 || strcmp(op,"S2RAD")==0 ||
                  strcmp(op,"STACK2RAD")==0 || strcmp(op,"PAIRRAD")==0 ||
                  strcmp(op,"DRADICAL")==0 || strcmp(op,"2RADICAL")==0);
    long x = 0, y = 0;
    if (is_mu){
      if (a > 0){
        if (a == 1) x = 1;
        else {
          long n = a; int k = 0; x = 1;
          if ((n % 2) == 0){ n /= 2; k++; if ((n % 2) == 0) x = 0; }
          if (x){
            for (long p = 3; p * p <= n; p += 2){
              if ((n % p) == 0){ n /= p; k++; if ((n % p) == 0){ x = 0; break; } }
            }
            if (x){ if (n > 1) k++; x = (k & 1) ? -1 : 1; }
          }
        }
      }
      if (b > 0){
        if (b == 1) y = 1;
        else {
          long n = b; int k = 0; y = 1;
          if ((n % 2) == 0){ n /= 2; k++; if ((n % 2) == 0) y = 0; }
          if (y){
            for (long p = 3; p * p <= n; p += 2){
              if ((n % p) == 0){ n /= p; k++; if ((n % p) == 0){ y = 0; break; } }
            }
            if (y){ if (n > 1) k++; y = (k & 1) ? -1 : 1; }
          }
        }
      }
    } else if (is_rad){
      if (a > 0){
        if (a == 1) x = 1;
        else {
          long n = a; x = 1;
          if ((n % 2) == 0){ x *= 2; while ((n % 2) == 0) n /= 2; }
          for (long p = 3; p * p <= n; p += 2){
            if ((n % p) == 0){ x *= p; while ((n % p) == 0) n /= p; }
          }
          if (n > 1) x *= n;
        }
      }
      if (b > 0){
        if (b == 1) y = 1;
        else {
          long n = b; y = 1;
          if ((n % 2) == 0){ y *= 2; while ((n % 2) == 0) n /= 2; }
          for (long p = 3; p * p <= n; p += 2){
            if ((n % p) == 0){ y *= p; while ((n % p) == 0) n /= p; }
          }
          if (n > 1) y *= n;
        }
      }
    } else {
      /* SQFREE / ISSQFREE */
      if (a > 0){
        if (a == 1) x = 1;
        else {
          long n = a; x = 1;
          if ((n % 2) == 0){ n /= 2; if ((n % 2) == 0) x = 0; }
          if (x){
            for (long p = 3; p * p <= n; p += 2){
              if ((n % p) == 0){ n /= p; if ((n % p) == 0){ x = 0; break; } }
            }
          }
        }
      }
      if (b > 0){
        if (b == 1) y = 1;
        else {
          long n = b; y = 1;
          if ((n % 2) == 0){ n /= 2; if ((n % 2) == 0) y = 0; }
          if (y){
            for (long p = 3; p * p <= n; p += 2){
              if ((n % p) == 0){ n /= p; if ((n % p) == 0){ y = 0; break; } }
            }
          }
        }
      }
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-2 dual-stack modular: DADDMOD · DSUBMOD (a b c d ma mb → (a±c)%ma (b±d)%mb) */
  if (kw(&L->cur,"DADDMOD")||kw(&L->cur,"2ADDMOD")||kw(&L->cur,"S2ADDMOD")||
      kw(&L->cur,"STACK2ADDMOD")||kw(&L->cur,"PAIRADDMOD")||
      kw(&L->cur,"DSUBMOD")||kw(&L->cur,"2SUBMOD")||kw(&L->cur,"S2SUBMOD")||
      kw(&L->cur,"STACK2SUBMOD")||kw(&L->cur,"PAIRSUBMOD")){
    /* a b c d ma mb → (a±c) mod ma, (b±d) mod mb; m<=0 → 0; result in [0,m) */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_add = (strcmp(op,"DADDMOD")==0 || strcmp(op,"2ADDMOD")==0 ||
                  strcmp(op,"S2ADDMOD")==0 || strcmp(op,"STACK2ADDMOD")==0 ||
                  strcmp(op,"PAIRADDMOD")==0);
    lex_next(L);
    if (vm->sp < 6){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long mb = vm->stack[--vm->sp];
    long ma = vm->stack[--vm->sp];
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = 0, y = 0;
    if (ma > 0){
      long aa = a % ma; if (aa < 0) aa += ma;
      long cc = c % ma; if (cc < 0) cc += ma;
      if (is_add) x = (aa + cc) % ma;
      else x = (aa - cc + ma) % ma;
    }
    if (mb > 0){
      long bb = b % mb; if (bb < 0) bb += mb;
      long dd = d % mb; if (dd < 0) dd += mb;
      if (is_add) y = (bb + dd) % mb;
      else y = (bb - dd + mb) % mb;
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-2 dual-stack modular mul/pow: DMULMOD · DPOWMOD */
  if (kw(&L->cur,"DMULMOD")||kw(&L->cur,"2MULMOD")||kw(&L->cur,"S2MULMOD")||
      kw(&L->cur,"STACK2MULMOD")||kw(&L->cur,"PAIRMULMOD")||
      kw(&L->cur,"DPOWMOD")||kw(&L->cur,"2POWMOD")||kw(&L->cur,"S2POWMOD")||
      kw(&L->cur,"STACK2POWMOD")||kw(&L->cur,"PAIRPOWMOD")){
    /* a b c d ma mb → f(a,c,ma) f(b,d,mb); m<=0 → 0; result in [0,m)
     * MUL: (a*c) mod m  POW: (a^c) mod m; neg exp → 0 */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_mul = (strcmp(op,"DMULMOD")==0 || strcmp(op,"2MULMOD")==0 ||
                  strcmp(op,"S2MULMOD")==0 || strcmp(op,"STACK2MULMOD")==0 ||
                  strcmp(op,"PAIRMULMOD")==0);
    lex_next(L);
    if (vm->sp < 6){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long mb = vm->stack[--vm->sp];
    long ma = vm->stack[--vm->sp];
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = 0, y = 0;
    if (is_mul){
      if (ma > 0){
        long aa = a % ma; if (aa < 0) aa += ma;
        long cc = c % ma; if (cc < 0) cc += ma;
        long acc = 0, xx = aa, yy = cc;
        while (yy > 0){
          if (yy & 1) acc = (acc + xx) % ma;
          xx = (xx + xx) % ma;
          yy >>= 1;
        }
        x = acc;
      }
      if (mb > 0){
        long bb = b % mb; if (bb < 0) bb += mb;
        long dd = d % mb; if (dd < 0) dd += mb;
        long acc = 0, xx = bb, yy = dd;
        while (yy > 0){
          if (yy & 1) acc = (acc + xx) % mb;
          xx = (xx + xx) % mb;
          yy >>= 1;
        }
        y = acc;
      }
    } else {
      /* DPOWMOD */
      if (ma > 0){
        if (c < 0) x = 0;
        else {
          long base = a % ma; if (base < 0) base += ma;
          long exp = c;
          long r = 1 % ma;
          while (exp > 0){
            if (exp & 1){
              long acc = 0, xx = r, yy = base;
              while (yy > 0){
                if (yy & 1) acc = (acc + xx) % ma;
                xx = (xx + xx) % ma;
                yy >>= 1;
              }
              r = acc;
            }
            {
              long acc = 0, xx = base, yy = base;
              while (yy > 0){
                if (yy & 1) acc = (acc + xx) % ma;
                xx = (xx + xx) % ma;
                yy >>= 1;
              }
              base = acc;
            }
            exp >>= 1;
          }
          x = r;
        }
      }
      if (mb > 0){
        if (d < 0) y = 0;
        else {
          long base = b % mb; if (base < 0) base += mb;
          long exp = d;
          long r = 1 % mb;
          while (exp > 0){
            if (exp & 1){
              long acc = 0, xx = r, yy = base;
              while (yy > 0){
                if (yy & 1) acc = (acc + xx) % mb;
                xx = (xx + xx) % mb;
                yy >>= 1;
              }
              r = acc;
            }
            {
              long acc = 0, xx = base, yy = base;
              while (yy > 0){
                if (yy & 1) acc = (acc + xx) % mb;
                xx = (xx + xx) % mb;
                yy >>= 1;
              }
              base = acc;
            }
            exp >>= 1;
          }
          y = r;
        }
      }
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-2 dual-stack immediate modular ALU: DADDMODN · DSUBMODN · DMULMODN */
  if (kw(&L->cur,"DADDMODN")||kw(&L->cur,"2ADDMODN")||kw(&L->cur,"S2ADDMODN")||
      kw(&L->cur,"STACK2ADDMODN")||kw(&L->cur,"PAIRADDMODN")||kw(&L->cur,"DADDMODIMM")||
      kw(&L->cur,"DSUBMODN")||kw(&L->cur,"2SUBMODN")||kw(&L->cur,"S2SUBMODN")||
      kw(&L->cur,"STACK2SUBMODN")||kw(&L->cur,"PAIRSUBMODN")||kw(&L->cur,"DSUBMODIMM")||
      kw(&L->cur,"DMULMODN")||kw(&L->cur,"2MULMODN")||kw(&L->cur,"S2MULMODN")||
      kw(&L->cur,"STACK2MULMODN")||kw(&L->cur,"PAIRMULMODN")||kw(&L->cur,"DMULMODIMM")){
    /* a b + k m → f(a) f(b); m<=0 → 0; result in [0,m)
     * ADD: (x+k)%m  SUB: (x-k)%m  MUL: (x*k)%m */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_add = (strcmp(op,"DADDMODN")==0 || strcmp(op,"2ADDMODN")==0 ||
                  strcmp(op,"S2ADDMODN")==0 || strcmp(op,"STACK2ADDMODN")==0 ||
                  strcmp(op,"PAIRADDMODN")==0 || strcmp(op,"DADDMODIMM")==0);
    int is_sub = (strcmp(op,"DSUBMODN")==0 || strcmp(op,"2SUBMODN")==0 ||
                  strcmp(op,"S2SUBMODN")==0 || strcmp(op,"STACK2SUBMODN")==0 ||
                  strcmp(op,"PAIRSUBMODN")==0 || strcmp(op,"DSUBMODIMM")==0);
    /* else DMULMODN */
    lex_next(L);
    long k = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (m > 0){
      long kk = k % m; if (kk < 0) kk += m;
      long aa = a % m; if (aa < 0) aa += m;
      long bb = b % m; if (bb < 0) bb += m;
      if (is_add){
        x = (aa + kk) % m;
        y = (bb + kk) % m;
      } else if (is_sub){
        x = (aa - kk + m) % m;
        y = (bb - kk + m) % m;
      } else {
        /* mul mod — binary multiply to avoid overflow for large m (use long long path via stepwise) */
        long accx = 0, accy = 0;
        long xx = aa, yy = bb, kk2 = kk;
        while (kk2 > 0){
          if (kk2 & 1){
            accx = (accx + xx) % m;
            accy = (accy + yy) % m;
          }
          xx = (xx + xx) % m;
          yy = (yy + yy) % m;
          kk2 >>= 1;
        }
        x = accx; y = accy;
      }
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-2 dual-stack modular inv/div: DMODINV · DMODDIV */

  if (kw(&L->cur,"DMODINV")||kw(&L->cur,"2MODINV")||kw(&L->cur,"S2MODINV")||
      kw(&L->cur,"STACK2MODINV")||kw(&L->cur,"PAIRMODINV")||kw(&L->cur,"DINVMOD")||
      kw(&L->cur,"2INVMOD")||kw(&L->cur,"S2INVMOD")||kw(&L->cur,"PAIRINVMOD")){
    /* a b ma mb → a^{-1} mod ma , b^{-1} mod mb ; 0 if none / m<=1 */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long mb = vm->stack[--vm->sp];
    long ma = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = 0, y = 0;
    if (ma > 1){
      long aa = a % ma; if (aa < 0) aa += ma;
      if (aa != 0){
        long t = 0, nt = 1;
        long rr = ma, nr = aa;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){ if (t < 0) t += ma; x = t; }
      }
    }
    if (mb > 1){
      long bb = b % mb; if (bb < 0) bb += mb;
      if (bb != 0){
        long t = 0, nt = 1;
        long rr = mb, nr = bb;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){ if (t < 0) t += mb; y = t; }
      }
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMODDIV")||kw(&L->cur,"2MODDIV")||kw(&L->cur,"S2MODDIV")||
      kw(&L->cur,"STACK2MODDIV")||kw(&L->cur,"PAIRMODDIV")||kw(&L->cur,"DDIVMODM")||
      kw(&L->cur,"2DIVMODM")||kw(&L->cur,"DMODDIVIDE")||kw(&L->cur,"2MODDIVIDE")){
    /* a b c d ma mb → (a * c^{-1}) mod ma , (b * d^{-1}) mod mb ; 0 if none */
    lex_next(L);
    if (vm->sp < 6){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long mb = vm->stack[--vm->sp];
    long ma = vm->stack[--vm->sp];
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = 0, y = 0;
    if (ma > 0){
      long cc = c % ma; if (cc < 0) cc += ma;
      if (cc != 0){
        long t = 0, nt = 1;
        long rr = ma, nr = cc;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){
          if (t < 0) t += ma;
          long aa = a % ma; if (aa < 0) aa += ma;
          long acc = 0, xx = aa, yy = t;
          while (yy > 0){
            if (yy & 1) acc = (acc + xx) % ma;
            xx = (xx + xx) % ma;
            yy >>= 1;
          }
          x = acc;
        }
      }
    }
    if (mb > 0){
      long dd = d % mb; if (dd < 0) dd += mb;
      if (dd != 0){
        long t = 0, nt = 1;
        long rr = mb, nr = dd;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){
          if (t < 0) t += mb;
          long bb = b % mb; if (bb < 0) bb += mb;
          long acc = 0, xx = bb, yy = t;
          while (yy > 0){
            if (yy & 1) acc = (acc + xx) % mb;
            xx = (xx + xx) % mb;
            yy >>= 1;
          }
          y = acc;
        }
      }
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-1 dual-stack immediate modular inv/pow/div: DMODINVN · DPOWMODN · DMODDIVN (dual of SMODINVN/SPOWMODN/SMODDIVN) */
  if (kw(&L->cur,"DMODINVN")||kw(&L->cur,"2MODINVN")||kw(&L->cur,"S2MODINVN")||
      kw(&L->cur,"STACK2MODINVN")||kw(&L->cur,"PAIRMODINVN")||kw(&L->cur,"DINVMODN")||
      kw(&L->cur,"2INVMODN")||kw(&L->cur,"DMODINVIMM")){
    /* a b + m → a^{-1} mod m , b^{-1} mod m ; 0 if none / m<=1 */
    lex_next(L);
    long m = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (m > 1){
      long aa = a % m; if (aa < 0) aa += m;
      if (aa != 0){
        long t = 0, nt = 1;
        long rr = m, nr = aa;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){ if (t < 0) t += m; x = t; }
      }
      long bb = b % m; if (bb < 0) bb += m;
      if (bb != 0){
        long t = 0, nt = 1;
        long rr = m, nr = bb;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){ if (t < 0) t += m; y = t; }
      }
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPOWMODN")||kw(&L->cur,"2POWMODN")||kw(&L->cur,"S2POWMODN")||
      kw(&L->cur,"STACK2POWMODN")||kw(&L->cur,"PAIRPOWMODN")||kw(&L->cur,"DPOWMODIMM")||
      kw(&L->cur,"2POWMODIMM")){
    /* a b + exp m → a^exp mod m , b^exp mod m ; m<=0 → 0; exp<0 → 0 */
    lex_next(L);
    long exp = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (m > 0 && exp >= 0){
      long basea = a % m; if (basea < 0) basea += m;
      long baseb = b % m; if (baseb < 0) baseb += m;
      x = 1 % m; y = 1 % m;
      long e = exp;
      long ba = basea, bb = baseb;
      while (e > 0){
        if (e & 1){
          long ya = x, xa = ba, acca = 0;
          long yb = y, xb = bb, accb = 0;
          while (ya > 0 || yb > 0){
            if (ya & 1) acca = (acca + xa) % m;
            if (yb & 1) accb = (accb + xb) % m;
            xa = (xa + xa) % m; xb = (xb + xb) % m;
            ya >>= 1; yb >>= 1;
          }
          x = acca; y = accb;
        }
        {
          long xa = ba, acca = 0, ya = ba;
          long xb = bb, accb = 0, yb = bb;
          while (ya > 0 || yb > 0){
            if (ya & 1) acca = (acca + xa) % m;
            if (yb & 1) accb = (accb + xb) % m;
            xa = (xa + xa) % m; xb = (xb + xb) % m;
            ya >>= 1; yb >>= 1;
          }
          ba = acca; bb = accb;
        }
        e >>= 1;
      }
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMODDIVN")||kw(&L->cur,"2MODDIVN")||kw(&L->cur,"S2MODDIVN")||
      kw(&L->cur,"STACK2MODDIVN")||kw(&L->cur,"PAIRMODDIVN")||kw(&L->cur,"DDIVMODMN")||
      kw(&L->cur,"2DIVMODMN")||kw(&L->cur,"DMODDIVIMM")){
    /* a b + c m → a*c^{-1} mod m , b*c^{-1} mod m ; 0 if none / m<=0 */
    lex_next(L);
    long c = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (m > 0){
      long cc = c % m; if (cc < 0) cc += m;
      if (cc != 0){
        long t = 0, nt = 1;
        long rr = m, nr = cc;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){
          if (t < 0) t += m;
          long aa = a % m; if (aa < 0) aa += m;
          long bb = b % m; if (bb < 0) bb += m;
          long acca = 0, xa = aa, ya = t;
          long accb = 0, xb = bb, yb = t;
          while (ya > 0 || yb > 0){
            if (ya & 1) acca = (acca + xa) % m;
            if (yb & 1) accb = (accb + xb) % m;
            xa = (xa + xa) % m; xb = (xb + xb) % m;
            ya >>= 1; yb >>= 1;
          }
          x = acca; y = accb;
        }
      }
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-6 dual-stack reverse imm modular: DSUBMODFROMN · DPOWMODFROMN · DMODDIVFROMN
   * (dual of SSUBMODFROMN/SPOWMODFROMN/SMODDIVFROMN; energy-math reverse after DMODDIVN) */
  if (kw(&L->cur,"DSUBMODFROMN")||kw(&L->cur,"2SUBMODFROMN")||kw(&L->cur,"S2SUBMODFROMN")||
      kw(&L->cur,"STACK2SUBMODFROMN")||kw(&L->cur,"PAIRSUBMODFROMN")||kw(&L->cur,"DRSUBMODN")||
      kw(&L->cur,"2RSUBMODN")||kw(&L->cur,"PAIRRSUBMODN")||kw(&L->cur,"DSUBMODFROMIMM")){
    /* a b + k m → (k-a)%m (k-b)%m ; m<=0 → 0,0 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (m > 0){
      long kk = k % m; if (kk < 0) kk += m;
      long aa = a % m; if (aa < 0) aa += m;
      long bb = b % m; if (bb < 0) bb += m;
      x = (kk - aa + m) % m;
      y = (kk - bb + m) % m;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPOWMODFROMN")||kw(&L->cur,"2POWMODFROMN")||kw(&L->cur,"S2POWMODFROMN")||
      kw(&L->cur,"STACK2POWMODFROMN")||kw(&L->cur,"PAIRPOWMODFROMN")||kw(&L->cur,"DRPOWMODN")||
      kw(&L->cur,"2RPOWMODN")||kw(&L->cur,"PAIRRPOWMODN")||kw(&L->cur,"DPOWMODFROMIMM")||
      kw(&L->cur,"DBASEPOWMODN")){
    /* a b + base m → base^a mod m , base^b mod m ; m<=0 or exp<0 → 0 */
    lex_next(L);
    long base_in = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long expa = vm->stack[vm->sp - 2];
    long expb = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (m > 0){
      long base0 = base_in % m; if (base0 < 0) base0 += m;
      if (expa >= 0){
        long base = base0, r = 1 % m, e = expa;
        while (e > 0){
          if (e & 1){
            long yy = r, xx = base, acc = 0;
            while (yy > 0){
              if (yy & 1) acc = (acc + xx) % m;
              xx = (xx + xx) % m;
              yy >>= 1;
            }
            r = acc;
          }
          {
            long xx = base, acc = 0, yy = base;
            while (yy > 0){
              if (yy & 1) acc = (acc + xx) % m;
              xx = (xx + xx) % m;
              yy >>= 1;
            }
            base = acc;
          }
          e >>= 1;
        }
        x = r;
      }
      if (expb >= 0){
        long base = base0, r = 1 % m, e = expb;
        while (e > 0){
          if (e & 1){
            long yy = r, xx = base, acc = 0;
            while (yy > 0){
              if (yy & 1) acc = (acc + xx) % m;
              xx = (xx + xx) % m;
              yy >>= 1;
            }
            r = acc;
          }
          {
            long xx = base, acc = 0, yy = base;
            while (yy > 0){
              if (yy & 1) acc = (acc + xx) % m;
              xx = (xx + xx) % m;
              yy >>= 1;
            }
            base = acc;
          }
          e >>= 1;
        }
        y = r;
      }
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMODDIVFROMN")||kw(&L->cur,"2MODDIVFROMN")||kw(&L->cur,"S2MODDIVFROMN")||
      kw(&L->cur,"STACK2MODDIVFROMN")||kw(&L->cur,"PAIRMODDIVFROMN")||kw(&L->cur,"DRMODDIVN")||
      kw(&L->cur,"2RMODDIVN")||kw(&L->cur,"PAIRRMODDIVN")||kw(&L->cur,"DMODDIVFROMIMM")||
      kw(&L->cur,"DDIVMODFROMN")){
    /* a b + c m → c*a^{-1} mod m , c*b^{-1} mod m ; 0 if none / m<=0 */
    lex_next(L);
    long c = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (m > 0){
      long cc = c % m; if (cc < 0) cc += m;
      long aa = a % m; if (aa < 0) aa += m;
      long bb = b % m; if (bb < 0) bb += m;
      if (aa != 0){
        long t = 0, nt = 1;
        long rr = m, nr = aa;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){
          if (t < 0) t += m;
          long acc = 0, xx = cc, yy = t;
          while (yy > 0){
            if (yy & 1) acc = (acc + xx) % m;
            xx = (xx + xx) % m;
            yy >>= 1;
          }
          x = acc;
        }
      }
      if (bb != 0){
        long t = 0, nt = 1;
        long rr = m, nr = bb;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){
          if (t < 0) t += m;
          long acc = 0, xx = cc, yy = t;
          while (yy > 0){
            if (yy & 1) acc = (acc + xx) % m;
            xx = (xx + xx) % m;
            yy >>= 1;
          }
          y = acc;
        }
      }
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 bound/select peel: DSIGN·D0*·DCLAMP·DSEL·DSEL2 (G_dual_margin) */
  /* digit-6 dual-stack bound/select: DSIGN · DCLAMP · DSEL/DMUX (energy-style bounds) */
  if (kw(&L->cur,"DSIGN")||kw(&L->cur,"2SIGN")||kw(&L->cur,"S2SIGN")||
      kw(&L->cur,"STACK2SIGN")||kw(&L->cur,"PAIRSIGN")||kw(&L->cur,"DSGN")){
    /* a b → sgn(a) sgn(b)  as -1/0/1 */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a > 0) ? 1 : ((a < 0) ? -1 : 0);
    long y = (b > 0) ? 1 : ((b < 0) ? -1 : 0);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack zero/sign predicates: D0EQ · D0NE · D0LT · D0GT (dual of SZ/SNZ/S0LT/S0GT) */
  if (kw(&L->cur,"D0EQ")||kw(&L->cur,"2_0EQ")||kw(&L->cur,"S20EQ")||
      kw(&L->cur,"STACK20EQ")||kw(&L->cur,"PAIR0EQ")||kw(&L->cur,"DZ")||
      kw(&L->cur,"2Z")||kw(&L->cur,"S2Z")||kw(&L->cur,"PAIRZ")||
      kw(&L->cur,"DZEROEQ")||kw(&L->cur,"2ZEROEQ")){
    /* a b → (a==0?1:0) (b==0?1:0) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a == 0) ? 1 : 0;
    long y = (b == 0) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"D0NE")||kw(&L->cur,"2_0NE")||kw(&L->cur,"S20NE")||
      kw(&L->cur,"STACK20NE")||kw(&L->cur,"PAIR0NE")||kw(&L->cur,"DNZ")||
      kw(&L->cur,"2NZ")||kw(&L->cur,"S2NZ")||kw(&L->cur,"PAIRNZ")||
      kw(&L->cur,"DZERONE")||kw(&L->cur,"2ZERONE")||kw(&L->cur,"D0NEQ")){
    /* a b → (a!=0?1:0) (b!=0?1:0) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a != 0) ? 1 : 0;
    long y = (b != 0) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"D0LT")||kw(&L->cur,"2_0LT")||kw(&L->cur,"S20LT")||
      kw(&L->cur,"STACK20LT")||kw(&L->cur,"PAIR0LT")||kw(&L->cur,"DNEGSGN")||
      kw(&L->cur,"2NEGSGN")||kw(&L->cur,"D0NEG")||kw(&L->cur,"PAIR0NEG")){
    /* a b → (a<0?1:0) (b<0?1:0) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a < 0) ? 1 : 0;
    long y = (b < 0) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"D0GT")||kw(&L->cur,"2_0GT")||kw(&L->cur,"S20GT")||
      kw(&L->cur,"STACK20GT")||kw(&L->cur,"PAIR0GT")||kw(&L->cur,"DPOS")||
      kw(&L->cur,"2POS")||kw(&L->cur,"S2POS")||kw(&L->cur,"PAIRPOS")||
      kw(&L->cur,"D0POS")||kw(&L->cur,"PAIR0POS")){
    /* a b → (a>0?1:0) (b>0?1:0) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a > 0) ? 1 : 0;
    long y = (b > 0) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 dual-stack zero-bound preds: D0LE · D0GE (complete D0* after D0LT/D0GT; energy-sign plane) */
  if (kw(&L->cur,"D0LE")||kw(&L->cur,"2_0LE")||kw(&L->cur,"S20LE")||
      kw(&L->cur,"STACK20LE")||kw(&L->cur,"PAIR0LE")||kw(&L->cur,"DNONPOS")||
      kw(&L->cur,"2NONPOS")||kw(&L->cur,"D0LEQ")||kw(&L->cur,"PAIR0LEQ")){
    /* a b → (a<=0?1:0) (b<=0?1:0) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a <= 0) ? 1 : 0;
    long y = (b <= 0) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"D0GE")||kw(&L->cur,"2_0GE")||kw(&L->cur,"S20GE")||
      kw(&L->cur,"STACK20GE")||kw(&L->cur,"PAIR0GE")||kw(&L->cur,"DNONNEG")||
      kw(&L->cur,"2NONNEG")||kw(&L->cur,"D0GEQ")||kw(&L->cur,"PAIR0GEQ")){
    /* a b → (a>=0?1:0) (b>=0?1:0) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a >= 0) ? 1 : 0;
    long y = (b >= 0) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DCLAMP")||kw(&L->cur,"2CLAMP")||kw(&L->cur,"S2CLAMP")||
      kw(&L->cur,"STACK2CLAMP")||kw(&L->cur,"PAIRCLAMP")||kw(&L->cur,"DCLMP")){
    /* a b lo hi → clamp(a,[lo,hi]) clamp(b,[lo,hi]); lo/hi swapped if inverted */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (lo > hi){ long t = lo; lo = hi; hi = t; }
    long x = a, y = b;
    if (x < lo) x = lo;
    if (x > hi) x = hi;
    if (y < lo) y = lo;
    if (y > hi) y = hi;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSEL")||kw(&L->cur,"2SEL")||kw(&L->cur,"S2SEL")||
      kw(&L->cur,"STACK2SEL")||kw(&L->cur,"PAIRSEL")||
      kw(&L->cur,"DMUX")||kw(&L->cur,"2MUX")||kw(&L->cur,"S2MUX")||
      kw(&L->cur,"STACK2MUX")||kw(&L->cur,"PAIRMUX")){
    /* fa fb ta tb c → (c?ta:fa) (c?tb:fb) — shared cond on TOS */
    lex_next(L);
    if (vm->sp < 5){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long c = vm->stack[--vm->sp];
    long tb = vm->stack[--vm->sp];
    long ta = vm->stack[--vm->sp];
    long fb = vm->stack[--vm->sp];
    long fa = vm->stack[--vm->sp];
    long x = c ? ta : fa;
    long y = c ? tb : fb;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack control: DSEL2 · DMUX2 (per-lane cond mux) */
  if (kw(&L->cur,"DSEL2")||kw(&L->cur,"2SEL2")||kw(&L->cur,"S2SEL2")||
      kw(&L->cur,"STACK2SEL2")||kw(&L->cur,"PAIRSEL2")||
      kw(&L->cur,"DMUX2")||kw(&L->cur,"2MUX2")||kw(&L->cur,"S2MUX2")||
      kw(&L->cur,"STACK2MUX2")||kw(&L->cur,"PAIRMUX2")){
    /* fa fb ta tb ca cb → (ca?ta:fa) (cb?tb:fb) — independent lane conds */
    lex_next(L);
    if (vm->sp < 6){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long cb = vm->stack[--vm->sp];
    long ca = vm->stack[--vm->sp];
    long tb = vm->stack[--vm->sp];
    long ta = vm->stack[--vm->sp];
    long fb = vm->stack[--vm->sp];
    long fa = vm->stack[--vm->sp];
    long x = ca ? ta : fa;
    long y = cb ? tb : fb;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-1 dual-stack conditionals: DNIPIF · DKEEPIF (pair forms of SNIPIF/SKEEPIF) */
  if (kw(&L->cur,"DNIPIF")||kw(&L->cur,"2NIPIF")||kw(&L->cur,"S2NIPIF")||
      kw(&L->cur,"STACK2NIPIF")||kw(&L->cur,"PAIRNIPIF")||kw(&L->cur,"DCNIP")||
      kw(&L->cur,"2CNIP")){
    /* a b c d f → (f?b:a) (f?d:c) — shared flag choose-per-lane */
    lex_next(L);
    if (vm->sp < 5){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long f = vm->stack[--vm->sp];
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = f ? b : a;
    long y = f ? d : c;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DKEEPIF")||kw(&L->cur,"2KEEPIF")||kw(&L->cur,"S2KEEPIF")||
      kw(&L->cur,"STACK2KEEPIF")||kw(&L->cur,"PAIRKEEPIF")||kw(&L->cur,"DQKEEP")||
      kw(&L->cur,"2QKEEP")||kw(&L->cur,"DKEEPWHEN")||kw(&L->cur,"2KEEPWHEN")){
    /* a b f → if f leave a b else drop both */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long f = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (f){
      vm->stack[vm->sp++] = a;
      vm->stack[vm->sp++] = b;
      var_set_num(vm,"LAST_N",b); vm->last_n=b;
    } else {
      long last = (vm->sp > 0) ? vm->stack[vm->sp - 1] : 0;
      var_set_num(vm,"LAST_N",last); vm->last_n=last;
    }
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 conditionals ext peel: DSWAPIF·DDROPIF·DDUPIF (G_dual_margin) */
  /* digit-1 dual-stack conditionals ext: DSWAPIF · DDROPIF · DDUPIF */
  if (kw(&L->cur,"DSWAPIF")||kw(&L->cur,"2SWAPIF")||kw(&L->cur,"S2SWAPIF")||
      kw(&L->cur,"STACK2SWAPIF")||kw(&L->cur,"PAIRSWAPIF")||kw(&L->cur,"DQSWAP")||
      kw(&L->cur,"2QSWAP")||kw(&L->cur,"DCSWAP")||kw(&L->cur,"2CSWAP")){
    /* a b f → if f then b a else a b  (pair conditional swap; dual of SSWAPIF) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long f = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (f){ long t = a; a = b; b = t; }
    vm->stack[vm->sp++] = a;
    vm->stack[vm->sp++] = b;
    var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DDROPIF")||kw(&L->cur,"2DROPIF")||kw(&L->cur,"S2DROPIF")||
      kw(&L->cur,"STACK2DROPIF")||kw(&L->cur,"PAIRDROPIF")||kw(&L->cur,"DQDROP")||
      kw(&L->cur,"2QDROP")||kw(&L->cur,"DDROPWHEN")||kw(&L->cur,"2DROPWHEN")){
    /* a b f → if f drop both else leave a b  (inverse of DKEEPIF) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long f = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (!f){
      vm->stack[vm->sp++] = a;
      vm->stack[vm->sp++] = b;
      var_set_num(vm,"LAST_N",b); vm->last_n=b;
    } else {
      long last = (vm->sp > 0) ? vm->stack[vm->sp - 1] : 0;
      var_set_num(vm,"LAST_N",last); vm->last_n=last;
    }
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DDUPIF")||kw(&L->cur,"2DUPIF")||kw(&L->cur,"S2DUPIF")||
      kw(&L->cur,"STACK2DUPIF")||kw(&L->cur,"PAIRDUPIF")||kw(&L->cur,"D2QDUP")||
      kw(&L->cur,"2QDUP")||kw(&L->cur,"DDUPWHEN")||kw(&L->cur,"2DUPWHEN")){
    /* a b f → if f then a b a b else a b  (conditional pair-dup)
     * alias D2QDUP avoids clobbering single-stack QDUP/?DUP */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long f = vm->stack[--vm->sp];
    long b = vm->stack[vm->sp - 1];
    long a = vm->stack[vm->sp - 2];
    if (f){
      if (vm->sp + 2 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
      vm->stack[vm->sp++] = a;
      vm->stack[vm->sp++] = b;
    }
    var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-1 dual-stack conditionals ext2: DOVERIF · DTUCKIF (combinator forms) */
  if (kw(&L->cur,"DOVERIF")||kw(&L->cur,"2OVERIF")||kw(&L->cur,"S2OVERIF")||
      kw(&L->cur,"STACK2OVERIF")||kw(&L->cur,"PAIROVERIF")||kw(&L->cur,"DQOVER")||
      kw(&L->cur,"2QOVER")||kw(&L->cur,"DOVERWHEN")||kw(&L->cur,"2OVERWHEN")){
    /* a b f → if f then a b a else a b  (conditional OVER; copy a under TOS) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long f = vm->stack[--vm->sp];
    long b = vm->stack[vm->sp - 1];
    long a = vm->stack[vm->sp - 2];
    if (f){
      if (vm->sp + 1 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
      vm->stack[vm->sp++] = a;
      var_set_num(vm,"LAST_N",a); vm->last_n=a;
    } else {
      var_set_num(vm,"LAST_N",b); vm->last_n=b;
    }
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DTUCKIF")||kw(&L->cur,"2TUCKIF")||kw(&L->cur,"S2TUCKIF")||
      kw(&L->cur,"STACK2TUCKIF")||kw(&L->cur,"PAIRTUCKIF")||kw(&L->cur,"DQTUCK")||
      kw(&L->cur,"2QTUCK")||kw(&L->cur,"DTUCKWHEN")||kw(&L->cur,"2TUCKWHEN")){
    /* a b f → if f then b a b else a b  (conditional TUCK) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long f = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (f){
      if (vm->sp + 3 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
      vm->stack[vm->sp++] = b;
      vm->stack[vm->sp++] = a;
      vm->stack[vm->sp++] = b;
      var_set_num(vm,"LAST_N",b); vm->last_n=b;
    } else {
      vm->stack[vm->sp++] = a;
      vm->stack[vm->sp++] = b;
      var_set_num(vm,"LAST_N",b); vm->last_n=b;
    }
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-1 dual-stack conditionals ext3: DROTIF · DRROTIF (3-item rotate forms) */
  if (kw(&L->cur,"DROTIF")||kw(&L->cur,"2ROTIF")||kw(&L->cur,"S2ROTIF")||
      kw(&L->cur,"STACK2ROTIF")||kw(&L->cur,"PAIRROTIF")||kw(&L->cur,"DQROT")||
      kw(&L->cur,"2QROT")||kw(&L->cur,"DROTWHEN")||kw(&L->cur,"2ROTWHEN")){
    /* a b c f → if f then b c a else a b c  (conditional ROT) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long f = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (f){
      vm->stack[vm->sp++] = b;
      vm->stack[vm->sp++] = c;
      vm->stack[vm->sp++] = a;
      var_set_num(vm,"LAST_N",a); vm->last_n=a;
    } else {
      vm->stack[vm->sp++] = a;
      vm->stack[vm->sp++] = b;
      vm->stack[vm->sp++] = c;
      var_set_num(vm,"LAST_N",c); vm->last_n=c;
    }
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DRROTIF")||kw(&L->cur,"2RROTIF")||kw(&L->cur,"S2RROTIF")||
      kw(&L->cur,"STACK2RROTIF")||kw(&L->cur,"PAIRRROTIF")||kw(&L->cur,"DQRROT")||
      kw(&L->cur,"2QRROT")||kw(&L->cur,"DRROTWHEN")||kw(&L->cur,"2RROTWHEN")||
      kw(&L->cur,"DNROTIF")||kw(&L->cur,"2NROTIF")){
    /* a b c f → if f then c a b else a b c  (conditional RROT / -ROT) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long f = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (f){
      vm->stack[vm->sp++] = c;
      vm->stack[vm->sp++] = a;
      vm->stack[vm->sp++] = b;
      var_set_num(vm,"LAST_N",b); vm->last_n=b;
    } else {
      vm->stack[vm->sp++] = a;
      vm->stack[vm->sp++] = b;
      vm->stack[vm->sp++] = c;
      var_set_num(vm,"LAST_N",c); vm->last_n=c;
    }
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-1 dual-stack shared-flag gates: DSHGATE · DZEROIF (complete after per-lane DGATE) */
  if (kw(&L->cur,"DSHGATE")||kw(&L->cur,"2SHGATE")||kw(&L->cur,"S2SHGATE")||
      kw(&L->cur,"STACK2SHGATE")||kw(&L->cur,"PAIRSHGATE")||kw(&L->cur,"DGATES")||
      kw(&L->cur,"2GATES")||kw(&L->cur,"S2GATES")||kw(&L->cur,"PAIRGATES")||
      kw(&L->cur,"DZEROUNLESS")||kw(&L->cur,"2ZEROUNLESS")||kw(&L->cur,"S2ZEROUNLESS")||
      kw(&L->cur,"PAIRZEROUNLESS")){
    /* a b f → (f?a:0) (f?b:0)  shared-flag dual of DGATE (one mask for both lanes) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long f = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = f ? a : 0;
    long y = f ? b : 0;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DZEROIF")||kw(&L->cur,"2ZEROIF")||kw(&L->cur,"S2ZEROIF")||
      kw(&L->cur,"STACK2ZEROIF")||kw(&L->cur,"PAIRZEROIF")||kw(&L->cur,"DZAPIF")||
      kw(&L->cur,"2ZAPIF")||kw(&L->cur,"S2ZAPIF")||kw(&L->cur,"PAIRZAPIF")||
      kw(&L->cur,"DQZERO")||kw(&L->cur,"2QZERO")||kw(&L->cur,"DZEROWHEN")||
      kw(&L->cur,"2ZEROWHEN")){
    /* a b f → (f?0:a) (f?0:b)  shared-flag soft-kill / zero-if (inverse of DSHGATE) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long f = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = f ? 0 : a;
    long y = f ? 0 : b;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-6 dual-stack flow metrics: DAVG · DDIST · DHAMM (energy-style distance) */
  if (kw(&L->cur,"DAVG")||kw(&L->cur,"2AVG")||kw(&L->cur,"S2AVG")||
      kw(&L->cur,"STACK2AVG")||kw(&L->cur,"PAIRAVG")||kw(&L->cur,"DMEAN")||
      kw(&L->cur,"2MEAN")||
      kw(&L->cur,"DDIST")||kw(&L->cur,"2DIST")||kw(&L->cur,"S2DIST")||
      kw(&L->cur,"STACK2DIST")||kw(&L->cur,"PAIRDIST")||kw(&L->cur,"DABSDIFF")||
      kw(&L->cur,"2ABSDIFF")||
      kw(&L->cur,"DHAMM")||kw(&L->cur,"2HAMM")||kw(&L->cur,"S2HAMM")||
      kw(&L->cur,"STACK2HAMM")||kw(&L->cur,"PAIRHAMM")||kw(&L->cur,"DHAMMING")||
      kw(&L->cur,"2HAMMING")||kw(&L->cur,"DPOPDIFF")||kw(&L->cur,"2POPDIFF")){
    /* a b c d → f(a,c) f(b,d)
     * AVG: truncated mean (a+c)/2
     * DIST: |a-c|
     * HAMM: popcount(a^c) bit distance */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    int is_avg = (strcmp(op,"DAVG")==0 || strcmp(op,"2AVG")==0 || strcmp(op,"S2AVG")==0 ||
                  strcmp(op,"STACK2AVG")==0 || strcmp(op,"PAIRAVG")==0 ||
                  strcmp(op,"DMEAN")==0 || strcmp(op,"2MEAN")==0);
    int is_dist = (strcmp(op,"DDIST")==0 || strcmp(op,"2DIST")==0 || strcmp(op,"S2DIST")==0 ||
                   strcmp(op,"STACK2DIST")==0 || strcmp(op,"PAIRDIST")==0 ||
                   strcmp(op,"DABSDIFF")==0 || strcmp(op,"2ABSDIFF")==0);
    long x = 0, y = 0;
    if (is_avg){
      x = (a + c) / 2;
      y = (b + d) / 2;
    } else if (is_dist){
      long dx = a - c; if (dx < 0) dx = -dx;
      long dy = b - d; if (dy < 0) dy = -dy;
      x = dx; y = dy;
    } else {
      unsigned long ua = (unsigned long)(a ^ c), ub = (unsigned long)(b ^ d);
      while (ua){ x += (long)(ua & 1ul); ua >>= 1; }
      while (ub){ y += (long)(ub & 1ul); ub >>= 1; }
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* energy power/means peel: DSUMSQ·DDIFFSQ·DGEOM·DHARM·DRMS (G_dual_margin) */
  /* digit-6 dual-stack energy power: DSUMSQ · DDIFFSQ (building blocks for DRMS/error) */
  if (kw(&L->cur,"DSUMSQ")||kw(&L->cur,"2SUMSQ")||kw(&L->cur,"S2SUMSQ")||
      kw(&L->cur,"STACK2SUMSQ")||kw(&L->cur,"PAIRSUMSQ")||kw(&L->cur,"DSSQ")||
      kw(&L->cur,"2SSQ")||kw(&L->cur,"DPOW2SUM")||kw(&L->cur,"2POW2SUM")){
    /* a b c d → a*a+c*c  b*b+d*d  (energy sum-of-squares / power metric) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = a * a + c * c;
    long y = b * b + d * d;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DDIFFSQ")||kw(&L->cur,"2DIFFSQ")||kw(&L->cur,"S2DIFFSQ")||
      kw(&L->cur,"STACK2DIFFSQ")||kw(&L->cur,"PAIRDIFFSQ")||kw(&L->cur,"DSQDIFF")||
      kw(&L->cur,"2SQDIFF")||kw(&L->cur,"DERRSQ")||kw(&L->cur,"2ERRSQ")){
    /* a b c d → (a-c)² (b-d)²  energy residual / squared error */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long dx = a - c;
    long dy = b - d;
    long x = dx * dx;
    long y = dy * dy;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 dual-stack energy means: DGEOM · DHARM · DRMS (complete after DAVG) */
  if (kw(&L->cur,"DGEOM")||kw(&L->cur,"2GEOM")||kw(&L->cur,"S2GEOM")||
      kw(&L->cur,"STACK2GEOM")||kw(&L->cur,"PAIRGEOM")||kw(&L->cur,"DGEOMEAN")||
      kw(&L->cur,"2GEOMEAN")||
      kw(&L->cur,"DHARM")||kw(&L->cur,"2HARM")||kw(&L->cur,"S2HARM")||
      kw(&L->cur,"STACK2HARM")||kw(&L->cur,"PAIRHARM")||kw(&L->cur,"DHARMMEAN")||
      kw(&L->cur,"2HARMMEAN")||
      kw(&L->cur,"DRMS")||kw(&L->cur,"2RMS")||kw(&L->cur,"S2RMS")||
      kw(&L->cur,"STACK2RMS")||kw(&L->cur,"PAIRRMS")||kw(&L->cur,"DROOTMS")||
      kw(&L->cur,"2ROOTMS")){
    /* a b c d → f(a,c) f(b,d)
     * GEOM: floor(sqrt(a*c)) for a,c>=0 else 0 (geometric mean)
     * HARM: 2*a*c/(a+c) if a+c!=0 else 0 (harmonic mean)
     * RMS:  floor(sqrt((a*a+c*c)/2)) root-mean-square */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    int is_geom = (strcmp(op,"DGEOM")==0 || strcmp(op,"2GEOM")==0 || strcmp(op,"S2GEOM")==0 ||
                   strcmp(op,"STACK2GEOM")==0 || strcmp(op,"PAIRGEOM")==0 ||
                   strcmp(op,"DGEOMEAN")==0 || strcmp(op,"2GEOMEAN")==0);
    int is_harm = (strcmp(op,"DHARM")==0 || strcmp(op,"2HARM")==0 || strcmp(op,"S2HARM")==0 ||
                   strcmp(op,"STACK2HARM")==0 || strcmp(op,"PAIRHARM")==0 ||
                   strcmp(op,"DHARMMEAN")==0 || strcmp(op,"2HARMMEAN")==0);
    long x = 0, y = 0;
    if (is_geom){
      if (a >= 0 && c >= 0){
        unsigned long long p = (unsigned long long)a * (unsigned long long)c;
        unsigned long long t = 0, s = p + 1;
        if (s == 0){ /* p == ULLONG_MAX */
          t = 1ull << 32; /* isqrt ceiling bound for max */
          while (t * t > p) t--;
          while ((t + 1) * (t + 1) <= p) t++;
        } else {
          while (t + 1 <= p / (t + 1)) t++;
        }
        x = (long)t;
      }
      if (b >= 0 && d >= 0){
        unsigned long long p = (unsigned long long)b * (unsigned long long)d;
        unsigned long long t = 0;
        if (p == ~0ull){
          t = 1ull << 32;
          while (t * t > p) t--;
          while ((t + 1) * (t + 1) <= p) t++;
        } else {
          while (t + 1 <= p / (t + 1)) t++;
        }
        y = (long)t;
      }
    } else if (is_harm){
      long s1 = a + c;
      long s2 = b + d;
      if (s1 != 0) x = (2 * a * c) / s1;
      if (s2 != 0) y = (2 * b * d) / s2;
    } else {
      /* RMS */
      {
        long long aa = (long long)a * (long long)a;
        long long cc = (long long)c * (long long)c;
        unsigned long long sum = (unsigned long long)(aa + cc) / 2ull;
        unsigned long long t = 0;
        if (sum == ~0ull){
          t = 1ull << 32;
          while (t * t > sum) t--;
          while ((t + 1) * (t + 1) <= sum) t++;
        } else {
          while (t + 1 <= sum / (t + 1)) t++;
        }
        x = (long)t;
      }
      {
        long long bb = (long long)b * (long long)b;
        long long dd = (long long)d * (long long)d;
        unsigned long long sum = (unsigned long long)(bb + dd) / 2ull;
        unsigned long long t = 0;
        if (sum == ~0ull){
          t = 1ull << 32;
          while (t * t > sum) t--;
          while ((t + 1) * (t + 1) <= sum) t++;
        } else {
          while (t + 1 <= sum / (t + 1)) t++;
        }
        y = (long)t;
      }
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* RNG+sat energy peel: DRAND·DSATADD/SUB/MUL/DIV (G_dual_margin) */
  /* digit-6 dual-stack RNG + sat energy: DRAND · DSATADD · DSATSUB */
  if (kw(&L->cur,"DRAND")||kw(&L->cur,"2RAND")||kw(&L->cur,"S2RAND")||
      kw(&L->cur,"STACK2RAND")||kw(&L->cur,"PAIRRAND")||kw(&L->cur,"2RND")||
      kw(&L->cur,"DRND")){
    /* a b → rand[0,a) rand[0,b); max<=0 → 10 (same as SRAND) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long mb = vm->stack[vm->sp - 1];
    long ma = vm->stack[vm->sp - 2];
    if (ma < 1) ma = 10;
    if (mb < 1) mb = 10;
    uint32_t xg = vm->rng;
    xg ^= xg << 13; xg ^= xg >> 17; xg ^= xg << 5;
    if (!xg) xg = 1;
    long ra = (long)(xg % (uint32_t)ma);
    xg ^= xg << 13; xg ^= xg >> 17; xg ^= xg << 5;
    if (!xg) xg = 1;
    long rb = (long)(xg % (uint32_t)mb);
    vm->rng = xg;
    vm->stack[vm->sp - 2] = ra;
    vm->stack[vm->sp - 1] = rb;
    var_set_num(vm,"LAST_N",rb); vm->last_n=rb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSATADD")||kw(&L->cur,"2SATADD")||kw(&L->cur,"S2SATADD")||
      kw(&L->cur,"STACK2SATADD")||kw(&L->cur,"PAIRSATADD")||
      kw(&L->cur,"DSATSUB")||kw(&L->cur,"2SATSUB")||kw(&L->cur,"S2SATSUB")||
      kw(&L->cur,"STACK2SATSUB")||kw(&L->cur,"PAIRSATSUB")||
      kw(&L->cur,"DSATMUL")||kw(&L->cur,"2SATMUL")||kw(&L->cur,"S2SATMUL")||
      kw(&L->cur,"STACK2SATMUL")||kw(&L->cur,"PAIRSATMUL")||
      kw(&L->cur,"DSATDIV")||kw(&L->cur,"2SATDIV")||kw(&L->cur,"S2SATDIV")||
      kw(&L->cur,"STACK2SATDIV")||kw(&L->cur,"PAIRSATDIV")){
    /* a b c d → sat(a⋆c) sat(b⋆d) — energy-style saturating pair ALU */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    int is_add = (strcmp(op,"DSATADD")==0 || strcmp(op,"2SATADD")==0 ||
                  strcmp(op,"S2SATADD")==0 || strcmp(op,"STACK2SATADD")==0 ||
                  strcmp(op,"PAIRSATADD")==0);
    int is_sub = (strcmp(op,"DSATSUB")==0 || strcmp(op,"2SATSUB")==0 ||
                  strcmp(op,"S2SATSUB")==0 || strcmp(op,"STACK2SATSUB")==0 ||
                  strcmp(op,"PAIRSATSUB")==0);
    int is_mul = (strcmp(op,"DSATMUL")==0 || strcmp(op,"2SATMUL")==0 ||
                  strcmp(op,"S2SATMUL")==0 || strcmp(op,"STACK2SATMUL")==0 ||
                  strcmp(op,"PAIRSATMUL")==0);
    long x, y;
    if (is_add){
      if (c > 0 && a > LONG_MAX - c) x = LONG_MAX;
      else if (c < 0 && a < LONG_MIN - c) x = LONG_MIN;
      else x = a + c;
      if (d > 0 && b > LONG_MAX - d) y = LONG_MAX;
      else if (d < 0 && b < LONG_MIN - d) y = LONG_MIN;
      else y = b + d;
    } else if (is_sub){
      if (c > 0 && a < LONG_MIN + c) x = LONG_MIN;
      else if (c < 0 && a > LONG_MAX + c) x = LONG_MAX;
      else x = a - c;
      if (d > 0 && b < LONG_MIN + d) y = LONG_MIN;
      else if (d < 0 && b > LONG_MAX + d) y = LONG_MAX;
      else y = b - d;
    } else if (is_mul){
      /* DSATMUL */
      if (a == 0 || c == 0) x = 0;
      else {
        __int128 p = (__int128)a * (__int128)c;
        if (p > (__int128)LONG_MAX) x = LONG_MAX;
        else if (p < (__int128)LONG_MIN) x = LONG_MIN;
        else x = (long)p;
      }
      if (b == 0 || d == 0) y = 0;
      else {
        __int128 p = (__int128)b * (__int128)d;
        if (p > (__int128)LONG_MAX) y = LONG_MAX;
        else if (p < (__int128)LONG_MIN) y = LONG_MIN;
        else y = (long)p;
      }
    } else {
      /* DSATDIV — pair trunc-toward-zero; /0 → 0; LONG_MIN/-1 → LONG_MAX */
      if (c == 0) x = 0;
      else if (a == LONG_MIN && c == -1) x = LONG_MAX;
      else x = a / c;
      if (d == 0) y = 0;
      else if (b == LONG_MIN && d == -1) y = LONG_MAX;
      else y = b / d;
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-6 dual-stack energy wrap + hypot: DWRAP · DHYPOT */
  if (kw(&L->cur,"DWRAP")||kw(&L->cur,"2WRAP")||kw(&L->cur,"S2WRAP")||
      kw(&L->cur,"STACK2WRAP")||kw(&L->cur,"PAIRWRAP")||kw(&L->cur,"DWMOD")||
      kw(&L->cur,"2WMOD")||kw(&L->cur,"DWRAPMOD")||kw(&L->cur,"2WRAPMOD")||
      kw(&L->cur,"S2WRAPMOD")||kw(&L->cur,"PAIRWRAPMOD")){
    /* a b ma mb → wrap(a,ma) wrap(b,mb) in [0,m); m<=0 → 0 */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long mb = vm->stack[--vm->sp];
    long ma = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = 0, y = 0;
    if (ma > 0){ x = a % ma; if (x < 0) x += ma; }
    if (mb > 0){ y = b % mb; if (y < 0) y += mb; }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 dual-stack imm energy wrap: DWRAPN · DWMODN (shared modulus m; dual of SWMODN) */
  if (kw(&L->cur,"DWRAPN")||kw(&L->cur,"2WRAPN")||kw(&L->cur,"S2WRAPN")||
      kw(&L->cur,"STACK2WRAPN")||kw(&L->cur,"PAIRWRAPN")||kw(&L->cur,"DWMODN")||
      kw(&L->cur,"2WMODN")||kw(&L->cur,"S2WMODN")||kw(&L->cur,"PAIRWMODN")||
      kw(&L->cur,"DWRAPMODN")||kw(&L->cur,"DWRAPIMM")||kw(&L->cur,"PAIRWRAPIMM")){
    /* a b + m → wrap(a,m) wrap(b,m) in [0,m); m<=0 → 0,0 */
    lex_next(L);
    long m = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (m > 0){
      x = a % m; if (x < 0) x += m;
      y = b % m; if (y < 0) y += m;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DHYPOT")||kw(&L->cur,"2HYPOT")||kw(&L->cur,"S2HYPOT")||
      kw(&L->cur,"STACK2HYPOT")||kw(&L->cur,"PAIRHYPOT")||kw(&L->cur,"DHYP")||
      kw(&L->cur,"2HYP")||kw(&L->cur,"S2HYP")||kw(&L->cur,"PAIRHYP")){
    /* a b c d → isqrt(a²+c²) isqrt(b²+d²); energy-style Euclidean pair length */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = 0, y = 0;
    {
      __int128 s = (__int128)a * (__int128)a + (__int128)c * (__int128)c;
      if (s > 0){
        /* integer sqrt via binary search */
        __int128 lo = 0, hi = s;
        if (hi > (__int128)LONG_MAX) hi = (__int128)LONG_MAX;
        while (lo < hi){
          __int128 mid = lo + (hi - lo + 1) / 2;
          if (mid * mid <= s) lo = mid;
          else hi = mid - 1;
        }
        x = (long)lo;
      }
    }
    {
      __int128 s = (__int128)b * (__int128)b + (__int128)d * (__int128)d;
      if (s > 0){
        __int128 lo = 0, hi = s;
        if (hi > (__int128)LONG_MAX) hi = (__int128)LONG_MAX;
        while (lo < hi){
          __int128 mid = lo + (hi - lo + 1) / 2;
          if (mid * mid <= s) lo = mid;
          else hi = mid - 1;
        }
        y = (long)lo;
      }
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* energy mix peel: DPCT·DLERP (G_dual_margin) */
  /* digit-6 dual-stack energy mix: DPCT · DLERP */
  if (kw(&L->cur,"DPCT")||kw(&L->cur,"2PCT")||kw(&L->cur,"S2PCT")||
      kw(&L->cur,"STACK2PCT")||kw(&L->cur,"PAIRPCT")||kw(&L->cur,"DPERCENT")||
      kw(&L->cur,"2PERCENT")||kw(&L->cur,"S2PERCENT")||kw(&L->cur,"PAIRPERCENT")){
    /* a b c d → (a*100)/c (b*100)/d; /0 → 0  (energy percent of whole) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = c ? (a * 100 / c) : 0;
    long y = d ? (b * 100 / d) : 0;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DLERP")||kw(&L->cur,"2LERP")||kw(&L->cur,"S2LERP")||
      kw(&L->cur,"STACK2LERP")||kw(&L->cur,"PAIRLERP")||kw(&L->cur,"DMIX")||
      kw(&L->cur,"2MIX")||kw(&L->cur,"S2MIX")||kw(&L->cur,"PAIRMIX")){
    /* a b c d t → lerp(a,c,t) lerp(b,d,t) with t in 0..100 percent:
     * a + (c-a)*t/100 ; t clamped to [0,100] */
    lex_next(L);
    if (vm->sp < 5){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long t = vm->stack[--vm->sp];
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (t < 0) t = 0;
    if (t > 100) t = 100;
    long x = a + (c - a) * t / 100;
    long y = b + (d - b) * t / 100;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 dual-stack energy scale/clip: DSCALE · DCLIP100 */
  if (kw(&L->cur,"DSCALE")||kw(&L->cur,"2SCALE")||kw(&L->cur,"S2SCALE")||
      kw(&L->cur,"STACK2SCALE")||kw(&L->cur,"PAIRSCALE")||kw(&L->cur,"DSCL")||
      kw(&L->cur,"2SCL")){
    /* a b sa sb → a*sa/100  b*sb/100  (percent scale; 100=identity) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long sb = vm->stack[--vm->sp];
    long sa = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = a * sa / 100;
    long y = b * sb / 100;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DCLIP100")||kw(&L->cur,"2CLIP100")||kw(&L->cur,"S2CLIP100")||
      kw(&L->cur,"STACK2CLIP100")||kw(&L->cur,"PAIRCLIP100")||kw(&L->cur,"DCLIPPCT")||
      kw(&L->cur,"2CLIPPCT")||kw(&L->cur,"DENCLIP")||kw(&L->cur,"2ENCLIP")||
      kw(&L->cur,"PAIRCLIPPCT")){
    /* a b → clamp(a,0,100) clamp(b,0,100) energy-plane percent bound */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    if (a < 0) a = 0;
    if (a > 100) a = 100;
    if (b < 0) b = 0;
    if (b > 100) b = 100;
    vm->stack[vm->sp - 2] = a;
    vm->stack[vm->sp - 1] = b;
    var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-6 dual-stack energy unit/complement: DCLIP01 · DCOMP100 */
  if (kw(&L->cur,"DCLIP01")||kw(&L->cur,"2CLIP01")||kw(&L->cur,"S2CLIP01")||
      kw(&L->cur,"STACK2CLIP01")||kw(&L->cur,"PAIRCLIP01")||kw(&L->cur,"DUNIT")||
      kw(&L->cur,"2UNIT")||kw(&L->cur,"DCLAMP01")||kw(&L->cur,"2CLAMP01")||
      kw(&L->cur,"PAIRCLAMP01")){
    /* a b → clamp(a,0,1) clamp(b,0,1) unit-interval energy gate */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    if (a < 0) a = 0;
    if (a > 1) a = 1;
    if (b < 0) b = 0;
    if (b > 1) b = 1;
    vm->stack[vm->sp - 2] = a;
    vm->stack[vm->sp - 1] = b;
    var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DCOMP100")||kw(&L->cur,"2COMP100")||kw(&L->cur,"S2COMP100")||
      kw(&L->cur,"STACK2COMP100")||kw(&L->cur,"PAIRCOMP100")||kw(&L->cur,"DENCOMP")||
      kw(&L->cur,"2ENCOMP")||kw(&L->cur,"DINV100")||kw(&L->cur,"2INV100")||
      kw(&L->cur,"PAIRINV100")){
    /* a b → 100-a  100-b  energy-plane complement (remaining capacity) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 100 - a;
    long y = 100 - b;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 dual-stack energy step activation: DSTEP · DHEAVI */
  if (kw(&L->cur,"DSTEP")||kw(&L->cur,"2STEP")||kw(&L->cur,"S2STEP")||
      kw(&L->cur,"STACK2STEP")||kw(&L->cur,"PAIRSTEP")||kw(&L->cur,"DHEAVI")||
      kw(&L->cur,"2HEAVI")||kw(&L->cur,"DHEAVISIDE")||kw(&L->cur,"2HEAVISIDE")||
      kw(&L->cur,"DUNITSTEP")||kw(&L->cur,"2UNITSTEP")){
    /* a b → (a>0?1:0) (b>0?1:0)  unit-step / Heaviside energy gate (0 at zero) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a > 0) ? 1 : 0;
    long y = (b > 0) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-6 dual-stack energy soft activations: DLEAKY · DSOFTSIGN */
  if (kw(&L->cur,"DLEAKY")||kw(&L->cur,"2LEAKY")||kw(&L->cur,"S2LEAKY")||
      kw(&L->cur,"STACK2LEAKY")||kw(&L->cur,"PAIRLEAKY")||kw(&L->cur,"DLEAKYRELU")||
      kw(&L->cur,"2LEAKYRELU")||kw(&L->cur,"S2LEAKYRELU")||kw(&L->cur,"PAIRLEAKYRELU")||
      kw(&L->cur,"STACK2LEAKYRELU")){
    /* a b → leaky(a) leaky(b); leaky(x)= x>=0 ? x : x/4  (fixed 1/4 leak, trunc toward 0) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a >= 0) ? a : (a / 4);
    long y = (b >= 0) ? b : (b / 4);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSOFTSIGN")||kw(&L->cur,"2SOFTSIGN")||kw(&L->cur,"S2SOFTSIGN")||
      kw(&L->cur,"STACK2SOFTSIGN")||kw(&L->cur,"PAIRSOFTSIGN")||kw(&L->cur,"DSOFTSGN")||
      kw(&L->cur,"2SOFTSGN")||kw(&L->cur,"S2SOFTSGN")||kw(&L->cur,"PAIRSOFTSGN")||
      kw(&L->cur,"STACK2SOFTSGN")){
    /* a b → softsign100(a) softsign100(b)
     * softsign100(v) = sign(v) * floor(100*|v| / (100+|v|))  → energy plane (-100,100) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x, y;
    {
      if (a == 0) x = 0;
      else if (a == LONG_MIN) x = -100;
      else if (a > 0){
        if (a > LONG_MAX / 100) x = 100;
        else x = (100 * a) / (100 + a);
      } else {
        long aa = -a;
        if (aa > LONG_MAX / 100) x = -100;
        else x = -((100 * aa) / (100 + aa));
      }
    }
    {
      if (b == 0) y = 0;
      else if (b == LONG_MIN) y = -100;
      else if (b > 0){
        if (b > LONG_MAX / 100) y = 100;
        else y = (100 * b) / (100 + b);
      } else {
        long bb = -b;
        if (bb > LONG_MAX / 100) y = -100;
        else y = -((100 * bb) / (100 + bb));
      }
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 dual-stack energy invert + normalize: DINV · DNORM100 */
  if (kw(&L->cur,"DINV")||kw(&L->cur,"2INV")||kw(&L->cur,"S2INV")||
      kw(&L->cur,"STACK2INV")||kw(&L->cur,"PAIRINV")||kw(&L->cur,"DRECIP")||
      kw(&L->cur,"2RECIP")||kw(&L->cur,"S2RECIP")||kw(&L->cur,"STACK2RECIP")||
      kw(&L->cur,"PAIRRECIP")){
    /* a b → (a!=0?1/a:0) (b!=0?1/b:0)  integer reciprocal energy invert */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[vm->sp - 1];
    long a = vm->stack[vm->sp - 2];
    long x = a ? (1 / a) : 0;
    long y = b ? (1 / b) : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNORM100")||kw(&L->cur,"2NORM100")||kw(&L->cur,"S2NORM100")||
      kw(&L->cur,"STACK2NORM100")||kw(&L->cur,"PAIRNORM100")||kw(&L->cur,"DENORM")||
      kw(&L->cur,"2ENORM")||kw(&L->cur,"S2ENORM")||kw(&L->cur,"PAIRENORM")||
      kw(&L->cur,"DNORME")||kw(&L->cur,"2NORME")){
    /* a b → a*100/m  b*100/m  where m = max(|a|,|b|); m==0 → 0,0
     * maps pair onto energy plane with peak at ±100 */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[vm->sp - 1];
    long a = vm->stack[vm->sp - 2];
    long aa = a < 0 ? -a : a;
    long bb = b < 0 ? -b : b;
    long m = aa > bb ? aa : bb;
    long x = 0, y = 0;
    if (m != 0){
      x = a * 100 / m;
      y = b * 100 / m;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 dual-stack energy activations: DRELU6 · DDEADZ (gate/noise) */
  if (kw(&L->cur,"DRELU6")||kw(&L->cur,"2RELU6")||kw(&L->cur,"S2RELU6")||
      kw(&L->cur,"STACK2RELU6")||kw(&L->cur,"PAIRRELU6")||kw(&L->cur,"DCLAMP6")||
      kw(&L->cur,"2CLAMP6")||kw(&L->cur,"S2CLAMP6")||kw(&L->cur,"PAIRCLAMP6")){
    /* a b → clamp(a,0,6) clamp(b,0,6) — classic ReLU6 energy activation */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    if (a < 0) a = 0;
    if (a > 6) a = 6;
    if (b < 0) b = 0;
    if (b > 6) b = 6;
    vm->stack[vm->sp - 2] = a;
    vm->stack[vm->sp - 1] = b;
    var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DDEADZ")||kw(&L->cur,"2DEADZ")||kw(&L->cur,"S2DEADZ")||
      kw(&L->cur,"STACK2DEADZ")||kw(&L->cur,"PAIRDEADZ")||kw(&L->cur,"DDEADZONE")||
      kw(&L->cur,"2DEADZONE")||kw(&L->cur,"S2DEADZONE")||kw(&L->cur,"PAIRDEADZONE")||
      kw(&L->cur,"DDEAD")||kw(&L->cur,"2DEAD")){
    /* a b za zb → (|a|<=za?0:a) (|b|<=zb?0:b); z<0 treated as 0
     * energy-style deadzone / noise gate on dual channel */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long zb = vm->stack[--vm->sp];
    long za = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (za < 0) za = 0;
    if (zb < 0) zb = 0;
    long aa = a < 0 ? -a : a;
    long bb = b < 0 ? -b : b;
    long x = (aa <= za) ? 0 : a;
    long y = (bb <= zb) ? 0 : b;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  if (kw(&L->cur,"DRANDBITS")||kw(&L->cur,"2RANDBITS")||kw(&L->cur,"S2RANDBITS")||
      kw(&L->cur,"STACK2RANDBITS")||kw(&L->cur,"PAIRRANDBITS")||kw(&L->cur,"2RBITS")||
      kw(&L->cur,"DRBITS")){
    /* a b → U[0, 2^a) U[0, 2^b); widths clamped 0..62 (0 → 0) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long nb = vm->stack[vm->sp - 1];
    long na = vm->stack[vm->sp - 2];
    if (na < 0) na = 0;
    if (nb < 0) nb = 0;
    if (na > 62) na = 62;
    if (nb > 62) nb = 62;
    uint32_t xg = vm->rng;
    long ra = 0, rb = 0;
    if (na > 0){
      unsigned long span = 1UL << (unsigned)na;
      xg ^= xg << 13; xg ^= xg >> 17; xg ^= xg << 5;
      if (!xg) xg = 1;
      /* mix two xorshift draws for wider spans */
      uint32_t lo = xg;
      xg ^= xg << 13; xg ^= xg >> 17; xg ^= xg << 5;
      if (!xg) xg = 1;
      unsigned long long u = ((unsigned long long)xg << 32) | (unsigned long long)lo;
      ra = (long)(u % span);
    }
    if (nb > 0){
      unsigned long span = 1UL << (unsigned)nb;
      xg ^= xg << 13; xg ^= xg >> 17; xg ^= xg << 5;
      if (!xg) xg = 1;
      uint32_t lo = xg;
      xg ^= xg << 13; xg ^= xg >> 17; xg ^= xg << 5;
      if (!xg) xg = 1;
      unsigned long long u = ((unsigned long long)xg << 32) | (unsigned long long)lo;
      rb = (long)(u % span);
    }
    vm->rng = xg;
    vm->stack[vm->sp - 2] = ra;
    vm->stack[vm->sp - 1] = rb;
    var_set_num(vm,"LAST_N",rb); vm->last_n=rb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DRANDRANGE")||kw(&L->cur,"2RANDRANGE")||kw(&L->cur,"S2RANDRANGE")||
      kw(&L->cur,"STACK2RANDRANGE")||kw(&L->cur,"PAIRRANDRANGE")||kw(&L->cur,"2RANDIN")||
      kw(&L->cur,"DRANDIN")||kw(&L->cur,"2RANDBETWEEN")){
    /* a b c d → uniform[a,c] uniform[b,d] inclusive; swap ends if inverted */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long lo1 = a, hi1 = c, lo2 = b, hi2 = d;
    if (lo1 > hi1){ long t = lo1; lo1 = hi1; hi1 = t; }
    if (lo2 > hi2){ long t = lo2; lo2 = hi2; hi2 = t; }
    uint32_t xg = vm->rng;
    long span1 = hi1 - lo1 + 1;
    if (span1 < 1) span1 = 1;
    xg ^= xg << 13; xg ^= xg >> 17; xg ^= xg << 5;
    if (!xg) xg = 1;
    long x = lo1 + (long)(xg % (uint32_t)span1);
    long span2 = hi2 - lo2 + 1;
    if (span2 < 1) span2 = 1;
    xg ^= xg << 13; xg ^= xg >> 17; xg ^= xg << 5;
    if (!xg) xg = 1;
    long y = lo2 + (long)(xg % (uint32_t)span2);
    vm->rng = xg;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack 3-way compare: DCMP · DUCMP (−1/0/+1) */
  if (kw(&L->cur,"DCMP")||kw(&L->cur,"2CMP")||kw(&L->cur,"S2CMP")||
      kw(&L->cur,"STACK2CMP")||kw(&L->cur,"PAIRCMP")||kw(&L->cur,"DICMP")||
      kw(&L->cur,"2ICMP")||kw(&L->cur,"DCMP3")||kw(&L->cur,"2CMP3")||
      kw(&L->cur,"DUCMP")||kw(&L->cur,"2UCMP")||kw(&L->cur,"S2UCMP")||
      kw(&L->cur,"STACK2UCMP")||kw(&L->cur,"PAIRUCMP")||kw(&L->cur,"DUCMP3")||
      kw(&L->cur,"2UCMP3")){
    /* a b c d → cmp(a,c) cmp(b,d) as −1 / 0 / +1
     * DCMP signed; DUCMP unsigned */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_u = (strcmp(op,"DUCMP")==0 || strcmp(op,"2UCMP")==0 ||
                strcmp(op,"S2UCMP")==0 || strcmp(op,"STACK2UCMP")==0 ||
                strcmp(op,"PAIRUCMP")==0 || strcmp(op,"DUCMP3")==0 ||
                strcmp(op,"2UCMP3")==0);
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = 0, y = 0;
    if (is_u){
      unsigned long ua = (unsigned long)a, uc = (unsigned long)c;
      unsigned long ub = (unsigned long)b, ud = (unsigned long)d;
      if (ua < uc) x = -1; else if (ua > uc) x = 1; else x = 0;
      if (ub < ud) y = -1; else if (ub > ud) y = 1; else y = 0;
    } else {
      if (a < c) x = -1; else if (a > c) x = 1; else x = 0;
      if (b < d) y = -1; else if (b > d) y = 1; else y = 0;
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack unary control: DINC · DDEC · DNOT · DEQZ · DNEZ */
  if (kw(&L->cur,"DINC")||kw(&L->cur,"2INC")||kw(&L->cur,"S2INC")||
      kw(&L->cur,"STACK2INC")||kw(&L->cur,"PAIRINC")||
      kw(&L->cur,"DDEC")||kw(&L->cur,"2DEC")||kw(&L->cur,"S2DEC")||
      kw(&L->cur,"STACK2DEC")||kw(&L->cur,"PAIRDEC")||
      kw(&L->cur,"DNOT")||kw(&L->cur,"2NOT")||kw(&L->cur,"S2NOT")||
      kw(&L->cur,"STACK2NOT")||kw(&L->cur,"PAIRNOT")||
      kw(&L->cur,"DEQZ")||kw(&L->cur,"2EQZ")||kw(&L->cur,"S2EQZ")||
      kw(&L->cur,"STACK2EQZ")||kw(&L->cur,"PAIREQZ")||
      kw(&L->cur,"DNEZ")||kw(&L->cur,"2NEZ")||kw(&L->cur,"S2NEZ")||
      kw(&L->cur,"STACK2NEZ")||kw(&L->cur,"PAIRNEZ")){
    /* a b → f(a) f(b) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x, y;
    int is_inc = (strcmp(op,"DINC")==0 || strcmp(op,"2INC")==0 || strcmp(op,"S2INC")==0 ||
                  strcmp(op,"STACK2INC")==0 || strcmp(op,"PAIRINC")==0);
    int is_dec = (strcmp(op,"DDEC")==0 || strcmp(op,"2DEC")==0 || strcmp(op,"S2DEC")==0 ||
                  strcmp(op,"STACK2DEC")==0 || strcmp(op,"PAIRDEC")==0);
    int is_not = (strcmp(op,"DNOT")==0 || strcmp(op,"2NOT")==0 || strcmp(op,"S2NOT")==0 ||
                  strcmp(op,"STACK2NOT")==0 || strcmp(op,"PAIRNOT")==0);
    int is_eqz = (strcmp(op,"DEQZ")==0 || strcmp(op,"2EQZ")==0 || strcmp(op,"S2EQZ")==0 ||
                  strcmp(op,"STACK2EQZ")==0 || strcmp(op,"PAIREQZ")==0);
    /* else DNEZ */
    if (is_inc){ x = a + 1; y = b + 1; }
    else if (is_dec){ x = a - 1; y = b - 1; }
    else if (is_not){ x = ~a; y = ~b; }
    else if (is_eqz){ x = (a == 0) ? 1 : 0; y = (b == 0) ? 1 : 0; }
    else { x = (a != 0) ? 1 : 0; y = (b != 0) ? 1 : 0; }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-1 dual-stack unary predicates: DODD · DEVEN · DLTZ · DGTZ · DLEZ · DGEZ */
  if (kw(&L->cur,"DODD")||kw(&L->cur,"2ODD")||kw(&L->cur,"S2ODD")||
      kw(&L->cur,"STACK2ODD")||kw(&L->cur,"PAIRODD")||
      kw(&L->cur,"DEVEN")||kw(&L->cur,"2EVEN")||kw(&L->cur,"S2EVEN")||
      kw(&L->cur,"STACK2EVEN")||kw(&L->cur,"PAIREVEN")||
      kw(&L->cur,"DLTZ")||kw(&L->cur,"2LTZ")||kw(&L->cur,"S2LTZ")||
      kw(&L->cur,"STACK2LTZ")||kw(&L->cur,"PAIRLTZ")||
      kw(&L->cur,"DGTZ")||kw(&L->cur,"2GTZ")||kw(&L->cur,"S2GTZ")||
      kw(&L->cur,"STACK2GTZ")||kw(&L->cur,"PAIRGTZ")||
      kw(&L->cur,"DLEZ")||kw(&L->cur,"2LEZ")||kw(&L->cur,"S2LEZ")||
      kw(&L->cur,"STACK2LEZ")||kw(&L->cur,"PAIRLEZ")||
      kw(&L->cur,"DGEZ")||kw(&L->cur,"2GEZ")||kw(&L->cur,"S2GEZ")||
      kw(&L->cur,"STACK2GEZ")||kw(&L->cur,"PAIRGEZ")){
    /* a b → pred(a) pred(b) as 0/1 */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_odd = (strcmp(op,"DODD")==0 || strcmp(op,"2ODD")==0 || strcmp(op,"S2ODD")==0 ||
                  strcmp(op,"STACK2ODD")==0 || strcmp(op,"PAIRODD")==0);
    int is_even = (strcmp(op,"DEVEN")==0 || strcmp(op,"2EVEN")==0 || strcmp(op,"S2EVEN")==0 ||
                   strcmp(op,"STACK2EVEN")==0 || strcmp(op,"PAIREVEN")==0);
    int is_ltz = (strcmp(op,"DLTZ")==0 || strcmp(op,"2LTZ")==0 || strcmp(op,"S2LTZ")==0 ||
                  strcmp(op,"STACK2LTZ")==0 || strcmp(op,"PAIRLTZ")==0);
    int is_gtz = (strcmp(op,"DGTZ")==0 || strcmp(op,"2GTZ")==0 || strcmp(op,"S2GTZ")==0 ||
                  strcmp(op,"STACK2GTZ")==0 || strcmp(op,"PAIRGTZ")==0);
    int is_lez = (strcmp(op,"DLEZ")==0 || strcmp(op,"2LEZ")==0 || strcmp(op,"S2LEZ")==0 ||
                  strcmp(op,"STACK2LEZ")==0 || strcmp(op,"PAIRLEZ")==0);
    long x, y;
    if (is_odd){ x = (a & 1L) ? 1 : 0; y = (b & 1L) ? 1 : 0; }
    else if (is_even){ x = (a & 1L) ? 0 : 1; y = (b & 1L) ? 0 : 1; }
    else if (is_ltz){ x = (a < 0) ? 1 : 0; y = (b < 0) ? 1 : 0; }
    else if (is_gtz){ x = (a > 0) ? 1 : 0; y = (b > 0) ? 1 : 0; }
    else if (is_lez){ x = (a <= 0) ? 1 : 0; y = (b <= 0) ? 1 : 0; }
    else { x = (a >= 0) ? 1 : 0; y = (b >= 0) ? 1 : 0; }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack control gates: DTHRESH · DGATE */
  if (kw(&L->cur,"DTHRESH")||kw(&L->cur,"2THRESH")||kw(&L->cur,"S2THRESH")||
      kw(&L->cur,"STACK2THRESH")||kw(&L->cur,"PAIRTHRESH")||kw(&L->cur,"DTH")||
      kw(&L->cur,"2TH")){
    /* a b ta tb → (a>=ta?1:0) (b>=tb?1:0)  dual threshold predicates */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long tb = vm->stack[--vm->sp];
    long ta = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = (a >= ta) ? 1 : 0;
    long y = (b >= tb) ? 1 : 0;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGATE")||kw(&L->cur,"2GATE")||kw(&L->cur,"S2GATE")||
      kw(&L->cur,"STACK2GATE")||kw(&L->cur,"PAIRGATE")||kw(&L->cur,"DANDIF")||
      kw(&L->cur,"2ANDIF")||kw(&L->cur,"S2ANDIF")||kw(&L->cur,"PAIRANDIF")){
    /* a b ga gb → (ga!=0?a:0) (gb!=0?b:0)  dual boolean gate / mask pass */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long gb = vm->stack[--vm->sp];
    long ga = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = ga ? a : 0;
    long y = gb ? b : 0;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack unary control ext: DRELU · DCOPYSIGN */
  if (kw(&L->cur,"DRELU")||kw(&L->cur,"2RELU")||kw(&L->cur,"S2RELU")||
      kw(&L->cur,"STACK2RELU")||kw(&L->cur,"PAIRRELU")||kw(&L->cur,"DCLAMP0")||
      kw(&L->cur,"2CLAMP0")||kw(&L->cur,"S2CLAMP0")||kw(&L->cur,"PAIRCLAMP0")){
    /* a b → max(0,a) max(0,b)  (ReLU / clamp-at-zero) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    if (a < 0) a = 0;
    if (b < 0) b = 0;
    vm->stack[vm->sp - 2] = a;
    vm->stack[vm->sp - 1] = b;
    var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DCOPYSIGN")||kw(&L->cur,"2COPYSIGN")||kw(&L->cur,"S2COPYSIGN")||
      kw(&L->cur,"STACK2COPYSIGN")||kw(&L->cur,"PAIRCOPYSIGN")||kw(&L->cur,"DCSIGN")||
      kw(&L->cur,"2CSIGN")||kw(&L->cur,"S2CSIGN")||kw(&L->cur,"PAIRCSIGN")){
    /* a b sa sb → |a| with sign of sa, |b| with sign of sb; zero sign → + */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long sb = vm->stack[--vm->sp];
    long sa = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long ax = a < 0 ? -a : a;
    long bx = b < 0 ? -b : b;
    long x = (sa < 0) ? -ax : ax;
    long y = (sb < 0) ? -bx : bx;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* digit-1 dual-stack select: DMEDIAN · DMAXABS · DMINABS */
  if (kw(&L->cur,"DMEDIAN")||kw(&L->cur,"2MEDIAN")||kw(&L->cur,"S2MEDIAN")||
      kw(&L->cur,"STACK2MEDIAN")||kw(&L->cur,"PAIRMEDIAN")||kw(&L->cur,"DMID3")||
      kw(&L->cur,"2MID3")||kw(&L->cur,"DMED")||kw(&L->cur,"2MED")||
      kw(&L->cur,"S2MED")||kw(&L->cur,"PAIRMED")){
    /* a b c d e f → med(a,c,e) med(b,d,f) */
    lex_next(L);
    if (vm->sp < 6){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long f = vm->stack[--vm->sp];
    long e = vm->stack[--vm->sp];
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x1 = a, y1 = c, z1 = e;
    if (x1 > y1){ long t=x1; x1=y1; y1=t; }
    if (y1 > z1){ long t=y1; y1=z1; z1=t; }
    if (x1 > y1){ long t=x1; x1=y1; y1=t; }
    long x2 = b, y2 = d, z2 = f;
    if (x2 > y2){ long t=x2; x2=y2; y2=t; }
    if (y2 > z2){ long t=y2; y2=z2; z2=t; }
    if (x2 > y2){ long t=x2; x2=y2; y2=t; }
    vm->stack[vm->sp++] = y1;
    vm->stack[vm->sp++] = y2;
    var_set_num(vm,"LAST_N",y2); vm->last_n=y2;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMAXABS")||kw(&L->cur,"2MAXABS")||kw(&L->cur,"S2MAXABS")||
      kw(&L->cur,"STACK2MAXABS")||kw(&L->cur,"PAIRMAXABS")||kw(&L->cur,"DABSMAX")||
      kw(&L->cur,"2ABSMAX")||
      kw(&L->cur,"DMINABS")||kw(&L->cur,"2MINABS")||kw(&L->cur,"S2MINABS")||
      kw(&L->cur,"STACK2MINABS")||kw(&L->cur,"PAIRMINABS")||kw(&L->cur,"DABSMIN")||
      kw(&L->cur,"2ABSMIN")){
    /* a b c d → max/min(|a|,|c|) max/min(|b|,|d|) */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_max = (strcmp(op,"DMAXABS")==0 || strcmp(op,"2MAXABS")==0 ||
                  strcmp(op,"S2MAXABS")==0 || strcmp(op,"STACK2MAXABS")==0 ||
                  strcmp(op,"PAIRMAXABS")==0 || strcmp(op,"DABSMAX")==0 ||
                  strcmp(op,"2ABSMAX")==0);
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long aa = a < 0 ? -a : a;
    long cc = c < 0 ? -c : c;
    long bb = b < 0 ? -b : b;
    long dd = d < 0 ? -d : d;
    long x, y;
    if (is_max){
      x = aa > cc ? aa : cc;
      y = bb > dd ? bb : dd;
    } else {
      x = aa < cc ? aa : cc;
      y = bb < dd ? bb : dd;
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack rotate: DROL · DROR (G peel from dual) */
  if (kw(&L->cur,"DROL")||kw(&L->cur,"2ROL")||kw(&L->cur,"S2ROL")||
      kw(&L->cur,"STACK2ROL")||kw(&L->cur,"PAIRROL")||kw(&L->cur,"DROTL")||
      kw(&L->cur,"DROR")||kw(&L->cur,"2ROR")||kw(&L->cur,"S2ROR")||
      kw(&L->cur,"STACK2ROR")||kw(&L->cur,"PAIRROR")||kw(&L->cur,"DROTR")){
    /* a b c d → rot(a,c) rot(b,d); amounts mod 64, neg→0 */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (c < 0) c = 0;
    if (d < 0) d = 0;
    unsigned uc = (unsigned)(c & 63);
    unsigned ud = (unsigned)(d & 63);
    int is_rol = (strcmp(op,"DROL")==0 || strcmp(op,"2ROL")==0 || strcmp(op,"S2ROL")==0 ||
                  strcmp(op,"STACK2ROL")==0 || strcmp(op,"PAIRROL")==0 ||
                  strcmp(op,"DROTL")==0);
    unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
    long x, y;
    if (is_rol){
      x = (uc == 0) ? a : (long)((ua << uc) | (ua >> (64u - uc)));
      y = (ud == 0) ? b : (long)((ub << ud) | (ub >> (64u - ud)));
    } else {
      x = (uc == 0) ? a : (long)((ua >> uc) | (ua << (64u - uc)));
      y = (ud == 0) ? b : (long)((ub >> ud) | (ub << (64u - ud)));
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 dual-stack fixed-width rotate: DROL4 · DROR4 · DROL8 · DROR8 · DROL16 · DROR16 */
  if (kw(&L->cur,"DROL4")||kw(&L->cur,"2ROL4")||kw(&L->cur,"S2ROL4")||
      kw(&L->cur,"STACK2ROL4")||kw(&L->cur,"PAIRROL4")||kw(&L->cur,"DROTL4")||
      kw(&L->cur,"DROR4")||kw(&L->cur,"2ROR4")||kw(&L->cur,"S2ROR4")||
      kw(&L->cur,"STACK2ROR4")||kw(&L->cur,"PAIRROR4")||kw(&L->cur,"DROTR4")||
      kw(&L->cur,"DROL8")||kw(&L->cur,"2ROL8")||kw(&L->cur,"S2ROL8")||
      kw(&L->cur,"STACK2ROL8")||kw(&L->cur,"PAIRROL8")||kw(&L->cur,"DROTL8")||
      kw(&L->cur,"DROR8")||kw(&L->cur,"2ROR8")||kw(&L->cur,"S2ROR8")||
      kw(&L->cur,"STACK2ROR8")||kw(&L->cur,"PAIRROR8")||kw(&L->cur,"DROTR8")||
      kw(&L->cur,"DROL16")||kw(&L->cur,"2ROL16")||kw(&L->cur,"S2ROL16")||
      kw(&L->cur,"STACK2ROL16")||kw(&L->cur,"PAIRROL16")||kw(&L->cur,"DROTL16")||
      kw(&L->cur,"DROR16")||kw(&L->cur,"2ROR16")||kw(&L->cur,"S2ROR16")||
      kw(&L->cur,"STACK2ROR16")||kw(&L->cur,"PAIRROR16")||kw(&L->cur,"DROTR16")){
    /* a b c d → rot_w(a,c) rot_w(b,d); w∈{4,8,16}; amount mod w, neg→0; result masked */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (c < 0) c = 0;
    if (d < 0) d = 0;
    int is_rol = (strstr(op,"ROL") != NULL || strstr(op,"ROTL") != NULL);
    /* width: prefer 16, then 8, else 4 (check longest digit suffix first) */
    int is_w16 = (strstr(op,"16") != NULL);
    int is_w8 = (!is_w16 && strstr(op,"8") != NULL);
    unsigned bits = is_w16 ? 16u : (is_w8 ? 8u : 4u);
    unsigned mask = is_w16 ? 0xFFFFu : (is_w8 ? 0xFFu : 0xFu);
    unsigned uc = (unsigned)c & (bits - 1u);
    unsigned ud = (unsigned)d & (bits - 1u);
    unsigned wa = (unsigned)a & mask, wb = (unsigned)b & mask;
    long x, y;
    if (is_rol){
      x = (uc == 0) ? (long)wa : (long)(((wa << uc) | (wa >> (bits - uc))) & mask);
      y = (ud == 0) ? (long)wb : (long)(((wb << ud) | (wb >> (bits - ud))) & mask);
    } else {
      x = (uc == 0) ? (long)wa : (long)(((wa >> uc) | (wa << (bits - uc))) & mask);
      y = (ud == 0) ? (long)wb : (long)(((wb >> ud) | (wb << (bits - ud))) & mask);
    }
    (void)is_rol; /* used above */
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 dual-stack nibble swap: DNIBSWAP (low-byte nibble exchange) */
  if (kw(&L->cur,"DNIBSWAP")||kw(&L->cur,"2NIBSWAP")||kw(&L->cur,"S2NIBSWAP")||
      kw(&L->cur,"STACK2NIBSWAP")||kw(&L->cur,"PAIRNIBSWAP")||kw(&L->cur,"DSWAPNIB")||
      kw(&L->cur,"2SWAPNIB")||kw(&L->cur,"DNIBXCHG")||kw(&L->cur,"2NIBXCHG")){
    /* a b → swap_lo_nibbles(a) swap_lo_nibbles(b); result low byte only
     * ((x&0xF)<<4)|((x>>4)&0xF) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    unsigned ua = (unsigned)a & 0xFFu;
    unsigned ub = (unsigned)b & 0xFFu;
    long x = (long)(((ua & 0x0Fu) << 4) | ((ua >> 4) & 0x0Fu));
    long y = (long)(((ub & 0x0Fu) << 4) | ((ub >> 4) & 0x0Fu));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DWITHIN")||kw(&L->cur,"2WITHIN")||kw(&L->cur,"S2WITHIN")||
      kw(&L->cur,"STACK2WITHIN")||kw(&L->cur,"PAIRWITHIN")||
      kw(&L->cur,"DBETWEEN")||kw(&L->cur,"2BETWEEN")||kw(&L->cur,"S2BETWEEN")||
      kw(&L->cur,"STACK2BETWEEN")||kw(&L->cur,"PAIRBETWEEN")||kw(&L->cur,"DINRANGE")){
    /* a b lo hi → inrange(a) inrange(b); WITHIN half-open [lo,hi), BETWEEN closed [lo,hi] */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    int is_within = (strcmp(op,"DWITHIN")==0 || strcmp(op,"2WITHIN")==0 ||
                     strcmp(op,"S2WITHIN")==0 || strcmp(op,"STACK2WITHIN")==0 ||
                     strcmp(op,"PAIRWITHIN")==0);
    long x, y;
    if (is_within){
      /* Forth WITHIN: lo <= n < hi (no swap) */
      x = (a >= lo && a < hi) ? 1 : 0;
      y = (b >= lo && b < hi) ? 1 : 0;
    } else {
      long lo2 = lo, hi2 = hi;
      if (lo2 > hi2){ long t = lo2; lo2 = hi2; hi2 = t; }
      x = (a >= lo2 && a <= hi2) ? 1 : 0;
      y = (b >= lo2 && b <= hi2) ? 1 : 0;
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
