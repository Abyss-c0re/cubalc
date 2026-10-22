/* CubalC lang — lang_ops_dual_clip.c
 * DUAL clip/sext/rotate plane: DCLIP32…DROL32/DROR32 (G peel from dual).
 * Mesh vote G_split_dual_cell · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_dual_clip.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

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
  return 0;
}
