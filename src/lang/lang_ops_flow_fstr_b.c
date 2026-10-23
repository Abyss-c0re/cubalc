/* flow fstr peel from flow_class */
#include "lang/cubalc_lang_ops_flow_fstr_b.h"
#include "lang/cubalc_lang_ops_flow_fstr_context.h"
#include "lang/cubalc_lang_ops_flow_fstr_lifecycle.h"
#include "lang/cubalc_lang_ops_flow_fstr_info.h"
#include "lang/cubalc_lang_ops_flow_fstr_hasexist.h"
#include "lang/cubalc_lang_ops_flow_fstr_hasfield.h"
#include "lang/cubalc_lang_ops_flow_fstr_hasmeth.h"
#include "lang/cubalc_lang_ops_flow_fstr_kinship.h"
#include "lang/cubalc_lang_ops_flow_fstr_cousin.h"
#include "lang/cubalc_lang_ops_flow_fstr_dumpobj.h"
#include "lang/cubalc_lang_ops_flow_fstr_listobjs.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_b(VM *vm, Lex *L) {
  /* context OVERRIDE/ISCLASS/THIS → lang_ops_flow_fstr_context.c */
  {
    int cx = cubalc_lang_ops_flow_try_fstr_context(vm, L);
    if (cx != 0) return cx;
  }

  /* lifecycle DELETE/CLONE/RENAME → lang_ops_flow_fstr_lifecycle.c */
  {
    int lc = cubalc_lang_ops_flow_try_fstr_lifecycle(vm, L);
    if (lc != 0) return lc;
  }

  /* CLASSINFO/METHODINFO/FNINFO → lang_ops_flow_fstr_info.c */
  {
    int info = cubalc_lang_ops_flow_try_fstr_info(vm, L);
    if (info != 0) return info;
  }

  /* HASCLASS/HASOBJ/HASFN → lang_ops_flow_fstr_hasexist.c */
  {
    int he = cubalc_lang_ops_flow_try_fstr_hasexist(vm, L);
    if (he != 0) return he;
  }

  /* HASFIELD cluster → lang_ops_flow_fstr_hasfield.c (B_fstr_b_peel) */
  {
    int hf = cubalc_lang_ops_flow_try_fstr_hasfield(vm, L);
    if (hf != 0) return hf;
  }

  /* HASMETHOD cluster → lang_ops_flow_fstr_hasmeth.c (B_fstr_b_peel) */
  {
    int hm = cubalc_lang_ops_flow_try_fstr_hasmeth(vm, L);
    if (hm != 0) return hm;
  }

  /* kinship CHILDCOUNT..ARESIBLINGS → lang_ops_flow_fstr_kinship.c (B_fstr_b_peel) */
  {
    int kn = cubalc_lang_ops_flow_try_fstr_kinship(vm, L);
    if (kn != 0) return kn;
  }



  /* cousin COUSINCOUNT..NTHCOUSIN → lang_ops_flow_fstr_cousin.c (B_fstr_b_peel) */
  {
    int cz = cubalc_lang_ops_flow_try_fstr_cousin(vm, L);
    if (cz != 0) return cz;
  }






  /* LISTCLASSES/LISTOBJS/PICK/SHUFFLE → lang_ops_flow_fstr_listobjs.c (B_fstr_b_peel) */
  {
    int lo = cubalc_lang_ops_flow_try_fstr_listobjs(vm, L);
    if (lo != 0) return lo;
  }










  /* DUMPOBJ/OBJINFO → lang_ops_flow_fstr_dumpobj.c (B_fstr_b_peel) */
  {
    int dmp = cubalc_lang_ops_flow_try_fstr_dumpobj(vm, L);
    if (dmp != 0) return dmp;
  }

  return 0;
}
