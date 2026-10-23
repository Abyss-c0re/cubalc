/* CubalC lang — lang_ops_dual_datapath.c
 * DUAL datapath: DCLIP4·DSEXT4·DZEXT4·DCLIP8·DCLIP16·DSEXT8·DSEXT16·DZEXT8·DZEXT16 (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_datapath.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_datapath(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-9 dual-stack data-path nibble: DCLIP4 · DSEXT4 · DZEXT4 */
  if (kw(&L->cur,"DCLIP4")||kw(&L->cur,"2CLIP4")||kw(&L->cur,"S2CLIP4")||
      kw(&L->cur,"STACK2CLIP4")||kw(&L->cur,"PAIRCLIP4")||kw(&L->cur,"DCLIPN")||
      kw(&L->cur,"2CLIPN")||
      kw(&L->cur,"DSEXT4")||kw(&L->cur,"2SEXT4")||kw(&L->cur,"S2SEXT4")||
      kw(&L->cur,"STACK2SEXT4")||kw(&L->cur,"PAIRSEXT4")||kw(&L->cur,"DSEXTN")||
      kw(&L->cur,"2SEXTN")||
      kw(&L->cur,"DZEXT4")||kw(&L->cur,"2ZEXT4")||kw(&L->cur,"S2ZEXT4")||
      kw(&L->cur,"STACK2ZEXT4")||kw(&L->cur,"PAIRZEXT4")||kw(&L->cur,"DZEXTN")||
      kw(&L->cur,"2ZEXTN")||kw(&L->cur,"DZEROEXT4")){
    /* a b → f(a) f(b)
     * CLIP4: clamp to unsigned nibble [0,15]
     * SEXT4: sign-extend low 4 bits
     * ZEXT4: zero-extend low 4 bits */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_clip = (strcmp(op,"DCLIP4")==0 || strcmp(op,"2CLIP4")==0 ||
                   strcmp(op,"S2CLIP4")==0 || strcmp(op,"STACK2CLIP4")==0 ||
                   strcmp(op,"PAIRCLIP4")==0 || strcmp(op,"DCLIPN")==0 ||
                   strcmp(op,"2CLIPN")==0);
    int is_sext = (strcmp(op,"DSEXT4")==0 || strcmp(op,"2SEXT4")==0 ||
                   strcmp(op,"S2SEXT4")==0 || strcmp(op,"STACK2SEXT4")==0 ||
                   strcmp(op,"PAIRSEXT4")==0 || strcmp(op,"DSEXTN")==0 ||
                   strcmp(op,"2SEXTN")==0);
    long x, y;
    if (is_clip){
      x = a < 0 ? 0 : (a > 15 ? 15 : a);
      y = b < 0 ? 0 : (b > 15 ? 15 : b);
    } else if (is_sext){
      x = a & 0xFL; if (x & 0x8L) x |= ~0xFL;
      y = b & 0xFL; if (y & 0x8L) y |= ~0xFL;
    } else {
      x = a & 0xFL;
      y = b & 0xFL;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 dual-stack data-path: DCLIP8 · DCLIP16 · DSEXT8 · DSEXT16 */
  if (kw(&L->cur,"DCLIP8")||kw(&L->cur,"2CLIP8")||kw(&L->cur,"S2CLIP8")||
      kw(&L->cur,"STACK2CLIP8")||kw(&L->cur,"PAIRCLIP8")||
      kw(&L->cur,"DCLIP16")||kw(&L->cur,"2CLIP16")||kw(&L->cur,"S2CLIP16")||
      kw(&L->cur,"STACK2CLIP16")||kw(&L->cur,"PAIRCLIP16")||
      kw(&L->cur,"DSEXT8")||kw(&L->cur,"2SEXT8")||kw(&L->cur,"S2SEXT8")||
      kw(&L->cur,"STACK2SEXT8")||kw(&L->cur,"PAIRSEXT8")||kw(&L->cur,"DSEXTB")||
      kw(&L->cur,"2SEXTB")||
      kw(&L->cur,"DSEXT16")||kw(&L->cur,"2SEXT16")||kw(&L->cur,"S2SEXT16")||
      kw(&L->cur,"STACK2SEXT16")||kw(&L->cur,"PAIRSEXT16")||kw(&L->cur,"DSEXTW")||
      kw(&L->cur,"2SEXTW")){
    /* a b → f(a) f(b)
     * CLIP8: clamp to [0,255]; CLIP16: [0,65535]
     * SEXT8/16: sign-extend low 8/16 bits */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_c8 = (strcmp(op,"DCLIP8")==0 || strcmp(op,"2CLIP8")==0 || strcmp(op,"S2CLIP8")==0 ||
                 strcmp(op,"STACK2CLIP8")==0 || strcmp(op,"PAIRCLIP8")==0);
    int is_c16 = (strcmp(op,"DCLIP16")==0 || strcmp(op,"2CLIP16")==0 || strcmp(op,"S2CLIP16")==0 ||
                  strcmp(op,"STACK2CLIP16")==0 || strcmp(op,"PAIRCLIP16")==0);
    int is_s8 = (strcmp(op,"DSEXT8")==0 || strcmp(op,"2SEXT8")==0 || strcmp(op,"S2SEXT8")==0 ||
                 strcmp(op,"STACK2SEXT8")==0 || strcmp(op,"PAIRSEXT8")==0 ||
                 strcmp(op,"DSEXTB")==0 || strcmp(op,"2SEXTB")==0);
    long x, y;
    if (is_c8){
      x = a < 0 ? 0 : (a > 255 ? 255 : a);
      y = b < 0 ? 0 : (b > 255 ? 255 : b);
    } else if (is_c16){
      x = a < 0 ? 0 : (a > 65535 ? 65535 : a);
      y = b < 0 ? 0 : (b > 65535 ? 65535 : b);
    } else if (is_s8){
      x = a & 0xFFL; if (x & 0x80L) x |= ~0xFFL;
      y = b & 0xFFL; if (y & 0x80L) y |= ~0xFFL;
    } else {
      /* SEXT16 */
      x = a & 0xFFFFL; if (x & 0x8000L) x |= ~0xFFFFL;
      y = b & 0xFFFFL; if (y & 0x8000L) y |= ~0xFFFFL;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 dual-stack data-path zero-extend: DZEXT8 · DZEXT16 */
  if (kw(&L->cur,"DZEXT8")||kw(&L->cur,"2ZEXT8")||kw(&L->cur,"S2ZEXT8")||
      kw(&L->cur,"STACK2ZEXT8")||kw(&L->cur,"PAIRZEXT8")||kw(&L->cur,"DZEXTB")||
      kw(&L->cur,"2ZEXTB")||kw(&L->cur,"DZEROEXT8")||
      kw(&L->cur,"DZEXT16")||kw(&L->cur,"2ZEXT16")||kw(&L->cur,"S2ZEXT16")||
      kw(&L->cur,"STACK2ZEXT16")||kw(&L->cur,"PAIRZEXT16")||kw(&L->cur,"DZEXTW")||
      kw(&L->cur,"2ZEXTW")||kw(&L->cur,"DZEROEXT16")){
    /* a b → zero-extend low 8/16 bits (unsigned width mask) */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is8 = (strcmp(op,"DZEXT8")==0 || strcmp(op,"2ZEXT8")==0 ||
               strcmp(op,"S2ZEXT8")==0 || strcmp(op,"STACK2ZEXT8")==0 ||
               strcmp(op,"PAIRZEXT8")==0 || strcmp(op,"DZEXTB")==0 ||
               strcmp(op,"2ZEXTB")==0 || strcmp(op,"DZEROEXT8")==0);
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x, y;
    if (is8){
      x = a & 0xFFL;
      y = b & 0xFFL;
    } else {
      x = a & 0xFFFFL;
      y = b & 0xFFFFL;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
