/* CubalC SMX cascade/soft plane — peeled from lang_ops_smx.
 * HOLD_FLASH discipline: mesh sticky only; meaningful forms preferred.
 */
#include "lang/cubalc_lang_ops_smx_pneuma_a.h"
#include "lang/cubalc_lang_ops_smx_pneuma_sphere0.h"
#include "lang/cubalc_lang_ops_smx_pneuma_sphere1.h"
#include "lang/cubalc_lang_ops_smx_pneuma_sphere2.h"
#include "lang/cubalc_lang_ops_smx_pneuma_ward.h"
#include "lang/cubalc_lang_ops_smx_pneuma_neural.h"
#include "lang/cubalc_lang_ops_smx_core.h"
#include "lang/cubalc_lang_internal.h"
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <sys/socket.h>
#endif

int cubalc_lang_ops_smx_try_pneuma_a(VM *vm, Lex *L) {
  /* spheres PNEUMA..ATMOSPHERE → lang_ops_smx_pneuma_sphere0.c (B_pneuma_peel) */
  {
    int s0 = cubalc_lang_ops_smx_try_pneuma_sphere0(vm, L);
    if (s0 != 0) return s0;
  }

  /* spheres LITH..MESO → lang_ops_smx_pneuma_sphere1.c (B_pneuma_peel) */
  {
    int s1 = cubalc_lang_ops_smx_try_pneuma_sphere1(vm, L);
    if (s1 != 0) return s1;
  }

  /* cosmic INTERSTELLAR..MULTIVERSE → lang_ops_smx_pneuma_sphere2.c */
  {
    int s2 = cubalc_lang_ops_smx_try_pneuma_sphere2(vm, L);
    if (s2 != 0) return s2;
  }

  /* ward AUTOHEAL..KEEPER → lang_ops_smx_pneuma_ward.c */
  {
    int wd = cubalc_lang_ops_smx_try_pneuma_ward(vm, L);
    if (wd != 0) return wd;
  }


  /* neural THALAMUS..OLIGO → lang_ops_smx_pneuma_neural.c (B_pneuma_peel) */
  {
    int nr = cubalc_lang_ops_smx_try_pneuma_neural(vm, L);
    if (nr != 0) return nr;
  }






  return 0;
}
