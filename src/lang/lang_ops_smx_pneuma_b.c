/* CubalC SMX cascade/soft plane — peeled from lang_ops_smx.
 * HOLD_FLASH discipline: mesh sticky only; meaningful forms preferred.
 * Pure dispatcher: glia · gliax · vaso · hepatic shards.
 */
#include "lang/cubalc_lang_ops_smx_pneuma_b.h"
#include "lang/cubalc_lang_ops_smx_pneuma_glia.h"
#include "lang/cubalc_lang_ops_smx_pneuma_gliax.h"
#include "lang/cubalc_lang_ops_smx_pneuma_vaso.h"
#include "lang/cubalc_lang_ops_smx_pneuma_hepatic.h"
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

int cubalc_lang_ops_smx_try_pneuma_b(VM *vm, Lex *L) {
  /* glia SCHWANN..MULLER → lang_ops_smx_pneuma_glia.c */
  {
    int gl = cubalc_lang_ops_smx_try_pneuma_glia(vm, L);
    if (gl != 0) return gl;
  }

  /* extended glia TANY..BASEMENT → lang_ops_smx_pneuma_gliax.c */
  {
    int gx = cubalc_lang_ops_smx_try_pneuma_gliax(vm, L);
    if (gx != 0) return gx;
  }

  /* vascular CAPILLARY..CAVA → lang_ops_smx_pneuma_vaso.c */
  {
    int vs = cubalc_lang_ops_smx_try_pneuma_vaso(vm, L);
    if (vs != 0) return vs;
  }

  /* hepatic PORTAL..CBD → lang_ops_smx_pneuma_hepatic.c (B_pneuma_b_peel) */
  {
    int hp = cubalc_lang_ops_smx_try_pneuma_hepatic(vm, L);
    if (hp != 0) return hp;
  }

  return 0;
}
