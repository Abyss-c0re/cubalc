/* CubalC lang — lang_ops_dual_imm_field.c
 * DUAL imm field: DBYTEN·DNIBN·DWORDN·DGET32N get/set/clr (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_field.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_field(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-5 dual-stack imm byte field: DBYTEN · DSETBYTEN · DCLRBYTEN (dual of SBYTEN/SSETBYTEN/SCLRBYTEN) */
  if (kw(&L->cur,"DBYTEN")||kw(&L->cur,"2BYTEN")||kw(&L->cur,"S2BYTEN")||
      kw(&L->cur,"STACK2BYTEN")||kw(&L->cur,"PAIRBYTEN")||kw(&L->cur,"DGETBYTEN")||
      kw(&L->cur,"2GETBYTEN")||kw(&L->cur,"GETBYTEN2")){
    /* a b + n → byte n of a , byte n of b ; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned sh = (unsigned)(n * 8);
    long x = (long)(((unsigned long)vm->stack[vm->sp - 2] >> sh) & 0xFFul);
    long y = (long)(((unsigned long)vm->stack[vm->sp - 1] >> sh) & 0xFFul);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSETBYTEN")||kw(&L->cur,"2SETBYTEN")||kw(&L->cur,"S2SETBYTEN")||
      kw(&L->cur,"STACK2SETBYTEN")||kw(&L->cur,"PAIRSETBYTEN")||kw(&L->cur,"DPUTBYTEN")||
      kw(&L->cur,"2PUTBYTEN")||kw(&L->cur,"DSETBYIMM")){
    /* a b + field n → deposit field into byte n of each; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long x = (long)((ma & ~(0xFFul << sh)) | (f << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (f << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DCLRBYTEN")||kw(&L->cur,"2CLRBYTEN")||kw(&L->cur,"S2CLRBYTEN")||
      kw(&L->cur,"STACK2CLRBYTEN")||kw(&L->cur,"PAIRCLRBYTEN")||kw(&L->cur,"DZAPBYTEN")||
      kw(&L->cur,"2ZAPBYTEN")||kw(&L->cur,"DCLRBYIMM")){
    /* a b + n → clear byte n of each; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    long x = (long)((unsigned long)vm->stack[vm->sp - 2] & ~(0xFFul << sh));
    long y = (long)((unsigned long)vm->stack[vm->sp - 1] & ~(0xFFul << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm nibble field: DNIBN · DSETNIBN · DCLRNIBN (dual of SNIBN/SSETNIBN/SCLRNIBN) */
  if (kw(&L->cur,"DNIBN")||kw(&L->cur,"2NIBN")||kw(&L->cur,"S2NIBN")||
      kw(&L->cur,"STACK2NIBN")||kw(&L->cur,"PAIRNIBN")||kw(&L->cur,"DGETNIBN")||
      kw(&L->cur,"2GETNIBN")||kw(&L->cur,"DNIBBLEN")){
    /* a b + n → nibble n of a , nibble n of b ; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned sh = (unsigned)(n * 4);
    long x = (long)(((unsigned long)vm->stack[vm->sp - 2] >> sh) & 0xFul);
    long y = (long)(((unsigned long)vm->stack[vm->sp - 1] >> sh) & 0xFul);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSETNIBN")||kw(&L->cur,"2SETNIBN")||kw(&L->cur,"S2SETNIBN")||
      kw(&L->cur,"STACK2SETNIBN")||kw(&L->cur,"PAIRSETNIBN")||kw(&L->cur,"DPUTNIBN")||
      kw(&L->cur,"2PUTNIBN")||kw(&L->cur,"DSETNIBIMM")){
    /* a b + field n → deposit field into nibble n of each; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long x = (long)((ma & ~(0xFul << sh)) | (f << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (f << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DCLRNIBN")||kw(&L->cur,"2CLRNIBN")||kw(&L->cur,"S2CLRNIBN")||
      kw(&L->cur,"STACK2CLRNIBN")||kw(&L->cur,"PAIRCLRNIBN")||kw(&L->cur,"DZAPNIBN")||
      kw(&L->cur,"2ZAPNIBN")||kw(&L->cur,"DCLRNIBIMM")){
    /* a b + n → clear nibble n of each; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    long x = (long)((unsigned long)vm->stack[vm->sp - 2] & ~(0xFul << sh));
    long y = (long)((unsigned long)vm->stack[vm->sp - 1] & ~(0xFul << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm 16-bit halfword field: DWORDN · DSET16N · DCLR16N (dual of SWORDN/SSET16N/SCLR16N) */
  if (kw(&L->cur,"DWORDN")||kw(&L->cur,"2WORDN")||kw(&L->cur,"S2WORDN")||
      kw(&L->cur,"STACK2WORDN")||kw(&L->cur,"PAIRWORDN")||kw(&L->cur,"DGET16N")||
      kw(&L->cur,"2GET16N")||kw(&L->cur,"DHALFN")||kw(&L->cur,"2HALFN")||
      kw(&L->cur,"DGETHALF")||kw(&L->cur,"GET16N2")){
    /* a b + n → halfword n of a , halfword n of b ; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned sh = (unsigned)(n * 16);
    long x = (long)(((unsigned long)vm->stack[vm->sp - 2] >> sh) & 0xFFFFul);
    long y = (long)(((unsigned long)vm->stack[vm->sp - 1] >> sh) & 0xFFFFul);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSET16N")||kw(&L->cur,"2SET16N")||kw(&L->cur,"S2SET16N")||
      kw(&L->cur,"STACK2SET16N")||kw(&L->cur,"PAIRSET16N")||kw(&L->cur,"DSETWORDN")||
      kw(&L->cur,"2SETWORDN")||kw(&L->cur,"DPUT16N")||kw(&L->cur,"2PUT16N")||
      kw(&L->cur,"DSETHALFN")||kw(&L->cur,"DSETWIMM")){
    /* a b + field n → deposit field into halfword n of each; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long x = (long)((ma & ~(0xFFFFul << sh)) | (f << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (f << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DCLR16N")||kw(&L->cur,"2CLR16N")||kw(&L->cur,"S2CLR16N")||
      kw(&L->cur,"STACK2CLR16N")||kw(&L->cur,"PAIRCLR16N")||kw(&L->cur,"DCLRWORDN")||
      kw(&L->cur,"2CLRWORDN")||kw(&L->cur,"DZAP16N")||kw(&L->cur,"2ZAP16N")||
      kw(&L->cur,"DCLRHAFN")||kw(&L->cur,"DCLRWIMM")){
    /* a b + n → clear halfword n of each; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    long x = (long)((unsigned long)vm->stack[vm->sp - 2] & ~(0xFFFFul << sh));
    long y = (long)((unsigned long)vm->stack[vm->sp - 1] & ~(0xFFFFul << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 dual-stack imm 32-bit field: DGET32N · DSET32N · DCLR32N (dual of SGET32N/SSET32N/SCLR32N) */
  if (kw(&L->cur,"DGET32N")||kw(&L->cur,"S2GET32N")||kw(&L->cur,"STACK2GET32N")||
      kw(&L->cur,"PAIRGET32N")||kw(&L->cur,"DWORD32N")||kw(&L->cur,"S2WORD32N")||
      kw(&L->cur,"PAIRWORD32N")||kw(&L->cur,"DGETDWN")||kw(&L->cur,"GET32N2")){
    /* a b + n → word n of a , word n of b ; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned sh = (unsigned)(n * 32);
    long x = (long)(((unsigned long)vm->stack[vm->sp - 2] >> sh) & 0xFFFFFFFFul);
    long y = (long)(((unsigned long)vm->stack[vm->sp - 1] >> sh) & 0xFFFFFFFFul);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSET32N")||kw(&L->cur,"S2SET32N")||kw(&L->cur,"STACK2SET32N")||
      kw(&L->cur,"PAIRSET32N")||kw(&L->cur,"DSETWORD32N")||kw(&L->cur,"S2SETWORD32N")||
      kw(&L->cur,"DPUT32N")||kw(&L->cur,"S2PUT32N")||kw(&L->cur,"DSETDWN")||
      kw(&L->cur,"DSETW32IMM")){
    /* a b + field n → deposit field into word n of each; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (f << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (f << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DCLR32N")||kw(&L->cur,"S2CLR32N")||kw(&L->cur,"STACK2CLR32N")||
      kw(&L->cur,"PAIRCLR32N")||kw(&L->cur,"DCLRWORD32N")||kw(&L->cur,"S2CLRWORD32N")||
      kw(&L->cur,"DZAP32N")||kw(&L->cur,"S2ZAP32N")||kw(&L->cur,"DCLRDWN")||
      kw(&L->cur,"DCLRW32IMM")){
    /* a b + n → clear word n of each; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long sh = (unsigned long)(n * 32);
    long x = (long)((unsigned long)vm->stack[vm->sp - 2] & ~(0xFFFFFFFFul << sh));
    long y = (long)((unsigned long)vm->stack[vm->sp - 1] & ~(0xFFFFFFFFul << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
