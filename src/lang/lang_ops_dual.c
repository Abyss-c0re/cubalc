/* CubalC lang — lang_ops_dual.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual(VM *vm, Lex *L){
  /* plane ops_dual: L12580-20288 */
  /* digit-7 dual-stack pair ALU: DADD · DSUB · DMUL (vector pairs a b + c d) */
  if (kw(&L->cur,"DADD")||kw(&L->cur,"2ADD")||kw(&L->cur,"S2ADD")||
      kw(&L->cur,"STACK2ADD")||kw(&L->cur,"PAIRADD")){
    /* a b c d → (a+c) (b+d) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = a + c;
    long y = b + d;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSUB")||kw(&L->cur,"2SUB")||kw(&L->cur,"S2SUB")||
      kw(&L->cur,"STACK2SUB")||kw(&L->cur,"PAIRSUB")){
    /* a b c d → (a-c) (b-d) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = a - c;
    long y = b - d;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMUL")||kw(&L->cur,"2MUL")||kw(&L->cur,"S2MUL")||
      kw(&L->cur,"STACK2MUL")||kw(&L->cur,"PAIRMUL")){
    /* a b c d → (a*c) (b*d) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = a * c;
    long y = b * d;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack pair ALU ext: DDIV · DMOD · DMIN · DMAX */
  if (kw(&L->cur,"DDIV")||kw(&L->cur,"2DIV")||kw(&L->cur,"S2DIV")||
      kw(&L->cur,"STACK2DIV")||kw(&L->cur,"PAIRDIV")){
    /* a b c d → (a/c) (b/d); divisor 0 → 0 */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = c ? (a / c) : 0;
    long y = d ? (b / d) : 0;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 dual-stack div modes: DDIVCEIL · DDIVFLOOR (pair ceildiv/floordiv) */
  if (kw(&L->cur,"DDIVCEIL")||kw(&L->cur,"2DIVCEIL")||kw(&L->cur,"S2DIVCEIL")||
      kw(&L->cur,"STACK2DIVCEIL")||kw(&L->cur,"PAIRDIVCEIL")||kw(&L->cur,"2CEILDIV")||
      kw(&L->cur,"DCEILDIV")||
      kw(&L->cur,"DDIVFLOOR")||kw(&L->cur,"2DIVFLOOR")||kw(&L->cur,"S2DIVFLOOR")||
      kw(&L->cur,"STACK2DIVFLOOR")||kw(&L->cur,"PAIRDIVFLOOR")||kw(&L->cur,"2FLOORDIV")||
      kw(&L->cur,"DFLOORDIV")){
    /* a b c d → f(a,c) f(b,d); divisor 0 → 0
     * CEIL: toward +∞ for same-sign positive path; mixed/neg uses trunc toward zero for ceil when signs differ (match SDIVCEIL)
     * FLOOR: toward −∞ (adjust when rem and signs differ) */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_ceil = (strcmp(op,"DDIVCEIL")==0 || strcmp(op,"2DIVCEIL")==0 ||
                   strcmp(op,"S2DIVCEIL")==0 || strcmp(op,"STACK2DIVCEIL")==0 ||
                   strcmp(op,"PAIRDIVCEIL")==0 || strcmp(op,"2CEILDIV")==0 ||
                   strcmp(op,"DCEILDIV")==0);
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = 0, y = 0;
    if (is_ceil){
      if (c == 0) x = 0;
      else if (a >= 0 && c > 0) x = (a + c - 1) / c;
      else if (a <= 0 && c < 0){
        long aa = -a, cc = -c;
        x = (aa + cc - 1) / cc;
      } else x = a / c;
      if (d == 0) y = 0;
      else if (b >= 0 && d > 0) y = (b + d - 1) / d;
      else if (b <= 0 && d < 0){
        long bb = -b, dd = -d;
        y = (bb + dd - 1) / dd;
      } else y = b / d;
    } else {
      if (c == 0) x = 0;
      else {
        long q = a / c, rem = a % c;
        if (rem != 0 && ((a < 0) != (c < 0))) q--;
        x = q;
      }
      if (d == 0) y = 0;
      else {
        long q = b / d, rem = b % d;
        if (rem != 0 && ((b < 0) != (d < 0))) q--;
        y = q;
      }
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMOD")||kw(&L->cur,"2MOD")||kw(&L->cur,"S2MOD")||
      kw(&L->cur,"STACK2MOD")||kw(&L->cur,"PAIRMOD")||kw(&L->cur,"DREM")){
    /* a b c d → (a%c) (b%d); divisor 0 → 0 */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = c ? (a % c) : 0;
    long y = d ? (b % d) : 0;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMIN")||kw(&L->cur,"2MIN")||kw(&L->cur,"S2MIN")||
      kw(&L->cur,"STACK2MIN")||kw(&L->cur,"PAIRMIN")){
    /* a b c d → min(a,c) min(b,d) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = a < c ? a : c;
    long y = b < d ? b : d;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMAX")||kw(&L->cur,"2MAX")||kw(&L->cur,"S2MAX")||
      kw(&L->cur,"STACK2MAX")||kw(&L->cur,"PAIRMAX")){
    /* a b c d → max(a,c) max(b,d) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = a > c ? a : c;
    long y = b > d ? b : d;
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
  /* digit-4 dual-stack rotate + range predicates: DROL · DROR · DWITHIN · DBETWEEN */
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
  /* digit-5 dual-stack extended bitwise: DNAND · DNOR · DXNOR · DANDN */
  if (kw(&L->cur,"DNAND")||kw(&L->cur,"2NAND")||kw(&L->cur,"S2NAND")||
      kw(&L->cur,"STACK2NAND")||kw(&L->cur,"PAIRNAND")||
      kw(&L->cur,"DNOR")||kw(&L->cur,"2NOR")||kw(&L->cur,"S2NOR")||
      kw(&L->cur,"STACK2NOR")||kw(&L->cur,"PAIRNOR")||
      kw(&L->cur,"DXNOR")||kw(&L->cur,"2XNOR")||kw(&L->cur,"S2XNOR")||
      kw(&L->cur,"STACK2XNOR")||kw(&L->cur,"PAIRXNOR")||kw(&L->cur,"DEQV")||
      kw(&L->cur,"DANDN")||kw(&L->cur,"2ANDN")||kw(&L->cur,"S2ANDN")||
      kw(&L->cur,"STACK2ANDN")||kw(&L->cur,"PAIRANDN")||kw(&L->cur,"DBIC")){
    /* a b c d → f(a,c) f(b,d) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    int is_nand = (strcmp(op,"DNAND")==0 || strcmp(op,"2NAND")==0 || strcmp(op,"S2NAND")==0 ||
                   strcmp(op,"STACK2NAND")==0 || strcmp(op,"PAIRNAND")==0);
    int is_nor = (strcmp(op,"DNOR")==0 || strcmp(op,"2NOR")==0 || strcmp(op,"S2NOR")==0 ||
                  strcmp(op,"STACK2NOR")==0 || strcmp(op,"PAIRNOR")==0);
    int is_xnor = (strcmp(op,"DXNOR")==0 || strcmp(op,"2XNOR")==0 || strcmp(op,"S2XNOR")==0 ||
                   strcmp(op,"STACK2XNOR")==0 || strcmp(op,"PAIRXNOR")==0 ||
                   strcmp(op,"DEQV")==0);
    /* else DANDN / BIC */
    long x, y;
    if (is_nand){ x = ~(a & c); y = ~(b & d); }
    else if (is_nor){ x = ~(a | c); y = ~(b | d); }
    else if (is_xnor){ x = ~(a ^ c); y = ~(b ^ d); }
    else { x = a & ~c; y = b & ~d; }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack bit metrics: DPOPCNT · DCLZ · DCTZ (unary pair) */
  if (kw(&L->cur,"DPOPCNT")||kw(&L->cur,"2POPCNT")||kw(&L->cur,"S2POPCNT")||
      kw(&L->cur,"STACK2POPCNT")||kw(&L->cur,"PAIRPOPCNT")||kw(&L->cur,"2PCNT")||
      kw(&L->cur,"DPCNT")||
      kw(&L->cur,"DCLZ")||kw(&L->cur,"2CLZ")||kw(&L->cur,"S2CLZ")||
      kw(&L->cur,"STACK2CLZ")||kw(&L->cur,"PAIRCLZ")||
      kw(&L->cur,"DCTZ")||kw(&L->cur,"2CTZ")||kw(&L->cur,"S2CTZ")||
      kw(&L->cur,"STACK2CTZ")||kw(&L->cur,"PAIRCTZ")){
    /* a b → metric(a) metric(b); zero → 64 for clz/ctz */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_pop = (strcmp(op,"DPOPCNT")==0 || strcmp(op,"2POPCNT")==0 ||
                  strcmp(op,"S2POPCNT")==0 || strcmp(op,"STACK2POPCNT")==0 ||
                  strcmp(op,"PAIRPOPCNT")==0 || strcmp(op,"2PCNT")==0 ||
                  strcmp(op,"DPCNT")==0);
    int is_clz = (strcmp(op,"DCLZ")==0 || strcmp(op,"2CLZ")==0 || strcmp(op,"S2CLZ")==0 ||
                  strcmp(op,"STACK2CLZ")==0 || strcmp(op,"PAIRCLZ")==0);
    long x = 0, y = 0;
    if (is_pop){
      unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
      while (ua){ x += (long)(ua & 1ul); ua >>= 1; }
      while (ub){ y += (long)(ub & 1ul); ub >>= 1; }
    } else if (is_clz){
      unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
      if (ua == 0) x = 64;
      else { for (int i = 63; i >= 0; i--){ if (ua & (1ul << (unsigned)i)) break; x++; } }
      if (ub == 0) y = 64;
      else { for (int i = 63; i >= 0; i--){ if (ub & (1ul << (unsigned)i)) break; y++; } }
    } else {
      /* CTZ */
      unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
      if (ua == 0) x = 64;
      else { while ((ua & 1ul) == 0){ x++; ua >>= 1; } }
      if (ub == 0) y = 64;
      else { while ((ub & 1ul) == 0){ y++; ub >>= 1; } }
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack bit-path: DORN · DBREV · DPARITY */
  if (kw(&L->cur,"DORN")||kw(&L->cur,"2ORN")||kw(&L->cur,"S2ORN")||
      kw(&L->cur,"STACK2ORN")||kw(&L->cur,"PAIRORN")||kw(&L->cur,"DORNOT")){
    /* a b c d → (a|~c) (b|~d) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = a | ~c;
    long y = b | ~d;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DBREV")||kw(&L->cur,"2BREV")||kw(&L->cur,"S2BREV")||
      kw(&L->cur,"STACK2BREV")||kw(&L->cur,"PAIRBREV")||kw(&L->cur,"DBITREV")||
      kw(&L->cur,"2BITREV")||kw(&L->cur,"DREVBITS")||
      kw(&L->cur,"DPARITY")||kw(&L->cur,"2PARITY")||kw(&L->cur,"S2PARITY")||
      kw(&L->cur,"STACK2PARITY")||kw(&L->cur,"PAIRPARITY")||kw(&L->cur,"2PAR")||
      kw(&L->cur,"DPAR")){
    /* a b → f(a) f(b); BREV = reverse low 32 bits; PARITY = popcount mod 2 */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_brev = (strcmp(op,"DBREV")==0 || strcmp(op,"2BREV")==0 || strcmp(op,"S2BREV")==0 ||
                   strcmp(op,"STACK2BREV")==0 || strcmp(op,"PAIRBREV")==0 ||
                   strcmp(op,"DBITREV")==0 || strcmp(op,"2BITREV")==0 ||
                   strcmp(op,"DREVBITS")==0);
    long x, y;
    if (is_brev){
      unsigned int wa = (unsigned int)a, wb = (unsigned int)b;
      unsigned int ra = 0, rb = 0;
      for (int i = 0; i < 32; i++){
        ra = (ra << 1) | (wa & 1u); wa >>= 1;
        rb = (rb << 1) | (wb & 1u); wb >>= 1;
      }
      x = (long)ra; y = (long)rb;
    } else {
      unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
      int na = 0, nb = 0;
      while (ua){ na ^= (int)(ua & 1u); ua >>= 1; }
      while (ub){ nb ^= (int)(ub & 1u); ub >>= 1; }
      x = (long)na; y = (long)nb;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack bit isolate/mask: DBLS · DBLC · DMASK */
  if (kw(&L->cur,"DBLS")||kw(&L->cur,"2BLS")||kw(&L->cur,"S2BLS")||
      kw(&L->cur,"STACK2BLS")||kw(&L->cur,"PAIRBLS")||kw(&L->cur,"DBLSI")||
      kw(&L->cur,"2BLSI")||kw(&L->cur,"DISOLB")||
      kw(&L->cur,"DBLC")||kw(&L->cur,"2BLC")||kw(&L->cur,"S2BLC")||
      kw(&L->cur,"STACK2BLC")||kw(&L->cur,"PAIRBLC")||kw(&L->cur,"DBLSR")||
      kw(&L->cur,"2BLSR")||kw(&L->cur,"DCLRBLS")||
      kw(&L->cur,"DMASK")||kw(&L->cur,"2MASK")||kw(&L->cur,"S2MASK")||
      kw(&L->cur,"STACK2MASK")||kw(&L->cur,"PAIRMASK")||kw(&L->cur,"DBITMASK")||
      kw(&L->cur,"2BITMASK")){
    /* a b → f(a) f(b)
     * BLS/BLSI: isolate lowest set bit  x & -x  (0 if zero)
     * BLC/BLSR: clear lowest set bit    x & (x-1)
     * MASK: low-n-bit mask (1<<n)-1; n clamped 0..64 (64 → all ones) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_bls = (strcmp(op,"DBLS")==0 || strcmp(op,"2BLS")==0 || strcmp(op,"S2BLS")==0 ||
                  strcmp(op,"STACK2BLS")==0 || strcmp(op,"PAIRBLS")==0 ||
                  strcmp(op,"DBLSI")==0 || strcmp(op,"2BLSI")==0 ||
                  strcmp(op,"DISOLB")==0);
    int is_blc = (strcmp(op,"DBLC")==0 || strcmp(op,"2BLC")==0 || strcmp(op,"S2BLC")==0 ||
                  strcmp(op,"STACK2BLC")==0 || strcmp(op,"PAIRBLC")==0 ||
                  strcmp(op,"DBLSR")==0 || strcmp(op,"2BLSR")==0 ||
                  strcmp(op,"DCLRBLS")==0);
    long x, y;
    if (is_bls){
      x = a ? (a & -a) : 0;
      y = b ? (b & -b) : 0;
    } else if (is_blc){
      x = a ? (a & (a - 1)) : 0;
      y = b ? (b & (b - 1)) : 0;
    } else {
      /* DMASK — width → low-n ones */
      long na = a, nb = b;
      if (na < 0) na = 0;
      if (nb < 0) nb = 0;
      if (na > 64) na = 64;
      if (nb > 64) nb = 64;
      if (na == 0) x = 0;
      else if (na == 64) x = (long)(~0UL);
      else x = (long)((1UL << (unsigned)na) - 1UL);
      if (nb == 0) y = 0;
      else if (nb == 64) y = (long)(~0UL);
      else y = (long)((1UL << (unsigned)nb) - 1UL);
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack bitfield pos: DBTEST · DSETB · DCLRB · DFLIPB */
  if (kw(&L->cur,"DBTEST")||kw(&L->cur,"2BTEST")||kw(&L->cur,"S2BTEST")||
      kw(&L->cur,"STACK2BTEST")||kw(&L->cur,"PAIRBTEST")||kw(&L->cur,"2BITT")||
      kw(&L->cur,"DBITT")||
      kw(&L->cur,"DSETB")||kw(&L->cur,"2SETB")||kw(&L->cur,"S2SETB")||
      kw(&L->cur,"STACK2SETB")||kw(&L->cur,"PAIRSETB")||kw(&L->cur,"2SETBIT")||
      kw(&L->cur,"DSETBIT")||
      kw(&L->cur,"DCLRB")||kw(&L->cur,"2CLRB")||kw(&L->cur,"S2CLRB")||
      kw(&L->cur,"STACK2CLRB")||kw(&L->cur,"PAIRCLRB")||kw(&L->cur,"2CLRBIT")||
      kw(&L->cur,"DCLRBIT")||
      kw(&L->cur,"DFLIPB")||kw(&L->cur,"2FLIPB")||kw(&L->cur,"S2FLIPB")||
      kw(&L->cur,"STACK2FLIPB")||kw(&L->cur,"PAIRFLIPB")||kw(&L->cur,"2FLIPBIT")||
      kw(&L->cur,"DFLIPBIT")){
    /* a b na nb → f(a,na) f(b,nb); indices clamped 0..63
     * BTEST: bit → 0/1
     * SETB:  set bit
     * CLRB:  clear bit
     * FLIPB: toggle bit */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long nb = vm->stack[--vm->sp];
    long na = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (na < 0) na = 0;
    if (nb < 0) nb = 0;
    if (na > 63) na = 63;
    if (nb > 63) nb = 63;
    unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
    unsigned long ba = 1UL << (unsigned)na;
    unsigned long bb = 1UL << (unsigned)nb;
    int is_test = (strcmp(op,"DBTEST")==0 || strcmp(op,"2BTEST")==0 || strcmp(op,"S2BTEST")==0 ||
                   strcmp(op,"STACK2BTEST")==0 || strcmp(op,"PAIRBTEST")==0 ||
                   strcmp(op,"2BITT")==0 || strcmp(op,"DBITT")==0);
    int is_set = (strcmp(op,"DSETB")==0 || strcmp(op,"2SETB")==0 || strcmp(op,"S2SETB")==0 ||
                  strcmp(op,"STACK2SETB")==0 || strcmp(op,"PAIRSETB")==0 ||
                  strcmp(op,"2SETBIT")==0 || strcmp(op,"DSETBIT")==0);
    int is_clr = (strcmp(op,"DCLRB")==0 || strcmp(op,"2CLRB")==0 || strcmp(op,"S2CLRB")==0 ||
                  strcmp(op,"STACK2CLRB")==0 || strcmp(op,"PAIRCLRB")==0 ||
                  strcmp(op,"2CLRBIT")==0 || strcmp(op,"DCLRBIT")==0);
    long x, y;
    if (is_test){
      x = (ua & ba) ? 1 : 0;
      y = (ub & bb) ? 1 : 0;
    } else if (is_set){
      x = (long)(ua | ba);
      y = (long)(ub | bb);
    } else if (is_clr){
      x = (long)(ua & ~ba);
      y = (long)(ub & ~bb);
    } else {
      /* FLIPB */
      x = (long)(ua ^ ba);
      y = (long)(ub ^ bb);
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 dual-stack immediate bitfield: DSETBN · DBTESTN (dual of SSETBN/SBTESTN) */
  if (kw(&L->cur,"DSETBN")||kw(&L->cur,"2SETBN")||kw(&L->cur,"S2SETBN")||
      kw(&L->cur,"STACK2SETBN")||kw(&L->cur,"PAIRSETBN")||kw(&L->cur,"DSETBITN")||
      kw(&L->cur,"2SETBITN")||kw(&L->cur,"PAIRSETBITN")){
    /* a b + n → a|(1<<n)  b|(1<<n); n clamped 0..63 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    unsigned long bit = 1ul << (unsigned)n;
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long ub = (unsigned long)vm->stack[vm->sp - 1];
    long x = (long)(ua | bit);
    long y = (long)(ub | bit);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DCLRBN")||kw(&L->cur,"2CLRBN")||kw(&L->cur,"S2CLRBN")||
      kw(&L->cur,"STACK2CLRBN")||kw(&L->cur,"PAIRCLRBN")||kw(&L->cur,"DCLRBITN")||
      kw(&L->cur,"2CLRBITN")||kw(&L->cur,"PAIRCLRBITN")){
    /* a b + n → a&~(1<<n)  b&~(1<<n); n clamped 0..63 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    unsigned long bit = 1ul << (unsigned)n;
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long ub = (unsigned long)vm->stack[vm->sp - 1];
    long x = (long)(ua & ~bit);
    long y = (long)(ub & ~bit);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DBTESTN")||kw(&L->cur,"2BTESTN")||kw(&L->cur,"S2BTESTN")||
      kw(&L->cur,"STACK2BTESTN")||kw(&L->cur,"PAIRBTESTN")||kw(&L->cur,"DBITN")||
      kw(&L->cur,"2BITN")||kw(&L->cur,"DTESTBITN")||kw(&L->cur,"2TESTBITN")||
      kw(&L->cur,"PAIRBITN")){
    /* a b + n → bit_n(a) bit_n(b) as 0/1; n clamped 0..63 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    unsigned long bit = 1ul << (unsigned)n;
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long ub = (unsigned long)vm->stack[vm->sp - 1];
    long x = (ua & bit) ? 1 : 0;
    long y = (ub & bit) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 control-flag dual flip: DFLIPBN (complete after DSETBN/DCLRBN/DBTESTN; dual of SFLIPBN) */
  if (kw(&L->cur,"DFLIPBN")||kw(&L->cur,"2FLIPBN")||kw(&L->cur,"S2FLIPBN")||
      kw(&L->cur,"STACK2FLIPBN")||kw(&L->cur,"PAIRFLIPBN")||kw(&L->cur,"DFLIPBITN")||
      kw(&L->cur,"2FLIPBITN")||kw(&L->cur,"DTGLBN")||kw(&L->cur,"2TGLBN")||
      kw(&L->cur,"PAIRTGLBN")||kw(&L->cur,"DFLPBN")){
    /* a b + n → a^(1<<n)  b^(1<<n); n clamped 0..63 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    unsigned long bit = 1ul << (unsigned)n;
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long ub = (unsigned long)vm->stack[vm->sp - 1];
    long x = (long)(ua ^ bit);
    long y = (long)(ub ^ bit);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 dual-stack data-path imm mask/extract: DMASKN · DANDMN · DBEXTN (dual of SMASKN/SBEXTN) */
  if (kw(&L->cur,"DMASKN")||kw(&L->cur,"2MASKN")||kw(&L->cur,"S2MASKN")||
      kw(&L->cur,"STACK2MASKN")||kw(&L->cur,"PAIRMASKN")||kw(&L->cur,"DONESN")||
      kw(&L->cur,"2ONESN")||kw(&L->cur,"DLOWMASKN")||kw(&L->cur,"2LOWMASKN")){
    /* a b + n → m m  with m = low-n-bit mask (1<<n)-1; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long v = (long)m;
    vm->stack[vm->sp - 2] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DANDMN")||kw(&L->cur,"2ANDMN")||kw(&L->cur,"S2ANDMN")||
      kw(&L->cur,"STACK2ANDMN")||kw(&L->cur,"PAIRANDMN")||kw(&L->cur,"DKEEPLN")||
      kw(&L->cur,"2KEEPLN")||kw(&L->cur,"DLOWANDN")||kw(&L->cur,"2LOWANDN")||
      kw(&L->cur,"DMASKAND")||kw(&L->cur,"2MASKAND")){
    /* a b + n → (a&m) (b&m) with m = low-n mask; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (long)((unsigned long)a & m);
    long y = (long)((unsigned long)b & m);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DBEXTN")||kw(&L->cur,"2BEXTN")||kw(&L->cur,"S2BEXTN")||
      kw(&L->cur,"STACK2BEXTN")||kw(&L->cur,"PAIRBEXTN")||kw(&L->cur,"DBITEXTN")||
      kw(&L->cur,"2BITEXTN")||kw(&L->cur,"DEXTN")||kw(&L->cur,"2EXTN")){
    /* a b + pos width → extract width bits at pos from each */
    lex_next(L);
    long pos = parse_expr(vm,L);
    long width = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (pos < 0) pos = 0;
    if (pos > 62){
      vm->stack[vm->sp - 2] = 0;
      vm->stack[vm->sp - 1] = 0;
      var_set_num(vm,"LAST_N",0); vm->last_n=0;
      var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
    }
    if (width < 1) width = 0;
    if (width > 63 - pos) width = 63 - pos;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (width > 0){
      unsigned long mask = (width >= 63) ? ~0ul : ((1ul << (unsigned)width) - 1ul);
      x = (long)(((unsigned long)a >> (unsigned)pos) & mask);
      y = (long)(((unsigned long)b >> (unsigned)pos) & mask);
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 dual-stack data-path imm deposit + low-n or/xor (complete DMASKN/DANDMN/DBEXTN) */
  if (kw(&L->cur,"DBDEPN")||kw(&L->cur,"2BDEPN")||kw(&L->cur,"S2BDEPN")||
      kw(&L->cur,"STACK2BDEPN")||kw(&L->cur,"PAIRBDEPN")||kw(&L->cur,"DBITDEPN")||
      kw(&L->cur,"2BITDEPN")||kw(&L->cur,"DDEPN")||kw(&L->cur,"2DEPN")){
    /* a b + field pos → deposit low 8 bits of field into each lane at pos (dual of SBDEPN) */
    lex_next(L);
    long field = parse_expr(vm,L);
    long pos = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long width = 8;
    if (pos < 0) pos = 0;
    if (pos > 62){
      var_set_num(vm,"LAST_N",b); vm->last_n=b;
      var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
    }
    if (width > 63 - pos) width = 63 - pos;
    long x = a, y = b;
    if (width > 0){
      unsigned long mask = (width >= 63) ? ~0ul : ((1ul << (unsigned)width) - 1ul);
      unsigned long f = (unsigned long)field & mask;
      unsigned long ba = (unsigned long)a;
      unsigned long bb = (unsigned long)b;
      ba = (ba & ~(mask << (unsigned)pos)) | (f << (unsigned)pos);
      bb = (bb & ~(mask << (unsigned)pos)) | (f << (unsigned)pos);
      x = (long)ba;
      y = (long)bb;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DORMN")||kw(&L->cur,"2ORMN")||kw(&L->cur,"S2ORMN")||
      kw(&L->cur,"STACK2ORMN")||kw(&L->cur,"PAIRORMN")||kw(&L->cur,"DSETLN")||
      kw(&L->cur,"2SETLN")||kw(&L->cur,"DLOWORN")||kw(&L->cur,"2LOWORN")||
      kw(&L->cur,"DMASKOR")||kw(&L->cur,"2MASKOR")){
    /* a b + n → (a|m) (b|m) with m = low-n mask; n clamped 0..64 (energy bit-fill) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (long)((unsigned long)a | m);
    long y = (long)((unsigned long)b | m);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXORMN")||kw(&L->cur,"2XORMN")||kw(&L->cur,"S2XORMN")||
      kw(&L->cur,"STACK2XORMN")||kw(&L->cur,"PAIRXORMN")||kw(&L->cur,"DFLIPLN")||
      kw(&L->cur,"2FLIPLN")||kw(&L->cur,"DLOWXORN")||kw(&L->cur,"2LOWXORN")||
      kw(&L->cur,"DMASKXOR")||kw(&L->cur,"2MASKXOR")){
    /* a b + n → (a^m) (b^m) with m = low-n mask; n clamped 0..64 (toggle low plane) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (long)((unsigned long)a ^ m);
    long y = (long)((unsigned long)b ^ m);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack inverted low-n mask: DNANDMN · DNORMN · DXNORMN (dual of DANDMN/DORMN/DXORMN inverted) */
  if (kw(&L->cur,"DNANDMN")||kw(&L->cur,"2NANDMN")||kw(&L->cur,"S2NANDMN")||
      kw(&L->cur,"STACK2NANDMN")||kw(&L->cur,"PAIRNANDMN")||kw(&L->cur,"DLOWNANDN")||
      kw(&L->cur,"2LOWNANDN")||kw(&L->cur,"DMASKNAND")||kw(&L->cur,"2MASKNAND")){
    /* a b + n → ~(a&m) ~(b&m) with m = low-n mask; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (long)~((unsigned long)a & m);
    long y = (long)~((unsigned long)b & m);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNORMN")||kw(&L->cur,"2NORMN")||kw(&L->cur,"S2NORMN")||
      kw(&L->cur,"STACK2NORMN")||kw(&L->cur,"PAIRNORMN")||kw(&L->cur,"DLOWNORN")||
      kw(&L->cur,"2LOWNORN")||kw(&L->cur,"DMASKNOR")||kw(&L->cur,"2MASKNOR")){
    /* a b + n → ~(a|m) ~(b|m) with m = low-n mask; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (long)~((unsigned long)a | m);
    long y = (long)~((unsigned long)b | m);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXNORMN")||kw(&L->cur,"2XNORMN")||kw(&L->cur,"S2XNORMN")||
      kw(&L->cur,"STACK2XNORMN")||kw(&L->cur,"PAIRXNORMN")||kw(&L->cur,"DLOWXNORN")||
      kw(&L->cur,"2LOWXNORN")||kw(&L->cur,"DMASKXNOR")||kw(&L->cur,"2MASKXNOR")||
      kw(&L->cur,"DEQUIVMN")||kw(&L->cur,"2EQUIVMN")){
    /* a b + n → ~(a^m) ~(b^m) with m = low-n mask; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (long)~((unsigned long)a ^ m);
    long y = (long)~((unsigned long)b ^ m);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack data-path high-mask + clear-low (complete DMASKN/DANDMN low plane) */
  if (kw(&L->cur,"DHMASKN")||kw(&L->cur,"2HMASKN")||kw(&L->cur,"S2HMASKN")||
      kw(&L->cur,"STACK2HMASKN")||kw(&L->cur,"PAIRHMASKN")||kw(&L->cur,"DHIMASKN")||
      kw(&L->cur,"2HIMASKN")||kw(&L->cur,"DHIGHMASKN")||kw(&L->cur,"2HIGHMASKN")){
    /* a b + n → hm hm  with hm = high-n-bit mask; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long v = (long)m;
    vm->stack[vm->sp - 2] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DANDHN")||kw(&L->cur,"2ANDHN")||kw(&L->cur,"S2ANDHN")||
      kw(&L->cur,"STACK2ANDHN")||kw(&L->cur,"PAIRANDHN")||kw(&L->cur,"DKEEPHN")||
      kw(&L->cur,"2KEEPHN")||kw(&L->cur,"DHIGHANDN")||kw(&L->cur,"2HIGHANDN")||
      kw(&L->cur,"DMASKANDH")||kw(&L->cur,"2MASKANDH")){
    /* a b + n → (a&hm) (b&hm) keep high n bits; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (long)((unsigned long)a & m);
    long y = (long)((unsigned long)b & m);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DCLRLN")||kw(&L->cur,"2CLRLN")||kw(&L->cur,"S2CLRLN")||
      kw(&L->cur,"STACK2CLRLN")||kw(&L->cur,"PAIRCLRLN")||kw(&L->cur,"DCLEARLN")||
      kw(&L->cur,"2CLEARLN")||kw(&L->cur,"DZAPLN")||kw(&L->cur,"2ZAPLN")||
      kw(&L->cur,"DLOWCLRN")||kw(&L->cur,"2LOWCLRN")){
    /* a b + n → clear low n bits: x &= ~lowmask; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (long)((unsigned long)a & ~m);
    long y = (long)((unsigned long)b & ~m);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 dual-stack high-plane or/xor/clear (complete DHMASKN/DANDHN after low DORMN/DXORMN) */
  if (kw(&L->cur,"DORHN")||kw(&L->cur,"2ORHN")||kw(&L->cur,"S2ORHN")||
      kw(&L->cur,"STACK2ORHN")||kw(&L->cur,"PAIRORHN")||kw(&L->cur,"DSETHN")||
      kw(&L->cur,"2SETHN")||kw(&L->cur,"DHIGHORN")||kw(&L->cur,"2HIGHORN")||
      kw(&L->cur,"DMASKORH")||kw(&L->cur,"2MASKORH")){
    /* a b + n → (a|hm) (b|hm) set high n bits; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (long)((unsigned long)a | m);
    long y = (long)((unsigned long)b | m);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXORHN")||kw(&L->cur,"2XORHN")||kw(&L->cur,"S2XORHN")||
      kw(&L->cur,"STACK2XORHN")||kw(&L->cur,"PAIRXORHN")||kw(&L->cur,"DFLIPHN")||
      kw(&L->cur,"2FLIPHN")||kw(&L->cur,"DHIGHXORN")||kw(&L->cur,"2HIGHXORN")||
      kw(&L->cur,"DMASKXORH")||kw(&L->cur,"2MASKXORH")){
    /* a b + n → (a^hm) (b^hm) toggle high n bits; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (long)((unsigned long)a ^ m);
    long y = (long)((unsigned long)b ^ m);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DCLRHN")||kw(&L->cur,"2CLRHN")||kw(&L->cur,"S2CLRHN")||
      kw(&L->cur,"STACK2CLRHN")||kw(&L->cur,"PAIRCLRHN")||kw(&L->cur,"DCLEARHN")||
      kw(&L->cur,"2CLEARHN")||kw(&L->cur,"DZAPHN")||kw(&L->cur,"2ZAPHN")||
      kw(&L->cur,"DHIGHCLRN")||kw(&L->cur,"2HIGHCLRN")){
    /* a b + n → clear high n bits: x &= ~himask; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (long)((unsigned long)a & ~m);
    long y = (long)((unsigned long)b & ~m);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack inverted high-n mask: DNANDHN · DNORHN · DXNORHN (complete high plane after DNANDMN) */
  if (kw(&L->cur,"DNANDHN")||kw(&L->cur,"2NANDHN")||kw(&L->cur,"S2NANDHN")||
      kw(&L->cur,"STACK2NANDHN")||kw(&L->cur,"PAIRNANDHN")||kw(&L->cur,"DHIGHNANDN")||
      kw(&L->cur,"2HIGHNANDN")||kw(&L->cur,"DMASKNANDH")||kw(&L->cur,"2MASKNANDH")){
    /* a b + n → ~(a&hm) ~(b&hm); n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (long)~((unsigned long)a & m);
    long y = (long)~((unsigned long)b & m);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNORHN")||kw(&L->cur,"2NORHN")||kw(&L->cur,"S2NORHN")||
      kw(&L->cur,"STACK2NORHN")||kw(&L->cur,"PAIRNORHN")||kw(&L->cur,"DHIGHNORN")||
      kw(&L->cur,"2HIGHNORN")||kw(&L->cur,"DMASKNORH")||kw(&L->cur,"2MASKNORH")){
    /* a b + n → ~(a|hm) ~(b|hm); n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (long)~((unsigned long)a | m);
    long y = (long)~((unsigned long)b | m);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXNORHN")||kw(&L->cur,"2XNORHN")||kw(&L->cur,"S2XNORHN")||
      kw(&L->cur,"STACK2XNORHN")||kw(&L->cur,"PAIRXNORHN")||kw(&L->cur,"DHIGHXNORN")||
      kw(&L->cur,"2HIGHXNORN")||kw(&L->cur,"DMASKXNORH")||kw(&L->cur,"2MASKXNORH")||
      kw(&L->cur,"DEQUIVHN")||kw(&L->cur,"2EQUIVHN")){
    /* a b + n → ~(a^hm) ~(b^hm); n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (long)~((unsigned long)a ^ m);
    long y = (long)~((unsigned long)b ^ m);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 dual-stack low-n bitfield metrics: DPOPMN · DANYMN · DALLMN */
  if (kw(&L->cur,"DPOPMN")||kw(&L->cur,"2POPMN")||kw(&L->cur,"S2POPMN")||
      kw(&L->cur,"STACK2POPMN")||kw(&L->cur,"PAIRPOPMN")||kw(&L->cur,"DPCNTMN")||
      kw(&L->cur,"2PCNTMN")||kw(&L->cur,"DONESMN")||kw(&L->cur,"2ONESMN")||
      kw(&L->cur,"DLOWPOPN")||kw(&L->cur,"2LOWPOPN")){
    /* a b + n → popcount(a&m) popcount(b&m); m=low-n; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 2] & m;
    unsigned long ub = (unsigned long)vm->stack[vm->sp - 1] & m;
    long x = 0, y = 0;
    while (ua){ x += (long)(ua & 1ul); ua >>= 1; }
    while (ub){ y += (long)(ub & 1ul); ub >>= 1; }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DANYMN")||kw(&L->cur,"2ANYMN")||kw(&L->cur,"S2ANYMN")||
      kw(&L->cur,"STACK2ANYMN")||kw(&L->cur,"PAIRANYMN")||kw(&L->cur,"DLOWANYN")||
      kw(&L->cur,"2LOWANYN")||kw(&L->cur,"DTESTANYN")||kw(&L->cur,"2TESTANYN")){
    /* a b + n → ((a&m)!=0)?1:0  ((b&m)!=0)?1:0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (((unsigned long)a & m) != 0) ? 1 : 0;
    long y = (((unsigned long)b & m) != 0) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DALLMN")||kw(&L->cur,"2ALLMN")||kw(&L->cur,"S2ALLMN")||
      kw(&L->cur,"STACK2ALLMN")||kw(&L->cur,"PAIRALLMN")||kw(&L->cur,"DLOWALLN")||
      kw(&L->cur,"2LOWALLN")||kw(&L->cur,"DTESTALLN")||kw(&L->cur,"2TESTALLN")){
    /* a b + n → ((a&m)==m)?1:0  ((b&m)==m)?1:0 ; n=0 → 1 (vacuous) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (((unsigned long)a & m) == m) ? 1 : 0;
    long y = (((unsigned long)b & m) == m) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 dual-stack high-n bitfield metrics: DPOPHN · DANYHN · DALLHN (dual of low-n plane) */
  if (kw(&L->cur,"DPOPHN")||kw(&L->cur,"2POPHN")||kw(&L->cur,"S2POPHN")||
      kw(&L->cur,"STACK2POPHN")||kw(&L->cur,"PAIRPOPHN")||kw(&L->cur,"DPCNTHN")||
      kw(&L->cur,"2PCNTHN")||kw(&L->cur,"DONESHN")||kw(&L->cur,"2ONESHN")||
      kw(&L->cur,"DHIGHPOPN")||kw(&L->cur,"2HIGHPOPN")){
    /* a b + n → popcount(a&hm) popcount(b&hm); hm=high-n; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 2] & m;
    unsigned long ub = (unsigned long)vm->stack[vm->sp - 1] & m;
    long x = 0, y = 0;
    while (ua){ x += (long)(ua & 1ul); ua >>= 1; }
    while (ub){ y += (long)(ub & 1ul); ub >>= 1; }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DANYHN")||kw(&L->cur,"2ANYHN")||kw(&L->cur,"S2ANYHN")||
      kw(&L->cur,"STACK2ANYHN")||kw(&L->cur,"PAIRANYHN")||kw(&L->cur,"DHIGHANYN")||
      kw(&L->cur,"2HIGHANYN")||kw(&L->cur,"DTESTANYHN")||kw(&L->cur,"2TESTANYHN")){
    /* a b + n → ((a&hm)!=0)?1:0  ((b&hm)!=0)?1:0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (((unsigned long)a & m) != 0) ? 1 : 0;
    long y = (((unsigned long)b & m) != 0) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DALLHN")||kw(&L->cur,"2ALLHN")||kw(&L->cur,"S2ALLHN")||
      kw(&L->cur,"STACK2ALLHN")||kw(&L->cur,"PAIRALLHN")||kw(&L->cur,"DHIGHALLN")||
      kw(&L->cur,"2HIGHALLN")||kw(&L->cur,"DTESTALLHN")||kw(&L->cur,"2TESTALLHN")){
    /* a b + n → ((a&hm)==hm)?1:0  ((b&hm)==hm)?1:0 ; n=0 → 1 (vacuous) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (((unsigned long)a & m) == m) ? 1 : 0;
    long y = (((unsigned long)b & m) == m) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 dual-stack low-n field reverse/rotate: DBREVN · DROLBN · DRORBN */
  if (kw(&L->cur,"DBREVN")||kw(&L->cur,"2BREVN")||kw(&L->cur,"S2BREVN")||
      kw(&L->cur,"STACK2BREVN")||kw(&L->cur,"PAIRBREVN")||kw(&L->cur,"DREVLOWN")||
      kw(&L->cur,"2REVLOWN")||kw(&L->cur,"DBITREVN")||kw(&L->cur,"2BITREVN")){
    /* a b + n → reverse low n bits of each; high bits kept; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a, y = b;
    if (n > 0 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m, lb = (unsigned long)b & m;
      unsigned long ra = 0, rb = 0;
      for (long i = 0; i < n; i++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
        rb = (rb << 1) | (lb & 1ul); lb >>= 1;
      }
      x = (long)(((unsigned long)a & ~m) | (ra & m));
      y = (long)(((unsigned long)b & ~m) | (rb & m));
    } else if (n >= 64){
      /* full reverse 64 */
      unsigned long la = (unsigned long)a, lb = (unsigned long)b;
      unsigned long ra = 0, rb = 0;
      for (int i = 0; i < 64; i++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
        rb = (rb << 1) | (lb & 1ul); lb >>= 1;
      }
      x = (long)ra; y = (long)rb;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROLBN")||kw(&L->cur,"2ROLBN")||kw(&L->cur,"S2ROLBN")||
      kw(&L->cur,"STACK2ROLBN")||kw(&L->cur,"PAIRROLBN")||kw(&L->cur,"DROTLBN")||
      kw(&L->cur,"2ROTLBN")||kw(&L->cur,"DLOWROLN")||kw(&L->cur,"2LOWROLN")){
    /* a b + n → rotate left by 1 within low n bits; high kept; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a, y = b;
    if (n >= 2 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m, lb = (unsigned long)b & m;
      la = ((la << 1) | (la >> (unsigned)(n - 1))) & m;
      lb = ((lb << 1) | (lb >> (unsigned)(n - 1))) & m;
      x = (long)(((unsigned long)a & ~m) | la);
      y = (long)(((unsigned long)b & ~m) | lb);
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
      x = (long)((ua << 1) | (ua >> 63));
      y = (long)((ub << 1) | (ub >> 63));
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DRORBN")||kw(&L->cur,"2RORBN")||kw(&L->cur,"S2RORBN")||
      kw(&L->cur,"STACK2RORBN")||kw(&L->cur,"PAIRRORBN")||kw(&L->cur,"DROTRBN")||
      kw(&L->cur,"2ROTRBN")||kw(&L->cur,"DLOWRORN")||kw(&L->cur,"2LOWRORN")){
    /* a b + n → rotate right by 1 within low n bits; high kept; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a, y = b;
    if (n >= 2 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m, lb = (unsigned long)b & m;
      la = ((la >> 1) | (la << (unsigned)(n - 1))) & m;
      lb = ((lb >> 1) | (lb << (unsigned)(n - 1))) & m;
      x = (long)(((unsigned long)a & ~m) | la);
      y = (long)(((unsigned long)b & ~m) | lb);
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
      x = (long)((ua >> 1) | (ua << 63));
      y = (long)((ub >> 1) | (ub << 63));
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack high-n field reverse/rotate: DBREVHN · DROLHN · DRORHN */
  if (kw(&L->cur,"DBREVHN")||kw(&L->cur,"2BREVHN")||kw(&L->cur,"S2BREVHN")||
      kw(&L->cur,"STACK2BREVHN")||kw(&L->cur,"PAIRBREVHN")||kw(&L->cur,"DREVHIGHN")||
      kw(&L->cur,"2REVHIGHN")||kw(&L->cur,"DBITREVHN")||kw(&L->cur,"2BITREVHN")){
    /* a b + n → reverse high n bits of each; low bits kept; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a, y = b;
    if (n > 0 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long lb = ((unsigned long)b & m) >> sh;
      unsigned long ra = 0, rb = 0;
      for (long i = 0; i < n; i++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
        rb = (rb << 1) | (lb & 1ul); lb >>= 1;
      }
      x = (long)(((unsigned long)a & ~m) | ((ra << sh) & m));
      y = (long)(((unsigned long)b & ~m) | ((rb << sh) & m));
    } else if (n >= 64){
      unsigned long la = (unsigned long)a, lb = (unsigned long)b;
      unsigned long ra = 0, rb = 0;
      for (int i = 0; i < 64; i++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
        rb = (rb << 1) | (lb & 1ul); lb >>= 1;
      }
      x = (long)ra; y = (long)rb;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROLHN")||kw(&L->cur,"2ROLHN")||kw(&L->cur,"S2ROLHN")||
      kw(&L->cur,"STACK2ROLHN")||kw(&L->cur,"PAIRROLHN")||kw(&L->cur,"DROTLHN")||
      kw(&L->cur,"2ROTLHN")||kw(&L->cur,"DHIGHROLN")||kw(&L->cur,"2HIGHROLN")){
    /* a b + n → rotate left by 1 within high n bits; low kept; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a, y = b;
    if (n >= 2 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long lb = ((unsigned long)b & m) >> sh;
      unsigned long fm = (1ul << (unsigned)n) - 1ul;
      la = ((la << 1) | (la >> (unsigned)(n - 1))) & fm;
      lb = ((lb << 1) | (lb >> (unsigned)(n - 1))) & fm;
      x = (long)(((unsigned long)a & ~m) | ((la << sh) & m));
      y = (long)(((unsigned long)b & ~m) | ((lb << sh) & m));
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
      x = (long)((ua << 1) | (ua >> 63));
      y = (long)((ub << 1) | (ub >> 63));
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DRORHN")||kw(&L->cur,"2RORHN")||kw(&L->cur,"S2RORHN")||
      kw(&L->cur,"STACK2RORHN")||kw(&L->cur,"PAIRRORHN")||kw(&L->cur,"DROTRHN")||
      kw(&L->cur,"2ROTRHN")||kw(&L->cur,"DHIGHRORN")||kw(&L->cur,"2HIGHRORN")){
    /* a b + n → rotate right by 1 within high n bits; low kept; n clamped 0..64 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a, y = b;
    if (n >= 2 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long lb = ((unsigned long)b & m) >> sh;
      unsigned long fm = (1ul << (unsigned)n) - 1ul;
      la = ((la >> 1) | (la << (unsigned)(n - 1))) & fm;
      lb = ((lb >> 1) | (lb << (unsigned)(n - 1))) & fm;
      x = (long)(((unsigned long)a & ~m) | ((la << sh) & m));
      y = (long)(((unsigned long)b & ~m) | ((lb << sh) & m));
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
      x = (long)((ua >> 1) | (ua << 63));
      y = (long)((ub >> 1) | (ub << 63));
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack immediate rotate: DROLN · DRORN (dual of SROLN/SRORN) */
  if (kw(&L->cur,"DROLN")||kw(&L->cur,"2ROLN")||kw(&L->cur,"S2ROLN")||
      kw(&L->cur,"STACK2ROLN")||kw(&L->cur,"PAIRROLN")||kw(&L->cur,"DROTLN")||
      kw(&L->cur,"2ROTLN")||kw(&L->cur,"PAIRROTLN")||kw(&L->cur,"DROLIMM")){
    /* a b + n → rotl(a,n) rotl(b,n); n mod 64; n<0 → 0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    unsigned uk = (unsigned)(n & 63);
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long ub = (unsigned long)vm->stack[vm->sp - 1];
    long x = (uk == 0) ? (long)ua : (long)((ua << uk) | (ua >> (64u - uk)));
    long y = (uk == 0) ? (long)ub : (long)((ub << uk) | (ub >> (64u - uk)));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DRORN")||kw(&L->cur,"2RORN")||kw(&L->cur,"S2RORN")||
      kw(&L->cur,"STACK2RORN")||kw(&L->cur,"PAIRRORN")||kw(&L->cur,"DROTRN")||
      kw(&L->cur,"2ROTRN")||kw(&L->cur,"PAIRROTRN")||kw(&L->cur,"DRORIMM")){
    /* a b + n → rotr(a,n) rotr(b,n); n mod 64; n<0 → 0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    unsigned uk = (unsigned)(n & 63);
    unsigned long ua = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long ub = (unsigned long)vm->stack[vm->sp - 1];
    long x = (uk == 0) ? (long)ua : (long)((ua >> uk) | (ua << (64u - uk)));
    long y = (uk == 0) ? (long)ub : (long)((ub >> uk) | (ub << (64u - uk)));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack immediate shift: DSHLN · DSHRN · DSARN (dual of SSHLN/SSHRN/SSARN) */
  if (kw(&L->cur,"DSHLN")||kw(&L->cur,"2SHLN")||kw(&L->cur,"S2SHLN")||
      kw(&L->cur,"STACK2SHLN")||kw(&L->cur,"PAIRSHLN")||kw(&L->cur,"DSHLIMM")||
      kw(&L->cur,"2SHLIMM")||kw(&L->cur,"PAIRSHLIMM")){
    /* a b + n → (a<<n) (b<<n); n clamped 0..63 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (long)((unsigned long)a << (unsigned)n);
    long y = (long)((unsigned long)b << (unsigned)n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSHRN")||kw(&L->cur,"2SHRN")||kw(&L->cur,"S2SHRN")||
      kw(&L->cur,"STACK2SHRN")||kw(&L->cur,"PAIRSHRN")||kw(&L->cur,"DSHRIMM")||
      kw(&L->cur,"2SHRIMM")||kw(&L->cur,"PAIRSHRIMM")){
    /* a b + n → logical (a>>n) (b>>n); n clamped 0..63 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (long)((unsigned long)a >> (unsigned)n);
    long y = (long)((unsigned long)b >> (unsigned)n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSARN")||kw(&L->cur,"2SARN")||kw(&L->cur,"S2SARN")||
      kw(&L->cur,"STACK2SARN")||kw(&L->cur,"PAIRSARN")||kw(&L->cur,"DASHRN")||
      kw(&L->cur,"2ASHRN")||kw(&L->cur,"DSARIMM")||kw(&L->cur,"PAIRASHRN")){
    /* a b + n → arithmetic (a>>n) (b>>n); n clamped 0..63 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a >> n;
    long y = b >> n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack immediate ALU: DADDN · DSUBN · DMULN (dual of SADDN/SSUBN/SMULN) */
  if (kw(&L->cur,"DADDN")||kw(&L->cur,"2ADDN")||kw(&L->cur,"S2ADDN")||
      kw(&L->cur,"STACK2ADDN")||kw(&L->cur,"PAIRADDN")||kw(&L->cur,"DADDIMM")||
      kw(&L->cur,"2ADDIMM")||kw(&L->cur,"PAIRADDIMM")||kw(&L->cur,"DPLUSN")){
    /* a b + n → (a+n) (b+n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a + n;
    long y = b + n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSUBN")||kw(&L->cur,"2SUBN")||kw(&L->cur,"S2SUBN")||
      kw(&L->cur,"STACK2SUBN")||kw(&L->cur,"PAIRSUBN")||kw(&L->cur,"DSUBIMM")||
      kw(&L->cur,"2SUBIMM")||kw(&L->cur,"PAIRSUBIMM")||kw(&L->cur,"DMINUSN")){
    /* a b + n → (a-n) (b-n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a - n;
    long y = b - n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMULN")||kw(&L->cur,"2MULN")||kw(&L->cur,"S2MULN")||
      kw(&L->cur,"STACK2MULN")||kw(&L->cur,"PAIRMULN")||kw(&L->cur,"DMULIMM")||
      kw(&L->cur,"2MULIMM")||kw(&L->cur,"PAIRMULIMM")||kw(&L->cur,"DTIMESN")){
    /* a b + n → (a*n) (b*n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a * n;
    long y = b * n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack imm saturating ALU: DSATADDN · DSATSUBN · DSATMULN (dual of SSATADDN) */
  if (kw(&L->cur,"DSATADDN")||kw(&L->cur,"S2SATADDN")||kw(&L->cur,"STACK2SATADDN")||
      kw(&L->cur,"PAIRSATADDN")||kw(&L->cur,"DADDSATN")||kw(&L->cur,"PAIRADDSATN")||
      kw(&L->cur,"DSATADDIMM")){
    /* a b + n → sat(a+n) sat(b+n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x, y;
    if (n > 0 && a > LONG_MAX - n) x = LONG_MAX;
    else if (n < 0 && a < LONG_MIN - n) x = LONG_MIN;
    else x = a + n;
    if (n > 0 && b > LONG_MAX - n) y = LONG_MAX;
    else if (n < 0 && b < LONG_MIN - n) y = LONG_MIN;
    else y = b + n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSATSUBN")||kw(&L->cur,"S2SATSUBN")||kw(&L->cur,"STACK2SATSUBN")||
      kw(&L->cur,"PAIRSATSUBN")||kw(&L->cur,"DSUBSATN")||kw(&L->cur,"PAIRSUBSATN")||
      kw(&L->cur,"DSATSUBIMM")){
    /* a b + n → sat(a-n) sat(b-n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x, y;
    if (n > 0 && a < LONG_MIN + n) x = LONG_MIN;
    else if (n < 0 && a > LONG_MAX + n) x = LONG_MAX;
    else x = a - n;
    if (n > 0 && b < LONG_MIN + n) y = LONG_MIN;
    else if (n < 0 && b > LONG_MAX + n) y = LONG_MAX;
    else y = b - n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSATMULN")||kw(&L->cur,"S2SATMULN")||kw(&L->cur,"STACK2SATMULN")||
      kw(&L->cur,"PAIRSATMULN")||kw(&L->cur,"DMULSATN")||kw(&L->cur,"PAIRMULSATN")||
      kw(&L->cur,"DSATMULIMM")){
    /* a b + n → sat(a*n) sat(b*n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x, y;
    if (a == 0 || n == 0) x = 0;
    else {
      __int128 p = (__int128)a * (__int128)n;
      if (p > (__int128)LONG_MAX) x = LONG_MAX;
      else if (p < (__int128)LONG_MIN) x = LONG_MIN;
      else x = (long)p;
    }
    if (b == 0 || n == 0) y = 0;
    else {
      __int128 p = (__int128)b * (__int128)n;
      if (p > (__int128)LONG_MAX) y = LONG_MAX;
      else if (p < (__int128)LONG_MIN) y = LONG_MIN;
      else y = (long)p;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 dual-stack imm sat div: DSATDIVN (completes sat imm ALU + energy plane) */
  if (kw(&L->cur,"DSATDIVN")||kw(&L->cur,"S2SATDIVN")||kw(&L->cur,"STACK2SATDIVN")||
      kw(&L->cur,"PAIRSATDIVN")||kw(&L->cur,"DDIVSATN")||kw(&L->cur,"PAIRDIVSATN")||
      kw(&L->cur,"DSATDIVIMM")){
    /* a b + n → sat(a/n) sat(b/n); n==0 → 0; LONG_MIN/-1 → LONG_MAX */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x, y;
    if (n == 0) x = 0;
    else if (a == LONG_MIN && n == -1) x = LONG_MAX;
    else x = a / n;
    if (n == 0) y = 0;
    else if (b == LONG_MIN && n == -1) y = LONG_MAX;
    else y = b / n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 dual-stack imm reverse sat: DSATSUBFROMN · DSATDIVFROMN (dual of SSATSUBFROMN plane) */
  if (kw(&L->cur,"DSATSUBFROMN")||kw(&L->cur,"S2SATSUBFROMN")||kw(&L->cur,"STACK2SATSUBFROMN")||
      kw(&L->cur,"PAIRSATSUBFROMN")||kw(&L->cur,"DSATRSUBN")||kw(&L->cur,"PAIRSATRSUBN")||
      kw(&L->cur,"DSATSUBFROMIMM")||kw(&L->cur,"DNSATSUBN")){
    /* a b + n → sat(n-a) sat(n-b) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x, y;
    if (a > 0 && n < LONG_MIN + a) x = LONG_MIN;
    else if (a < 0 && n > LONG_MAX + a) x = LONG_MAX;
    else x = n - a;
    if (b > 0 && n < LONG_MIN + b) y = LONG_MIN;
    else if (b < 0 && n > LONG_MAX + b) y = LONG_MAX;
    else y = n - b;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSATDIVFROMN")||kw(&L->cur,"S2SATDIVFROMN")||kw(&L->cur,"STACK2SATDIVFROMN")||
      kw(&L->cur,"PAIRSATDIVFROMN")||kw(&L->cur,"DSATRDIVN")||kw(&L->cur,"PAIRSATRDIVN")||
      kw(&L->cur,"DSATDIVFROMIMM")||kw(&L->cur,"DNSATDIVN")){
    /* a b + n → sat(n/a) sat(n/b); lane0 → 0; LONG_MIN/-1 → LONG_MAX */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x, y;
    if (a == 0) x = 0;
    else if (n == LONG_MIN && a == -1) x = LONG_MAX;
    else x = n / a;
    if (b == 0) y = 0;
    else if (n == LONG_MIN && b == -1) y = LONG_MAX;
    else y = n / b;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack immediate div/mod: DDIVN · DMODN (dual of SDIVN/SMODN; complete imm ALU) */
  if (kw(&L->cur,"DDIVN")||kw(&L->cur,"2DIVN")||kw(&L->cur,"S2DIVN")||
      kw(&L->cur,"STACK2DIVN")||kw(&L->cur,"PAIRDIVN")||kw(&L->cur,"DDIVIMM")||
      kw(&L->cur,"2DIVIMM")||kw(&L->cur,"PAIRDIVIMM")||kw(&L->cur,"DQUOTN")){
    /* a b + n → (a/n) (b/n); n==0 → 0,0 soft */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (n == 0) ? 0 : (a / n);
    long y = (n == 0) ? 0 : (b / n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMODN")||kw(&L->cur,"2MODN")||kw(&L->cur,"S2MODN")||
      kw(&L->cur,"STACK2MODN")||kw(&L->cur,"PAIRMODN")||kw(&L->cur,"DMODIMM")||
      kw(&L->cur,"2MODIMM")||kw(&L->cur,"PAIRMODIMM")||kw(&L->cur,"DREMN")){
    /* a b + n → (a%n) (b%n); n==0 → 0,0 soft */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (n == 0) ? 0 : (a % n);
    long y = (n == 0) ? 0 : (b % n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm ceil/floor div: DDIVCEILN · DDIVFLOORN (dual of SDIVCEILN plane) */
  if (kw(&L->cur,"DDIVCEILN")||kw(&L->cur,"2DIVCEILN")||kw(&L->cur,"S2DIVCEILN")||
      kw(&L->cur,"STACK2DIVCEILN")||kw(&L->cur,"PAIRDIVCEILN")||kw(&L->cur,"DCEILDIVN")||
      kw(&L->cur,"2CEILDIVN")||kw(&L->cur,"PAIRCEILDIVN")||kw(&L->cur,"DDIVCEILIMM")){
    /* a b + n → ceil(a/n) ceil(b/n); n==0 → 0,0 soft */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (n != 0){
      if (a >= 0 && n > 0) x = (a + n - 1) / n;
      else if (a <= 0 && n < 0){ long aa = -a, nn = -n; x = (aa + nn - 1) / nn; }
      else x = a / n;
      if (b >= 0 && n > 0) y = (b + n - 1) / n;
      else if (b <= 0 && n < 0){ long bb = -b, nn = -n; y = (bb + nn - 1) / nn; }
      else y = b / n;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DDIVFLOORN")||kw(&L->cur,"2DIVFLOORN")||kw(&L->cur,"S2DIVFLOORN")||
      kw(&L->cur,"STACK2DIVFLOORN")||kw(&L->cur,"PAIRDIVFLOORN")||kw(&L->cur,"DFLOORDIVN")||
      kw(&L->cur,"2FLOORDIVN")||kw(&L->cur,"PAIRFLOORDIVN")||kw(&L->cur,"DDIVFLOORIMM")){
    /* a b + n → floor(a/n) floor(b/n); n==0 → 0,0 soft */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (n != 0){
      long qx = a / n, rx = a % n;
      if (rx != 0 && ((a < 0) != (n < 0))) qx--;
      x = qx;
      long qy = b / n, ry = b % n;
      if (ry != 0 && ((b < 0) != (n < 0))) qy--;
      y = qy;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 dual-stack imm reverse ALU: DSUBFROMN · DDIVFROMN · DMODFROMN (dual of SSUBFROMN plane) */
  if (kw(&L->cur,"DSUBFROMN")||kw(&L->cur,"S2SUBFROMN")||kw(&L->cur,"STACK2SUBFROMN")||
      kw(&L->cur,"PAIRSUBFROMN")||kw(&L->cur,"DRSUBN")||kw(&L->cur,"PAIRRSUBN")||
      kw(&L->cur,"DNSUBN")||kw(&L->cur,"DSUBFROMIMM")){
    /* a b + n → (n-a) (n-b) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = n - a;
    long y = n - b;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DDIVFROMN")||kw(&L->cur,"S2DIVFROMN")||kw(&L->cur,"STACK2DIVFROMN")||
      kw(&L->cur,"PAIRDIVFROMN")||kw(&L->cur,"DRDIVN")||kw(&L->cur,"PAIRRDIVN")||
      kw(&L->cur,"DDIVFROMIMM")){
    /* a b + n → (a==0?0:n/a) (b==0?0:n/b) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a == 0) ? 0 : (n / a);
    long y = (b == 0) ? 0 : (n / b);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMODFROMN")||kw(&L->cur,"S2MODFROMN")||kw(&L->cur,"STACK2MODFROMN")||
      kw(&L->cur,"PAIRMODFROMN")||kw(&L->cur,"DRMODN")||kw(&L->cur,"PAIRRMODN")||
      kw(&L->cur,"DREMFROMN")||kw(&L->cur,"PAIRREMFROMN")||kw(&L->cur,"DMODFROMIMM")){
    /* a b + n → (a==0?0:n%a) (b==0?0:n%b) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a == 0) ? 0 : (n % a);
    long y = (b == 0) ? 0 : (n % b);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 dual-stack imm reverse unsigned: DUDIVFROMN · DUMODFROMN (dual of SUDIVFROMN plane) */
  if (kw(&L->cur,"DUDIVFROMN")||kw(&L->cur,"S2UDIVFROMN")||kw(&L->cur,"STACK2UDIVFROMN")||
      kw(&L->cur,"PAIRUDIVFROMN")||kw(&L->cur,"DRUDIVN")||kw(&L->cur,"PAIRRUDIVN")||
      kw(&L->cur,"DUDIVFROMIMM")||kw(&L->cur,"NUDIVFROMN")){
    /* a b + n → unsigned (a==0?0:n/a) (b==0?0:n/b) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (a != 0) x = (long)((unsigned long)n / (unsigned long)a);
    if (b != 0) y = (long)((unsigned long)n / (unsigned long)b);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DUMODFROMN")||kw(&L->cur,"S2UMODFROMN")||kw(&L->cur,"STACK2UMODFROMN")||
      kw(&L->cur,"PAIRUMODFROMN")||kw(&L->cur,"DRUMODN")||kw(&L->cur,"PAIRRUMODN")||
      kw(&L->cur,"DUMODFROMIMM")||kw(&L->cur,"DUREMFROMN")||kw(&L->cur,"PAIRUREMFROMN")){
    /* a b + n → unsigned (a==0?0:n%a) (b==0?0:n%b) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = 0, y = 0;
    if (a != 0) x = (long)((unsigned long)n % (unsigned long)a);
    if (b != 0) y = (long)((unsigned long)n % (unsigned long)b);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack immediate bitwise mask: DANDI · DORI · DXORI (dual of SANDI/SORI/SXORI) */
  if (kw(&L->cur,"DANDI")||kw(&L->cur,"2ANDI")||kw(&L->cur,"S2ANDI")||
      kw(&L->cur,"STACK2ANDI")||kw(&L->cur,"PAIRANDI")||kw(&L->cur,"DANDIMM")||
      kw(&L->cur,"2ANDIMM")||kw(&L->cur,"PAIRANDIMM")){
    /* a b + n → (a&n) (b&n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a & n;
    long y = b & n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DORI")||kw(&L->cur,"2ORI")||kw(&L->cur,"S2ORI")||
      kw(&L->cur,"STACK2ORI")||kw(&L->cur,"PAIRORI")||kw(&L->cur,"DORIMM")||
      kw(&L->cur,"2ORIMM")||kw(&L->cur,"PAIRORIMM")){
    /* a b + n → (a|n) (b|n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a | n;
    long y = b | n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXORI")||kw(&L->cur,"2XORI")||kw(&L->cur,"S2XORI")||
      kw(&L->cur,"STACK2XORI")||kw(&L->cur,"PAIRXORI")||kw(&L->cur,"DXORIMM")||
      kw(&L->cur,"2XORIMM")||kw(&L->cur,"PAIRXORIMM")){
    /* a b + n → (a^n) (b^n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a ^ n;
    long y = b ^ n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack immediate inverted bitwise: DNANDI · DNORI · DXNORI (dual of SNANDI/SNORI/SXNORI) */
  if (kw(&L->cur,"DNANDI")||kw(&L->cur,"2NANDI")||kw(&L->cur,"S2NANDI")||
      kw(&L->cur,"STACK2NANDI")||kw(&L->cur,"PAIRNANDI")||kw(&L->cur,"DNANDIMM")||
      kw(&L->cur,"2NANDIMM")||kw(&L->cur,"PAIRNANDIMM")){
    /* a b + n → ~(a&n) ~(b&n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = ~(a & n);
    long y = ~(b & n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNORI")||kw(&L->cur,"2NORI")||kw(&L->cur,"S2NORI")||
      kw(&L->cur,"STACK2NORI")||kw(&L->cur,"PAIRNORI")||kw(&L->cur,"DNORIMM")||
      kw(&L->cur,"2NORIMM")||kw(&L->cur,"PAIRNORIMM")){
    /* a b + n → ~(a|n) ~(b|n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = ~(a | n);
    long y = ~(b | n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXNORI")||kw(&L->cur,"2XNORI")||kw(&L->cur,"S2XNORI")||
      kw(&L->cur,"STACK2XNORI")||kw(&L->cur,"PAIRXNORI")||kw(&L->cur,"DXNORIMM")||
      kw(&L->cur,"2XNORIMM")||kw(&L->cur,"PAIRXNORIMM")||kw(&L->cur,"DEQUIVI")||
      kw(&L->cur,"2EQUIVI")){
    /* a b + n → ~(a^n) ~(b^n)  (bitwise equivalence with constant) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = ~(a ^ n);
    long y = ~(b ^ n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 dual-stack immediate ANDN plane: DANDNI · DORNI · DXORNI (dual of SANDNI/SORNI/SXORNI) */
  if (kw(&L->cur,"DANDNI")||kw(&L->cur,"2ANDNI")||kw(&L->cur,"S2ANDNI")||
      kw(&L->cur,"STACK2ANDNI")||kw(&L->cur,"PAIRANDNI")||kw(&L->cur,"DBICI")||
      kw(&L->cur,"DBICIMM")||kw(&L->cur,"DANDNOTI")||kw(&L->cur,"2ANDNOTI")||
      kw(&L->cur,"PAIRANDNOTI")){
    /* a b + n → (a&~n) (b&~n)  clear bits set in n on both tops */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a & ~n;
    long y = b & ~n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DORNI")||kw(&L->cur,"2ORNI")||kw(&L->cur,"S2ORNI")||
      kw(&L->cur,"STACK2ORNI")||kw(&L->cur,"PAIRORNI")||kw(&L->cur,"DORNOTI")||
      kw(&L->cur,"2ORNOTI")||kw(&L->cur,"PAIRORNOTI")){
    /* a b + n → (a|~n) (b|~n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a | ~n;
    long y = b | ~n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXORNI")||kw(&L->cur,"2XORNI")||kw(&L->cur,"S2XORNI")||
      kw(&L->cur,"STACK2XORNI")||kw(&L->cur,"PAIRXORNI")||kw(&L->cur,"DXORNOTI")||
      kw(&L->cur,"2XORNOTI")||kw(&L->cur,"PAIRXORNOTI")){
    /* a b + n → (a^~n) (b^~n)  (equiv DXNORI / ~(a^n) ~(b^n)) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a ^ ~n;
    long y = b ^ ~n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack imm inverted ANDN plane: DNANDNI · DNORNI · DXNORNI (dual of SNANDNI) */
  if (kw(&L->cur,"DNANDNI")||kw(&L->cur,"S2NANDNI")||kw(&L->cur,"STACK2NANDNI")||
      kw(&L->cur,"PAIRNANDNI")||kw(&L->cur,"DINVERTANDNI")||kw(&L->cur,"DNANDNOTI")||
      kw(&L->cur,"PAIRNANDNOTI")){
    /* a b + n → ~(a&~n) ~(b&~n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = ~(a & ~n);
    long y = ~(b & ~n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNORNI")||kw(&L->cur,"S2NORNI")||kw(&L->cur,"STACK2NORNI")||
      kw(&L->cur,"PAIRNORNI")||kw(&L->cur,"DINVERTORNI")||kw(&L->cur,"DNORNOTI")||
      kw(&L->cur,"PAIRNORNOTI")){
    /* a b + n → ~(a|~n) ~(b|~n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = ~(a | ~n);
    long y = ~(b | ~n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXNORNI")||kw(&L->cur,"S2XNORNI")||kw(&L->cur,"STACK2XNORNI")||
      kw(&L->cur,"PAIRXNORNI")||kw(&L->cur,"DEQUIVNI")||kw(&L->cur,"DXNORNOTI")||
      kw(&L->cur,"PAIRXNORNOTI")||kw(&L->cur,"PAIRSEQUIVNI")){
    /* a b + n → ~(a^~n) ~(b^~n)  (equiv DXORI / a^n , b^n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = ~(a ^ ~n);
    long y = ~(b ^ ~n);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 dual-stack reverse imm ANDN: DANDNFROMN · DORNFROMN · DXORNFROMN
   * (a b + n → (n op ~a) (n op ~b); dual of SANDNFROMN after DANDNI plane) */
  if (kw(&L->cur,"DANDNFROMN")||kw(&L->cur,"2ANDNFROMN")||kw(&L->cur,"S2ANDNFROMN")||
      kw(&L->cur,"STACK2ANDNFROMN")||kw(&L->cur,"PAIRANDNFROMN")||kw(&L->cur,"DBICFROMN")||
      kw(&L->cur,"2BICFROMN")||kw(&L->cur,"PAIRBICFROMN")||kw(&L->cur,"DANDNFROMIMM")||
      kw(&L->cur,"DRANDNFROMN")||kw(&L->cur,"PAIRANDNOTFROMN")){
    /* a b + n → (n & ~a) (n & ~b) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = n & ~a;
    long y = n & ~b;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DORNFROMN")||kw(&L->cur,"2ORNFROMN")||kw(&L->cur,"S2ORNFROMN")||
      kw(&L->cur,"STACK2ORNFROMN")||kw(&L->cur,"PAIRORNFROMN")||kw(&L->cur,"DORNOTFROMN")||
      kw(&L->cur,"2ORNOTFROMN")||kw(&L->cur,"PAIRORNOTFROMN")||kw(&L->cur,"DORNFROMIMM")||
      kw(&L->cur,"DRORNFROMN")){
    /* a b + n → (n | ~a) (n | ~b) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = n | ~a;
    long y = n | ~b;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXORNFROMN")||kw(&L->cur,"2XORNFROMN")||kw(&L->cur,"S2XORNFROMN")||
      kw(&L->cur,"STACK2XORNFROMN")||kw(&L->cur,"PAIRXORNFROMN")||kw(&L->cur,"DXORNOTFROMN")||
      kw(&L->cur,"2XORNOTFROMN")||kw(&L->cur,"PAIRXORNOTFROMN")||kw(&L->cur,"DXORNFROMIMM")||
      kw(&L->cur,"DRXORNFROMN")){
    /* a b + n → (n ^ ~a) (n ^ ~b) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = n ^ ~a;
    long y = n ^ ~b;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 dual-stack reverse imm inverted ANDN: DNANDNFROMN · DNORNFROMN · DXNORNFROMN
   * (a b + n → ~(n op ~a) ~(n op ~b); dual of SNANDNFROMN after DANDNFROMN plane) */
  if (kw(&L->cur,"DNANDNFROMN")||kw(&L->cur,"2NANDNFROMN")||kw(&L->cur,"S2NANDNFROMN")||
      kw(&L->cur,"STACK2NANDNFROMN")||kw(&L->cur,"PAIRNANDNFROMN")||kw(&L->cur,"DINVERTANDNFROMN")||
      kw(&L->cur,"DNANDNFROMIMM")||kw(&L->cur,"DRNANDNFROMN")||kw(&L->cur,"PAIRNANDNOTFROMN")){
    /* a b + n → ~(n & ~a) ~(n & ~b)  (= (~n|a) (~n|b)) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = ~(n & ~a);
    long y = ~(n & ~b);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNORNFROMN")||kw(&L->cur,"2NORNFROMN")||kw(&L->cur,"S2NORNFROMN")||
      kw(&L->cur,"STACK2NORNFROMN")||kw(&L->cur,"PAIRNORNFROMN")||kw(&L->cur,"DINVERTORNFROMN")||
      kw(&L->cur,"DNORNFROMIMM")||kw(&L->cur,"DRNORNFROMN")||kw(&L->cur,"PAIRNORNOTFROMN")){
    /* a b + n → ~(n | ~a) ~(n | ~b)  (= (~n&a) (~n&b)) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = ~(n | ~a);
    long y = ~(n | ~b);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXNORNFROMN")||kw(&L->cur,"2XNORNFROMN")||kw(&L->cur,"S2XNORNFROMN")||
      kw(&L->cur,"STACK2XNORNFROMN")||kw(&L->cur,"PAIRXNORNFROMN")||kw(&L->cur,"DEQUIVNFROMN")||
      kw(&L->cur,"DXNORNFROMIMM")||kw(&L->cur,"DRXNORNFROMN")||kw(&L->cur,"PAIRXNORNOTFROMN")){
    /* a b + n → ~(n ^ ~a) ~(n ^ ~b)  (equiv (n^a) (n^b)) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = ~(n ^ ~a);
    long y = ~(n ^ ~b);
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 dual-stack immediate min/max/clamp: DMINN · DMAXN · DCLAMPN (dual of SMINN/SMAXN) */
  if (kw(&L->cur,"DMINN")||kw(&L->cur,"2MINN")||kw(&L->cur,"S2MINN")||
      kw(&L->cur,"STACK2MINN")||kw(&L->cur,"PAIRMINN")||kw(&L->cur,"DMINIMM")||
      kw(&L->cur,"2MINIMM")||kw(&L->cur,"PAIRMINIMM")){
    /* a b + n → min(a,n) min(b,n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a < n ? a : n;
    long y = b < n ? b : n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMAXN")||kw(&L->cur,"2MAXN")||kw(&L->cur,"S2MAXN")||
      kw(&L->cur,"STACK2MAXN")||kw(&L->cur,"PAIRMAXN")||kw(&L->cur,"DMAXIMM")||
      kw(&L->cur,"2MAXIMM")||kw(&L->cur,"PAIRMAXIMM")){
    /* a b + n → max(a,n) max(b,n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a > n ? a : n;
    long y = b > n ? b : n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
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
  if (kw(&L->cur,"DCLAMPN")||kw(&L->cur,"2CLAMPN")||kw(&L->cur,"S2CLAMPN")||
      kw(&L->cur,"STACK2CLAMPN")||kw(&L->cur,"PAIRCLAMPN")||kw(&L->cur,"DCLAMPIMM")||
      kw(&L->cur,"2CLAMPIMM")||kw(&L->cur,"PAIRCLAMPIMM")||kw(&L->cur,"DBOUNDN")){
    /* a b + lo hi → clamp(a,[lo,hi]) clamp(b,[lo,hi]); swap lo/hi if needed */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (lo > hi){ long t=lo; lo=hi; hi=t; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a; if (x < lo) x = lo; if (x > hi) x = hi;
    long y = b; if (y < lo) y = lo; if (y > hi) y = hi;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack imm range preds: DBETWEENN · DWITHINN (dual of SBETWEENN/SWITHINN; complete DCLAMPN) */
  if (kw(&L->cur,"DBETWEENN")||kw(&L->cur,"S2BETWEENN")||kw(&L->cur,"STACK2BETWEENN")||
      kw(&L->cur,"PAIRBETWEENN")||kw(&L->cur,"DINRANGEN")||kw(&L->cur,"PAIRINRANGEN")||
      kw(&L->cur,"DBETWEENIMM")||kw(&L->cur,"PAIRBETWEENIMM")){
    /* a b + lo hi → (a in [lo,hi]?1:0) (b in [lo,hi]?1:0); swap lo/hi if needed */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (lo > hi){ long t=lo; lo=hi; hi=t; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a >= lo && a <= hi) ? 1 : 0;
    long y = (b >= lo && b <= hi) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DWITHINN")||kw(&L->cur,"S2WITHINN")||kw(&L->cur,"STACK2WITHINN")||
      kw(&L->cur,"PAIRWITHINN")||kw(&L->cur,"DINTERVALN")||kw(&L->cur,"PAIRINTERVALN")||
      kw(&L->cur,"DWITHINIMM")||kw(&L->cur,"PAIRWITHINIMM")){
    /* a b + lo hi → (lo<=a<hi?1:0) (lo<=b<hi?1:0); hi exclusive, no lo/hi swap */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a >= lo && a < hi) ? 1 : 0;
    long y = (b >= lo && b < hi) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack immediate compare: DEQN · DLTN · DGTN (dual of SEQN/SLTN/SGTN) */
  if (kw(&L->cur,"DEQN")||kw(&L->cur,"2EQN")||kw(&L->cur,"S2EQN")||
      kw(&L->cur,"STACK2EQN")||kw(&L->cur,"PAIREQN")||kw(&L->cur,"DEQIMM")||
      kw(&L->cur,"2EQIMM")||kw(&L->cur,"PAIREQIMM")){
    /* a b + n → (a==n?1:0) (b==n?1:0) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a == n) ? 1 : 0;
    long y = (b == n) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DLTN")||kw(&L->cur,"2LTN")||kw(&L->cur,"S2LTN")||
      kw(&L->cur,"STACK2LTN")||kw(&L->cur,"PAIRLTN")||kw(&L->cur,"DLTIMM")||
      kw(&L->cur,"2LTIMM")||kw(&L->cur,"PAIRLTIMM")){
    /* a b + n → (a<n?1:0) (b<n?1:0) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a < n) ? 1 : 0;
    long y = (b < n) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGTN")||kw(&L->cur,"2GTN")||kw(&L->cur,"S2GTN")||
      kw(&L->cur,"STACK2GTN")||kw(&L->cur,"PAIRGTN")||kw(&L->cur,"DGTIMM")||
      kw(&L->cur,"2GTIMM")||kw(&L->cur,"PAIRGTIMM")){
    /* a b + n → (a>n?1:0) (b>n?1:0) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a > n) ? 1 : 0;
    long y = (b > n) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack immediate compare ext: DNEN · DLENN · DGENN (dual of SNEN/SLENN/SGENN) */
  if (kw(&L->cur,"DNEN")||kw(&L->cur,"2NEN")||kw(&L->cur,"S2NEN")||
      kw(&L->cur,"STACK2NEN")||kw(&L->cur,"PAIRNEN")||kw(&L->cur,"DNEIMM")||
      kw(&L->cur,"2NEIMM")||kw(&L->cur,"PAIRNEIMM")||kw(&L->cur,"DNEQN")){
    /* a b + n → (a!=n?1:0) (b!=n?1:0) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a != n) ? 1 : 0;
    long y = (b != n) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DLENN")||kw(&L->cur,"2LENN")||kw(&L->cur,"S2LENN")||
      kw(&L->cur,"STACK2LENN")||kw(&L->cur,"PAIRLENN")||kw(&L->cur,"DLEQN")||
      kw(&L->cur,"2LEQN")||kw(&L->cur,"DLEIMM")||kw(&L->cur,"2LEIMM")||
      kw(&L->cur,"PAIRLEQN")){
    /* a b + n → (a<=n?1:0) (b<=n?1:0) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a <= n) ? 1 : 0;
    long y = (b <= n) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGENN")||kw(&L->cur,"2GENN")||kw(&L->cur,"S2GENN")||
      kw(&L->cur,"STACK2GENN")||kw(&L->cur,"PAIRGENN")||kw(&L->cur,"DGEQN")||
      kw(&L->cur,"2GEQN")||kw(&L->cur,"DGEIMM")||kw(&L->cur,"2GEIMM")||
      kw(&L->cur,"PAIRGEQN")){
    /* a b + n → (a>=n?1:0) (b>=n?1:0) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a >= n) ? 1 : 0;
    long y = (b >= n) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
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
  /* digit-3 dual-stack parallel extract/deposit: DPEXT · DPDEP (BMI2-style) */
  if (kw(&L->cur,"DPEXT")||kw(&L->cur,"2PEXT")||kw(&L->cur,"S2PEXT")||
      kw(&L->cur,"STACK2PEXT")||kw(&L->cur,"PAIRPEXT")||kw(&L->cur,"2PEXTRACT")||
      kw(&L->cur,"DPEXTRACT")||
      kw(&L->cur,"DPDEP")||kw(&L->cur,"2PDEP")||kw(&L->cur,"S2PDEP")||
      kw(&L->cur,"STACK2PDEP")||kw(&L->cur,"PAIRPDEP")||kw(&L->cur,"2PDEPOSIT")||
      kw(&L->cur,"DPDEPOSIT")){
    /* a b ma mb → f(a,ma) f(b,mb)
     * PEXT: gather bits of src under mask ones into low bits
     * PDEP: scatter low bits of src into mask ones positions */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_dep = (strcmp(op,"DPDEP")==0 || strcmp(op,"2PDEP")==0 || strcmp(op,"S2PDEP")==0 ||
                  strcmp(op,"STACK2PDEP")==0 || strcmp(op,"PAIRPDEP")==0 ||
                  strcmp(op,"2PDEPOSIT")==0 || strcmp(op,"DPDEPOSIT")==0);
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long long mb = (unsigned long long)vm->stack[--vm->sp];
    unsigned long long ma = (unsigned long long)vm->stack[--vm->sp];
    unsigned long long sb = (unsigned long long)vm->stack[--vm->sp];
    unsigned long long sa = (unsigned long long)vm->stack[--vm->sp];
    unsigned long long ra = 0, rb = 0;
    if (is_dep){
      unsigned long long ba = 1, bb = 1;
      for (int i = 0; i < 64; i++){
        if ((ma >> i) & 1ull){
          if (sa & ba) ra |= (1ull << i);
          ba <<= 1;
        }
        if ((mb >> i) & 1ull){
          if (sb & bb) rb |= (1ull << i);
          bb <<= 1;
        }
      }
    } else {
      unsigned long long ka = 0, kb = 0;
      for (int i = 0; i < 64; i++){
        if ((ma >> i) & 1ull){
          if ((sa >> i) & 1ull) ra |= (1ull << ka);
          ka++;
        }
        if ((mb >> i) & 1ull){
          if ((sb >> i) & 1ull) rb |= (1ull << kb);
          kb++;
        }
      }
    }
    long x = (long)ra, y = (long)rb;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack Morton zip/unzip: DZIP · DUNZIP */
  if (kw(&L->cur,"DZIP")||kw(&L->cur,"2ZIP")||kw(&L->cur,"S2ZIP")||
      kw(&L->cur,"STACK2ZIP")||kw(&L->cur,"PAIRZIP")||kw(&L->cur,"2MORTON")||
      kw(&L->cur,"DMORTON")){
    /* a b c d → zip(a,c) zip(b,d); low 32 bits of each lane interleaved
     * zip(x,y): bit i of x → bit 2i; bit i of y → bit 2i+1 */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long long d = (unsigned long long)(unsigned int)vm->stack[--vm->sp];
    unsigned long long c = (unsigned long long)(unsigned int)vm->stack[--vm->sp];
    unsigned long long b = (unsigned long long)(unsigned int)vm->stack[--vm->sp];
    unsigned long long a = (unsigned long long)(unsigned int)vm->stack[--vm->sp];
    unsigned long long rx = 0, ry = 0;
    for (int i = 0; i < 32; i++){
      if ((a >> i) & 1ull) rx |= (1ull << (2 * i));
      if ((c >> i) & 1ull) rx |= (1ull << (2 * i + 1));
      if ((b >> i) & 1ull) ry |= (1ull << (2 * i));
      if ((d >> i) & 1ull) ry |= (1ull << (2 * i + 1));
    }
    long x = (long)rx, y = (long)ry;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DUNZIP")||kw(&L->cur,"2UNZIP")||kw(&L->cur,"S2UNZIP")||
      kw(&L->cur,"STACK2UNZIP")||kw(&L->cur,"PAIRUNZIP")||kw(&L->cur,"2DEMORTON")||
      kw(&L->cur,"DDEMORTON")){
    /* a b → even(a) even(b) odd(a) odd(b)
     * even = bits at even positions packed low; odd = bits at odd positions
     * stack ends with TOS = odd(b) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 2 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long long zb = (unsigned long long)vm->stack[--vm->sp];
    unsigned long long za = (unsigned long long)vm->stack[--vm->sp];
    unsigned long long ea = 0, oa = 0, eb = 0, ob = 0;
    for (int i = 0; i < 32; i++){
      if ((za >> (2 * i)) & 1ull) ea |= (1ull << i);
      if ((za >> (2 * i + 1)) & 1ull) oa |= (1ull << i);
      if ((zb >> (2 * i)) & 1ull) eb |= (1ull << i);
      if ((zb >> (2 * i + 1)) & 1ull) ob |= (1ull << i);
    }
    vm->stack[vm->sp++] = (long)ea;
    vm->stack[vm->sp++] = (long)eb;
    vm->stack[vm->sp++] = (long)oa;
    vm->stack[vm->sp++] = (long)ob;
    var_set_num(vm,"LAST_N",(long)ob); vm->last_n=(long)ob;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 dual-stack field extract/deposit: DBEXT · DBDEP */
  if (kw(&L->cur,"DBEXT")||kw(&L->cur,"2BEXT")||kw(&L->cur,"S2BEXT")||
      kw(&L->cur,"STACK2BEXT")||kw(&L->cur,"PAIRBEXT")||kw(&L->cur,"2BITEXT")||
      kw(&L->cur,"DBITEXT")){
    /* a b pos width → extract width bits at pos from each (pos/width shared) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long width = vm->stack[--vm->sp];
    long pos = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (pos < 0) pos = 0;
    if (pos > 62){
      vm->stack[vm->sp++] = 0;
      vm->stack[vm->sp++] = 0;
      var_set_num(vm,"LAST_N",0); vm->last_n=0;
      var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
    }
    if (width < 1) width = 0;
    if (width > 63 - pos) width = 63 - pos;
    long x = 0, y = 0;
    if (width > 0){
      unsigned long mask = (width >= 63) ? ~0ul : ((1ul << (unsigned)width) - 1ul);
      x = (long)(((unsigned long)a >> (unsigned)pos) & mask);
      y = (long)(((unsigned long)b >> (unsigned)pos) & mask);
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DBDEP")||kw(&L->cur,"2BDEP")||kw(&L->cur,"S2BDEP")||
      kw(&L->cur,"STACK2BDEP")||kw(&L->cur,"PAIRBDEP")||kw(&L->cur,"2BITDEP")||
      kw(&L->cur,"DBITDEP")){
    /* a b fa fb pos → deposit low 8 bits of fa/fb into a/b at pos (SBDEP dual) */
    lex_next(L);
    if (vm->sp < 5){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long pos = vm->stack[--vm->sp];
    long fb = vm->stack[--vm->sp];
    long fa = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long width = 8;
    if (pos < 0) pos = 0;
    if (pos > 62){
      vm->stack[vm->sp++] = a;
      vm->stack[vm->sp++] = b;
      var_set_num(vm,"LAST_N",b); vm->last_n=b;
      var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
    }
    if (width > 63 - pos) width = 63 - pos;
    long x = a, y = b;
    if (width > 0){
      unsigned long mask = (width >= 63) ? ~0ul : ((1ul << (unsigned)width) - 1ul);
      unsigned long ba = (unsigned long)a;
      unsigned long bb = (unsigned long)b;
      unsigned long faa = (unsigned long)fa & mask;
      unsigned long fbb = (unsigned long)fb & mask;
      ba = (ba & ~(mask << (unsigned)pos)) | (faa << (unsigned)pos);
      bb = (bb & ~(mask << (unsigned)pos)) | (fbb << (unsigned)pos);
      x = (long)ba; y = (long)bb;
    }
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack bit-position metrics: DFFS · DFLS · DBWIDTH (unary pair) */
  if (kw(&L->cur,"DFFS")||kw(&L->cur,"2FFS")||kw(&L->cur,"S2FFS")||
      kw(&L->cur,"STACK2FFS")||kw(&L->cur,"PAIRFFS")||kw(&L->cur,"2FINDLS")||
      kw(&L->cur,"DFINDFS")||
      kw(&L->cur,"DFLS")||kw(&L->cur,"2FLS")||kw(&L->cur,"S2FLS")||
      kw(&L->cur,"STACK2FLS")||kw(&L->cur,"PAIRFLS")||kw(&L->cur,"2MSB")||
      kw(&L->cur,"DMSB")||
      kw(&L->cur,"DBWIDTH")||kw(&L->cur,"2BWIDTH")||kw(&L->cur,"S2BWIDTH")||
      kw(&L->cur,"STACK2BWIDTH")||kw(&L->cur,"PAIRBWIDTH")||kw(&L->cur,"2BITWIDTH")||
      kw(&L->cur,"DBITWIDTH")){
    /* a b → metric(a) metric(b)
     * FFS: 1-based index of lowest 1-bit (0 if zero)
     * FLS: 1-based index of highest 1-bit (0 if zero)
     * BWIDTH: minimal bits to represent unsigned (0 if zero) = FLS */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_ffs = (strcmp(op,"DFFS")==0 || strcmp(op,"2FFS")==0 || strcmp(op,"S2FFS")==0 ||
                  strcmp(op,"STACK2FFS")==0 || strcmp(op,"PAIRFFS")==0 ||
                  strcmp(op,"2FINDLS")==0 || strcmp(op,"DFINDFS")==0);
    int is_fls = (strcmp(op,"DFLS")==0 || strcmp(op,"2FLS")==0 || strcmp(op,"S2FLS")==0 ||
                  strcmp(op,"STACK2FLS")==0 || strcmp(op,"PAIRFLS")==0 ||
                  strcmp(op,"2MSB")==0 || strcmp(op,"DMSB")==0);
    /* else DBWIDTH / BITWIDTH */
    long x = 0, y = 0;
    if (is_ffs){
      if (a != 0){
        unsigned long ua = (unsigned long)a; x = 1;
        while ((ua & 1ul) == 0){ x++; ua >>= 1; }
      }
      if (b != 0){
        unsigned long ub = (unsigned long)b; y = 1;
        while ((ub & 1ul) == 0){ y++; ub >>= 1; }
      }
    } else {
      /* FLS and BWIDTH share highest-bit position; both return 0 for zero */
      if (a != 0){
        unsigned long ua = (unsigned long)a;
        for (int i = 63; i >= 0; i--){
          if (ua & (1ul << (unsigned)i)){ x = (long)(i + 1); break; }
        }
      }
      if (b != 0){
        unsigned long ub = (unsigned long)b;
        for (int i = 63; i >= 0; i--){
          if (ub & (1ul << (unsigned)i)){ y = (long)(i + 1); break; }
        }
      }
      (void)is_fls; /* FLS == BWIDTH numeric result for unsigned words */
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack bit/power metrics: DBHSI · DCEILPOW2 */
  if (kw(&L->cur,"DBHSI")||kw(&L->cur,"2BHSI")||kw(&L->cur,"S2BHSI")||
      kw(&L->cur,"STACK2BHSI")||kw(&L->cur,"PAIRBHSI")||kw(&L->cur,"2HIBIT")||
      kw(&L->cur,"DHIBIT")||
      kw(&L->cur,"DCEILPOW2")||kw(&L->cur,"2CEILPOW2")||kw(&L->cur,"S2CEILPOW2")||
      kw(&L->cur,"STACK2CEILPOW2")||kw(&L->cur,"PAIRCEILPOW2")||kw(&L->cur,"2NEXTPOW2")||
      kw(&L->cur,"DNEXTPOW2")||kw(&L->cur,"2CPOW2")||kw(&L->cur,"DCPOW2")){
    /* a b → f(a) f(b)
     * BHSI/HIBIT: isolate highest set bit (value with only that bit); 0 → 0
     * CEILPOW2/NEXTPOW2: smallest power of 2 ≥ n; n<=0 → 0; overflow → 0 */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_bhsi = (strcmp(op,"DBHSI")==0 || strcmp(op,"2BHSI")==0 || strcmp(op,"S2BHSI")==0 ||
                   strcmp(op,"STACK2BHSI")==0 || strcmp(op,"PAIRBHSI")==0 ||
                   strcmp(op,"2HIBIT")==0 || strcmp(op,"DHIBIT")==0);
    long x = 0, y = 0;
    if (is_bhsi){
      if (a != 0){
        unsigned long ua = (unsigned long)a;
        for (int i = 63; i >= 0; i--){
          if (ua & (1ul << (unsigned)i)){ x = (long)(1ul << (unsigned)i); break; }
        }
      }
      if (b != 0){
        unsigned long ub = (unsigned long)b;
        for (int i = 63; i >= 0; i--){
          if (ub & (1ul << (unsigned)i)){ y = (long)(1ul << (unsigned)i); break; }
        }
      }
    } else {
      /* CEILPOW2 */
      if (a > 0){
        if (a == 1) x = 1;
        else {
          unsigned long u = (unsigned long)a;
          if ((u & (u - 1ul)) == 0ul) x = a;
          else if (a <= (1L << 62)){
            x = 1;
            while (x < a){
              if (x > (1L << 61)){ x = 0; break; }
              x <<= 1;
            }
          }
        }
      }
      if (b > 0){
        if (b == 1) y = 1;
        else {
          unsigned long u = (unsigned long)b;
          if ((u & (u - 1ul)) == 0ul) y = b;
          else if (b <= (1L << 62)){
            y = 1;
            while (y < b){
              if (y > (1L << 61)){ y = 0; break; }
              y <<= 1;
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
  /* digit-0 dual-stack ones-metrics + power-of-2: DCLO · DCTO · DISPOW2 */
  if (kw(&L->cur,"DCLO")||kw(&L->cur,"2CLO")||kw(&L->cur,"S2CLO")||
      kw(&L->cur,"STACK2CLO")||kw(&L->cur,"PAIRCLO")||
      kw(&L->cur,"DCTO")||kw(&L->cur,"2CTO")||kw(&L->cur,"S2CTO")||
      kw(&L->cur,"STACK2CTO")||kw(&L->cur,"PAIRCTO")||
      kw(&L->cur,"DISPOW2")||kw(&L->cur,"2ISPOW2")||kw(&L->cur,"S2ISPOW2")||
      kw(&L->cur,"STACK2ISPOW2")||kw(&L->cur,"PAIRISPOW2")||kw(&L->cur,"2POW2P")||
      kw(&L->cur,"DPOW2P")){
    /* a b → metric(a) metric(b)
     * CLO: count leading ones in 64-bit word
     * CTO: count trailing ones
     * ISPOW2: 1 if exactly one bit set and value > 0 */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_clo = (strcmp(op,"DCLO")==0 || strcmp(op,"2CLO")==0 || strcmp(op,"S2CLO")==0 ||
                  strcmp(op,"STACK2CLO")==0 || strcmp(op,"PAIRCLO")==0);
    int is_cto = (strcmp(op,"DCTO")==0 || strcmp(op,"2CTO")==0 || strcmp(op,"S2CTO")==0 ||
                  strcmp(op,"STACK2CTO")==0 || strcmp(op,"PAIRCTO")==0);
    long x = 0, y = 0;
    if (is_clo){
      unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
      for (int i = 63; i >= 0; i--){
        if ((ua & (1ul << (unsigned)i)) == 0) break;
        x++;
      }
      for (int i = 63; i >= 0; i--){
        if ((ub & (1ul << (unsigned)i)) == 0) break;
        y++;
      }
    } else if (is_cto){
      unsigned long ua = (unsigned long)a, ub = (unsigned long)b;
      while (ua & 1ul){ x++; ua >>= 1; if (x >= 64) break; }
      while (ub & 1ul){ y++; ub >>= 1; if (y >= 64) break; }
    } else {
      /* ISPOW2 / POW2P */
      if (a > 0){
        unsigned long ua = (unsigned long)a;
        x = ((ua & (ua - 1ul)) == 0ul) ? 1 : 0;
      }
      if (b > 0){
        unsigned long ub = (unsigned long)b;
        y = ((ub & (ub - 1ul)) == 0ul) ? 1 : 0;
      }
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
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
  /* digit-7 dual-stack imm 32-bit field bitwise: DAND32N · DOR32N · DXOR32N (dual of SAND32N/SOR32N/SXOR32N) */
  if (kw(&L->cur,"DAND32N")||kw(&L->cur,"S2AND32N")||kw(&L->cur,"STACK2AND32N")||
      kw(&L->cur,"PAIRAND32N")||kw(&L->cur,"DANDWN")||kw(&L->cur,"DKEEP32N")||
      kw(&L->cur,"DANDWIMM")){
    /* a b + field n → word n of each &= field; n clamped 0..1 */
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
    unsigned long wa = ((ma >> sh) & 0xFFFFFFFFul) & f;
    unsigned long wb = ((mb >> sh) & 0xFFFFFFFFul) & f;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DOR32N")||kw(&L->cur,"S2OR32N")||kw(&L->cur,"STACK2OR32N")||
      kw(&L->cur,"PAIROR32N")||kw(&L->cur,"DORWN")||kw(&L->cur,"DSETOR32N")||
      kw(&L->cur,"DORWIMM")){
    /* a b + field n → word n of each |= field; n clamped 0..1 */
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
    unsigned long wa = ((ma >> sh) & 0xFFFFFFFFul) | f;
    unsigned long wb = ((mb >> sh) & 0xFFFFFFFFul) | f;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXOR32N")||kw(&L->cur,"S2XOR32N")||kw(&L->cur,"STACK2XOR32N")||
      kw(&L->cur,"PAIRXOR32N")||kw(&L->cur,"DXORWN")||kw(&L->cur,"DFLIP32N")||
      kw(&L->cur,"DXORWIMM")){
    /* a b + field n → word n of each ^= field; n clamped 0..1 */
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
    unsigned long wa = ((ma >> sh) & 0xFFFFFFFFul) ^ f;
    unsigned long wb = ((mb >> sh) & 0xFFFFFFFFul) ^ f;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack imm inverted 32-bit field: DNAND32N · DNOR32N · DXNOR32N (dual of SNAND32N; complete dual inverted ladder) */
  if (kw(&L->cur,"DNAND32N")||kw(&L->cur,"S2NAND32N")||kw(&L->cur,"STACK2NAND32N")||
      kw(&L->cur,"PAIRNAND32N")||kw(&L->cur,"DNANDWN")||kw(&L->cur,"DINVERTAND32N")||
      kw(&L->cur,"DNANDWIMM")){
    /* a b + field n → word n of each = ~(word & field) & 0xFFFFFFFF; n clamped 0..1 */
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
    unsigned long wa = (~(((ma >> sh) & 0xFFFFFFFFul) & f)) & 0xFFFFFFFFul;
    unsigned long wb = (~(((mb >> sh) & 0xFFFFFFFFul) & f)) & 0xFFFFFFFFul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNOR32N")||kw(&L->cur,"S2NOR32N")||kw(&L->cur,"STACK2NOR32N")||
      kw(&L->cur,"PAIRNOR32N")||kw(&L->cur,"DNORWN")||kw(&L->cur,"DINVERTOR32N")||
      kw(&L->cur,"DNORWIMM")){
    /* a b + field n → word n of each = ~(word | field) & 0xFFFFFFFF; n clamped 0..1 */
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
    unsigned long wa = (~(((ma >> sh) & 0xFFFFFFFFul) | f)) & 0xFFFFFFFFul;
    unsigned long wb = (~(((mb >> sh) & 0xFFFFFFFFul) | f)) & 0xFFFFFFFFul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXNOR32N")||kw(&L->cur,"S2XNOR32N")||kw(&L->cur,"STACK2XNOR32N")||
      kw(&L->cur,"PAIRXNOR32N")||kw(&L->cur,"DXNORWN")||kw(&L->cur,"DEQUIV32N")||
      kw(&L->cur,"DXNORWIMM")){
    /* a b + field n → word n of each = ~(word ^ field) & 0xFFFFFFFF; n clamped 0..1 */
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
    unsigned long wa = (~(((ma >> sh) & 0xFFFFFFFFul) ^ f)) & 0xFFFFFFFFul;
    unsigned long wb = (~(((mb >> sh) & 0xFFFFFFFFul) ^ f)) & 0xFFFFFFFFul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 dual-stack imm 16-bit field bitwise: DAND16N · DOR16N · DXOR16N (dual of SAND16N/SOR16N/SXOR16N) */
  if (kw(&L->cur,"DAND16N")||kw(&L->cur,"S2AND16N")||kw(&L->cur,"STACK2AND16N")||
      kw(&L->cur,"PAIRAND16N")||kw(&L->cur,"DANDHIMM")||kw(&L->cur,"DKEEP16N")||
      kw(&L->cur,"DANDHN16")){
    /* a b + field n → halfword n of each &= field; n clamped 0..3 */
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
    unsigned long wa = ((ma >> sh) & 0xFFFFul) & f;
    unsigned long wb = ((mb >> sh) & 0xFFFFul) & f;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DOR16N")||kw(&L->cur,"S2OR16N")||kw(&L->cur,"STACK2OR16N")||
      kw(&L->cur,"PAIROR16N")||kw(&L->cur,"DORHIMM")||kw(&L->cur,"DSETOR16N")||
      kw(&L->cur,"DORHN16")){
    /* a b + field n → halfword n of each |= field; n clamped 0..3 */
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
    unsigned long wa = ((ma >> sh) & 0xFFFFul) | f;
    unsigned long wb = ((mb >> sh) & 0xFFFFul) | f;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXOR16N")||kw(&L->cur,"S2XOR16N")||kw(&L->cur,"STACK2XOR16N")||
      kw(&L->cur,"PAIRXOR16N")||kw(&L->cur,"DXORHIMM")||kw(&L->cur,"DFLIP16N")||
      kw(&L->cur,"DXORHN16")){
    /* a b + field n → halfword n of each ^= field; n clamped 0..3 */
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
    unsigned long wa = ((ma >> sh) & 0xFFFFul) ^ f;
    unsigned long wb = ((mb >> sh) & 0xFFFFul) ^ f;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 dual-stack imm inverted 16-bit field: DNAND16N · DNOR16N · DXNOR16N (dual of SNAND16N/SNOR16N/SXNOR16N) */
  if (kw(&L->cur,"DNAND16N")||kw(&L->cur,"S2NAND16N")||kw(&L->cur,"STACK2NAND16N")||
      kw(&L->cur,"PAIRNAND16N")||kw(&L->cur,"DNANDHIMM")||kw(&L->cur,"DINVERTAND16N")||
      kw(&L->cur,"DNANDHN16")){
    /* a b + field n → halfword n of each = ~(half & field) & 0xFFFF; n clamped 0..3 */
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
    unsigned long wa = (~(((ma >> sh) & 0xFFFFul) & f)) & 0xFFFFul;
    unsigned long wb = (~(((mb >> sh) & 0xFFFFul) & f)) & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNOR16N")||kw(&L->cur,"S2NOR16N")||kw(&L->cur,"STACK2NOR16N")||
      kw(&L->cur,"PAIRNOR16N")||kw(&L->cur,"DNORHIMM")||kw(&L->cur,"DINVERTOR16N")||
      kw(&L->cur,"DNORHN16")){
    /* a b + field n → halfword n of each = ~(half | field) & 0xFFFF; n clamped 0..3 */
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
    unsigned long wa = (~(((ma >> sh) & 0xFFFFul) | f)) & 0xFFFFul;
    unsigned long wb = (~(((mb >> sh) & 0xFFFFul) | f)) & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXNOR16N")||kw(&L->cur,"S2XNOR16N")||kw(&L->cur,"STACK2XNOR16N")||
      kw(&L->cur,"PAIRXNOR16N")||kw(&L->cur,"DXNORHIMM")||kw(&L->cur,"DEQUIV16N")||
      kw(&L->cur,"DXNORHN16")){
    /* a b + field n → halfword n of each = ~(half ^ field) & 0xFFFF; n clamped 0..3 */
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
    unsigned long wa = (~(((ma >> sh) & 0xFFFFul) ^ f)) & 0xFFFFul;
    unsigned long wb = (~(((mb >> sh) & 0xFFFFul) ^ f)) & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 dual-stack imm 16-bit field arith: DADD16N · DSUB16N · DMUL16N
   * (dual of SADD16N/SSUB16N/SMUL16N; wrap uint16 halfword plane on top two cells) */
  if (kw(&L->cur,"DADD16N")||kw(&L->cur,"S2ADD16N")||kw(&L->cur,"STACK2ADD16N")||
      kw(&L->cur,"PAIRADD16N")||kw(&L->cur,"DADDHIMM")||kw(&L->cur,"DINC16N")||
      kw(&L->cur,"DADDHN16")){
    /* a b + field n → halfword n of each = (hw + field) & 0xFFFF; n clamped 0..3 */
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
    unsigned long wa = (((ma >> sh) & 0xFFFFul) + f) & 0xFFFFul;
    unsigned long wb = (((mb >> sh) & 0xFFFFul) + f) & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSUB16N")||kw(&L->cur,"S2SUB16N")||kw(&L->cur,"STACK2SUB16N")||
      kw(&L->cur,"PAIRSUB16N")||kw(&L->cur,"DSUBHIMM")||kw(&L->cur,"DDEC16N")||
      kw(&L->cur,"DSUBHN16")){
    /* a b + field n → halfword n of each = (hw - field) & 0xFFFF; n clamped 0..3 */
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
    unsigned long wa = (((ma >> sh) & 0xFFFFul) - f) & 0xFFFFul;
    unsigned long wb = (((mb >> sh) & 0xFFFFul) - f) & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMUL16N")||kw(&L->cur,"S2MUL16N")||kw(&L->cur,"STACK2MUL16N")||
      kw(&L->cur,"PAIRMUL16N")||kw(&L->cur,"DMULHIMM")||kw(&L->cur,"DTIMES16N")||
      kw(&L->cur,"DMULHN16")){
    /* a b + field n → halfword n of each = (hw * field) & 0xFFFF; n clamped 0..3 */
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
    unsigned long wa = (((ma >> sh) & 0xFFFFul) * f) & 0xFFFFul;
    unsigned long wb = (((mb >> sh) & 0xFFFFul) * f) & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack imm 16-bit field div/mod/min: DDIV16N · DMOD16N · DMIN16N
   * (dual of SDIV16N/SMOD16N/SMIN16N; field 0 → div/mod 0 on each lane) */
  if (kw(&L->cur,"DDIV16N")||kw(&L->cur,"S2DIV16N")||kw(&L->cur,"STACK2DIV16N")||
      kw(&L->cur,"PAIRDIV16N")||kw(&L->cur,"DDIVHIMM")||kw(&L->cur,"DQUO16N")||
      kw(&L->cur,"DDIVHN16")){
    /* a b + field n → halfword n of each = hw / field (field 0 → 0); n clamped 0..3 */
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
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long wa = (f == 0) ? 0ul : (va / f);
    unsigned long wb = (f == 0) ? 0ul : (vb / f);
    long x = (long)((ma & ~(0xFFFFul << sh)) | ((wa & 0xFFFFul) << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | ((wb & 0xFFFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMOD16N")||kw(&L->cur,"S2MOD16N")||kw(&L->cur,"STACK2MOD16N")||
      kw(&L->cur,"PAIRMOD16N")||kw(&L->cur,"DMODHIMM")||kw(&L->cur,"DREM16N")||
      kw(&L->cur,"DMODHN16")){
    /* a b + field n → halfword n of each = hw % field (field 0 → 0); n clamped 0..3 */
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
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long wa = (f == 0) ? 0ul : (va % f);
    unsigned long wb = (f == 0) ? 0ul : (vb % f);
    long x = (long)((ma & ~(0xFFFFul << sh)) | ((wa & 0xFFFFul) << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | ((wb & 0xFFFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMIN16N")||kw(&L->cur,"S2MIN16N")||kw(&L->cur,"STACK2MIN16N")||
      kw(&L->cur,"PAIRMIN16N")||kw(&L->cur,"DMINHIMM")||kw(&L->cur,"DLE16N")||
      kw(&L->cur,"DMINHN16")){
    /* a b + field n → halfword n of each = min(hw, field); n clamped 0..3 */
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
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long wa = (va < f) ? va : f;
    unsigned long wb = (vb < f) ? vb : f;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack imm 16-bit field max+eq: DMAX16N · DEQ16N · DNE16N
   * (dual of SMAX16N/SEQ16N/SNE16N; complete min/max + equality on pair halfwords) */
  if (kw(&L->cur,"DMAX16N")||kw(&L->cur,"S2MAX16N")||kw(&L->cur,"STACK2MAX16N")||
      kw(&L->cur,"PAIRMAX16N")||kw(&L->cur,"DMAXHIMM")||kw(&L->cur,"DGE16N")||
      kw(&L->cur,"DMAXHN16")){
    /* a b + field n → halfword n of each = max(hw, field); n clamped 0..3 */
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
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long wa = (va > f) ? va : f;
    unsigned long wb = (vb > f) ? vb : f;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DEQ16N")||kw(&L->cur,"S2EQ16N")||kw(&L->cur,"STACK2EQ16N")||
      kw(&L->cur,"PAIREQ16N")||kw(&L->cur,"DEQHIMM")||kw(&L->cur,"DCMPEQ16N")||
      kw(&L->cur,"DEQHN16")){
    /* a b + field n → halfword n of each = (hw == field) ? 1 : 0; n clamped 0..3 */
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
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long wa = (va == f) ? 1ul : 0ul;
    unsigned long wb = (vb == f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNE16N")||kw(&L->cur,"S2NE16N")||kw(&L->cur,"STACK2NE16N")||
      kw(&L->cur,"PAIRNE16N")||kw(&L->cur,"DNEHIMM")||kw(&L->cur,"DCMPNE16N")||
      kw(&L->cur,"DNEHN16")){
    /* a b + field n → halfword n of each = (hw != field) ? 1 : 0; n clamped 0..3 */
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
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long wa = (va != f) ? 1ul : 0ul;
    unsigned long wb = (vb != f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm 8-bit field arith: DADD8N · DSUB8N · DMUL8N
   * (dual of SADD8N/SSUB8N/SMUL8N; wrap uint8 byte plane on top two cells after dual halfword ALU) */
  if (kw(&L->cur,"DADD8N")||kw(&L->cur,"S2ADD8N")||kw(&L->cur,"STACK2ADD8N")||
      kw(&L->cur,"PAIRADD8N")||kw(&L->cur,"DADDBIMM")||kw(&L->cur,"DINC8N")||
      kw(&L->cur,"DADDBN")){
    /* a b + field n → byte n of each = (byte + field) & 0xFF; n clamped 0..7 */
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
    unsigned long wa = (((ma >> sh) & 0xFFul) + f) & 0xFFul;
    unsigned long wb = (((mb >> sh) & 0xFFul) + f) & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSUB8N")||kw(&L->cur,"S2SUB8N")||kw(&L->cur,"STACK2SUB8N")||
      kw(&L->cur,"PAIRSUB8N")||kw(&L->cur,"DSUBBIMM")||kw(&L->cur,"DDEC8N")||
      kw(&L->cur,"DSUBBN")){
    /* a b + field n → byte n of each = (byte - field) & 0xFF; n clamped 0..7 */
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
    unsigned long wa = (((ma >> sh) & 0xFFul) - f) & 0xFFul;
    unsigned long wb = (((mb >> sh) & 0xFFul) - f) & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMUL8N")||kw(&L->cur,"S2MUL8N")||kw(&L->cur,"STACK2MUL8N")||
      kw(&L->cur,"PAIRMUL8N")||kw(&L->cur,"DMULBIMM")||kw(&L->cur,"DTIMES8N")||
      kw(&L->cur,"DMULBN")){
    /* a b + field n → byte n of each = (byte * field) & 0xFF; n clamped 0..7 */
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
    unsigned long wa = (((ma >> sh) & 0xFFul) * f) & 0xFFul;
    unsigned long wb = (((mb >> sh) & 0xFFul) * f) & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack imm 8-bit field div/mod/min: DDIV8N · DMOD8N · DMIN8N
   * (dual of SDIV8N/SMOD8N/SMIN8N; field 0 → div/mod 0 on each lane) */
  if (kw(&L->cur,"DDIV8N")||kw(&L->cur,"S2DIV8N")||kw(&L->cur,"STACK2DIV8N")||
      kw(&L->cur,"PAIRDIV8N")||kw(&L->cur,"DDIVBIMM")||kw(&L->cur,"DQUO8N")||
      kw(&L->cur,"DDIVBN")){
    /* a b + field n → byte n of each = byte / field (field 0 → 0); n clamped 0..7 */
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
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long wa = (f == 0) ? 0ul : (va / f);
    unsigned long wb = (f == 0) ? 0ul : (vb / f);
    long x = (long)((ma & ~(0xFFul << sh)) | ((wa & 0xFFul) << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | ((wb & 0xFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMOD8N")||kw(&L->cur,"S2MOD8N")||kw(&L->cur,"STACK2MOD8N")||
      kw(&L->cur,"PAIRMOD8N")||kw(&L->cur,"DMODBIMM")||kw(&L->cur,"DREM8N")||
      kw(&L->cur,"DMODBN")){
    /* a b + field n → byte n of each = byte % field (field 0 → 0); n clamped 0..7 */
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
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long wa = (f == 0) ? 0ul : (va % f);
    unsigned long wb = (f == 0) ? 0ul : (vb % f);
    long x = (long)((ma & ~(0xFFul << sh)) | ((wa & 0xFFul) << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | ((wb & 0xFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMIN8N")||kw(&L->cur,"S2MIN8N")||kw(&L->cur,"STACK2MIN8N")||
      kw(&L->cur,"PAIRMIN8N")||kw(&L->cur,"DMINBIMM")||kw(&L->cur,"DLE8N")||
      kw(&L->cur,"DMINBN")){
    /* a b + field n → byte n of each = min(byte, field); n clamped 0..7 */
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
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long wa = (va < f) ? va : f;
    unsigned long wb = (vb < f) ? vb : f;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm 8-bit field max+eq: DMAX8N · DEQ8N · DNE8N
   * (dual of SMAX8N/SEQ8N/SNE8N; complete min/max + equality on pair bytes) */
  if (kw(&L->cur,"DMAX8N")||kw(&L->cur,"S2MAX8N")||kw(&L->cur,"STACK2MAX8N")||
      kw(&L->cur,"PAIRMAX8N")||kw(&L->cur,"DMAXBIMM")||kw(&L->cur,"DGE8N")||
      kw(&L->cur,"DMAXBN")){
    /* a b + field n → byte n of each = max(byte, field); n clamped 0..7 */
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
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long wa = (va > f) ? va : f;
    unsigned long wb = (vb > f) ? vb : f;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DEQ8N")||kw(&L->cur,"S2EQ8N")||kw(&L->cur,"STACK2EQ8N")||
      kw(&L->cur,"PAIREQ8N")||kw(&L->cur,"DEQBIMM")||kw(&L->cur,"DCMPEQ8N")||
      kw(&L->cur,"DEQBN")){
    /* a b + field n → byte n of each = (byte == field) ? 1 : 0; n clamped 0..7 */
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
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long wa = (va == f) ? 1ul : 0ul;
    unsigned long wb = (vb == f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNE8N")||kw(&L->cur,"S2NE8N")||kw(&L->cur,"STACK2NE8N")||
      kw(&L->cur,"PAIRNE8N")||kw(&L->cur,"DNEBIMM")||kw(&L->cur,"DCMPNE8N")||
      kw(&L->cur,"DNEBN")){
    /* a b + field n → byte n of each = (byte != field) ? 1 : 0; n clamped 0..7 */
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
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long wa = (va != f) ? 1ul : 0ul;
    unsigned long wb = (vb != f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack imm 8-bit field bitwise: DAND8N · DOR8N · DXOR8N (dual of SAND8N/SOR8N/SXOR8N) */
  if (kw(&L->cur,"DAND8N")||kw(&L->cur,"S2AND8N")||kw(&L->cur,"STACK2AND8N")||
      kw(&L->cur,"PAIRAND8N")||kw(&L->cur,"DANDBIMM")||kw(&L->cur,"DKEEP8N")||
      kw(&L->cur,"DANDBN")){
    /* a b + field n → byte n of each &= field; n clamped 0..7 */
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
    unsigned long wa = ((ma >> sh) & 0xFFul) & f;
    unsigned long wb = ((mb >> sh) & 0xFFul) & f;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DOR8N")||kw(&L->cur,"S2OR8N")||kw(&L->cur,"STACK2OR8N")||
      kw(&L->cur,"PAIROR8N")||kw(&L->cur,"DORBIMM")||kw(&L->cur,"DSETOR8N")||
      kw(&L->cur,"DORBN")){
    /* a b + field n → byte n of each |= field; n clamped 0..7 */
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
    unsigned long wa = ((ma >> sh) & 0xFFul) | f;
    unsigned long wb = ((mb >> sh) & 0xFFul) | f;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXOR8N")||kw(&L->cur,"S2XOR8N")||kw(&L->cur,"STACK2XOR8N")||
      kw(&L->cur,"PAIRXOR8N")||kw(&L->cur,"DXORBIMM")||kw(&L->cur,"DFLIP8N")||
      kw(&L->cur,"DXORBN")){
    /* a b + field n → byte n of each ^= field; n clamped 0..7 */
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
    unsigned long wa = ((ma >> sh) & 0xFFul) ^ f;
    unsigned long wb = ((mb >> sh) & 0xFFul) ^ f;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 dual-stack imm inverted 8-bit field: DNAND8N · DNOR8N · DXNOR8N (dual of SNAND8N/SNOR8N/SXNOR8N) */
  if (kw(&L->cur,"DNAND8N")||kw(&L->cur,"S2NAND8N")||kw(&L->cur,"STACK2NAND8N")||
      kw(&L->cur,"PAIRNAND8N")||kw(&L->cur,"DNANDBIMM")||kw(&L->cur,"DINVERTAND8N")||
      kw(&L->cur,"DNANDBN")){
    /* a b + field n → byte n of each = ~(byte & field) & 0xFF; n clamped 0..7 */
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
    unsigned long wa = (~(((ma >> sh) & 0xFFul) & f)) & 0xFFul;
    unsigned long wb = (~(((mb >> sh) & 0xFFul) & f)) & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNOR8N")||kw(&L->cur,"S2NOR8N")||kw(&L->cur,"STACK2NOR8N")||
      kw(&L->cur,"PAIRNOR8N")||kw(&L->cur,"DNORBIMM")||kw(&L->cur,"DINVERTOR8N")||
      kw(&L->cur,"DNORBN")){
    /* a b + field n → byte n of each = ~(byte | field) & 0xFF; n clamped 0..7 */
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
    unsigned long wa = (~(((ma >> sh) & 0xFFul) | f)) & 0xFFul;
    unsigned long wb = (~(((mb >> sh) & 0xFFul) | f)) & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXNOR8N")||kw(&L->cur,"S2XNOR8N")||kw(&L->cur,"STACK2XNOR8N")||
      kw(&L->cur,"PAIRXNOR8N")||kw(&L->cur,"DXNORBIMM")||kw(&L->cur,"DEQUIV8N")||
      kw(&L->cur,"DXNORBN")){
    /* a b + field n → byte n of each = ~(byte ^ field) & 0xFF; n clamped 0..7 */
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
    unsigned long wa = (~(((ma >> sh) & 0xFFul) ^ f)) & 0xFFul;
    unsigned long wb = (~(((mb >> sh) & 0xFFul) ^ f)) & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 dual-stack imm 4-bit field arith: DADD4N · DSUB4N · DMUL4N
   * (dual of SADD4N/SSUB4N/SMUL4N; wrap uint4 nibble plane on top two cells after dual 8n ALU) */
  if (kw(&L->cur,"DADD4N")||kw(&L->cur,"S2ADD4N")||kw(&L->cur,"STACK2ADD4N")||
      kw(&L->cur,"PAIRADD4N")||kw(&L->cur,"DADDNIMM")||kw(&L->cur,"DINC4N")||
      kw(&L->cur,"DADDNIBN")){
    /* a b + field n → nibble n of each = (nib + field) & 0xF; n clamped 0..15 */
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
    unsigned long wa = (((ma >> sh) & 0xFul) + f) & 0xFul;
    unsigned long wb = (((mb >> sh) & 0xFul) + f) & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSUB4N")||kw(&L->cur,"S2SUB4N")||kw(&L->cur,"STACK2SUB4N")||
      kw(&L->cur,"PAIRSUB4N")||kw(&L->cur,"DSUBNIMM")||kw(&L->cur,"DDEC4N")||
      kw(&L->cur,"DSUBNIBN")){
    /* a b + field n → nibble n of each = (nib - field) & 0xF; n clamped 0..15 */
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
    unsigned long wa = (((ma >> sh) & 0xFul) - f) & 0xFul;
    unsigned long wb = (((mb >> sh) & 0xFul) - f) & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMUL4N")||kw(&L->cur,"S2MUL4N")||kw(&L->cur,"STACK2MUL4N")||
      kw(&L->cur,"PAIRMUL4N")||kw(&L->cur,"DMULNIMM")||kw(&L->cur,"DTIMES4N")||
      kw(&L->cur,"DMULNIBN")){
    /* a b + field n → nibble n of each = (nib * field) & 0xF; n clamped 0..15 */
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
    unsigned long wa = (((ma >> sh) & 0xFul) * f) & 0xFul;
    unsigned long wb = (((mb >> sh) & 0xFul) * f) & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack imm 4-bit field div/mod/min: DDIV4N · DMOD4N · DMIN4N
   * (dual of SDIV4N/SMOD4N/SMIN4N; field 0 → div/mod 0 on each lane) */
  if (kw(&L->cur,"DDIV4N")||kw(&L->cur,"S2DIV4N")||kw(&L->cur,"STACK2DIV4N")||
      kw(&L->cur,"PAIRDIV4N")||kw(&L->cur,"DDIVNIMM")||kw(&L->cur,"DQUO4N")||
      kw(&L->cur,"DDIVNIBN")){
    /* a b + field n → nibble n of each = nib / field (field 0 → 0); n clamped 0..15 */
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
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (f == 0) ? 0ul : (va / f);
    unsigned long wb = (f == 0) ? 0ul : (vb / f);
    long x = (long)((ma & ~(0xFul << sh)) | ((wa & 0xFul) << sh));
    long y = (long)((mb & ~(0xFul << sh)) | ((wb & 0xFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMOD4N")||kw(&L->cur,"S2MOD4N")||kw(&L->cur,"STACK2MOD4N")||
      kw(&L->cur,"PAIRMOD4N")||kw(&L->cur,"DMODNIMM")||kw(&L->cur,"DREM4N")||
      kw(&L->cur,"DMODNIBN")){
    /* a b + field n → nibble n of each = nib % field (field 0 → 0); n clamped 0..15 */
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
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (f == 0) ? 0ul : (va % f);
    unsigned long wb = (f == 0) ? 0ul : (vb % f);
    long x = (long)((ma & ~(0xFul << sh)) | ((wa & 0xFul) << sh));
    long y = (long)((mb & ~(0xFul << sh)) | ((wb & 0xFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMIN4N")||kw(&L->cur,"S2MIN4N")||kw(&L->cur,"STACK2MIN4N")||
      kw(&L->cur,"PAIRMIN4N")||kw(&L->cur,"DMINNIMM")||kw(&L->cur,"DLE4N")||
      kw(&L->cur,"DMINNIBN")){
    /* a b + field n → nibble n of each = min(nib, field); n clamped 0..15 */
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
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (va < f) ? va : f;
    unsigned long wb = (vb < f) ? vb : f;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 dual-stack imm 4-bit field max+eq: DMAX4N · DEQ4N · DNE4N
   * (dual of SMAX4N/SEQ4N/SNE4N; complete min/max + equality on pair nibbles) */
  if (kw(&L->cur,"DMAX4N")||kw(&L->cur,"S2MAX4N")||kw(&L->cur,"STACK2MAX4N")||
      kw(&L->cur,"PAIRMAX4N")||kw(&L->cur,"DMAXNIMM")||kw(&L->cur,"DGE4N")||
      kw(&L->cur,"DMAXNIBN")){
    /* a b + field n → nibble n of each = max(nib, field); n clamped 0..15 */
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
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (va > f) ? va : f;
    unsigned long wb = (vb > f) ? vb : f;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DEQ4N")||kw(&L->cur,"S2EQ4N")||kw(&L->cur,"STACK2EQ4N")||
      kw(&L->cur,"PAIREQ4N")||kw(&L->cur,"DEQNIMM")||kw(&L->cur,"DCMPEQ4N")||
      kw(&L->cur,"DEQNIBN")){
    /* a b + field n → nibble n of each = (nib == field) ? 1 : 0; n clamped 0..15 */
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
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (va == f) ? 1ul : 0ul;
    unsigned long wb = (vb == f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNE4N")||kw(&L->cur,"S2NE4N")||kw(&L->cur,"STACK2NE4N")||
      kw(&L->cur,"PAIRNE4N")||kw(&L->cur,"DNENIMM")||kw(&L->cur,"DCMPNE4N")||
      kw(&L->cur,"DNENIBN")){
    /* a b + field n → nibble n of each = (nib != field) ? 1 : 0; n clamped 0..15 */
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
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (va != f) ? 1ul : 0ul;
    unsigned long wb = (vb != f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack imm 4-bit field bitwise: DAND4N · DOR4N · DXOR4N (dual of SAND4N/SOR4N/SXOR4N) */
  if (kw(&L->cur,"DAND4N")||kw(&L->cur,"S2AND4N")||kw(&L->cur,"STACK2AND4N")||
      kw(&L->cur,"PAIRAND4N")||kw(&L->cur,"DANDNIMM")||kw(&L->cur,"DKEEP4N")||
      kw(&L->cur,"DANDNIBN")){
    /* a b + field n → nibble n of each &= field; n clamped 0..15 */
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
    unsigned long wa = ((ma >> sh) & 0xFul) & f;
    unsigned long wb = ((mb >> sh) & 0xFul) & f;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DOR4N")||kw(&L->cur,"S2OR4N")||kw(&L->cur,"STACK2OR4N")||
      kw(&L->cur,"PAIROR4N")||kw(&L->cur,"DORNIMM")||kw(&L->cur,"DSETOR4N")||
      kw(&L->cur,"DORNIBN")){
    /* a b + field n → nibble n of each |= field; n clamped 0..15 */
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
    unsigned long wa = ((ma >> sh) & 0xFul) | f;
    unsigned long wb = ((mb >> sh) & 0xFul) | f;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXOR4N")||kw(&L->cur,"S2XOR4N")||kw(&L->cur,"STACK2XOR4N")||
      kw(&L->cur,"PAIRXOR4N")||kw(&L->cur,"DXORNIMM")||kw(&L->cur,"DFLIP4N")||
      kw(&L->cur,"DXORNIBN")){
    /* a b + field n → nibble n of each ^= field; n clamped 0..15 */
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
    unsigned long wa = ((ma >> sh) & 0xFul) ^ f;
    unsigned long wb = ((mb >> sh) & 0xFul) ^ f;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack imm inverted 4-bit field: DNAND4N · DNOR4N · DXNOR4N (dual of SNAND4N/SNOR4N/SXNOR4N) */
  if (kw(&L->cur,"DNAND4N")||kw(&L->cur,"S2NAND4N")||kw(&L->cur,"STACK2NAND4N")||
      kw(&L->cur,"PAIRNAND4N")||kw(&L->cur,"DNANDNIMM")||kw(&L->cur,"DINVERTAND4N")||
      kw(&L->cur,"DNANDNIBN")){
    /* a b + field n → nibble n of each = ~(nibble & field) & 0xF; n clamped 0..15 */
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
    unsigned long wa = (~(((ma >> sh) & 0xFul) & f)) & 0xFul;
    unsigned long wb = (~(((mb >> sh) & 0xFul) & f)) & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNOR4N")||kw(&L->cur,"S2NOR4N")||kw(&L->cur,"STACK2NOR4N")||
      kw(&L->cur,"PAIRNOR4N")||kw(&L->cur,"DNORNIMM")||kw(&L->cur,"DINVERTOR4N")||
      kw(&L->cur,"DNORNIBN")){
    /* a b + field n → nibble n of each = ~(nibble | field) & 0xF; n clamped 0..15 */
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
    unsigned long wa = (~(((ma >> sh) & 0xFul) | f)) & 0xFul;
    unsigned long wb = (~(((mb >> sh) & 0xFul) | f)) & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXNOR4N")||kw(&L->cur,"S2XNOR4N")||kw(&L->cur,"STACK2XNOR4N")||
      kw(&L->cur,"PAIRXNOR4N")||kw(&L->cur,"DXNORNIMM")||kw(&L->cur,"DEQUIV4N")||
      kw(&L->cur,"DXNORNIBN")){
    /* a b + field n → nibble n of each = ~(nibble ^ field) & 0xF; n clamped 0..15 */
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
    unsigned long wa = (~(((ma >> sh) & 0xFul) ^ f)) & 0xFul;
    unsigned long wb = (~(((mb >> sh) & 0xFul) ^ f)) & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack imm 4-bit field unary+rotate: DNOT4N · DROL4N · DROR4N
   * (dual of SNOT4N/SROL4N/SROR4N; nibble-field unary plane on top two cells after dual 4n bitwise) */
  if (kw(&L->cur,"DNOT4N")||kw(&L->cur,"S2NOT4N")||kw(&L->cur,"STACK2NOT4N")||
      kw(&L->cur,"PAIRNOT4N")||kw(&L->cur,"DINV4N")||kw(&L->cur,"DINVERT4N")||
      kw(&L->cur,"DNOTNIBN")){
    /* a b + n → nibble n of each = ~nibble & 0xF; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (~((ma >> sh) & 0xFul)) & 0xFul;
    unsigned long wb = (~((mb >> sh) & 0xFul)) & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROL4N")||kw(&L->cur,"S2ROL4N")||kw(&L->cur,"STACK2ROL4N")||
      kw(&L->cur,"PAIRROL4N")||kw(&L->cur,"DROTL4N")||kw(&L->cur,"DROLNIBN")||
      kw(&L->cur,"DROTNIBL")){
    /* a b + k n → rotl4 nibble n of each by k&3; n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 3;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (kk == 0) ? va : (((va << (unsigned)kk) | (va >> (unsigned)(4 - kk))) & 0xFul);
    unsigned long wb = (kk == 0) ? vb : (((vb << (unsigned)kk) | (vb >> (unsigned)(4 - kk))) & 0xFul);
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROR4N")||kw(&L->cur,"S2ROR4N")||kw(&L->cur,"STACK2ROR4N")||
      kw(&L->cur,"PAIRROR4N")||kw(&L->cur,"DROTR4N")||kw(&L->cur,"DRORNIBN")||
      kw(&L->cur,"DROTNIBR")){
    /* a b + k n → rotr4 nibble n of each by k&3; n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 3;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (kk == 0) ? va : (((va >> (unsigned)kk) | (va << (unsigned)(4 - kk))) & 0xFul);
    unsigned long wb = (kk == 0) ? vb : (((vb >> (unsigned)kk) | (vb << (unsigned)(4 - kk))) & 0xFul);
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
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
  /* digit-3 dual-stack nibble plane: DLO4 · DHI4 · DPACK4 */
  if (kw(&L->cur,"DLO4")||kw(&L->cur,"2LO4")||kw(&L->cur,"S2LO4")||
      kw(&L->cur,"STACK2LO4")||kw(&L->cur,"PAIRLO4")||kw(&L->cur,"DLOWN")||
      kw(&L->cur,"2LOWN")||kw(&L->cur,"DNIBLO")||kw(&L->cur,"2NIBLO")||
      kw(&L->cur,"DHI4")||kw(&L->cur,"2HI4")||kw(&L->cur,"S2HI4")||
      kw(&L->cur,"STACK2HI4")||kw(&L->cur,"PAIRHI4")||kw(&L->cur,"DHIN")||
      kw(&L->cur,"2HIN")||kw(&L->cur,"DNIBHI")||kw(&L->cur,"2NIBHI")){
    /* a b → lo4/hi4 nibble of each (bits 0..3 / 4..7) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_lo = (strcmp(op,"DLO4")==0 || strcmp(op,"2LO4")==0 || strcmp(op,"S2LO4")==0 ||
                 strcmp(op,"STACK2LO4")==0 || strcmp(op,"PAIRLO4")==0 ||
                 strcmp(op,"DLOWN")==0 || strcmp(op,"2LOWN")==0 ||
                 strcmp(op,"DNIBLO")==0 || strcmp(op,"2NIBLO")==0);
    long x, y;
    if (is_lo){
      x = a & 0xFL;
      y = b & 0xFL;
    } else {
      x = (a >> 4) & 0xFL;
      y = (b >> 4) & 0xFL;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPACK4")||kw(&L->cur,"2PACK4")||kw(&L->cur,"S2PACK4")||
      kw(&L->cur,"STACK2PACK4")||kw(&L->cur,"PAIRPACK4")||kw(&L->cur,"DPACKN")||
      kw(&L->cur,"2PACKN")){
    /* a b c d → ((a&0xF)<<4)|(c&0xF)  ((b&0xF)<<4)|(d&0xF) — hi,lo nibbles → byte */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = ((a & 0xFL) << 4) | (c & 0xFL);
    long y = ((b & 0xFL) << 4) | (d & 0xFL);
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack nibble unpack: DUNPACK4 (inverse of DPACK4) */
  if (kw(&L->cur,"DUNPACK4")||kw(&L->cur,"2UNPACK4")||kw(&L->cur,"S2UNPACK4")||
      kw(&L->cur,"STACK2UNPACK4")||kw(&L->cur,"PAIRUNPACK4")||kw(&L->cur,"DUNPACKN")||
      kw(&L->cur,"2UNPACKN")||kw(&L->cur,"DNIBSPLIT")||kw(&L->cur,"2NIBSPLIT")){
    /* x y → (x>>4)&0xF  (y>>4)&0xF  x&0xF  y&0xF  — hi,hi,lo,lo so DPACK4 restores */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 2 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long y = vm->stack[--vm->sp];
    long x = vm->stack[--vm->sp];
    long ha = (x >> 4) & 0xFL;
    long hb = (y >> 4) & 0xFL;
    long la = x & 0xFL;
    long lb = y & 0xFL;
    vm->stack[vm->sp++] = ha;
    vm->stack[vm->sp++] = hb;
    vm->stack[vm->sp++] = la;
    vm->stack[vm->sp++] = lb;
    var_set_num(vm,"LAST_N",lb); vm->last_n=lb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack control-word pack: DLO8 · DHI8 · DPACK8 */
  if (kw(&L->cur,"DLO8")||kw(&L->cur,"2LO8")||kw(&L->cur,"S2LO8")||
      kw(&L->cur,"STACK2LO8")||kw(&L->cur,"PAIRLO8")||kw(&L->cur,"DLOWB")||
      kw(&L->cur,"2LOWB")||
      kw(&L->cur,"DHI8")||kw(&L->cur,"2HI8")||kw(&L->cur,"S2HI8")||
      kw(&L->cur,"STACK2HI8")||kw(&L->cur,"PAIRHI8")||kw(&L->cur,"DHIB")||
      kw(&L->cur,"2HIB")){
    /* a b → lo8/hi8 of each */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_lo = (strcmp(op,"DLO8")==0 || strcmp(op,"2LO8")==0 || strcmp(op,"S2LO8")==0 ||
                 strcmp(op,"STACK2LO8")==0 || strcmp(op,"PAIRLO8")==0 ||
                 strcmp(op,"DLOWB")==0 || strcmp(op,"2LOWB")==0);
    long x, y;
    if (is_lo){
      x = a & 0xFFL;
      y = b & 0xFFL;
    } else {
      x = (a >> 8) & 0xFFL;
      y = (b >> 8) & 0xFFL;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPACK8")||kw(&L->cur,"2PACK8")||kw(&L->cur,"S2PACK8")||
      kw(&L->cur,"STACK2PACK8")||kw(&L->cur,"PAIRPACK8")||kw(&L->cur,"DPACKB")||
      kw(&L->cur,"2PACKB")){
    /* a b c d → ((a&0xFF)<<8)|(c&0xFF)  ((b&0xFF)<<8)|(d&0xFF)  — hi,lo bytes */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = ((a & 0xFFL) << 8) | (c & 0xFFL);
    long y = ((b & 0xFFL) << 8) | (d & 0xFFL);
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack control-word halfpack: DLO16 · DHI16 · DPACK16 */
  if (kw(&L->cur,"DLO16")||kw(&L->cur,"2LO16")||kw(&L->cur,"S2LO16")||
      kw(&L->cur,"STACK2LO16")||kw(&L->cur,"PAIRLO16")||kw(&L->cur,"DLOWW")||
      kw(&L->cur,"2LOWW")||
      kw(&L->cur,"DHI16")||kw(&L->cur,"2HI16")||kw(&L->cur,"S2HI16")||
      kw(&L->cur,"STACK2HI16")||kw(&L->cur,"PAIRHI16")||kw(&L->cur,"DHIW")||
      kw(&L->cur,"2HIW")){
    /* a b → lo16/hi16 of each (bits 0..15 / 16..31) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_lo = (strcmp(op,"DLO16")==0 || strcmp(op,"2LO16")==0 || strcmp(op,"S2LO16")==0 ||
                 strcmp(op,"STACK2LO16")==0 || strcmp(op,"PAIRLO16")==0 ||
                 strcmp(op,"DLOWW")==0 || strcmp(op,"2LOWW")==0);
    long x, y;
    if (is_lo){
      x = a & 0xFFFFL;
      y = b & 0xFFFFL;
    } else {
      x = (a >> 16) & 0xFFFFL;
      y = (b >> 16) & 0xFFFFL;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPACK16")||kw(&L->cur,"2PACK16")||kw(&L->cur,"S2PACK16")||
      kw(&L->cur,"STACK2PACK16")||kw(&L->cur,"PAIRPACK16")||kw(&L->cur,"DPACKW")||
      kw(&L->cur,"2PACKW")){
    /* a b c d → ((a&0xFFFF)<<16)|(c&0xFFFF)  ((b&0xFFFF)<<16)|(d&0xFFFF) — hi,lo halfwords */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = ((a & 0xFFFFL) << 16) | (c & 0xFFFFL);
    long y = ((b & 0xFFFFL) << 16) | (d & 0xFFFFL);
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack control-word unpack: DUNPACK8 · DUNPACK16 (inverse of DPACK8/16) */
  if (kw(&L->cur,"DUNPACK8")||kw(&L->cur,"2UNPACK8")||kw(&L->cur,"S2UNPACK8")||
      kw(&L->cur,"STACK2UNPACK8")||kw(&L->cur,"PAIRUNPACK8")||kw(&L->cur,"DUNPACKB")||
      kw(&L->cur,"2UNPACKB")||kw(&L->cur,"DBYTSPLIT")||kw(&L->cur,"2BYTSPLIT")){
    /* x y → (x>>8)&0xFF  (y>>8)&0xFF  x&0xFF  y&0xFF  — hi,hi,lo,lo so DPACK8 restores */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 2 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long y = vm->stack[--vm->sp];
    long x = vm->stack[--vm->sp];
    long ha = (x >> 8) & 0xFFL;
    long hb = (y >> 8) & 0xFFL;
    long la = x & 0xFFL;
    long lb = y & 0xFFL;
    vm->stack[vm->sp++] = ha;
    vm->stack[vm->sp++] = hb;
    vm->stack[vm->sp++] = la;
    vm->stack[vm->sp++] = lb;
    var_set_num(vm,"LAST_N",lb); vm->last_n=lb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DUNPACK16")||kw(&L->cur,"2UNPACK16")||kw(&L->cur,"S2UNPACK16")||
      kw(&L->cur,"STACK2UNPACK16")||kw(&L->cur,"PAIRUNPACK16")||kw(&L->cur,"DUNPACKW")||
      kw(&L->cur,"2UNPACKW")||kw(&L->cur,"DHALFSPLIT")||kw(&L->cur,"2HALFSPLIT")){
    /* x y → (x>>16)&0xFFFF  (y>>16)&0xFFFF  x&0xFFFF  y&0xFFFF — hi,hi,lo,lo so DPACK16 restores */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 2 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long y = vm->stack[--vm->sp];
    long x = vm->stack[--vm->sp];
    long ha = (x >> 16) & 0xFFFFL;
    long hb = (y >> 16) & 0xFFFFL;
    long la = x & 0xFFFFL;
    long lb = y & 0xFFFFL;
    vm->stack[vm->sp++] = ha;
    vm->stack[vm->sp++] = hb;
    vm->stack[vm->sp++] = la;
    vm->stack[vm->sp++] = lb;
    var_set_num(vm,"LAST_N",lb); vm->last_n=lb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 dual-stack word pack: DLO32 · DHI32 · DPACK32 */
  if (kw(&L->cur,"DLO32")||kw(&L->cur,"2LO32")||kw(&L->cur,"S2LO32")||
      kw(&L->cur,"STACK2LO32")||kw(&L->cur,"PAIRLO32")||kw(&L->cur,"DLOWD")||
      kw(&L->cur,"2LOWD")||
      kw(&L->cur,"DHI32")||kw(&L->cur,"2HI32")||kw(&L->cur,"S2HI32")||
      kw(&L->cur,"STACK2HI32")||kw(&L->cur,"PAIRHI32")||kw(&L->cur,"DHID")||
      kw(&L->cur,"2HID")){
    /* a b → lo32/hi32 of each (bits 0..31 / 32..63) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    int is_lo = (strcmp(op,"DLO32")==0 || strcmp(op,"2LO32")==0 || strcmp(op,"S2LO32")==0 ||
                 strcmp(op,"STACK2LO32")==0 || strcmp(op,"PAIRLO32")==0 ||
                 strcmp(op,"DLOWD")==0 || strcmp(op,"2LOWD")==0);
    long x, y;
    if (is_lo){
      x = (long)(unsigned int)(unsigned long)a;
      y = (long)(unsigned int)(unsigned long)b;
    } else {
      x = (long)(unsigned int)((unsigned long long)(unsigned long)a >> 32);
      y = (long)(unsigned int)((unsigned long long)(unsigned long)b >> 32);
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPACK32")||kw(&L->cur,"2PACK32")||kw(&L->cur,"S2PACK32")||
      kw(&L->cur,"STACK2PACK32")||kw(&L->cur,"PAIRPACK32")||kw(&L->cur,"DPACKDW")||
      kw(&L->cur,"2PACKDW")){
    /* a b c d → ((a&0xFFFFFFFF)<<32)|(c&0xFFFFFFFF)  same for b,d — hi,lo words */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[--vm->sp];
    long c = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    unsigned long long xa = ((unsigned long long)(unsigned int)a << 32) |
                            (unsigned long long)(unsigned int)c;
    unsigned long long ya = ((unsigned long long)(unsigned int)b << 32) |
                            (unsigned long long)(unsigned int)d;
    long x = (long)xa;
    long y = (long)ya;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 dual-stack word unpack: DUNPACK32 (inverse of DPACK32; complete DLO32/DHI32 plane) */
  if (kw(&L->cur,"DUNPACK32")||kw(&L->cur,"2UNPACK32")||kw(&L->cur,"S2UNPACK32")||
      kw(&L->cur,"STACK2UNPACK32")||kw(&L->cur,"PAIRUNPACK32")||kw(&L->cur,"DUNPACKDW")||
      kw(&L->cur,"2UNPACKDW")||kw(&L->cur,"DWORDSPLIT")||kw(&L->cur,"2WORDSPLIT")){
    /* x y → hi32(x) hi32(y) lo32(x) lo32(y) — hi,hi,lo,lo so DPACK32 restores */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 2 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long y = vm->stack[--vm->sp];
    long x = vm->stack[--vm->sp];
    unsigned long long ux = (unsigned long long)(unsigned long)x;
    unsigned long long uy = (unsigned long long)(unsigned long)y;
    long ha = (long)(unsigned int)(ux >> 32);
    long hb = (long)(unsigned int)(uy >> 32);
    long la = (long)(unsigned int)ux;
    long lb = (long)(unsigned int)uy;
    vm->stack[vm->sp++] = ha;
    vm->stack[vm->sp++] = hb;
    vm->stack[vm->sp++] = la;
    vm->stack[vm->sp++] = lb;
    var_set_num(vm,"LAST_N",lb); vm->last_n=lb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
