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
  return 0;
}
