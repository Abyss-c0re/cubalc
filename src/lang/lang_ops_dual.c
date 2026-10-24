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
#include "lang/cubalc_lang_ops_dual_bitmet.h"
#include "lang/cubalc_lang_ops_dual_datapath.h"
#include "lang/cubalc_lang_ops_dual_imm_field.h"
#include "lang/cubalc_lang_ops_dual_imm_wbit.h"
#include "lang/cubalc_lang_ops_dual_imm_hbit.h"
#include "lang/cubalc_lang_ops_dual_imm_harith.h"
#include "lang/cubalc_lang_ops_dual_imm_hdiv.h"
#include "lang/cubalc_lang_ops_dual_imm_hcmp.h"
#include "lang/cubalc_lang_ops_dual_imm_hord.h"
#include "lang/cubalc_lang_ops_dual_imm_hsord.h"
#include "lang/cubalc_lang_ops_dual_imm_hsmm.h"
#include "lang/cubalc_lang_ops_dual_imm_hsclamp.h"
#include "lang/cubalc_lang_ops_dual_imm_hpred.h"
#include "lang/cubalc_lang_ops_dual_imm_h0ord.h"
#include "lang/cubalc_lang_ops_dual_imm_barith.h"
#include "lang/cubalc_lang_ops_dual_imm_bdiv.h"
#include "lang/cubalc_lang_ops_dual_imm_bcmp.h"
#include "lang/cubalc_lang_ops_dual_imm_bord.h"
#include "lang/cubalc_lang_ops_dual_imm_bsord.h"
#include "lang/cubalc_lang_ops_dual_imm_bsmm.h"
#include "lang/cubalc_lang_ops_dual_imm_bsclamp.h"
#include "lang/cubalc_lang_ops_dual_imm_bpred.h"
#include "lang/cubalc_lang_ops_dual_imm_b0ord.h"
#include "lang/cubalc_lang_ops_dual_imm_bbit.h"
#include "lang/cubalc_lang_ops_dual_imm_narith.h"
#include "lang/cubalc_lang_ops_dual_imm_ndiv.h"
#include "lang/cubalc_lang_ops_dual_imm_ncmp.h"
#include "lang/cubalc_lang_ops_dual_imm_nord.h"
#include "lang/cubalc_lang_ops_dual_imm_nsord.h"
#include "lang/cubalc_lang_ops_dual_imm_nsmm.h"

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
  /* bit metrics DFFS…DISPOW2 → lang_ops_dual_bitmet.c (B_dual_peel) */
  {
    int bm = cubalc_lang_ops_dual_try_bitmet(vm, L);
    if (bm != 0) return bm;
  }
  /* DFFS·DFLS·DBWIDTH·DBHSI·DCEILPOW2·DCLO·DCTO·DISPOW2 → dual_bitmet peel */
  /* datapath DCLIP4…DZEXT16 → lang_ops_dual_datapath.c (B_dual_peel) */
  {
    int dp = cubalc_lang_ops_dual_try_datapath(vm, L);
    if (dp != 0) return dp;
  }
  /* DCLIP4·DSEXT4·DZEXT4·DCLIP8·DCLIP16·DSEXT8·DSEXT16·DZEXT8·DZEXT16 → dual_datapath peel */
  /* imm field DBYTEN…DCLR32N → lang_ops_dual_imm_field.c (B_dual_peel) */
  {
    int ifd = cubalc_lang_ops_dual_try_imm_field(vm, L);
    if (ifd != 0) return ifd;
  }
  /* DBYTEN·DSETBYTEN·DCLRBYTEN·DNIBN·DWORDN·DGET32N·DSET32N·DCLR32N → dual_imm_field peel */
  /* imm 32-bit field bitwise DAND32N…DXNOR32N → lang_ops_dual_imm_wbit.c (B_dual_peel) */
  {
    int iw = cubalc_lang_ops_dual_try_imm_wbit(vm, L);
    if (iw != 0) return iw;
  }
  /* DAND32N·DOR32N·DXOR32N·DNAND32N·DNOR32N·DXNOR32N → dual_imm_wbit peel */
  /* imm 16-bit field bitwise DAND16N…DXNOR16N → lang_ops_dual_imm_hbit.c (B_dual_peel) */
  {
    int ih = cubalc_lang_ops_dual_try_imm_hbit(vm, L);
    if (ih != 0) return ih;
  }
  /* DAND16N·DOR16N·DXOR16N·DNAND16N·DNOR16N·DXNOR16N → dual_imm_hbit peel */
  /* imm 16-bit field arith DADD16N·DSUB16N·DMUL16N → lang_ops_dual_imm_harith.c (B_dual_peel) */
  {
    int ihar = cubalc_lang_ops_dual_try_imm_harith(vm, L);
    if (ihar != 0) return ihar;
  }
  /* DADD16N·DSUB16N·DMUL16N → dual_imm_harith peel */
  /* imm 16-bit field div/mod/min DDIV16N·DMOD16N·DMIN16N → lang_ops_dual_imm_hdiv.c (B_dual_peel) */
  {
    int ihd = cubalc_lang_ops_dual_try_imm_hdiv(vm, L);
    if (ihd != 0) return ihd;
  }
  /* DDIV16N·DMOD16N·DMIN16N → dual_imm_hdiv peel */
  /* imm 16-bit field max/eq/ne DMAX16N·DEQ16N·DNE16N → lang_ops_dual_imm_hcmp.c (B_dual_peel) */
  {
    int ihc = cubalc_lang_ops_dual_try_imm_hcmp(vm, L);
    if (ihc != 0) return ihc;
  }
  /* DMAX16N·DEQ16N·DNE16N → dual_imm_hcmp peel */
  /* imm 16-bit field ordered-cmp DLT16N·DGT16N·DLTE16N → lang_ops_dual_imm_hord.c (B_dual_peel) */
  {
    int iho = cubalc_lang_ops_dual_try_imm_hord(vm, L);
    if (iho != 0) return iho;
  }
  /* DLT16N·DGT16N·DLTE16N·DGTE16N → dual_imm_hord peel */
  /* DGTE16N → dual_imm_hord peel (complete unsigned ordered-cmp) */
  /* imm 16-bit signed ordered-cmp DLTS16N·DGTS16N·DLTES16N·DGTES16N → dual_imm_hsord (B_dual_peel) */
  {
    int ihs = cubalc_lang_ops_dual_try_imm_hsord(vm, L);
    if (ihs != 0) return ihs;
  }
  /* DLTS16N·DGTS16N·DLTES16N·DGTES16N → dual_imm_hsord peel */
  /* imm 16-bit signed min/max DMINS16N·DMAXS16N → dual_imm_hsmm (B_dual_peel) */
  {
    int ihm = cubalc_lang_ops_dual_try_imm_hsmm(vm, L);
    if (ihm != 0) return ihm;
  }
  /* DMINS16N·DMAXS16N → dual_imm_hsmm peel */
  /* imm 16-bit signed clamp/between DCLAMPS16N·DBETWEENS16N → dual_imm_hsclamp (B_dual_peel) */
  {
    int ihc2 = cubalc_lang_ops_dual_try_imm_hsclamp(vm, L);
    if (ihc2 != 0) return ihc2;
  }
  /* DCLAMPS16N·DBETWEENS16N·DWITHINS16N → dual_imm_hsclamp peel */
  /* DWITHINS16N → dual_imm_hsclamp peel (with DCLAMPS/DBETWEENS) */
  /* imm 16-bit sign/zero preds DSIGN16N·D0EQ16N·D0NE16N → dual_imm_hpred (B_dual_peel) */
  {
    int ihp = cubalc_lang_ops_dual_try_imm_hpred(vm, L);
    if (ihp != 0) return ihp;
  }
  /* DSIGN16N·D0EQ16N·D0NE16N → dual_imm_hpred peel */
  /* imm 16-bit signed zero-ord D0LT16N·D0GT16N·D0LE16N·D0GE16N → dual_imm_h0ord (B_dual_peel) */
  {
    int ih0 = cubalc_lang_ops_dual_try_imm_h0ord(vm, L);
    if (ih0 != 0) return ih0;
  }
  /* D0LT16N·D0GT16N·D0LE16N·D0GE16N → dual_imm_h0ord peel */
  /* imm 8-bit field arith DADD8N·DSUB8N·DMUL8N → dual_imm_barith (B_dual_peel) */
  {
    int iba = cubalc_lang_ops_dual_try_imm_barith(vm, L);
    if (iba != 0) return iba;
  }
  /* DADD8N·DSUB8N·DMUL8N → dual_imm_barith peel */
  /* imm 8-bit field div/mod/min DDIV8N·DMOD8N·DMIN8N → dual_imm_bdiv (B_dual_peel) */
  {
    int ibd = cubalc_lang_ops_dual_try_imm_bdiv(vm, L);
    if (ibd != 0) return ibd;
  }
  /* DDIV8N·DMOD8N·DMIN8N → dual_imm_bdiv peel */
  /* imm 8-bit field max/eq/ne DMAX8N·DEQ8N·DNE8N → dual_imm_bcmp (B_dual_peel) */
  {
    int ibc = cubalc_lang_ops_dual_try_imm_bcmp(vm, L);
    if (ibc != 0) return ibc;
  }
  /* DMAX8N·DEQ8N·DNE8N → dual_imm_bcmp peel */
  /* imm 8-bit field ordered-cmp DLT8N·DGT8N·DLTE8N·DGTE8N → dual_imm_bord (B_dual_peel) */
  {
    int ibo = cubalc_lang_ops_dual_try_imm_bord(vm, L);
    if (ibo != 0) return ibo;
  }
  /* DLT8N·DGT8N·DLTE8N·DGTE8N → dual_imm_bord peel */
  /* imm 8-bit signed ordered-cmp DLTS8N·DGTS8N·DLTES8N·DGTES8N → dual_imm_bsord (B_dual_peel) */
  {
    int ibs = cubalc_lang_ops_dual_try_imm_bsord(vm, L);
    if (ibs != 0) return ibs;
  }
  /* DLTS8N·DGTS8N·DLTES8N·DGTES8N → dual_imm_bsord peel */
  /* imm 8-bit signed min/max DMINS8N·DMAXS8N → dual_imm_bsmm (B_dual_peel) */
  {
    int ibm = cubalc_lang_ops_dual_try_imm_bsmm(vm, L);
    if (ibm != 0) return ibm;
  }
  /* DMINS8N·DMAXS8N → dual_imm_bsmm peel */
  /* imm 8-bit signed clamp/between DCLAMPS8N·DBETWEENS8N → dual_imm_bsclamp (B_dual_peel) */
  {
    int ibc = cubalc_lang_ops_dual_try_imm_bsclamp(vm, L);
    if (ibc != 0) return ibc;
  }
  /* DCLAMPS8N·DBETWEENS8N·DWITHINS8N → dual_imm_bsclamp peel */
  /* imm 8-bit sign/zero preds DSIGN8N·D0EQ8N·D0NE8N → dual_imm_bpred (B_dual_peel) */
  {
    int ibp = cubalc_lang_ops_dual_try_imm_bpred(vm, L);
    if (ibp != 0) return ibp;
  }
  /* DSIGN8N·D0EQ8N·D0NE8N → dual_imm_bpred peel */
  /* imm 8-bit signed zero-ord D0LT8N·D0GT8N·D0LE8N·D0GE8N → dual_imm_b0ord (B_dual_peel) */
  {
    int ib0 = cubalc_lang_ops_dual_try_imm_b0ord(vm, L);
    if (ib0 != 0) return ib0;
  }
  /* D0LT8N·D0GT8N·D0LE8N·D0GE8N → dual_imm_b0ord peel */
  /* imm 8-bit field bitwise DAND8N…DXNOR8N → lang_ops_dual_imm_bbit.c (B_dual_peel) */
  {
    int ibb = cubalc_lang_ops_dual_try_imm_bbit(vm, L);
    if (ibb != 0) return ibb;
  }
  /* DAND8N·DOR8N·DXOR8N·DNAND8N·DNOR8N·DXNOR8N → dual_imm_bbit peel */
  /* imm 4-bit field arith DADD4N·DSUB4N·DMUL4N → dual_imm_narith (B_dual_peel) */
  {
    int ina = cubalc_lang_ops_dual_try_imm_narith(vm, L);
    if (ina != 0) return ina;
  }
  /* DADD4N·DSUB4N·DMUL4N → dual_imm_narith peel */
  /* imm 4-bit field div/mod/min DDIV4N·DMOD4N·DMIN4N → dual_imm_ndiv (B_dual_peel) */
  {
    int ind = cubalc_lang_ops_dual_try_imm_ndiv(vm, L);
    if (ind != 0) return ind;
  }
  /* DDIV4N·DMOD4N·DMIN4N → dual_imm_ndiv peel */
  /* imm 4-bit field max/eq/ne DMAX4N·DEQ4N·DNE4N → dual_imm_ncmp (B_dual_peel) */
  {
    int inc = cubalc_lang_ops_dual_try_imm_ncmp(vm, L);
    if (inc != 0) return inc;
  }
  /* DMAX4N·DEQ4N·DNE4N → dual_imm_ncmp peel */
  /* imm 4-bit field ordered-cmp DLT4N·DGT4N·DLTE4N·DGTE4N → dual_imm_nord (B_dual_peel) */
  {
    int ino = cubalc_lang_ops_dual_try_imm_nord(vm, L);
    if (ino != 0) return ino;
  }
  /* DLT4N·DGT4N·DLTE4N·DGTE4N → dual_imm_nord peel */
  /* imm 4-bit signed ordered-cmp DLTS4N·DGTS4N·DLTES4N·DGTES4N → dual_imm_nsord (B_dual_peel) */
  {
    int ins = cubalc_lang_ops_dual_try_imm_nsord(vm, L);
    if (ins != 0) return ins;
  }
  /* DLTS4N·DGTS4N·DLTES4N·DGTES4N → dual_imm_nsord peel */
  /* imm 4-bit signed min/max DMINS4N·DMAXS4N → dual_imm_nsmm (B_dual_peel) */
  {
    int inm = cubalc_lang_ops_dual_try_imm_nsmm(vm, L);
    if (inm != 0) return inm;
  }
  /* DMINS4N·DMAXS4N → dual_imm_nsmm peel */
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
