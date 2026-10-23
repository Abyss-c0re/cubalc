/* CubalC lang — lang_ops_dual.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#include "lang/cubalc_lang_ops_dual_pack.h"
#include "lang/cubalc_lang_ops_dual_clip.h"
#include "lang/cubalc_lang_ops_dual_field32.h"
#include "lang/cubalc_lang_ops_dual_unary32.h"
#include "lang/cubalc_lang_ops_dual_unary16.h"
#include "lang/cubalc_lang_ops_dual_unary8.h"
#include "lang/cubalc_lang_ops_dual_unary4.h"
#include "lang/cubalc_lang_ops_dual_pair.h"
#include "lang/cubalc_lang_ops_dual_fieldrot.h"
#include "lang/cubalc_lang_ops_dual_immshift.h"
#include "lang/cubalc_lang_ops_dual_imm_alu.h"
#include "lang/cubalc_lang_ops_dual_imm_sat.h"
#include "lang/cubalc_lang_ops_dual_imm_div.h"
#include "lang/cubalc_lang_ops_dual_imm_bit.h"
#include "lang/cubalc_lang_ops_dual_imm_range.h"
#include "lang/cubalc_lang_ops_dual_imm_cmp.h"
#include "lang/cubalc_lang_ops_dual_bitpath.h"

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
  /* imm-4 unary/shift/metrics peel (DNOT4N…DZEXT4N) */
  {
    int u4 = cubalc_lang_ops_dual_try_unary4(vm, L);
    if (u4 != 0) return u4;
  }
  /* pair ALU DADD…DMAX → lang_ops_dual_pair.c (B_dual_peel) */
  {
    int pa = cubalc_lang_ops_dual_try_pair(vm, L);
    if (pa != 0) return pa;
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
  /* DNAND·DNOR·DXNOR·DANDN → dual_clip peel (G_dual_margin) */
  /* DPOPCNT·DCLZ·DCTZ → dual_clip peel (G_dual_margin) */
  /* DORN·DBREV·DPARITY → dual_clip peel (G_dual_margin) */
  /* DBLS·DBLC·DMASK → dual_clip peel (G_dual_margin) */
  /* DBTEST·DSETB·DCLRB·DFLIPB → dual_clip peel (G_dual_margin) */
  /* DSETBN·DBTESTN·DCLRBN (+aliases) → dual_clip peel (G_dual_margin) */
  /* DFLIPBN·DMASKN·DANDMN·DBEXTN → dual_clip peel (G_dual_margin) */
  /* DBDEPN·DORMN·DLOWORN·DMASKOR (+aliases) → dual_clip peel (G_dual_margin) */
  /* DNANDMN·DNORMN·DXNORMN·DLOWNANDN (+aliases) → dual_clip peel (G_dual_margin) */
  /* DHMASKN·DHIGHANDN·DCLEARLN (+aliases) → dual_clip peel (G_dual_margin) */
  /* DORHN·DXORHN·DCLEARHN·DFLIPHN (+aliases) → dual_clip peel (G_dual_margin) */
  /* DNANDHN·DNORHN·DXNORHN (+aliases) → dual_clip peel (G_dual_margin) */
  /* DPOPMN·DANYMN·DALLMN (+aliases) → dual_clip peel (G_dual_margin) */
  /* DPOPHN·DANYHN·DALLHN (+aliases) → dual_clip peel (G_dual_margin) */
  /* field reverse/rotate DBREVN…DRORHN → lang_ops_dual_fieldrot.c (B_dual_peel) */
  {
    int fr = cubalc_lang_ops_dual_try_fieldrot(vm, L);
    if (fr != 0) return fr;
  }
  /* DBREVN·DROLBN·DRORBN·DBREVHN·DROLHN·DRORHN → dual_fieldrot peel (B_dual_peel) */
  /* imm rotate/shift DROLN…DSARN → lang_ops_dual_immshift.c (B_dual_peel) */
  {
    int ish = cubalc_lang_ops_dual_try_immshift(vm, L);
    if (ish != 0) return ish;
  }
  /* DROLN·DRORN·DSHLN·DSHRN·DSARN → dual_immshift peel (B_dual_peel) */
  /* imm ALU DADDN…DMULN → lang_ops_dual_imm_alu.c (B_dual_peel) */
  {
    int ia = cubalc_lang_ops_dual_try_imm_alu(vm, L);
    if (ia != 0) return ia;
  }
  /* DADDN·DSUBN·DMULN → dual_imm_alu peel (B_dual_peel) */
  /* imm sat DSATADDN…DSATDIVFROMN → lang_ops_dual_imm_sat.c (B_dual_peel) */
  {
    int isat = cubalc_lang_ops_dual_try_imm_sat(vm, L);
    if (isat != 0) return isat;
  }
  /* DSATADDN·DSATSUBN·DSATMULN·DSATDIVN·DSATSUBFROMN·DSATDIVFROMN → dual_imm_sat peel */
  /* imm div DDIVN…DUMODFROMN → lang_ops_dual_imm_div.c (B_dual_peel) */
  {
    int idv = cubalc_lang_ops_dual_try_imm_div(vm, L);
    if (idv != 0) return idv;
  }
  /* DDIVN·DMODN·DDIVCEILN·DDIVFLOORN·DSUBFROMN…DUMODFROMN → dual_imm_div peel */
  /* imm bitwise DANDI…DXNORNFROMN → lang_ops_dual_imm_bit.c (B_dual_peel) */
  {
    int ibit = cubalc_lang_ops_dual_try_imm_bit(vm, L);
    if (ibit != 0) return ibit;
  }
  /* DANDI·DORI·DXORI·DNANDI·DNORI·DXNORI·DANDNI…DXNORNFROMN → dual_imm_bit peel */
  /* imm range DMINN…DWITHINN → lang_ops_dual_imm_range.c (B_dual_peel) */
  {
    int ir = cubalc_lang_ops_dual_try_imm_range(vm, L);
    if (ir != 0) return ir;
  }
  /* DMINN·DMAXN·DCLAMPN·DBETWEENN·DWITHINN → dual_imm_range peel */
  /* imm compare DEQN…DGENN → lang_ops_dual_imm_cmp.c (B_dual_peel) */
  {
    int ic = cubalc_lang_ops_dual_try_imm_cmp(vm, L);
    if (ic != 0) return ic;
  }
  /* DEQN·DLTN·DGTN·DNEN·DLENN·DGENN → dual_imm_cmp peel */
  /* bitpath DPEXT…DBDEP → lang_ops_dual_bitpath.c (B_dual_peel) */
  {
    int bp = cubalc_lang_ops_dual_try_bitpath(vm, L);
    if (bp != 0) return bp;
  }
  /* DPEXT·DPDEP·DZIP·DUNZIP·DBEXT·DBDEP → dual_bitpath peel */
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
  return 0;
}
