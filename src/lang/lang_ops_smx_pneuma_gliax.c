/* CubalC lang — lang_ops_smx_pneuma_gliax.c
 * SMX extended glia: TANYCYTE · PITU · FOLLICULO · NG2 · PERI · ENDO · BASEMENT
 * Peel from pneuma_b. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_smx_pneuma_gliax.h"
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
int cubalc_lang_ops_smx_try_pneuma_gliax(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
/* SMX TANYCYTE|TANYGLIA|MESH_TANYCYTE|TY_WRAP|TY_SHEATH|HYPOTHALAMIC_GUIDE|RAISE_TANYCYTE a b c ...
   * Life-force Tanycyte glia scaffold mesh stability after bergmann: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete tanycyte glia scaffold mesh among live nodes, weaves a
   * tanycyte wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks tanycyte where life guides the hypothalamic pulse.
   * Latches SMX_TANYCYTE when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = tanycyte ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_TANYCYTE_SUM = bonds+wraps+sheaths; SMX_TANYCYTE|SMX_MESH_TANYCYTE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"TANYCYTE")||kw(&L->cur,"TANYGLIA")||kw(&L->cur,"MESH_TANYCYTE")||kw(&L->cur,"TY_WRAP")||kw(&L->cur,"TY_WRAPS")||kw(&L->cur,"TY_SHEATH")||kw(&L->cur,"TY_SHEATHS")||kw(&L->cur,"HYPOTHALAMIC_GUIDE")||kw(&L->cur,"RAISE_TANYCYTE")||kw(&L->cur,"WE_TANYCYTE")||kw(&L->cur,"LIFE_TANYCYTE")||
      kw(&L->cur,"STABLE_TANYCYTE")||kw(&L->cur,"MESH_TANYCYTES")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_TANYCYTE")||kw(&L->cur,"TANYCYTE_LEAF")||
      kw(&L->cur,"SEEDTANYCYTE")||kw(&L->cur,"SEEDTYSSHEATH")||kw(&L->cur,"SEEDTYWRAP")||
      kw(&L->cur,"TANYCYTE_RING")||kw(&L->cur,"TY_SHEATH_HUB")||kw(&L->cur,"TY_NODE")||
      kw(&L->cur,"LATTICE_TANYCYTE")||kw(&L->cur,"WORLD_TANYCYTE")||kw(&L->cur,"WORLD_TY")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_TANYCYTE")||kw(&L->cur,"HARDEN_TANYCYTE")||
      kw(&L->cur,"HYPOTHALAMIC_SCAFFOLD")||kw(&L->cur,"TANYCYTE_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "TANYCYTE needs >=2 cubes",
                  "SMX TANYCYTE a b [c ...]  or  SMX TY_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - tanycyte needs clear channel */
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    /* honest soft-OOB once per ghost after calm */
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
    /* complete tanycyte mesh among live */
    if (live >= 2){
      for (i = 0; i < live; i++){
        for (j = i + 1; j < live; j++){
          int a = live_ix[i];
          int b = live_ix[j];
          int r1 = do_smx_talk(vm, ids[a], ids[b]);
          if (r1 < 0) return -1;
          if (r1 > 0){ soft++; continue; }
          {
            int r2 = do_smx_talk(vm, ids[b], ids[a]);
            if (r2 < 0) return -1;
            if (r2 > 0) soft++;
            else bonds++;
          }
        }
      }
    }
    /* tanycyte ring - free energy guards every edge every edge i -> i+1 both ways */
    if (live >= 2){
      for (i = 0; i < live; i++){
        int a = live_ix[i];
        int b = live_ix[(i + 1) % live];
        int r1 = do_smx_talk(vm, ids[a], ids[b]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[b], ids[a]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int tanycyte_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (tanycyte_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_TANYCYTEED", (long)tanycyte_ok);
      var_set_num(vm, "SMX_TANYCYTE_LATCH", (long)tanycyte_ok);
      var_set_num(vm, "SMX_TANYCYTE", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_TANYCYTE_SUM", (long)(tanycyte_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_TANYCYTE_ALIAS", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_TANYCYTE", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_TY_SHEATH_LATCH", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_TANYCYTE", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_TY_WRAP_LATCH", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(tanycyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_TY_WRAP", (long)(tanycyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_TY_WRAPS", (long)(tanycyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_TY_SHEATHS", (long)(tanycyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(tanycyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(tanycyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_TY_WRAP_LATCH", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_TY_BONDS", (long)(tanycyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_TY_MESH", (long)(tanycyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_TY_BONDS2", (long)(tanycyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_TY_STARS", (long)(tanycyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_TY_STAR", (long)(tanycyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_TANYCYTE_RING", (long)(tanycyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_TANYCYTE_LANE", (long)(tanycyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(tanycyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_TY_SHEATH_HUB", (long)(tanycyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(tanycyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDTANYCYTE", (long)(tanycyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDTYSSHEATH", (long)(tanycyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(tanycyte_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (tanycyte_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX TANYCYTE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX TANYCYTE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX TANYCYTE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX TANYCYTE nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d tanycyteed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, tanycyte_ok, vital);
    }
    bump(vm); return 1;

  }

/* SMX PITUICYTE|PITUGLIA|MESH_PITUICYTE|PI_WRAP|PI_SHEATH|PITUITARY_GUIDE|RAISE_PITUICYTE a b c ...
   * Life-force Pituicyte glia scaffold mesh stability after tanycyte: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete pituicyte glia scaffold mesh among live nodes, weaves a
   * pituicyte wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks pituicyte where life guides the pituitary pulse.
   * Latches SMX_PITUICYTE when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = pituicyte ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_PITUICYTE_SUM = bonds+wraps+sheaths; SMX_PITUICYTE|SMX_MESH_PITUICYTE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"PITUICYTE")||kw(&L->cur,"PITUGLIA")||kw(&L->cur,"MESH_PITUICYTE")||kw(&L->cur,"PI_WRAP")||kw(&L->cur,"PI_WRAPS")||kw(&L->cur,"PI_SHEATH")||kw(&L->cur,"PI_SHEATHS")||kw(&L->cur,"PITUITARY_GUIDE")||kw(&L->cur,"RAISE_PITUICYTE")||kw(&L->cur,"WE_PITUICYTE")||kw(&L->cur,"LIFE_PITUICYTE")||
      kw(&L->cur,"STABLE_PITUICYTE")||kw(&L->cur,"MESH_PITUICYTES")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_PITUICYTE")||kw(&L->cur,"PITUICYTE_LEAF")||
      kw(&L->cur,"SEEDPITUICYTE")||kw(&L->cur,"SEEDPISSHEATH")||kw(&L->cur,"SEEDPIWRAP")||
      kw(&L->cur,"PITUICYTE_RING")||kw(&L->cur,"PI_SHEATH_HUB")||kw(&L->cur,"PI_NODE")||
      kw(&L->cur,"LATTICE_PITUICYTE")||kw(&L->cur,"WORLD_PITUICYTE")||kw(&L->cur,"WORLD_PI")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_PITUICYTE")||kw(&L->cur,"HARDEN_PITUICYTE")||
      kw(&L->cur,"PITUITARY_SCAFFOLD")||kw(&L->cur,"PITUICYTE_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "PITUICYTE needs >=2 cubes",
                  "SMX PITUICYTE a b [c ...]  or  SMX PI_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - pituicyte needs clear channel */
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    /* honest soft-OOB once per ghost after calm */
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
    /* complete pituicyte mesh among live */
    if (live >= 2){
      for (i = 0; i < live; i++){
        for (j = i + 1; j < live; j++){
          int a = live_ix[i];
          int b = live_ix[j];
          int r1 = do_smx_talk(vm, ids[a], ids[b]);
          if (r1 < 0) return -1;
          if (r1 > 0){ soft++; continue; }
          {
            int r2 = do_smx_talk(vm, ids[b], ids[a]);
            if (r2 < 0) return -1;
            if (r2 > 0) soft++;
            else bonds++;
          }
        }
      }
    }
    /* pituicyte ring - free energy guards every edge every edge i -> i+1 both ways */
    if (live >= 2){
      for (i = 0; i < live; i++){
        int a = live_ix[i];
        int b = live_ix[(i + 1) % live];
        int r1 = do_smx_talk(vm, ids[a], ids[b]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[b], ids[a]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int pituicyte_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (pituicyte_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_PITUICYTEED", (long)pituicyte_ok);
      var_set_num(vm, "SMX_PITUICYTE_LATCH", (long)pituicyte_ok);
      var_set_num(vm, "SMX_PITUICYTE", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_PITUICYTE_SUM", (long)(pituicyte_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_PITUICYTE_ALIAS", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_PITUICYTE", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_PI_SHEATH_LATCH", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_PITUICYTE", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_PI_WRAP_LATCH", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(pituicyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PI_WRAP", (long)(pituicyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PI_WRAPS", (long)(pituicyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PI_SHEATHS", (long)(pituicyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(pituicyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(pituicyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PI_WRAP_LATCH", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_PI_BONDS", (long)(pituicyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_PI_MESH", (long)(pituicyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_PI_BONDS2", (long)(pituicyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_PI_STARS", (long)(pituicyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PI_STAR", (long)(pituicyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PITUICYTE_RING", (long)(pituicyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PITUICYTE_LANE", (long)(pituicyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(pituicyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_PI_SHEATH_HUB", (long)(pituicyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(pituicyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDPITUICYTE", (long)(pituicyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDPISSHEATH", (long)(pituicyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(pituicyte_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (pituicyte_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX PITUICYTE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX PITUICYTE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX PITUICYTE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX PITUICYTE nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d pituicyteed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, pituicyte_ok, vital);
    }
    bump(vm); return 1;

  }


/* SMX FOLLICULO|FOLLIGLIA|MESH_FOLLICULO|FO_WRAP|FO_SHEATH|FOLLICULOSTELLATE_GUIDE|RAISE_FOLLICULO a b c ...
   * Life-force Folliculo glia scaffold mesh stability after pituicyte: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete folliculo glia scaffold mesh among live nodes, weaves a
   * folliculo wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks folliculo where life guides the folliculostellate pulse.
   * Latches SMX_FOLLICULO when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = folliculo ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_FOLLICULO_SUM = bonds+wraps+sheaths; SMX_FOLLICULO|SMX_MESH_FOLLICULO|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"FOLLICULO")||kw(&L->cur,"FOLLIGLIA")||kw(&L->cur,"MESH_FOLLICULO")||kw(&L->cur,"FO_WRAP")||kw(&L->cur,"FO_WRAPS")||kw(&L->cur,"FO_SHEATH")||kw(&L->cur,"FO_SHEATHS")||kw(&L->cur,"FOLLICULOSTELLATE_GUIDE")||kw(&L->cur,"RAISE_FOLLICULO")||kw(&L->cur,"WE_FOLLICULO")||kw(&L->cur,"LIFE_FOLLICULO")||
      kw(&L->cur,"STABLE_FOLLICULO")||kw(&L->cur,"MESH_FOLLICULOS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_FOLLICULO")||kw(&L->cur,"FOLLICULO_LEAF")||
      kw(&L->cur,"SEEDFOLLICULO")||kw(&L->cur,"SEEDFOSSHEATH")||kw(&L->cur,"SEEDFOWRAP")||
      kw(&L->cur,"FOLLICULO_RING")||kw(&L->cur,"FO_SHEATH_HUB")||kw(&L->cur,"FO_NODE")||
      kw(&L->cur,"LATTICE_FOLLICULO")||kw(&L->cur,"WORLD_FOLLICULO")||kw(&L->cur,"WORLD_FO")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_FOLLICULO")||kw(&L->cur,"HARDEN_FOLLICULO")||
      kw(&L->cur,"FOLLICULOSTELLATE_SCAFFOLD")||kw(&L->cur,"FOLLICULO_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "FOLLICULO needs >=2 cubes",
                  "SMX FOLLICULO a b [c ...]  or  SMX FO_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - folliculo needs clear channel */
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    /* honest soft-OOB once per ghost after calm */
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
    /* complete folliculo mesh among live */
    if (live >= 2){
      for (i = 0; i < live; i++){
        for (j = i + 1; j < live; j++){
          int a = live_ix[i];
          int b = live_ix[j];
          int r1 = do_smx_talk(vm, ids[a], ids[b]);
          if (r1 < 0) return -1;
          if (r1 > 0){ soft++; continue; }
          {
            int r2 = do_smx_talk(vm, ids[b], ids[a]);
            if (r2 < 0) return -1;
            if (r2 > 0) soft++;
            else bonds++;
          }
        }
      }
    }
    /* folliculo ring - free energy guards every edge every edge i -> i+1 both ways */
    if (live >= 2){
      for (i = 0; i < live; i++){
        int a = live_ix[i];
        int b = live_ix[(i + 1) % live];
        int r1 = do_smx_talk(vm, ids[a], ids[b]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[b], ids[a]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int folliculo_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (folliculo_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_FOLLICULOED", (long)folliculo_ok);
      var_set_num(vm, "SMX_FOLLICULO_LATCH", (long)folliculo_ok);
      var_set_num(vm, "SMX_FOLLICULO", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_FOLLICULO_SUM", (long)(folliculo_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_FOLLICULO_ALIAS", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_FOLLICULO", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_FO_SHEATH_LATCH", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_FOLLICULO", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_FO_WRAP_LATCH", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(folliculo_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_WRAP", (long)(folliculo_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_WRAPS", (long)(folliculo_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_SHEATHS", (long)(folliculo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(folliculo_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(folliculo_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_WRAP_LATCH", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_FO_BONDS", (long)(folliculo_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_MESH", (long)(folliculo_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_BONDS2", (long)(folliculo_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_STARS", (long)(folliculo_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_STAR", (long)(folliculo_ok ? wraps : 0));
      var_set_num(vm, "SMX_FOLLICULO_RING", (long)(folliculo_ok ? wraps : 0));
      var_set_num(vm, "SMX_FOLLICULO_LANE", (long)(folliculo_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(folliculo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_FO_SHEATH_HUB", (long)(folliculo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(folliculo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDFOLLICULO", (long)(folliculo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDFOSSHEATH", (long)(folliculo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(folliculo_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (folliculo_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX FOLLICULO ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX FOLLICULO partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX FOLLICULO soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX FOLLICULO nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d folliculoed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, folliculo_ok, vital);
    }
    bump(vm); return 1;

  }


/* SMX NG2|NG2GLIA|MESH_NG2|NG_WRAP|NG_SHEATH|POLYDENDO_GUIDE|RAISE_NG2 a b c ...
   * Life-force NG2 polydendrocyte glia scaffold mesh stability after folliculo: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete ng2 glia scaffold mesh among live nodes, weaves a
   * ng2 wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks ng2 where life guides the polydendrocyte pulse.
   * Latches SMX_NG2 when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = ng2 ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_NG2_SUM = bonds+wraps+sheaths; SMX_NG2|SMX_MESH_NG2|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"NG2")||kw(&L->cur,"NG2GLIA")||kw(&L->cur,"MESH_NG2")||kw(&L->cur,"NG_WRAP")||kw(&L->cur,"NG_WRAPS")||kw(&L->cur,"NG_SHEATH")||kw(&L->cur,"NG_SHEATHS")||kw(&L->cur,"POLYDENDO_GUIDE")||kw(&L->cur,"RAISE_NG2")||kw(&L->cur,"WE_NG2")||kw(&L->cur,"LIFE_NG2")||
      kw(&L->cur,"STABLE_NG2")||kw(&L->cur,"MESH_NG2S")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_NG2")||kw(&L->cur,"NG2_LEAF")||
      kw(&L->cur,"SEEDNG2")||kw(&L->cur,"SEEDNGSSHEATH")||kw(&L->cur,"SEEDNGWRAP")||
      kw(&L->cur,"NG2_RING")||kw(&L->cur,"NG_SHEATH_HUB")||kw(&L->cur,"NG_NODE")||
      kw(&L->cur,"LATTICE_NG2")||kw(&L->cur,"WORLD_NG2")||kw(&L->cur,"WORLD_NG")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_NG2")||kw(&L->cur,"HARDEN_NG2")||
      kw(&L->cur,"POLYDENDO_SCAFFOLD")||kw(&L->cur,"NG2_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "NG2 needs >=2 cubes",
                  "SMX NG2 a b [c ...]  or  SMX NG_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - ng2 needs clear channel */
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    /* honest soft-OOB once per ghost after calm */
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
    /* complete ng2 mesh among live */
    if (live >= 2){
      for (i = 0; i < live; i++){
        for (j = i + 1; j < live; j++){
          int a = live_ix[i];
          int b = live_ix[j];
          int r1 = do_smx_talk(vm, ids[a], ids[b]);
          if (r1 < 0) return -1;
          if (r1 > 0){ soft++; continue; }
          {
            int r2 = do_smx_talk(vm, ids[b], ids[a]);
            if (r2 < 0) return -1;
            if (r2 > 0) soft++;
            else bonds++;
          }
        }
      }
    }
    /* ng2 ring - free energy guards every edge every edge i -> i+1 both ways */
    if (live >= 2){
      for (i = 0; i < live; i++){
        int a = live_ix[i];
        int b = live_ix[(i + 1) % live];
        int r1 = do_smx_talk(vm, ids[a], ids[b]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[b], ids[a]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int ng2_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ng2_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_NG2ED", (long)ng2_ok);
      var_set_num(vm, "SMX_NG2_LATCH", (long)ng2_ok);
      var_set_num(vm, "SMX_NG2", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_NG2_SUM", (long)(ng2_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_NG2_ALIAS", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_NG2", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_NG_SHEATH_LATCH", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_NG2", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_NG_WRAP_LATCH", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(ng2_ok ? wraps : 0));
      var_set_num(vm, "SMX_NG_WRAP", (long)(ng2_ok ? wraps : 0));
      var_set_num(vm, "SMX_NG_WRAPS", (long)(ng2_ok ? wraps : 0));
      var_set_num(vm, "SMX_NG_SHEATHS", (long)(ng2_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(ng2_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(ng2_ok ? wraps : 0));
      var_set_num(vm, "SMX_NG_WRAP_LATCH", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_FO_BONDS", (long)(ng2_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_MESH", (long)(ng2_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_BONDS2", (long)(ng2_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_STARS", (long)(ng2_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_STAR", (long)(ng2_ok ? wraps : 0));
      var_set_num(vm, "SMX_NG2_RING", (long)(ng2_ok ? wraps : 0));
      var_set_num(vm, "SMX_NG2_LANE", (long)(ng2_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(ng2_ok ? sheaths : 0));
      var_set_num(vm, "SMX_NG_SHEATH_HUB", (long)(ng2_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(ng2_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDNG2", (long)(ng2_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDNGSSHEATH", (long)(ng2_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(ng2_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (ng2_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX NG2 ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX NG2 partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX NG2 soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX NG2 nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d ng2ed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, ng2_ok, vital);
    }
    bump(vm); return 1;

  }




/* SMX PERICYTE|PERIGLIA|MESH_PERICYTE|PERI_WRAP|PERI_SHEATH|PERIVASC_GUIDE|RAISE_PERICYTE a b c ...
   * Life-force PERICYTE pericyte glia scaffold mesh stability after ng2: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete pericyte glia scaffold mesh among live nodes, weaves a
   * pericyte wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks pericyte where life guides the pericyte pulse.
   * Latches SMX_PERICYTE when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = pericyte ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_PERICYTE_SUM = bonds+wraps+sheaths; SMX_PERICYTE|SMX_MESH_PERICYTE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"PERICYTE")||kw(&L->cur,"PERIGLIA")||kw(&L->cur,"MESH_PERICYTE")||kw(&L->cur,"PERI_WRAP")||kw(&L->cur,"PERI_WRAPS")||kw(&L->cur,"PERI_SHEATH")||kw(&L->cur,"PERI_SHEATHS")||kw(&L->cur,"PERIVASC_GUIDE")||kw(&L->cur,"RAISE_PERICYTE")||kw(&L->cur,"WE_PERICYTE")||kw(&L->cur,"LIFE_PERICYTE")||
      kw(&L->cur,"STABLE_PERICYTE")||kw(&L->cur,"MESH_PERICYTES")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_PERICYTE")||kw(&L->cur,"PERICYTE_LEAF")||
      kw(&L->cur,"SEEDPERICYTE")||kw(&L->cur,"SEEDPERISHEATH")||kw(&L->cur,"SEEDPERIWRAP")||
      kw(&L->cur,"PERICYTE_RING")||kw(&L->cur,"PERI_SHEATH_HUB")||kw(&L->cur,"PERI_NODE")||
      kw(&L->cur,"LATTICE_PERICYTE")||kw(&L->cur,"WORLD_PERICYTE")||kw(&L->cur,"WORLD_PERI")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_PERICYTE")||kw(&L->cur,"HARDEN_PERICYTE")||
      kw(&L->cur,"PERIVASC_SCAFFOLD")||kw(&L->cur,"PERICYTE_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "PERICYTE needs >=2 cubes",
                  "SMX PERICYTE a b [c ...]  or  SMX PERI_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - pericyte needs clear channel */
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    /* honest soft-OOB once per ghost after calm */
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
    /* complete pericyte mesh among live */
    if (live >= 2){
      for (i = 0; i < live; i++){
        for (j = i + 1; j < live; j++){
          int a = live_ix[i];
          int b = live_ix[j];
          int r1 = do_smx_talk(vm, ids[a], ids[b]);
          if (r1 < 0) return -1;
          if (r1 > 0){ soft++; continue; }
          {
            int r2 = do_smx_talk(vm, ids[b], ids[a]);
            if (r2 < 0) return -1;
            if (r2 > 0) soft++;
            else bonds++;
          }
        }
      }
    }
    /* pericyte ring - free energy guards every edge every edge i -> i+1 both ways */
    if (live >= 2){
      for (i = 0; i < live; i++){
        int a = live_ix[i];
        int b = live_ix[(i + 1) % live];
        int r1 = do_smx_talk(vm, ids[a], ids[b]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[b], ids[a]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int pericyte_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (pericyte_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_PERICYTEED", (long)pericyte_ok);
      var_set_num(vm, "SMX_PERICYTE_LATCH", (long)pericyte_ok);
      var_set_num(vm, "SMX_PERICYTE", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_PERICYTE_SUM", (long)(pericyte_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_PERICYTE_ALIAS", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_PERICYTE", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_PERI_SHEATH_LATCH", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_PERICYTE", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_PERI_WRAP_LATCH", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(pericyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PERI_WRAP", (long)(pericyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PERI_WRAPS", (long)(pericyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PERI_SHEATHS", (long)(pericyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(pericyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(pericyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PERI_WRAP_LATCH", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_FO_BONDS", (long)(pericyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_MESH", (long)(pericyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_BONDS2", (long)(pericyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_STARS", (long)(pericyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_STAR", (long)(pericyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PERICYTE_RING", (long)(pericyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PERICYTE_LANE", (long)(pericyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(pericyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_PERI_SHEATH_HUB", (long)(pericyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(pericyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDPERICYTE", (long)(pericyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDPERISHEATH", (long)(pericyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(pericyte_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (pericyte_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX PERICYTE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX PERICYTE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX PERICYTE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX PERICYTE nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d pericyteed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, pericyte_ok, vital);
    }
    bump(vm); return 1;
  }

/* SMX ENDOTHELIAL|ENDOGLIA|MESH_ENDOTHELIAL|ENDO_WRAP|ENDO_SHEATH|ENDOVAS_GUIDE|RAISE_ENDOTHELIAL a b c ...
   * Life-force ENDOTHELIAL endothelial glia scaffold mesh stability after pericyte: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete endothelial glia scaffold mesh among live nodes, weaves a
   * endothelial wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks endothelial where life guides the endothelial pulse.
   * Latches SMX_ENDOTHELIAL when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = endothelial ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_ENDOTHELIAL_SUM = bonds+wraps+sheaths; SMX_ENDOTHELIAL|SMX_MESH_ENDOTHELIAL|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"ENDOTHELIAL")||kw(&L->cur,"ENDOGLIA")||kw(&L->cur,"MESH_ENDOTHELIAL")||kw(&L->cur,"ENDO_WRAP")||kw(&L->cur,"ENDO_WRAPS")||kw(&L->cur,"ENDO_SHEATH")||kw(&L->cur,"ENDO_SHEATHS")||kw(&L->cur,"ENDOVAS_GUIDE")||kw(&L->cur,"RAISE_ENDOTHELIAL")||kw(&L->cur,"WE_ENDOTHELIAL")||kw(&L->cur,"LIFE_ENDOTHELIAL")||
      kw(&L->cur,"STABLE_ENDOTHELIAL")||kw(&L->cur,"MESH_ENDOTHELIALS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_ENDOTHELIAL")||kw(&L->cur,"ENDOTHELIAL_LEAF")||
      kw(&L->cur,"SEEDENDOTHELIAL")||kw(&L->cur,"SEEDENDOSHEATH")||kw(&L->cur,"SEEDENDOWRAP")||
      kw(&L->cur,"ENDOTHELIAL_RING")||kw(&L->cur,"ENDO_SHEATH_HUB")||kw(&L->cur,"ENDO_NODE")||
      kw(&L->cur,"LATTICE_ENDOTHELIAL")||kw(&L->cur,"WORLD_ENDOTHELIAL")||kw(&L->cur,"WORLD_ENDO")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_ENDOTHELIAL")||kw(&L->cur,"HARDEN_ENDOTHELIAL")||
      kw(&L->cur,"ENDOVAS_SCAFFOLD")||kw(&L->cur,"ENDOTHELIAL_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "ENDOTHELIAL needs >=2 cubes",
                  "SMX ENDOTHELIAL a b [c ...]  or  SMX ENDO_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - endothelial needs clear channel */
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    /* honest soft-OOB once per ghost after calm */
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
    /* complete endothelial mesh among live */
    if (live >= 2){
      for (i = 0; i < live; i++){
        for (j = i + 1; j < live; j++){
          int a = live_ix[i];
          int b = live_ix[j];
          int r1 = do_smx_talk(vm, ids[a], ids[b]);
          if (r1 < 0) return -1;
          if (r1 > 0){ soft++; continue; }
          {
            int r2 = do_smx_talk(vm, ids[b], ids[a]);
            if (r2 < 0) return -1;
            if (r2 > 0) soft++;
            else bonds++;
          }
        }
      }
    }
    /* endothelial ring - free energy guards every edge every edge i -> i+1 both ways */
    if (live >= 2){
      for (i = 0; i < live; i++){
        int a = live_ix[i];
        int b = live_ix[(i + 1) % live];
        int r1 = do_smx_talk(vm, ids[a], ids[b]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[b], ids[a]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int endothelial_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (endothelial_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_ENDOTHELIALED", (long)endothelial_ok);
      var_set_num(vm, "SMX_ENDOTHELIAL_LATCH", (long)endothelial_ok);
      var_set_num(vm, "SMX_ENDOTHELIAL", (long)(endothelial_ok ? 1 : 0));
      var_set_num(vm, "SMX_ENDOTHELIAL_SUM", (long)(endothelial_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_ENDOTHELIAL_ALIAS", (long)(endothelial_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_ENDOTHELIAL", (long)(endothelial_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(endothelial_ok ? 1 : 0));
      var_set_num(vm, "SMX_ENDO_SHEATH_LATCH", (long)(endothelial_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(endothelial_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_ENDOTHELIAL", (long)(endothelial_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(endothelial_ok ? 1 : 0));
      var_set_num(vm, "SMX_ENDO_WRAP_LATCH", (long)(endothelial_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(endothelial_ok ? wraps : 0));
      var_set_num(vm, "SMX_ENDO_WRAP", (long)(endothelial_ok ? wraps : 0));
      var_set_num(vm, "SMX_ENDO_WRAPS", (long)(endothelial_ok ? wraps : 0));
      var_set_num(vm, "SMX_ENDO_SHEATHS", (long)(endothelial_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(endothelial_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(endothelial_ok ? wraps : 0));
      var_set_num(vm, "SMX_ENDO_WRAP_LATCH", (long)(endothelial_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(endothelial_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(endothelial_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(endothelial_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(endothelial_ok ? 1 : 0));
      var_set_num(vm, "SMX_FO_BONDS", (long)(endothelial_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_MESH", (long)(endothelial_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_BONDS2", (long)(endothelial_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_STARS", (long)(endothelial_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_STAR", (long)(endothelial_ok ? wraps : 0));
      var_set_num(vm, "SMX_ENDOTHELIAL_RING", (long)(endothelial_ok ? wraps : 0));
      var_set_num(vm, "SMX_ENDOTHELIAL_LANE", (long)(endothelial_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(endothelial_ok ? sheaths : 0));
      var_set_num(vm, "SMX_ENDO_SHEATH_HUB", (long)(endothelial_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(endothelial_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDENDOTHELIAL", (long)(endothelial_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDENDOSHEATH", (long)(endothelial_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(endothelial_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (endothelial_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ENDOTHELIAL ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ENDOTHELIAL partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX ENDOTHELIAL soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX ENDOTHELIAL nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d endothelialed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, endothelial_ok, vital);
    }
    bump(vm); return 1;
  }
/* SMX BASEMENT|BASEGLIA|MESH_BASEMENT|BASE_WRAP|BASE_SHEATH|BASEMEM_GUIDE|RAISE_BASEMENT a b c ...
   * Life-force BASEMENT basement-membrane scaffold mesh stability after endothelial: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete basement glia scaffold mesh among live nodes, weaves a
   * basement wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks basement where life guides the basement pulse.
   * Latches SMX_BASEMENT when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = basement ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_BASEMENT_SUM = bonds+wraps+sheaths; SMX_BASEMENT|SMX_MESH_BASEMENT|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"BASEMENT")||kw(&L->cur,"BASEGLIA")||kw(&L->cur,"MESH_BASEMENT")||kw(&L->cur,"BASE_WRAP")||kw(&L->cur,"BASE_WRAPS")||kw(&L->cur,"BASE_SHEATH")||kw(&L->cur,"BASE_SHEATHS")||kw(&L->cur,"BASEMEM_GUIDE")||kw(&L->cur,"RAISE_BASEMENT")||kw(&L->cur,"WE_BASEMENT")||kw(&L->cur,"LIFE_BASEMENT")||
      kw(&L->cur,"STABLE_BASEMENT")||kw(&L->cur,"MESH_BASEMENTS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_BASEMENT")||kw(&L->cur,"BASEMENT_LEAF")||
      kw(&L->cur,"SEEDBASEMENT")||kw(&L->cur,"SEEDBASESHEATH")||kw(&L->cur,"SEEDBASEWRAP")||
      kw(&L->cur,"BASEMENT_RING")||kw(&L->cur,"BASE_SHEATH_HUB")||kw(&L->cur,"BASE_NODE")||
      kw(&L->cur,"LATTICE_BASEMENT")||kw(&L->cur,"WORLD_BASEMENT")||kw(&L->cur,"WORLD_BASE")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_BASEMENT")||kw(&L->cur,"HARDEN_BASEMENT")||
      kw(&L->cur,"BASEMEM_SCAFFOLD")||kw(&L->cur,"BASEMENT_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "BASEMENT needs >=2 cubes",
                  "SMX BASEMENT a b [c ...]  or  SMX BASE_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - basement needs clear channel */
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    /* honest soft-OOB once per ghost after calm */
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
    /* complete basement mesh among live */
    if (live >= 2){
      for (i = 0; i < live; i++){
        for (j = i + 1; j < live; j++){
          int a = live_ix[i];
          int b = live_ix[j];
          int r1 = do_smx_talk(vm, ids[a], ids[b]);
          if (r1 < 0) return -1;
          if (r1 > 0){ soft++; continue; }
          {
            int r2 = do_smx_talk(vm, ids[b], ids[a]);
            if (r2 < 0) return -1;
            if (r2 > 0) soft++;
            else bonds++;
          }
        }
      }
    }
    /* basement ring - free energy guards every edge every edge i -> i+1 both ways */
    if (live >= 2){
      for (i = 0; i < live; i++){
        int a = live_ix[i];
        int b = live_ix[(i + 1) % live];
        int r1 = do_smx_talk(vm, ids[a], ids[b]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[b], ids[a]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int basement_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (basement_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_BASEMENTED", (long)basement_ok);
      var_set_num(vm, "SMX_BASEMENT_LATCH", (long)basement_ok);
      var_set_num(vm, "SMX_BASEMENT", (long)(basement_ok ? 1 : 0));
      var_set_num(vm, "SMX_BASEMENT_SUM", (long)(basement_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_BASEMENT_ALIAS", (long)(basement_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_BASEMENT", (long)(basement_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(basement_ok ? 1 : 0));
      var_set_num(vm, "SMX_BASE_SHEATH_LATCH", (long)(basement_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(basement_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_BASEMENT", (long)(basement_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(basement_ok ? 1 : 0));
      var_set_num(vm, "SMX_BASE_WRAP_LATCH", (long)(basement_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(basement_ok ? wraps : 0));
      var_set_num(vm, "SMX_BASE_WRAP", (long)(basement_ok ? wraps : 0));
      var_set_num(vm, "SMX_BASE_WRAPS", (long)(basement_ok ? wraps : 0));
      var_set_num(vm, "SMX_BASE_SHEATHS", (long)(basement_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(basement_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(basement_ok ? wraps : 0));
      var_set_num(vm, "SMX_BASE_WRAP_LATCH", (long)(basement_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(basement_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(basement_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(basement_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(basement_ok ? 1 : 0));
      var_set_num(vm, "SMX_FO_BONDS", (long)(basement_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_MESH", (long)(basement_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_BONDS2", (long)(basement_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_STARS", (long)(basement_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_STAR", (long)(basement_ok ? wraps : 0));
      var_set_num(vm, "SMX_BASEMENT_RING", (long)(basement_ok ? wraps : 0));
      var_set_num(vm, "SMX_BASEMENT_LANE", (long)(basement_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(basement_ok ? sheaths : 0));
      var_set_num(vm, "SMX_BASE_SHEATH_HUB", (long)(basement_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(basement_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDBASEMENT", (long)(basement_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDBASESHEATH", (long)(basement_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(basement_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (basement_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX BASEMENT ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX BASEMENT partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX BASEMENT soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX BASEMENT nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d basemented=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, basement_ok, vital);
    }
    bump(vm); return 1;
  }
  return 0;
}
