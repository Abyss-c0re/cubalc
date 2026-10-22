/* CubalC lang — lang_ops_dual.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#include "lang/cubalc_lang_ops_dual_pack.h"
#include "lang/cubalc_lang_ops_dual_clip.h"
#include "lang/cubalc_lang_ops_dual_field32.h"
#include "lang/cubalc_lang_ops_dual_unary32.h"
#include "lang/cubalc_lang_ops_dual_unary16.h"
#include "lang/cubalc_lang_ops_dual_unary8.h"

int cubalc_lang_ops_dual(VM *vm, Lex *L){
  /* pack/unpack peel (DLO4…DUNPACK32) */
  {
    int pr = cubalc_lang_ops_dual_try_pack(vm, L);
    if (pr != 0) return pr;
  }
  /* clip/sext/rotate peel (DCLIP32…DROR32) */
  {
    int cr = cubalc_lang_ops_dual_try_clip(vm, L);
    if (cr != 0) return cr;
  }
  /* imm-32 field peel (DADD32N…D0GE32N) */
  {
    int f32 = cubalc_lang_ops_dual_try_field32(vm, L);
    if (f32 != 0) return f32;
  }
  /* imm-32 unary/shift/metrics peel (DNOT32N…DPARITY32N) */
  {
    int u32 = cubalc_lang_ops_dual_try_unary32(vm, L);
    if (u32 != 0) return u32;
  }
  /* imm-16 unary/shift/metrics peel (DABS16N…DNEG16N) */
  {
    int u16 = cubalc_lang_ops_dual_try_unary16(vm, L);
    if (u16 != 0) return u16;
  }
  /* imm-8 unary/shift/metrics peel (DNOT8N…DZEXT8N) */
  {
    int u8 = cubalc_lang_ops_dual_try_unary8(vm, L);
    if (u8 != 0) return u8;
  }
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
  /* DAND·DOR·DXOR·DNEG·DABS → dual_clip peel (G_dual_margin) */
  /* DDBL·DHALF·DBSWAP → dual_clip peel (G_dual_margin) */
  /* DMADD·DMULHI → dual_clip peel (G_dual_margin) */
  /* DUMULHI·DBSWAP16·DBSWAP64 → dual_clip peel (G_dual_margin) */
  /* DADDOVF·DSUBOVF·DMULOVF → dual_clip peel (G_dual_margin) */
  /* DADDOVFN·DSUBOVFN·DMULOVFN → dual_clip peel (G_dual_margin) */
  /* DUADDOVF·DUSUBOVF·DUMULOVF → dual_clip peel (G_dual_margin) */
  /* DUDIV·DUMOD → dual_clip peel (G_dual_margin) */
  /* DUDIVN·DUMODN → dual_clip peel (G_dual_margin) */
  /* DUMIN·DUMAX → dual_clip peel (G_dual_margin) */
  /* DULT·DULE·DUGT·DUGE → dual_clip peel (G_dual_margin) */
  /* DADDC·DSUBB → dual_clip peel (G_dual_margin) */
  /* DADDCN·DSUBBN → dual_clip peel (G_dual_margin) */
  /* DADDC2·DSUBB2 → dual_clip peel (G_dual_margin) */
  /* DSHLC·DSHRC → dual_clip peel (G_dual_margin) */
  /* DSHLCC·DSHRCC → dual_clip peel (G_dual_margin) */
  /* DSHLCCN·DSHRCCN → dual_clip peel (G_dual_margin) */
  /* DNEGC2 → dual_clip peel (G_dual_margin) */
  /* DNEGCC → dual_clip peel (G_dual_margin) */
  /* DNEGCN → dual_clip peel (G_dual_margin) */
  /* DEQ·DNE·DLT·DLE·DGT·DGE → dual_clip peel (G_dual_margin) */
  /* DLAND·DLOR·DLXOR·DIMP → dual_clip peel (G_dual_margin) */
  /* DGCD·DLCM·DPOW → dual_clip peel (G_dual_margin) */
  /* DGCDN·DLCMN → dual_clip peel (G_dual_margin) */
  /* DCOPRIMEN·DPOWN → dual_clip peel (G_dual_margin) */
  /* DSHL·DSHR·DSAR → dual_clip peel (G_dual_margin) */
  /* DSHL4/8/16·DSHR4/8/16·DSAR4/8/16 → dual_clip peel (G_dual_margin) */
  /* DSHL32·DSHR32·DSAR32 → dual_clip peel (G_dual_margin) */
  /* DSQR·DISQRT·DCOPRIME → dual_clip peel (G_dual_margin) */
  /* DLOG2·DPHI·DISPRIME → dual_clip peel (G_dual_margin) */
  /* DFIB·DFACT·DLOG10·DPOW10 → dual_clip peel (G_dual_margin) */
  /* DMOBIUS·DRAD·DSQFREE → dual_clip peel (G_dual_margin) */
  /* DADDMOD·DSUBMOD → dual_clip peel (G_dual_margin) */
  /* DMULMOD·DPOWMOD → dual_clip peel (G_dual_margin) */
  /* DADDMODN·DSUBMODN·DMULMODN → dual_clip peel (G_dual_margin) */
  /* DMODINV·DMODDIV → dual_clip peel (G_dual_margin) */
  /* DMODINVN·DPOWMODN·DMODDIVN → dual_clip peel (G_dual_margin) */
  /* DSUBMODFROMN·DPOWMODFROMN·DMODDIVFROMN → dual_clip peel (G_dual_margin) */
  /* DSIGN·D0EQ·D0NE·D0LT·D0GT·D0LE·D0GE → dual_clip peel (G_dual_margin) */
  /* DCLAMP·DSEL·DMUX·DSEL2·DMUX2 → dual_clip peel (G_dual_margin) */
  /* DNIPIF·DKEEPIF → dual_clip peel (G_dual_margin) */
  /* DSWAPIF·DDROPIF·DDUPIF → dual_clip peel (G_dual_margin) */
  /* DOVERIF·DTUCKIF → dual_clip peel (G_dual_margin) */
  /* DROTIF·DRROTIF → dual_clip peel (G_dual_margin) */
  /* DSHGATE·DZEROIF → dual_clip peel (G_dual_margin) */
  /* DAVG·DDIST·DHAMM → dual_clip peel (G_dual_margin) */
  /* DSUMSQ·DDIFFSQ → dual_clip peel (G_dual_margin) */
  /* DGEOM·DHARM·DRMS → dual_clip peel (G_dual_margin) */
  /* DRAND·DSATADD·DSATSUB·DSATMUL·DSATDIV → dual_clip peel (G_dual_margin) */
  /* DWRAP·DWRAPN·DHYPOT → dual_clip peel (G_dual_margin) */
  /* DPCT·DLERP → dual_clip peel (G_dual_margin) */
  /* DSCALE·DCLIP100 → dual_clip peel (G_dual_margin) */
  /* DCLIP01·DCOMP100 → dual_clip peel (G_dual_margin) */
  /* DSTEP·DHEAVI → dual_clip peel (G_dual_margin) */
  /* DLEAKY·DSOFTSIGN → dual_clip peel (G_dual_margin) */
  /* DINV·DNORM100 → dual_clip peel (G_dual_margin) */
  /* DRELU6·DDEADZ → dual_clip peel (G_dual_margin) */
  /* DRANDBITS·DRANDRANGE → dual_clip peel (G_dual_margin) */
  /* DCMP·DUCMP → dual_clip peel (G_dual_margin) */
  /* DINC·DDEC·DNOT·DEQZ·DNEZ → dual_clip peel (G_dual_margin) */
  /* DODD·DEVEN·DLTZ·DGTZ·DLEZ·DGEZ → dual_clip peel (G_dual_margin) */
  /* DTHRESH·DGATE → dual_clip peel (G_dual_margin) */
  /* DRELU·DCOPYSIGN → dual_clip peel (G_dual_margin) */
  /* DMEDIAN·DMAXABS·DMINABS → dual_clip peel (G_dual_margin) */
  /* DROL·DROR → dual_clip peel (G_dual_margin) */
  /* DROL4·DROR4·DROL8·DROR8·DROL16·DROR16 → dual_clip peel (G_dual_margin) */
  /* DNIBSWAP → dual_clip peel (G_dual_margin) */
  /* DWITHIN·DBETWEEN·DINRANGE → dual_clip peel (G_dual_margin) */
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
  /* DUMINN·DUMAXN → dual_clip peel (G_dual_margin) */
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
  /* DULTN·DUGTN·DULEN·DUGEN → dual_clip peel (G_dual_margin) */
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
  /* digit-3 dual-stack imm 16-bit field ordered-cmp: DLT16N · DGT16N · DLTE16N
   * (dual of SLT16N/SGT16N/SLTE16N; unsigned halfword predicates after DEQ16N/DNE16N) */
  if (kw(&L->cur,"DLT16N")||kw(&L->cur,"S2LT16N")||kw(&L->cur,"STACK2LT16N")||
      kw(&L->cur,"PAIRLT16N")||kw(&L->cur,"DCMPLT16N")||kw(&L->cur,"DLTHIMM")||
      kw(&L->cur,"DLTHN16")){
    /* a b + field n → halfword n of each = (hw < field) ? 1 : 0; n clamped 0..3 */
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
    unsigned long wa = (va < f) ? 1ul : 0ul;
    unsigned long wb = (vb < f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGT16N")||kw(&L->cur,"S2GT16N")||kw(&L->cur,"STACK2GT16N")||
      kw(&L->cur,"PAIRGT16N")||kw(&L->cur,"DCMPGT16N")||kw(&L->cur,"DGTHIMM")||
      kw(&L->cur,"DGTHN16")){
    /* a b + field n → halfword n of each = (hw > field) ? 1 : 0; n clamped 0..3 */
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
    unsigned long wa = (va > f) ? 1ul : 0ul;
    unsigned long wb = (vb > f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DLTE16N")||kw(&L->cur,"S2LTE16N")||kw(&L->cur,"STACK2LTE16N")||
      kw(&L->cur,"PAIRLTE16N")||kw(&L->cur,"DCMPLE16N")||kw(&L->cur,"DLEQ16N")||
      kw(&L->cur,"DLTEHIMM")||kw(&L->cur,"DLTEHN16")){
    /* a b + field n → halfword n of each = (hw <= field) ? 1 : 0; n clamped 0..3 */
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
    unsigned long wa = (va <= f) ? 1ul : 0ul;
    unsigned long wb = (vb <= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack imm 16-bit field GTE: DGTE16N (dual of SGTE16N after DLTE16N) */
  if (kw(&L->cur,"DGTE16N")||kw(&L->cur,"S2GTE16N")||kw(&L->cur,"STACK2GTE16N")||
      kw(&L->cur,"PAIRGTE16N")||kw(&L->cur,"DCMPGE16N")||kw(&L->cur,"DGEQ16N")||
      kw(&L->cur,"DGTEHIMM")||kw(&L->cur,"DGTEHN16")){
    /* a b + field n → halfword n of each = (hw >= field) ? 1 : 0; n clamped 0..3 */
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
    unsigned long wa = (va >= f) ? 1ul : 0ul;
    unsigned long wb = (vb >= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 dual-stack imm 16-bit field signed ordered-cmp: DLTS16N · DGTS16N · DLTES16N · DGTES16N
   * (dual of SLTS16N/SGTS16N/SLTES16N/SGTES16N; signed int16 plane on top two) */
  if (kw(&L->cur,"DLTS16N")||kw(&L->cur,"S2LTS16N")||kw(&L->cur,"STACK2LTS16N")||
      kw(&L->cur,"PAIRLTS16N")||kw(&L->cur,"DCMPLTS16N")||kw(&L->cur,"DLTSSIGN16N")||
      kw(&L->cur,"DLTS16IMM")){
    /* a b + field n → halfword n of each = (int16(hw) < int16(field)) ? 1 : 0; n 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    long f = (long)((unsigned long)field & 0xFFFFul);
    if (f & 0x8000) f -= 65536;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    unsigned long wa = (va < f) ? 1ul : 0ul;
    unsigned long wb = (vb < f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGTS16N")||kw(&L->cur,"S2GTS16N")||kw(&L->cur,"STACK2GTS16N")||
      kw(&L->cur,"PAIRGTS16N")||kw(&L->cur,"DCMPGTS16N")||kw(&L->cur,"DGTSSIGN16N")||
      kw(&L->cur,"DGTS16IMM")){
    /* a b + field n → halfword n of each = (int16(hw) > int16(field)) ? 1 : 0; n 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    long f = (long)((unsigned long)field & 0xFFFFul);
    if (f & 0x8000) f -= 65536;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    unsigned long wa = (va > f) ? 1ul : 0ul;
    unsigned long wb = (vb > f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DLTES16N")||kw(&L->cur,"S2LTES16N")||kw(&L->cur,"STACK2LTES16N")||
      kw(&L->cur,"PAIRLTES16N")||kw(&L->cur,"DCMPLES16N")||kw(&L->cur,"DLEQS16N")||
      kw(&L->cur,"DLTES16IMM")){
    /* a b + field n → halfword n of each = (int16(hw) <= int16(field)) ? 1 : 0; n 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    long f = (long)((unsigned long)field & 0xFFFFul);
    if (f & 0x8000) f -= 65536;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    unsigned long wa = (va <= f) ? 1ul : 0ul;
    unsigned long wb = (vb <= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGTES16N")||kw(&L->cur,"S2GTES16N")||kw(&L->cur,"STACK2GTES16N")||
      kw(&L->cur,"PAIRGTES16N")||kw(&L->cur,"DCMPGES16N")||kw(&L->cur,"DGEQS16N")||
      kw(&L->cur,"DGTES16IMM")){
    /* a b + field n → halfword n of each = (int16(hw) >= int16(field)) ? 1 : 0; n 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    long f = (long)((unsigned long)field & 0xFFFFul);
    if (f & 0x8000) f -= 65536;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    unsigned long wa = (va >= f) ? 1ul : 0ul;
    unsigned long wb = (vb >= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 dual-stack imm 16-bit field signed min/max: DMINS16N · DMAXS16N
   * (dual of SMINS16N/SMAXS16N; halfword ladder of DMINS32N after DLTS16N) */
  if (kw(&L->cur,"DMINS16N")||kw(&L->cur,"S2MINS16N")||kw(&L->cur,"STACK2MINS16N")||
      kw(&L->cur,"PAIRMINS16N")||kw(&L->cur,"DMINSIGN16N")||kw(&L->cur,"DMINS16IMM")||
      kw(&L->cur,"DMINSI16N")){
    /* a b + field n → halfword n of each = min_signed(int16(hw), int16(field)); n 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    long f = (long)((unsigned long)field & 0xFFFFul);
    if (f & 0x8000) f -= 65536;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    long ma_m = (va < f) ? va : f;
    long mb_m = (vb < f) ? vb : f;
    unsigned long wa = (unsigned long)ma_m & 0xFFFFul;
    unsigned long wb = (unsigned long)mb_m & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMAXS16N")||kw(&L->cur,"S2MAXS16N")||kw(&L->cur,"STACK2MAXS16N")||
      kw(&L->cur,"PAIRMAXS16N")||kw(&L->cur,"DMAXSIGN16N")||kw(&L->cur,"DMAXS16IMM")||
      kw(&L->cur,"DMAXSI16N")){
    /* a b + field n → halfword n of each = max_signed(int16(hw), int16(field)); n 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    long f = (long)((unsigned long)field & 0xFFFFul);
    if (f & 0x8000) f -= 65536;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    long ma_m = (va > f) ? va : f;
    long mb_m = (vb > f) ? vb : f;
    unsigned long wa = (unsigned long)ma_m & 0xFFFFul;
    unsigned long wb = (unsigned long)mb_m & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm 16-bit field signed clamp+between: DCLAMPS16N · DBETWEENS16N
   * (dual of SCLAMPS16N/SBETWEENS16N; halfword ladder of DCLAMPS32N after DMINS16N) */
  if (kw(&L->cur,"DCLAMPS16N")||kw(&L->cur,"S2CLAMPS16N")||kw(&L->cur,"STACK2CLAMPS16N")||
      kw(&L->cur,"PAIRCLAMPS16N")||kw(&L->cur,"DCLAMPSIGN16N")||kw(&L->cur,"DCLAMPS16IMM")||
      kw(&L->cur,"DBOUNDS16N")){
    /* a b + lo hi n → halfword n of each = clamp_signed(int16(hw),[lo,hi]); n 0..3 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    long slo = (long)((unsigned long)lo & 0xFFFFul); if (slo & 0x8000) slo -= 65536;
    long shi = (long)((unsigned long)hi & 0xFFFFul); if (shi & 0x8000) shi -= 65536;
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    if (va < slo) va = slo; if (va > shi) va = shi;
    if (vb < slo) vb = slo; if (vb > shi) vb = shi;
    unsigned long wa = (unsigned long)va & 0xFFFFul;
    unsigned long wb = (unsigned long)vb & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DBETWEENS16N")||kw(&L->cur,"S2BETWEENS16N")||kw(&L->cur,"STACK2BETWEENS16N")||
      kw(&L->cur,"PAIRBETWEENS16N")||kw(&L->cur,"DINRANGES16N")||kw(&L->cur,"DBETWEENS16IMM")||
      kw(&L->cur,"DINRANGESIGN16N")){
    /* a b + lo hi n → halfword n of each = (lo <= int16(hw) <= hi) ? 1 : 0; n 0..3 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    long slo = (long)((unsigned long)lo & 0xFFFFul); if (slo & 0x8000) slo -= 65536;
    long shi = (long)((unsigned long)hi & 0xFFFFul); if (shi & 0x8000) shi -= 65536;
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    unsigned long wa = (va >= slo && va <= shi) ? 1ul : 0ul;
    unsigned long wb = (vb >= slo && vb <= shi) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm 16-bit field signed WITHIN: DWITHINS16N (dual of SWITHINS16N) */
  if (kw(&L->cur,"DWITHINS16N")||kw(&L->cur,"S2WITHINS16N")||kw(&L->cur,"STACK2WITHINS16N")||
      kw(&L->cur,"PAIRWITHINS16N")||kw(&L->cur,"DWITHINSIGN16N")||kw(&L->cur,"DWITHINS16IMM")||
      kw(&L->cur,"DINTERVALS16N")){
    /* a b + lo hi n → halfword n of each = (lo <= int16(hw) < hi) ? 1 : 0; n 0..3; no swap */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    long slo = (long)((unsigned long)lo & 0xFFFFul); if (slo & 0x8000) slo -= 65536;
    long shi = (long)((unsigned long)hi & 0xFFFFul); if (shi & 0x8000) shi -= 65536;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    unsigned long wa = (va >= slo && va < shi) ? 1ul : 0ul;
    unsigned long wb = (vb >= slo && vb < shi) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack imm 16-bit field signed signum: DSIGN16N (dual of SSIGN16N) */
  if (kw(&L->cur,"DSIGN16N")||kw(&L->cur,"S2SIGN16N")||kw(&L->cur,"STACK2SIGN16N")||
      kw(&L->cur,"PAIRSIGN16N")||kw(&L->cur,"DSIGNUM16N")||kw(&L->cur,"DSGN16N")||
      kw(&L->cur,"DSIGN16IMM")){
    /* a b + n → halfword n of each = sgn(int16): +1 / 0 / 0xFFFF; n 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    unsigned long wa = (va > 0) ? 1ul : ((va < 0) ? 0xFFFFul : 0ul);
    unsigned long wb = (vb > 0) ? 1ul : ((vb < 0) ? 0xFFFFul : 0ul);
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack imm 16-bit field zero-eq: D0EQ16N (dual of S0EQ16N) */
  if (kw(&L->cur,"D0EQ16N")||kw(&L->cur,"S20EQ16N")||kw(&L->cur,"STACK20EQ16N")||
      kw(&L->cur,"PAIR0EQ16N")||kw(&L->cur,"DZ16N")||kw(&L->cur,"DEQZ16N")||
      kw(&L->cur,"D0EQ16IMM")){
    /* a b + n → halfword n of each = (hw == 0) ? 1 : 0; n 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long wa = (va == 0) ? 1ul : 0ul;
    unsigned long wb = (vb == 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack imm 16-bit field nonzero: D0NE16N (dual of S0NE16N) */
  if (kw(&L->cur,"D0NE16N")||kw(&L->cur,"S20NE16N")||kw(&L->cur,"STACK20NE16N")||
      kw(&L->cur,"PAIR0NE16N")||kw(&L->cur,"DNZ16N")||kw(&L->cur,"DNEZ16N")||
      kw(&L->cur,"D0NE16IMM")){
    /* a b + n → halfword n of each = (hw != 0) ? 1 : 0; n 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long wa = (va != 0) ? 1ul : 0ul;
    unsigned long wb = (vb != 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 dual-stack imm 16-bit field signed LT0: D0LT16N (dual of S0LT16N) */
  if (kw(&L->cur,"D0LT16N")||kw(&L->cur,"S20LT16N")||kw(&L->cur,"STACK20LT16N")||
      kw(&L->cur,"PAIR0LT16N")||kw(&L->cur,"DLT016N")||kw(&L->cur,"DNEGZ16N")||
      kw(&L->cur,"D0LT16IMM")){
    /* a b + n → halfword n of each = (int16(hw) < 0) ? 1 : 0; n 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    unsigned long wa = (va < 0) ? 1ul : 0ul;
    unsigned long wb = (vb < 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 dual-stack imm 16-bit field signed GT0: D0GT16N (dual of S0GT16N) */
  if (kw(&L->cur,"D0GT16N")||kw(&L->cur,"S20GT16N")||kw(&L->cur,"STACK20GT16N")||
      kw(&L->cur,"PAIR0GT16N")||kw(&L->cur,"DGT016N")||kw(&L->cur,"DPOSZ16N")||
      kw(&L->cur,"D0GT16IMM")){
    /* a b + n → halfword n of each = (int16(hw) > 0) ? 1 : 0; n 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    unsigned long wa = (va > 0) ? 1ul : 0ul;
    unsigned long wb = (vb > 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack imm 16-bit field signed LE0: D0LE16N (dual of S0LE16N) */
  if (kw(&L->cur,"D0LE16N")||kw(&L->cur,"S20LE16N")||kw(&L->cur,"STACK20LE16N")||
      kw(&L->cur,"PAIR0LE16N")||kw(&L->cur,"DLE016N")||kw(&L->cur,"DNONPOS16N")||
      kw(&L->cur,"D0LE16IMM")){
    /* a b + n → halfword n of each = (int16(hw) <= 0) ? 1 : 0; n 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    unsigned long wa = (va <= 0) ? 1ul : 0ul;
    unsigned long wb = (vb <= 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack imm 16-bit field signed GE0: D0GE16N (dual of S0GE16N) */
  if (kw(&L->cur,"D0GE16N")||kw(&L->cur,"S20GE16N")||kw(&L->cur,"STACK20GE16N")||
      kw(&L->cur,"PAIR0GE16N")||kw(&L->cur,"DGE016N")||kw(&L->cur,"DNONNEG16N")||
      kw(&L->cur,"D0GE16IMM")){
    /* a b + n → halfword n of each = (int16(hw) >= 0) ? 1 : 0; n 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    unsigned long wa = (va >= 0) ? 1ul : 0ul;
    unsigned long wb = (vb >= 0) ? 1ul : 0ul;
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
  /* digit-3 dual-stack imm 8-bit field ordered-cmp: DLT8N · DGT8N · DLTE8N
   * (dual of SLT8N/SGT8N/SLTE8N; unsigned byte predicates after DEQ8N/DNE8N) */
  if (kw(&L->cur,"DLT8N")||kw(&L->cur,"S2LT8N")||kw(&L->cur,"STACK2LT8N")||
      kw(&L->cur,"PAIRLT8N")||kw(&L->cur,"DCMPLT8N")||kw(&L->cur,"DLTBIMM")||
      kw(&L->cur,"DLTBN")){
    /* a b + field n → byte n of each = (b < field) ? 1 : 0; n clamped 0..7 */
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
    unsigned long wa = (va < f) ? 1ul : 0ul;
    unsigned long wb = (vb < f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGT8N")||kw(&L->cur,"S2GT8N")||kw(&L->cur,"STACK2GT8N")||
      kw(&L->cur,"PAIRGT8N")||kw(&L->cur,"DCMPGT8N")||kw(&L->cur,"DGTBIMM")||
      kw(&L->cur,"DGTBN")){
    /* a b + field n → byte n of each = (b > field) ? 1 : 0; n clamped 0..7 */
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
    unsigned long wa = (va > f) ? 1ul : 0ul;
    unsigned long wb = (vb > f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DLTE8N")||kw(&L->cur,"S2LTE8N")||kw(&L->cur,"STACK2LTE8N")||
      kw(&L->cur,"PAIRLTE8N")||kw(&L->cur,"DCMPLE8N")||kw(&L->cur,"DLEQ8N")||
      kw(&L->cur,"DLTEBIMM")||kw(&L->cur,"DLTEBN")){
    /* a b + field n → byte n of each = (b <= field) ? 1 : 0; n clamped 0..7 */
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
    unsigned long wa = (va <= f) ? 1ul : 0ul;
    unsigned long wb = (vb <= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack imm 8-bit field GTE: DGTE8N (dual of SGTE8N after DLTE8N) */
  if (kw(&L->cur,"DGTE8N")||kw(&L->cur,"S2GTE8N")||kw(&L->cur,"STACK2GTE8N")||
      kw(&L->cur,"PAIRGTE8N")||kw(&L->cur,"DCMPGE8N")||kw(&L->cur,"DGEQ8N")||
      kw(&L->cur,"DGTEBIMM")||kw(&L->cur,"DGTEBN")){
    /* a b + field n → byte n of each = (b >= field) ? 1 : 0; n clamped 0..7 */
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
    unsigned long wa = (va >= f) ? 1ul : 0ul;
    unsigned long wb = (vb >= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack imm 8-bit field signed ordered-cmp: DLTS8N · DGTS8N · DLTES8N · DGTES8N
   * (dual of SLTS8N/SGTS8N/SLTES8N/SGTES8N; signed int8 plane on top two) */
  if (kw(&L->cur,"DLTS8N")||kw(&L->cur,"S2LTS8N")||kw(&L->cur,"STACK2LTS8N")||
      kw(&L->cur,"PAIRLTS8N")||kw(&L->cur,"DCMPLTS8N")||kw(&L->cur,"DLTSSIGN8N")||
      kw(&L->cur,"DLTS8IMM")){
    /* a b + field n → byte n of each = (int8(b) < int8(field)) ? 1 : 0; n 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    long f = (long)((unsigned long)field & 0xFFul);
    if (f & 0x80) f -= 256;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    unsigned long wa = (va < f) ? 1ul : 0ul;
    unsigned long wb = (vb < f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGTS8N")||kw(&L->cur,"S2GTS8N")||kw(&L->cur,"STACK2GTS8N")||
      kw(&L->cur,"PAIRGTS8N")||kw(&L->cur,"DCMPGTS8N")||kw(&L->cur,"DGTSSIGN8N")||
      kw(&L->cur,"DGTS8IMM")){
    /* a b + field n → byte n of each = (int8(b) > int8(field)) ? 1 : 0; n 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    long f = (long)((unsigned long)field & 0xFFul);
    if (f & 0x80) f -= 256;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    unsigned long wa = (va > f) ? 1ul : 0ul;
    unsigned long wb = (vb > f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DLTES8N")||kw(&L->cur,"S2LTES8N")||kw(&L->cur,"STACK2LTES8N")||
      kw(&L->cur,"PAIRLTES8N")||kw(&L->cur,"DCMPLES8N")||kw(&L->cur,"DLEQS8N")||
      kw(&L->cur,"DLTES8IMM")){
    /* a b + field n → byte n of each = (int8(b) <= int8(field)) ? 1 : 0; n 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    long f = (long)((unsigned long)field & 0xFFul);
    if (f & 0x80) f -= 256;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    unsigned long wa = (va <= f) ? 1ul : 0ul;
    unsigned long wb = (vb <= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGTES8N")||kw(&L->cur,"S2GTES8N")||kw(&L->cur,"STACK2GTES8N")||
      kw(&L->cur,"PAIRGTES8N")||kw(&L->cur,"DCMPGES8N")||kw(&L->cur,"DGEQS8N")||
      kw(&L->cur,"DGTES8IMM")){
    /* a b + field n → byte n of each = (int8(b) >= int8(field)) ? 1 : 0; n 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    long f = (long)((unsigned long)field & 0xFFul);
    if (f & 0x80) f -= 256;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    unsigned long wa = (va >= f) ? 1ul : 0ul;
    unsigned long wb = (vb >= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 dual-stack imm 8-bit field signed min/max: DMINS8N · DMAXS8N
   * (dual of SMINS8N/SMAXS8N; byte ladder of DMINS16N after DLTS8N) */
  if (kw(&L->cur,"DMINS8N")||kw(&L->cur,"S2MINS8N")||kw(&L->cur,"STACK2MINS8N")||
      kw(&L->cur,"PAIRMINS8N")||kw(&L->cur,"DMINSIGN8N")||kw(&L->cur,"DMINS8IMM")||
      kw(&L->cur,"DMINSI8N")){
    /* a b + field n → byte n of each = min_signed(int8(b), int8(field)); n 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    long f = (long)((unsigned long)field & 0xFFul);
    if (f & 0x80) f -= 256;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    long ma_m = (va < f) ? va : f;
    long mb_m = (vb < f) ? vb : f;
    unsigned long wa = (unsigned long)ma_m & 0xFFul;
    unsigned long wb = (unsigned long)mb_m & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMAXS8N")||kw(&L->cur,"S2MAXS8N")||kw(&L->cur,"STACK2MAXS8N")||
      kw(&L->cur,"PAIRMAXS8N")||kw(&L->cur,"DMAXSIGN8N")||kw(&L->cur,"DMAXS8IMM")||
      kw(&L->cur,"DMAXSI8N")){
    /* a b + field n → byte n of each = max_signed(int8(b), int8(field)); n 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    long f = (long)((unsigned long)field & 0xFFul);
    if (f & 0x80) f -= 256;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    long ma_m = (va > f) ? va : f;
    long mb_m = (vb > f) ? vb : f;
    unsigned long wa = (unsigned long)ma_m & 0xFFul;
    unsigned long wb = (unsigned long)mb_m & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 dual-stack imm 8-bit field signed clamp+between: DCLAMPS8N · DBETWEENS8N
   * (dual of SCLAMPS8N/SBETWEENS8N; byte ladder of DCLAMPS16N after DMINS8N) */
  if (kw(&L->cur,"DCLAMPS8N")||kw(&L->cur,"S2CLAMPS8N")||kw(&L->cur,"STACK2CLAMPS8N")||
      kw(&L->cur,"PAIRCLAMPS8N")||kw(&L->cur,"DCLAMPSIGN8N")||kw(&L->cur,"DCLAMPS8IMM")||
      kw(&L->cur,"DBOUNDS8N")){
    /* a b + lo hi n → byte n of each = clamp_signed(int8(b),[lo,hi]); n 0..7 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    long slo = (long)((unsigned long)lo & 0xFFul); if (slo & 0x80) slo -= 256;
    long shi = (long)((unsigned long)hi & 0xFFul); if (shi & 0x80) shi -= 256;
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    if (va < slo) va = slo; if (va > shi) va = shi;
    if (vb < slo) vb = slo; if (vb > shi) vb = shi;
    unsigned long wa = (unsigned long)va & 0xFFul;
    unsigned long wb = (unsigned long)vb & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DBETWEENS8N")||kw(&L->cur,"S2BETWEENS8N")||kw(&L->cur,"STACK2BETWEENS8N")||
      kw(&L->cur,"PAIRBETWEENS8N")||kw(&L->cur,"DINRANGES8N")||kw(&L->cur,"DBETWEENS8IMM")||
      kw(&L->cur,"DINRANGESIGN8N")){
    /* a b + lo hi n → byte n of each = (lo <= int8(b) <= hi) ? 1 : 0; n 0..7 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    long slo = (long)((unsigned long)lo & 0xFFul); if (slo & 0x80) slo -= 256;
    long shi = (long)((unsigned long)hi & 0xFFul); if (shi & 0x80) shi -= 256;
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    unsigned long wa = (va >= slo && va <= shi) ? 1ul : 0ul;
    unsigned long wb = (vb >= slo && vb <= shi) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack imm 8-bit field signed WITHIN: DWITHINS8N (dual of SWITHINS8N) */
  if (kw(&L->cur,"DWITHINS8N")||kw(&L->cur,"S2WITHINS8N")||kw(&L->cur,"STACK2WITHINS8N")||
      kw(&L->cur,"PAIRWITHINS8N")||kw(&L->cur,"DWITHINSIGN8N")||kw(&L->cur,"DWITHINS8IMM")||
      kw(&L->cur,"DINTERVALS8N")){
    /* a b + lo hi n → byte n of each = (lo <= int8(b) < hi) ? 1 : 0; n 0..7; no swap */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    long slo = (long)((unsigned long)lo & 0xFFul); if (slo & 0x80) slo -= 256;
    long shi = (long)((unsigned long)hi & 0xFFul); if (shi & 0x80) shi -= 256;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    unsigned long wa = (va >= slo && va < shi) ? 1ul : 0ul;
    unsigned long wb = (vb >= slo && vb < shi) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm 8-bit field signed signum: DSIGN8N (dual of SSIGN8N) */
  if (kw(&L->cur,"DSIGN8N")||kw(&L->cur,"S2SIGN8N")||kw(&L->cur,"STACK2SIGN8N")||
      kw(&L->cur,"PAIRSIGN8N")||kw(&L->cur,"DSIGNUM8N")||kw(&L->cur,"DSGN8N")||
      kw(&L->cur,"DSIGN8IMM")){
    /* a b + n → byte n of each = sgn(int8): +1 / 0 / 0xFF; n 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    unsigned long wa = (va > 0) ? 1ul : ((va < 0) ? 0xFFul : 0ul);
    unsigned long wb = (vb > 0) ? 1ul : ((vb < 0) ? 0xFFul : 0ul);
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm 8-bit field zero-eq: D0EQ8N (dual of S0EQ8N) */
  if (kw(&L->cur,"D0EQ8N")||kw(&L->cur,"S20EQ8N")||kw(&L->cur,"STACK20EQ8N")||
      kw(&L->cur,"PAIR0EQ8N")||kw(&L->cur,"DZ8N")||kw(&L->cur,"DEQZ8N")||
      kw(&L->cur,"D0EQ8IMM")){
    /* a b + n → byte n of each = (b == 0) ? 1 : 0; n 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long wa = (va == 0) ? 1ul : 0ul;
    unsigned long wb = (vb == 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack imm 8-bit field nonzero: D0NE8N (dual of S0NE8N) */
  if (kw(&L->cur,"D0NE8N")||kw(&L->cur,"S20NE8N")||kw(&L->cur,"STACK20NE8N")||
      kw(&L->cur,"PAIR0NE8N")||kw(&L->cur,"DNZ8N")||kw(&L->cur,"DNEZ8N")||
      kw(&L->cur,"D0NE8IMM")){
    /* a b + n → byte n of each = (b != 0) ? 1 : 0; n 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long wa = (va != 0) ? 1ul : 0ul;
    unsigned long wb = (vb != 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm 8-bit field signed LT0: D0LT8N (dual of S0LT8N) */
  if (kw(&L->cur,"D0LT8N")||kw(&L->cur,"S20LT8N")||kw(&L->cur,"STACK20LT8N")||
      kw(&L->cur,"PAIR0LT8N")||kw(&L->cur,"DLT08N")||kw(&L->cur,"DNEGZ8N")||
      kw(&L->cur,"D0LT8IMM")){
    /* a b + n → byte n of each = (int8(b) < 0) ? 1 : 0; n 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    unsigned long wa = (va < 0) ? 1ul : 0ul;
    unsigned long wb = (vb < 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack imm 8-bit field signed GT0: D0GT8N (dual of S0GT8N) */
  if (kw(&L->cur,"D0GT8N")||kw(&L->cur,"S20GT8N")||kw(&L->cur,"STACK20GT8N")||
      kw(&L->cur,"PAIR0GT8N")||kw(&L->cur,"DGT08N")||kw(&L->cur,"DPOSZ8N")||
      kw(&L->cur,"D0GT8IMM")){
    /* a b + n → byte n of each = (int8(b) > 0) ? 1 : 0; n 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    unsigned long wa = (va > 0) ? 1ul : 0ul;
    unsigned long wb = (vb > 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack imm 8-bit field signed LE0: D0LE8N (dual of S0LE8N) */
  if (kw(&L->cur,"D0LE8N")||kw(&L->cur,"S20LE8N")||kw(&L->cur,"STACK20LE8N")||
      kw(&L->cur,"PAIR0LE8N")||kw(&L->cur,"DLE08N")||kw(&L->cur,"DNONPOS8N")||
      kw(&L->cur,"D0LE8IMM")){
    /* a b + n → byte n of each = (int8(b) <= 0) ? 1 : 0; n 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    unsigned long wa = (va <= 0) ? 1ul : 0ul;
    unsigned long wb = (vb <= 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack imm 8-bit field signed GE0: D0GE8N (dual of S0GE8N) */
  if (kw(&L->cur,"D0GE8N")||kw(&L->cur,"S20GE8N")||kw(&L->cur,"STACK20GE8N")||
      kw(&L->cur,"PAIR0GE8N")||kw(&L->cur,"DGE08N")||kw(&L->cur,"DNONNEG8N")||
      kw(&L->cur,"D0GE8IMM")){
    /* a b + n → byte n of each = (int8(b) >= 0) ? 1 : 0; n 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    unsigned long wa = (va >= 0) ? 1ul : 0ul;
    unsigned long wb = (vb >= 0) ? 1ul : 0ul;
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
  /* digit-1 dual-stack imm 4-bit field ordered-cmp: DLT4N · DGT4N · DLTE4N
   * (dual of SLT4N/SGT4N/SLTE4N; unsigned nibble predicates after DEQ4N/DNE4N) */
  if (kw(&L->cur,"DLT4N")||kw(&L->cur,"S2LT4N")||kw(&L->cur,"STACK2LT4N")||
      kw(&L->cur,"PAIRLT4N")||kw(&L->cur,"DCMPLT4N")||kw(&L->cur,"DLTNIMM")||
      kw(&L->cur,"DLTNIBN")){
    /* a b + field n → nibble n of each = (nib < field) ? 1 : 0; n clamped 0..15 */
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
    unsigned long wa = (va < f) ? 1ul : 0ul;
    unsigned long wb = (vb < f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGT4N")||kw(&L->cur,"S2GT4N")||kw(&L->cur,"STACK2GT4N")||
      kw(&L->cur,"PAIRGT4N")||kw(&L->cur,"DCMPGT4N")||kw(&L->cur,"DGTNIMM")||
      kw(&L->cur,"DGTNIBN")){
    /* a b + field n → nibble n of each = (nib > field) ? 1 : 0; n clamped 0..15 */
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
    unsigned long wa = (va > f) ? 1ul : 0ul;
    unsigned long wb = (vb > f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DLTE4N")||kw(&L->cur,"S2LTE4N")||kw(&L->cur,"STACK2LTE4N")||
      kw(&L->cur,"PAIRLTE4N")||kw(&L->cur,"DCMPLE4N")||kw(&L->cur,"DLEQ4N")||
      kw(&L->cur,"DLTENIMM")||kw(&L->cur,"DLTENIBN")){
    /* a b + field n → nibble n of each = (nib <= field) ? 1 : 0; n clamped 0..15 */
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
    unsigned long wa = (va <= f) ? 1ul : 0ul;
    unsigned long wb = (vb <= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack imm 4-bit field GTE: DGTE4N (dual of SGTE4N after DLTE4N) */
  if (kw(&L->cur,"DGTE4N")||kw(&L->cur,"S2GTE4N")||kw(&L->cur,"STACK2GTE4N")||
      kw(&L->cur,"PAIRGTE4N")||kw(&L->cur,"DCMPGE4N")||kw(&L->cur,"DGEQ4N")||
      kw(&L->cur,"DGTENIMM")||kw(&L->cur,"DGTENIBN")){
    /* a b + field n → nibble n of each = (nib >= field) ? 1 : 0; n clamped 0..15 */
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
    unsigned long wa = (va >= f) ? 1ul : 0ul;
    unsigned long wb = (vb >= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack imm 4-bit field signed ordered-cmp: DLTS4N · DGTS4N · DLTES4N · DGTES4N
   * (dual of SLTS4N/SGTS4N/SLTES4N/SGTES4N; signed int4 plane on top two) */
  if (kw(&L->cur,"DLTS4N")||kw(&L->cur,"S2LTS4N")||kw(&L->cur,"STACK2LTS4N")||
      kw(&L->cur,"PAIRLTS4N")||kw(&L->cur,"DCMPLTS4N")||kw(&L->cur,"DLTSSIGN4N")||
      kw(&L->cur,"DLTS4IMM")){
    /* a b + field n → nibble n of each = (int4(nib) < int4(field)) ? 1 : 0; n 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    long f = (long)((unsigned long)field & 0xFul);
    if (f & 0x8) f -= 16;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul); if (va & 0x8) va -= 16;
    long vb = (long)((mb >> sh) & 0xFul); if (vb & 0x8) vb -= 16;
    unsigned long wa = (va < f) ? 1ul : 0ul;
    unsigned long wb = (vb < f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGTS4N")||kw(&L->cur,"S2GTS4N")||kw(&L->cur,"STACK2GTS4N")||
      kw(&L->cur,"PAIRGTS4N")||kw(&L->cur,"DCMPGTS4N")||kw(&L->cur,"DGTSSIGN4N")||
      kw(&L->cur,"DGTS4IMM")){
    /* a b + field n → nibble n of each = (int4(nib) > int4(field)) ? 1 : 0; n 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    long f = (long)((unsigned long)field & 0xFul);
    if (f & 0x8) f -= 16;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul); if (va & 0x8) va -= 16;
    long vb = (long)((mb >> sh) & 0xFul); if (vb & 0x8) vb -= 16;
    unsigned long wa = (va > f) ? 1ul : 0ul;
    unsigned long wb = (vb > f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DLTES4N")||kw(&L->cur,"S2LTES4N")||kw(&L->cur,"STACK2LTES4N")||
      kw(&L->cur,"PAIRLTES4N")||kw(&L->cur,"DCMPLES4N")||kw(&L->cur,"DLEQS4N")||
      kw(&L->cur,"DLTES4IMM")){
    /* a b + field n → nibble n of each = (int4(nib) <= int4(field)) ? 1 : 0; n 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    long f = (long)((unsigned long)field & 0xFul);
    if (f & 0x8) f -= 16;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul); if (va & 0x8) va -= 16;
    long vb = (long)((mb >> sh) & 0xFul); if (vb & 0x8) vb -= 16;
    unsigned long wa = (va <= f) ? 1ul : 0ul;
    unsigned long wb = (vb <= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGTES4N")||kw(&L->cur,"S2GTES4N")||kw(&L->cur,"STACK2GTES4N")||
      kw(&L->cur,"PAIRGTES4N")||kw(&L->cur,"DCMPGES4N")||kw(&L->cur,"DGEQS4N")||
      kw(&L->cur,"DGTES4IMM")){
    /* a b + field n → nibble n of each = (int4(nib) >= int4(field)) ? 1 : 0; n 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    long f = (long)((unsigned long)field & 0xFul);
    if (f & 0x8) f -= 16;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul); if (va & 0x8) va -= 16;
    long vb = (long)((mb >> sh) & 0xFul); if (vb & 0x8) vb -= 16;
    unsigned long wa = (va >= f) ? 1ul : 0ul;
    unsigned long wb = (vb >= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack imm 4-bit field signed min/max: DMINS4N · DMAXS4N
   * (dual of SMINS4N/SMAXS4N; nibble ladder of DMINS8N after DLTS4N; completes signed select all widths) */
  if (kw(&L->cur,"DMINS4N")||kw(&L->cur,"S2MINS4N")||kw(&L->cur,"STACK2MINS4N")||
      kw(&L->cur,"PAIRMINS4N")||kw(&L->cur,"DMINSIGN4N")||kw(&L->cur,"DMINS4IMM")||
      kw(&L->cur,"DMINSI4N")){
    /* a b + field n → nibble n of each = min_signed(int4(nib), int4(field)); n 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    long f = (long)((unsigned long)field & 0xFul);
    if (f & 0x8) f -= 16;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul); if (va & 0x8) va -= 16;
    long vb = (long)((mb >> sh) & 0xFul); if (vb & 0x8) vb -= 16;
    long ma_m = (va < f) ? va : f;
    long mb_m = (vb < f) ? vb : f;
    unsigned long wa = (unsigned long)ma_m & 0xFul;
    unsigned long wb = (unsigned long)mb_m & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMAXS4N")||kw(&L->cur,"S2MAXS4N")||kw(&L->cur,"STACK2MAXS4N")||
      kw(&L->cur,"PAIRMAXS4N")||kw(&L->cur,"DMAXSIGN4N")||kw(&L->cur,"DMAXS4IMM")||
      kw(&L->cur,"DMAXSI4N")){
    /* a b + field n → nibble n of each = max_signed(int4(nib), int4(field)); n 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    long f = (long)((unsigned long)field & 0xFul);
    if (f & 0x8) f -= 16;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul); if (va & 0x8) va -= 16;
    long vb = (long)((mb >> sh) & 0xFul); if (vb & 0x8) vb -= 16;
    long ma_m = (va > f) ? va : f;
    long mb_m = (vb > f) ? vb : f;
    unsigned long wa = (unsigned long)ma_m & 0xFul;
    unsigned long wb = (unsigned long)mb_m & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack imm 4-bit field signed clamp+between: DCLAMPS4N · DBETWEENS4N
   * (dual of SCLAMPS4N/SBETWEENS4N; completes signed range all widths on dual) */
  if (kw(&L->cur,"DCLAMPS4N")||kw(&L->cur,"S2CLAMPS4N")||kw(&L->cur,"STACK2CLAMPS4N")||
      kw(&L->cur,"PAIRCLAMPS4N")||kw(&L->cur,"DCLAMPSIGN4N")||kw(&L->cur,"DCLAMPS4IMM")||
      kw(&L->cur,"DBOUNDS4N")){
    /* a b + lo hi n → nibble n of each = clamp_signed(int4(nib),[lo,hi]); n 0..15 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    long slo = (long)((unsigned long)lo & 0xFul); if (slo & 0x8) slo -= 16;
    long shi = (long)((unsigned long)hi & 0xFul); if (shi & 0x8) shi -= 16;
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul); if (va & 0x8) va -= 16;
    long vb = (long)((mb >> sh) & 0xFul); if (vb & 0x8) vb -= 16;
    if (va < slo) va = slo; if (va > shi) va = shi;
    if (vb < slo) vb = slo; if (vb > shi) vb = shi;
    unsigned long wa = (unsigned long)va & 0xFul;
    unsigned long wb = (unsigned long)vb & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DBETWEENS4N")||kw(&L->cur,"S2BETWEENS4N")||kw(&L->cur,"STACK2BETWEENS4N")||
      kw(&L->cur,"PAIRBETWEENS4N")||kw(&L->cur,"DINRANGES4N")||kw(&L->cur,"DBETWEENS4IMM")||
      kw(&L->cur,"DINRANGESIGN4N")){
    /* a b + lo hi n → nibble n of each = (lo <= int4(nib) <= hi) ? 1 : 0; n 0..15 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    long slo = (long)((unsigned long)lo & 0xFul); if (slo & 0x8) slo -= 16;
    long shi = (long)((unsigned long)hi & 0xFul); if (shi & 0x8) shi -= 16;
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul); if (va & 0x8) va -= 16;
    long vb = (long)((mb >> sh) & 0xFul); if (vb & 0x8) vb -= 16;
    unsigned long wa = (va >= slo && va <= shi) ? 1ul : 0ul;
    unsigned long wb = (vb >= slo && vb <= shi) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack imm 4-bit field signed WITHIN: DWITHINS4N (completes WITHIN all widths dual) */
  if (kw(&L->cur,"DWITHINS4N")||kw(&L->cur,"S2WITHINS4N")||kw(&L->cur,"STACK2WITHINS4N")||
      kw(&L->cur,"PAIRWITHINS4N")||kw(&L->cur,"DWITHINSIGN4N")||kw(&L->cur,"DWITHINS4IMM")||
      kw(&L->cur,"DINTERVALS4N")){
    /* a b + lo hi n → nibble n of each = (lo <= int4(nib) < hi) ? 1 : 0; n 0..15; no swap */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    long slo = (long)((unsigned long)lo & 0xFul); if (slo & 0x8) slo -= 16;
    long shi = (long)((unsigned long)hi & 0xFul); if (shi & 0x8) shi -= 16;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul); if (va & 0x8) va -= 16;
    long vb = (long)((mb >> sh) & 0xFul); if (vb & 0x8) vb -= 16;
    unsigned long wa = (va >= slo && va < shi) ? 1ul : 0ul;
    unsigned long wb = (vb >= slo && vb < shi) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm 4-bit field signed signum: DSIGN4N (completes field signum all widths dual) */
  if (kw(&L->cur,"DSIGN4N")||kw(&L->cur,"S2SIGN4N")||kw(&L->cur,"STACK2SIGN4N")||
      kw(&L->cur,"PAIRSIGN4N")||kw(&L->cur,"DSIGNUM4N")||kw(&L->cur,"DSGN4N")||
      kw(&L->cur,"DSIGN4IMM")){
    /* a b + n → nibble n of each = sgn(int4): +1 / 0 / 0xF; n 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul); if (va & 0x8) va -= 16;
    long vb = (long)((mb >> sh) & 0xFul); if (vb & 0x8) vb -= 16;
    unsigned long wa = (va > 0) ? 1ul : ((va < 0) ? 0xFul : 0ul);
    unsigned long wb = (vb > 0) ? 1ul : ((vb < 0) ? 0xFul : 0ul);
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm 4-bit field zero-eq: D0EQ4N (completes field zero-eq all widths dual) */
  if (kw(&L->cur,"D0EQ4N")||kw(&L->cur,"S20EQ4N")||kw(&L->cur,"STACK20EQ4N")||
      kw(&L->cur,"PAIR0EQ4N")||kw(&L->cur,"DZ4N")||kw(&L->cur,"DEQZ4N")||
      kw(&L->cur,"D0EQ4IMM")){
    /* a b + n → nibble n of each = (nib == 0) ? 1 : 0; n 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (va == 0) ? 1ul : 0ul;
    unsigned long wb = (vb == 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack imm 4-bit field nonzero: D0NE4N (completes field nonzero all widths dual) */
  if (kw(&L->cur,"D0NE4N")||kw(&L->cur,"S20NE4N")||kw(&L->cur,"STACK20NE4N")||
      kw(&L->cur,"PAIR0NE4N")||kw(&L->cur,"DNZ4N")||kw(&L->cur,"DNEZ4N")||
      kw(&L->cur,"D0NE4IMM")){
    /* a b + n → nibble n of each = (nib != 0) ? 1 : 0; n 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (va != 0) ? 1ul : 0ul;
    unsigned long wb = (vb != 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm 4-bit field signed LT0: D0LT4N (completes field signed LT0 all widths dual) */
  if (kw(&L->cur,"D0LT4N")||kw(&L->cur,"S20LT4N")||kw(&L->cur,"STACK20LT4N")||
      kw(&L->cur,"PAIR0LT4N")||kw(&L->cur,"DLT04N")||kw(&L->cur,"DNEGZ4N")||
      kw(&L->cur,"D0LT4IMM")){
    /* a b + n → nibble n of each = (int4(nib) < 0) ? 1 : 0; n 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul); if (va & 0x8) va -= 16;
    long vb = (long)((mb >> sh) & 0xFul); if (vb & 0x8) vb -= 16;
    unsigned long wa = (va < 0) ? 1ul : 0ul;
    unsigned long wb = (vb < 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack imm 4-bit field signed GT0: D0GT4N (completes field signed GT0 all widths dual) */
  if (kw(&L->cur,"D0GT4N")||kw(&L->cur,"S20GT4N")||kw(&L->cur,"STACK20GT4N")||
      kw(&L->cur,"PAIR0GT4N")||kw(&L->cur,"DGT04N")||kw(&L->cur,"DPOSZ4N")||
      kw(&L->cur,"D0GT4IMM")){
    /* a b + n → nibble n of each = (int4(nib) > 0) ? 1 : 0; n 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul); if (va & 0x8) va -= 16;
    long vb = (long)((mb >> sh) & 0xFul); if (vb & 0x8) vb -= 16;
    unsigned long wa = (va > 0) ? 1ul : 0ul;
    unsigned long wb = (vb > 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack imm 4-bit field signed LE0: D0LE4N (completes field signed LE0 all widths dual) */
  if (kw(&L->cur,"D0LE4N")||kw(&L->cur,"S20LE4N")||kw(&L->cur,"STACK20LE4N")||
      kw(&L->cur,"PAIR0LE4N")||kw(&L->cur,"DLE04N")||kw(&L->cur,"DNONPOS4N")||
      kw(&L->cur,"D0LE4IMM")){
    /* a b + n → nibble n of each = (int4(nib) <= 0) ? 1 : 0; n 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul); if (va & 0x8) va -= 16;
    long vb = (long)((mb >> sh) & 0xFul); if (vb & 0x8) vb -= 16;
    unsigned long wa = (va <= 0) ? 1ul : 0ul;
    unsigned long wb = (vb <= 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack imm 4-bit field signed GE0: D0GE4N (completes field signed GE0 all widths dual) */
  if (kw(&L->cur,"D0GE4N")||kw(&L->cur,"S20GE4N")||kw(&L->cur,"STACK20GE4N")||
      kw(&L->cur,"PAIR0GE4N")||kw(&L->cur,"DGE04N")||kw(&L->cur,"DNONNEG4N")||
      kw(&L->cur,"D0GE4IMM")){
    /* a b + n → nibble n of each = (int4(nib) >= 0) ? 1 : 0; n 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul); if (va & 0x8) va -= 16;
    long vb = (long)((mb >> sh) & 0xFul); if (vb & 0x8) vb -= 16;
    unsigned long wa = (va >= 0) ? 1ul : 0ul;
    unsigned long wb = (vb >= 0) ? 1ul : 0ul;
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
  /* digit-2 dual-stack imm 4-bit field shift: DSHL4N · DSHR4N · DSAR4N
   * (dual of SSHL4N/SSHR4N/SSAR4N; nibble-field shift plane on top two cells after DNOT4N/DROL4N) */
  if (kw(&L->cur,"DSHL4N")||kw(&L->cur,"S2SHL4N")||kw(&L->cur,"STACK2SHL4N")||
      kw(&L->cur,"PAIRSHL4N")||kw(&L->cur,"DLSH4N")||kw(&L->cur,"DSHLNIBN")||
      kw(&L->cur,"DSHIFTNIBL")){
    /* a b + k n → nibble n of each = (uint4)<<k (k>=4 → 0); n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (kk >= 4) ? 0ul : ((va << (unsigned)kk) & 0xFul);
    unsigned long wb = (kk >= 4) ? 0ul : ((vb << (unsigned)kk) & 0xFul);
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSHR4N")||kw(&L->cur,"S2SHR4N")||kw(&L->cur,"STACK2SHR4N")||
      kw(&L->cur,"PAIRSHR4N")||kw(&L->cur,"DLSHR4N")||kw(&L->cur,"DSHRNIBN")||
      kw(&L->cur,"DSHIFTNIBR")){
    /* a b + k n → nibble n of each = (uint4)>>k logical (k>=4 → 0); n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (kk >= 4) ? 0ul : (va >> (unsigned)kk);
    unsigned long wb = (kk >= 4) ? 0ul : (vb >> (unsigned)kk);
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSAR4N")||kw(&L->cur,"S2SAR4N")||kw(&L->cur,"STACK2SAR4N")||
      kw(&L->cur,"PAIRSAR4N")||kw(&L->cur,"DASHR4N")||kw(&L->cur,"DSARNIBN")||
      kw(&L->cur,"DSARSHIFTNIB")){
    /* a b + k n → nibble n of each = (int4)>>k arithmetic (k>=4 → all sign); n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul);
    long vb = (long)((mb >> sh) & 0xFul);
    if (va & 0x8) va -= 16;
    if (vb & 0x8) vb -= 16;
    long sa, sb;
    if (kk >= 4){ sa = (va < 0) ? -1L : 0L; sb = (vb < 0) ? -1L : 0L; }
    else { sa = va >> kk; sb = vb >> kk; }
    unsigned long wa = (unsigned long)sa & 0xFul;
    unsigned long wb = (unsigned long)sb & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack imm 4-bit field metrics: DBITREV4N · DPOPCNT4N · DPARITY4N
   * (dual of SBITREV4N/SPOPCNT4N/SPARITY4N; nibble metrics plane on top two after DSHL4N) */
  if (kw(&L->cur,"DBITREV4N")||kw(&L->cur,"S2BITREV4N")||kw(&L->cur,"STACK2BITREV4N")||
      kw(&L->cur,"PAIRBITREV4N")||kw(&L->cur,"DBREV4N")||kw(&L->cur,"DREV4N")||
      kw(&L->cur,"DBITREVNIBN")){
    /* a b + n → nibble n of each = bitrev4(nibble); n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long ra = 0, rb = 0, ta = va, tb = vb;
    for (int b = 0; b < 4; b++){
      ra = (ra << 1) | (ta & 1u); ta >>= 1;
      rb = (rb << 1) | (tb & 1u); tb >>= 1;
    }
    long x = (long)((ma & ~(0xFul << sh)) | (ra << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (rb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPOPCNT4N")||kw(&L->cur,"S2POPCNT4N")||kw(&L->cur,"STACK2POPCNT4N")||
      kw(&L->cur,"PAIRPOPCNT4N")||kw(&L->cur,"DPCNT4N")||kw(&L->cur,"DPOPNIBN")||
      kw(&L->cur,"DNIBPOPN")){
    /* a b + n → nibble n of each = popcount(nibble); n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long pa = 0, pb = 0;
    while (va){ pa += (va & 1u); va >>= 1; }
    while (vb){ pb += (vb & 1u); vb >>= 1; }
    long x = (long)((ma & ~(0xFul << sh)) | (pa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (pb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DPARITY4N")||kw(&L->cur,"S2PARITY4N")||kw(&L->cur,"STACK2PARITY4N")||
      kw(&L->cur,"PAIRPARITY4N")||kw(&L->cur,"DXORRED4N")||kw(&L->cur,"DPARITYNIBN")||
      kw(&L->cur,"DNIBPARN")){
    /* a b + n → nibble n of each = xor-reduce(nibble) in low bit; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long pra = 0, prb = 0;
    while (va){ pra ^= (va & 1u); va >>= 1; }
    while (vb){ prb ^= (vb & 1u); vb >>= 1; }
    long x = (long)((ma & ~(0xFul << sh)) | (pra << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (prb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 dual-stack imm 4-bit field zeros+signed: DCLZ4N · DCTZ4N · DNEG4N
   * (dual of SCLZ4N/SCTZ4N/SNEG4N; nibble zeros+negate plane on top two after DPARITY4N) */
  if (kw(&L->cur,"DCLZ4N")||kw(&L->cur,"S2CLZ4N")||kw(&L->cur,"STACK2CLZ4N")||
      kw(&L->cur,"PAIRCLZ4N")||kw(&L->cur,"DNLZ4N")||kw(&L->cur,"DCLZNIBN")||
      kw(&L->cur,"DNIBCLZN")){
    /* a b + n → nibble n of each = clz4(nibble); 0 → 4; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned int wa = (unsigned int)((ma >> sh) & 0xFul);
    unsigned int wb = (unsigned int)((mb >> sh) & 0xFul);
    unsigned long cza = 0, czb = 0;
    if (wa == 0) cza = 4;
    else { for (int b = 3; b >= 0; b--){ if (wa & (1u << (unsigned)b)) break; cza++; } }
    if (wb == 0) czb = 4;
    else { for (int b = 3; b >= 0; b--){ if (wb & (1u << (unsigned)b)) break; czb++; } }
    long x = (long)((ma & ~(0xFul << sh)) | (cza << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (czb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DCTZ4N")||kw(&L->cur,"S2CTZ4N")||kw(&L->cur,"STACK2CTZ4N")||
      kw(&L->cur,"PAIRCTZ4N")||kw(&L->cur,"DNTZ4N")||kw(&L->cur,"DCTZNIBN")||
      kw(&L->cur,"DNIBCTZN")){
    /* a b + n → nibble n of each = ctz4(nibble); 0 → 4; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned int wa = (unsigned int)((ma >> sh) & 0xFul);
    unsigned int wb = (unsigned int)((mb >> sh) & 0xFul);
    unsigned long tza = 0, tzb = 0;
    if (wa == 0) tza = 4;
    else { while ((wa & 1u) == 0){ tza++; wa >>= 1; } }
    if (wb == 0) tzb = 4;
    else { while ((wb & 1u) == 0){ tzb++; wb >>= 1; } }
    long x = (long)((ma & ~(0xFul << sh)) | (tza << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (tzb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNEG4N")||kw(&L->cur,"S2NEG4N")||kw(&L->cur,"STACK2NEG4N")||
      kw(&L->cur,"PAIRNEG4N")||kw(&L->cur,"DINEG4N")||kw(&L->cur,"DNEGNIBN")||
      kw(&L->cur,"DNIBNEGN")){
    /* a b + n → nibble n of each = -(int4) as uint4; min int4 -8 stays 0x8; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul);
    long vb = (long)((mb >> sh) & 0xFul);
    if (va & 0x8) va -= 16;
    if (vb & 0x8) vb -= 16;
    long na = (va == -8L) ? -8L : -va;
    long nb = (vb == -8L) ? -8L : -vb;
    unsigned long nva = (unsigned long)(na & 0xF);
    unsigned long nvb = (unsigned long)(nb & 0xF);
    long x = (long)((ma & ~(0xFul << sh)) | (nva << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (nvb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 dual-stack imm 4-bit field abs+extend: DABS4N · DSEXT4N · DZEXT4N
   * (dual of SABS4N/SSEXT4N/SZEXT4N; complete dual nibble unary plane after DNEG4N) */
  if (kw(&L->cur,"DABS4N")||kw(&L->cur,"S2ABS4N")||kw(&L->cur,"STACK2ABS4N")||
      kw(&L->cur,"PAIRABS4N")||kw(&L->cur,"DIABS4N")||kw(&L->cur,"DABSNIBN")||
      kw(&L->cur,"DNIBABSN")){
    /* a b + n → nibble n of each = abs(int4); min -8 → 0x8; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul);
    long vb = (long)((mb >> sh) & 0xFul);
    if (va & 0x8) va -= 16;
    if (vb & 0x8) vb -= 16;
    long ara = (va < 0) ? -va : va;
    long arb = (vb < 0) ? -vb : vb;
    unsigned long nva = (unsigned long)(ara & 0xF);
    unsigned long nvb = (unsigned long)(arb & 0xF);
    long x = (long)((ma & ~(0xFul << sh)) | (nva << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (nvb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSEXT4N")||kw(&L->cur,"S2SEXT4N")||kw(&L->cur,"STACK2SEXT4N")||
      kw(&L->cur,"PAIRSEXT4N")||kw(&L->cur,"DSIGNEXT4N")||kw(&L->cur,"DSEXTNIBN")||
      kw(&L->cur,"DNIBSEXTN")){
    /* a b + n → each cell = sign-extend nibble n to full width; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul);
    long vb = (long)((mb >> sh) & 0xFul);
    if (va & 0x8) va -= 16;
    if (vb & 0x8) vb -= 16;
    vm->stack[vm->sp - 2] = va;
    vm->stack[vm->sp - 1] = vb;
    var_set_num(vm,"LAST_N",vb); vm->last_n=vb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DZEXT4N")||kw(&L->cur,"S2ZEXT4N")||kw(&L->cur,"STACK2ZEXT4N")||
      kw(&L->cur,"PAIRZEXT4N")||kw(&L->cur,"DZEROEXT4N")||kw(&L->cur,"DZEXTNIBN")||
      kw(&L->cur,"DNIBZEXTN")){
    /* a b + n → each cell = zero-extend nibble n (low4 only); n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long xa = (long)((ma >> sh) & 0xFul);
    long yb = (long)((mb >> sh) & 0xFul);
    vm->stack[vm->sp - 2] = xa;
    vm->stack[vm->sp - 1] = yb;
    var_set_num(vm,"LAST_N",yb); vm->last_n=yb;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
