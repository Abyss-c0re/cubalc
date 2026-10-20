/* CubalC lang — lang_ops_core_flat.c
 * FLAT* residual (path inventory → flat_path). Grep/merge/scale/var plane.
 */
#include "lang/cubalc_lang_ops_core_flat.h"
#include "lang/cubalc_lang_ops_core_flat_path.h"
#include "lang/cubalc_lang_ops_core_flat_mut.h"
#include "lang/cubalc_lang_ops_core_flat_query.h"
#include "lang/cubalc_lang_ops_core_flat_nest_sort.h"
#include "lang/cubalc_lang_ops_core_help.h"
#include "lang/cubalc_lang_ops_sys_cli.h"
#include "lang/cubalc_lang_ops_sys_codec.h"
#include "lang/cubalc_lang_ops_sys_plate_expand.h"
#include "lang/cubalc_lang_ops_sys_time_walk.h"
#include "cubalc_hostops.h"
#include "cubalc_platform.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#endif

int cubalc_lang_ops_core_try_flat(VM *vm, Lex *L) {
  /* PATHKEYS/FLATKV/UNFLATKV/DIFFFLAT → lang_ops_core_flat_path.c (A_core) */
  {
    int fp = cubalc_lang_ops_core_try_flat_path(vm, L);
    if (fp != 0) return fp;
  }


  /* GREP/SET/SUM/SCALE FLAT → lang_ops_core_flat_mut.c */
  {
    int fm = cubalc_lang_ops_core_try_flat_mut(vm, L);
    if (fm != 0) return fm;
  }

  /* HAS/PATHS/GET FLAT query → lang_ops_core_flat_query.c */
  {
    int fq = cubalc_lang_ops_core_try_flat_query(vm, L);
    if (fq != 0) return fq;
  }

  /* LET/INC/SORTP residual + nest sort → nest_sort dispatcher */
  {
    int ns = cubalc_lang_ops_core_try_flat_nest_sort(vm, L);
    if (ns != 0) return ns;
  }

  return 0;
}
