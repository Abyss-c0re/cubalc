/* CubalC lang — lang_ops_flow.c (COP/flow · pure C · cube is SoT)
 * Modular A3 dispatcher: class/fn/case/for/ctrl/ifvar/ifcmp/ifbag peels + IF residual.
 * Mesh vote thrash recovery · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_internal.h"
#include "cubalc_sot.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_ops_flow_class.h"
#include "lang/cubalc_lang_ops_flow_fn.h"
#include "lang/cubalc_lang_ops_flow_case.h"
#include "lang/cubalc_lang_ops_flow_for.h"
#include "lang/cubalc_lang_ops_flow_ctrl.h"
#include "lang/cubalc_lang_ops_flow_ifvar.h"
#include "lang/cubalc_lang_ops_flow_ifcmp.h"
#include "lang/cubalc_lang_ops_flow_ifbag.h"
#include "lang/cubalc_lang_ops_flow_ifjson.h"
#include "lang/cubalc_lang_ops_flow_ifsize.h"
#include "lang/cubalc_lang_ops_flow_iffresh.h"
#include "lang/cubalc_lang_ops_flow_iflen.h"
#include "lang/cubalc_lang_ops_flow_ifnum.h"
#include "lang/cubalc_lang_ops_flow_ifcanread.h"
#include "lang/cubalc_lang_ops_flow_ifobj.h"
#include "lang/cubalc_lang_ops_flow_ifenv.h"
#include "lang/cubalc_lang_ops_flow_iffile.h"
#include "lang/cubalc_lang_ops_flow_iffn.h"
#include "lang/cubalc_lang_ops_flow_ifbetween.h"
#include "lang/cubalc_lang_ops_flow_iffield.h"
#include "lang/cubalc_lang_ops_flow_ifjsoneq.h"
#include "lang/cubalc_lang_ops_flow_ifzero.h"
#include "lang/cubalc_lang_ops_flow_iflines.h"
#include "lang/cubalc_lang_ops_flow_ifeqfile.h"
#include "lang/cubalc_lang_ops_flow_ifmtime.h"
#include "lang/cubalc_lang_ops_flow_ifflag.h"

int cubalc_lang_ops_flow(VM *vm, Lex *L){
  {
    int cr = cubalc_lang_ops_flow_try_class(vm, L);
    if (cr != 0) return cr;
  }
  {
    int fr = cubalc_lang_ops_flow_try_fn(vm, L);
    if (fr != 0) return fr;
  }
  {
    int cs = cubalc_lang_ops_flow_try_case(vm, L);
    if (cs != 0) return cs;
  }
  /* OUT SENSOR name [ON|OFF|expr] — networkless lattice SoT */
  if (kw(&L->cur,"OUT")){
    Lex save=*L;
    lex_next(L);
    if (kw(&L->cur,"SENSOR")||kw(&L->cur,"SENSE")||kw(&L->cur,"NAME")){
      char nm[48]; long on=1;
      lex_next(L);
      if (L->cur.kind!=TK_IDENT && L->cur.kind!=TK_STR){
        fail(vm,"OUT SENSOR name"); return -1;
      }
      snprintf(nm,sizeof nm,"%s",L->cur.text); lex_next(L);
      if (kw(&L->cur,"ON")||kw(&L->cur,"OFF")||L->cur.kind==TK_NUM||
          L->cur.kind==TK_IDENT){
        if (kw(&L->cur,"ON")){ on=1; lex_next(L); }
        else if (kw(&L->cur,"OFF")){ on=0; lex_next(L); }
        else on = parse_expr(vm,L)!=0;
      }
      cubalc_sot_out(nm, on?1:0);
      bump(vm); return 1;
    }
    *L=save;
  }
  {
    int fo = cubalc_lang_ops_flow_try_for(vm, L);
    if (fo != 0) return fo;
  }
  {
    int ct = cubalc_lang_ops_flow_try_ctrl(vm, L);
    if (ct != 0) return ct;
  }
  {
    int iv = cubalc_lang_ops_flow_try_ifvar(vm, L);
    if (iv != 0) return iv;
  }
  {
    int ic = cubalc_lang_ops_flow_try_ifcmp(vm, L);
    if (ic != 0) return ic;
  }
  {
    int ib = cubalc_lang_ops_flow_try_ifbag(vm, L);
    if (ib != 0) return ib;
  }
  /* IFBETWEEN peel → lang_ops_flow_ifbetween.c (A3 residual) */
  {
    int ibt = cubalc_lang_ops_flow_try_ifbetween(vm, L);
    if (ibt != 0) return ibt;
  }


  /* IFFN peel → lang_ops_flow_iffn.c (A3 residual) */
  {
    int ifn = cubalc_lang_ops_flow_try_iffn(vm, L);
    if (ifn != 0) return ifn;
  }

  /* IFOBJ peel → lang_ops_flow_ifobj.c (A3 residual) */
  {
    int iob = cubalc_lang_ops_flow_try_ifobj(vm, L);
    if (iob != 0) return iob;
  }

  /* IFFILE peel → lang_ops_flow_iffile.c (A3 residual) */
  {
    int iff = cubalc_lang_ops_flow_try_iffile(vm, L);
    if (iff != 0) return iff;
  }

  /* IFENV peel → lang_ops_flow_ifenv.c (A3 residual) */
  {
    int iev = cubalc_lang_ops_flow_try_ifenv(vm, L);
    if (iev != 0) return iev;
  }

  /* IFFIELD peel → lang_ops_flow_iffield.c (A3 residual) */
  {
    int ifd = cubalc_lang_ops_flow_try_iffield(vm, L);
    if (ifd != 0) return ifd;
  }

  /* IFFRESH peel → lang_ops_flow_iffresh.c (A3 residual) */
  {
    int ifr = cubalc_lang_ops_flow_try_iffresh(vm, L);
    if (ifr != 0) return ifr;
  }


  /* IFBIG/IFSIZE peel → lang_ops_flow_ifsize.c (A3 residual) */
  {
    int isz = cubalc_lang_ops_flow_try_ifsize(vm, L);
    if (isz != 0) return isz;
  }


  /* IFNEWER peel → lang_ops_flow_ifmtime.c (A3 residual) */
  {
    int imt = cubalc_lang_ops_flow_try_ifmtime(vm, L);
    if (imt != 0) return imt;
  }


  /* IFEQFILE peel → lang_ops_flow_ifeqfile.c (A3 residual) */
  {
    int ieq = cubalc_lang_ops_flow_try_ifeqfile(vm, L);
    if (ieq != 0) return ieq;
  }


  /* IFJSONHAS peel → lang_ops_flow_ifjson.c (A3_flow) */
  {
    int ij = cubalc_lang_ops_flow_try_ifjson(vm, L);
    if (ij != 0) return ij;
  }

  /* IFNUM peel → lang_ops_flow_ifnum.c (A3 residual) */
  {
    int inu = cubalc_lang_ops_flow_try_ifnum(vm, L);
    if (inu != 0) return inu;
  }

  /* IFZERO peel → lang_ops_flow_ifzero.c (A3 residual) */
  {
    int iz = cubalc_lang_ops_flow_try_ifzero(vm, L);
    if (iz != 0) return iz;
  }

  /* IFCANREAD peel → lang_ops_flow_ifcanread.c (A3 residual) */
  {
    int icr = cubalc_lang_ops_flow_try_ifcanread(vm, L);
    if (icr != 0) return icr;
  }

  /* IFJSONEQ peel → lang_ops_flow_ifjsoneq.c (A3 residual) */
  {
    int ije = cubalc_lang_ops_flow_try_ifjsoneq(vm, L);
    if (ije != 0) return ije;
  }

  /* IFLEN peel → lang_ops_flow_iflen.c (A3 residual) */
  {
    int iln = cubalc_lang_ops_flow_try_iflen(vm, L);
    if (iln != 0) return iln;
  }


  /* IFLINES peel → lang_ops_flow_iflines.c (A3 residual) */
  {
    int ils = cubalc_lang_ops_flow_try_iflines(vm, L);
    if (ils != 0) return ils;
  }



  /* IFHASFLAG peel → lang_ops_flow_ifflag.c (A3 residual) */
  {
    int iff = cubalc_lang_ops_flow_try_ifflag(vm, L);
    if (iff != 0) return iff;
  }


  if (kw(&L->cur,"IF")){
    lex_next(L);
    /* chain: IF c [THEN] ... ELIF c [THEN] ... ELSE ... END
     * THEN optional â agents write newline IF bodies without glue. */
    for(;;){
      int aln = L->cur.line;
      long cond=parse_expr(vm,L);
      skip_nl(L);
      if (kw(&L->cur,"THEN")){ lex_next(L); skip_nl(L); }
      /* if next is ELSE/ELIF/END with no body, still valid empty arm */
      if (L->cur.kind==TK_EOF){
        char ebuf[160];
        snprintf(ebuf, sizeof ebuf,
                 "IF without END line %d â IF cond [THEN] â¦ END", aln);
        fail(vm, ebuf); return -1;
      }
      Lex body_start=*L;
      int depth=1;
      while (L->cur.kind!=TK_EOF && depth>0){
        if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
          lex_next(L);
          if (kw(&L->cur,"IF")) lex_next(L);
          continue;
        }
        if (kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||kw(&L->cur,"WHILE")||
            kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")||
            kw(&L->cur,"UNTIL")||kw(&L->cur,"TIMES")) depth++;
        else if ((kw(&L->cur,"ELSE")||kw(&L->cur,"ELIF")||kw(&L->cur,"ELSEIF")) && depth==1) break;
        else if (kw(&L->cur,"END")){ depth--; if (depth==0) break; }
        lex_next(L);
      }
      if (depth>1){
        char ebuf[160];
        snprintf(ebuf, sizeof ebuf,
                 "IF without END line %d â close with END (or ELSE/ELIF)", aln);
        fail(vm, ebuf); return -1;
      }
      if (cond){
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END","ELSE")<0) return -1;
        /* also stop at ELIF */
        /* skip to final END */
        depth=1;
        while (L->cur.kind!=TK_EOF && depth>0){
          if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
            lex_next(L);
            if (kw(&L->cur,"IF")) lex_next(L);
            continue;
          }
          if (kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||kw(&L->cur,"WHILE")||
              kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")||
              kw(&L->cur,"UNTIL")||kw(&L->cur,"TIMES")) depth++;
          else if (kw(&L->cur,"END")){ depth--; if(depth==0) break; }
          lex_next(L);
        }
        if (kw(&L->cur,"END")) lex_next(L);
        bump(vm); return 1;
      }
      /* not taken */
      if (kw(&L->cur,"ELIF")||kw(&L->cur,"ELSEIF")){ lex_next(L); continue; }
      if (kw(&L->cur,"ELSE")){
        lex_next(L); skip_nl(L);
        Lex body=*L;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        /* advance outer L from ELSE-body start to matching END (body is a copy) */
        depth=1;
        while (L->cur.kind!=TK_EOF && depth>0){
          if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
            lex_next(L);
            if (kw(&L->cur,"IF")) lex_next(L);
            continue;
          }
          if (kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||kw(&L->cur,"WHILE")||
              kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")||
              kw(&L->cur,"UNTIL")||kw(&L->cur,"TIMES")||kw(&L->cur,"UNLESS")||kw(&L->cur,"CASE"))
            depth++;
          else if (kw(&L->cur,"END")){ depth--; if(depth==0) break; }
          lex_next(L);
        }
        if (kw(&L->cur,"END")) lex_next(L);
        bump(vm); return 1;
      }
      if (kw(&L->cur,"END")){ lex_next(L); bump(vm); return 1; }
      {
        char ebuf[160];
        snprintf(ebuf, sizeof ebuf,
                 "IF chain broken line %d â expected ELSE/ELIF/END", aln);
        fail(vm, ebuf); return -1;
      }
    }
  }
  if (kw(&L->cur,"END")||kw(&L->cur,"ELSE")||kw(&L->cur,"ELIF")||kw(&L->cur,"ELSEIF")||kw(&L->cur,"THEN")||
      kw(&L->cur,"FINALLY")||kw(&L->cur,"CLEANUP")||kw(&L->cur,"ALWAYS")||kw(&L->cur,"AFTERALL")||
      kw(&L->cur,"CATCH")||kw(&L->cur,"ONERR")||kw(&L->cur,"ON_ERR")||kw(&L->cur,"EXCEPT")||
      kw(&L->cur,"RECOVER")){
    return 0; /* stop marker for nested bodies */
  }

  /* Unknown here: return 0 so parse_form can emit did-you-mean + HELP tip. */
  return 0;
}
