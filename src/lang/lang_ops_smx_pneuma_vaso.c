/* CubalC lang — lang_ops_smx_pneuma_vaso.c
 * SMX vascular: CAPILLARY · VENULE · ARTERY · VEIN · AORTA · CAVA
 * Peel from pneuma_b. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_smx_pneuma_vaso.h"
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
int cubalc_lang_ops_smx_try_pneuma_vaso(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
/* SMX CAPILLARY|CAPIGLIA|MESH_CAPILLARY|CAPI_WRAP|CAPI_SHEATH|CAPILLARY_GUIDE|RAISE_CAPILLARY a b c ...
   * Life-force CAPILLARY capillary-membrane scaffold mesh stability after basement: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete capillary glia scaffold mesh among live nodes, weaves a
   * capillary wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks capillary where life guides the capillary pulse.
   * Latches SMX_CAPILLARY when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = capillary ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_CAPILLARY_SUM = bonds+wraps+sheaths; SMX_CAPILLARY|SMX_MESH_CAPILLARY|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"CAPILLARY")||kw(&L->cur,"CAPIGLIA")||kw(&L->cur,"MESH_CAPILLARY")||kw(&L->cur,"CAPI_WRAP")||kw(&L->cur,"CAPI_WRAPS")||kw(&L->cur,"CAPI_SHEATH")||kw(&L->cur,"CAPI_SHEATHS")||kw(&L->cur,"CAPILLARY_GUIDE")||kw(&L->cur,"RAISE_CAPILLARY")||kw(&L->cur,"WE_CAPILLARY")||kw(&L->cur,"LIFE_CAPILLARY")||
      kw(&L->cur,"STABLE_CAPILLARY")||kw(&L->cur,"MESH_CAPILLARYS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_CAPILLARY")||kw(&L->cur,"CAPILLARY_LEAF")||
      kw(&L->cur,"SEEDCAPILLARY")||kw(&L->cur,"SEEDCAPISHEATH")||kw(&L->cur,"SEEDCAPIWRAP")||
      kw(&L->cur,"CAPILLARY_RING")||kw(&L->cur,"CAPI_SHEATH_HUB")||kw(&L->cur,"CAPI_NODE")||
      kw(&L->cur,"LATTICE_CAPILLARY")||kw(&L->cur,"WORLD_CAPILLARY")||kw(&L->cur,"WORLD_CAPI")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_CAPILLARY")||kw(&L->cur,"HARDEN_CAPILLARY")||
      kw(&L->cur,"CAPILLARY_SCAFFOLD")||kw(&L->cur,"CAPILLARY_MESH")){
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
      smx_fail_at(vm, aln, "CAPILLARY needs >=2 cubes",
                  "SMX CAPILLARY a b [c ...]  or  SMX CAPI_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - capillary needs clear channel */
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
    /* complete capillary mesh among live */
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
    /* capillary ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int capillary_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (capillary_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_CAPILLARYED", (long)capillary_ok);
      var_set_num(vm, "SMX_CAPILLARY_LATCH", (long)capillary_ok);
      var_set_num(vm, "SMX_CAPILLARY", (long)(capillary_ok ? 1 : 0));
      var_set_num(vm, "SMX_CAPILLARY_SUM", (long)(capillary_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_CAPILLARY_ALIAS", (long)(capillary_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_CAPILLARY", (long)(capillary_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(capillary_ok ? 1 : 0));
      var_set_num(vm, "SMX_CAPI_SHEATH_LATCH", (long)(capillary_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(capillary_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_CAPILLARY", (long)(capillary_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(capillary_ok ? 1 : 0));
      var_set_num(vm, "SMX_CAPI_WRAP_LATCH", (long)(capillary_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(capillary_ok ? wraps : 0));
      var_set_num(vm, "SMX_CAPI_WRAP", (long)(capillary_ok ? wraps : 0));
      var_set_num(vm, "SMX_CAPI_WRAPS", (long)(capillary_ok ? wraps : 0));
      var_set_num(vm, "SMX_CAPI_SHEATHS", (long)(capillary_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(capillary_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(capillary_ok ? wraps : 0));
      var_set_num(vm, "SMX_CAPI_WRAP_LATCH", (long)(capillary_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(capillary_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(capillary_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(capillary_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(capillary_ok ? 1 : 0));
      var_set_num(vm, "SMX_FO_BONDS", (long)(capillary_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_MESH", (long)(capillary_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_BONDS2", (long)(capillary_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_STARS", (long)(capillary_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_STAR", (long)(capillary_ok ? wraps : 0));
      var_set_num(vm, "SMX_CAPILLARY_RING", (long)(capillary_ok ? wraps : 0));
      var_set_num(vm, "SMX_CAPILLARY_LANE", (long)(capillary_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(capillary_ok ? sheaths : 0));
      var_set_num(vm, "SMX_CAPI_SHEATH_HUB", (long)(capillary_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(capillary_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDCAPILLARY", (long)(capillary_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDCAPISHEATH", (long)(capillary_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(capillary_ok ? live : 0));
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
      if (capillary_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CAPILLARY ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CAPILLARY partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX CAPILLARY soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX CAPILLARY nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d capillaryed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, capillary_ok, vital);
    }
    bump(vm); return 1;
  }

/* SMX VENULE|VENUGLIA|MESH_VENULE|VENU_WRAP|VENU_SHEATH|VENULE_GUIDE|RAISE_VENULE a b c ...
   * Life-force VENULE venule-membrane return scaffold mesh stability after capillary: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete venule return scaffold mesh among live nodes, weaves a
   * venule wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks venule where life guides the venule pulse.
   * Latches SMX_VENULE when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = venule ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_VENULE_SUM = bonds+wraps+sheaths; SMX_VENULE|SMX_MESH_VENULE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"VENULE")||kw(&L->cur,"VENUGLIA")||kw(&L->cur,"MESH_VENULE")||kw(&L->cur,"VENU_WRAP")||kw(&L->cur,"VENU_WRAPS")||kw(&L->cur,"VENU_SHEATH")||kw(&L->cur,"VENU_SHEATHS")||kw(&L->cur,"VENULE_GUIDE")||kw(&L->cur,"RAISE_VENULE")||kw(&L->cur,"WE_VENULE")||kw(&L->cur,"LIFE_VENULE")||
      kw(&L->cur,"STABLE_VENULE")||kw(&L->cur,"MESH_VENULES")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_VENULE")||kw(&L->cur,"VENULE_LEAF")||
      kw(&L->cur,"SEEDVENULE")||kw(&L->cur,"SEEDVENUSHEATH")||kw(&L->cur,"SEEDVENUWRAP")||
      kw(&L->cur,"VENULE_RING")||kw(&L->cur,"VENU_SHEATH_HUB")||kw(&L->cur,"VENU_NODE")||
      kw(&L->cur,"LATTICE_VENULE")||kw(&L->cur,"WORLD_VENULE")||kw(&L->cur,"WORLD_VENU")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_VENULE")||kw(&L->cur,"HARDEN_VENULE")||
      kw(&L->cur,"VENULE_SCAFFOLD")||kw(&L->cur,"VENULE_MESH")){
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
      smx_fail_at(vm, aln, "VENULE needs >=2 cubes",
                  "SMX VENULE a b [c ...]  or  SMX VENU_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - venule needs clear channel */
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
    /* complete venule mesh among live */
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
    /* venule ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int venule_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (venule_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_VENULEED", (long)venule_ok);
      var_set_num(vm, "SMX_VENULE_LATCH", (long)venule_ok);
      var_set_num(vm, "SMX_VENULE", (long)(venule_ok ? 1 : 0));
      var_set_num(vm, "SMX_VENULE_SUM", (long)(venule_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_VENULE_ALIAS", (long)(venule_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_VENULE", (long)(venule_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(venule_ok ? 1 : 0));
      var_set_num(vm, "SMX_VENU_SHEATH_LATCH", (long)(venule_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(venule_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_VENULE", (long)(venule_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(venule_ok ? 1 : 0));
      var_set_num(vm, "SMX_VENU_WRAP_LATCH", (long)(venule_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(venule_ok ? wraps : 0));
      var_set_num(vm, "SMX_VENU_WRAP", (long)(venule_ok ? wraps : 0));
      var_set_num(vm, "SMX_VENU_WRAPS", (long)(venule_ok ? wraps : 0));
      var_set_num(vm, "SMX_VENU_SHEATHS", (long)(venule_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(venule_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(venule_ok ? wraps : 0));
      var_set_num(vm, "SMX_VENU_WRAP_LATCH", (long)(venule_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(venule_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(venule_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(venule_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(venule_ok ? 1 : 0));
      var_set_num(vm, "SMX_FO_BONDS", (long)(venule_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_MESH", (long)(venule_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_BONDS2", (long)(venule_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_STARS", (long)(venule_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_STAR", (long)(venule_ok ? wraps : 0));
      var_set_num(vm, "SMX_VENULE_RING", (long)(venule_ok ? wraps : 0));
      var_set_num(vm, "SMX_VENULE_LANE", (long)(venule_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(venule_ok ? sheaths : 0));
      var_set_num(vm, "SMX_VENU_SHEATH_HUB", (long)(venule_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(venule_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDVENULE", (long)(venule_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDVENUSHEATH", (long)(venule_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(venule_ok ? live : 0));
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
      if (venule_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX VENULE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX VENULE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX VENULE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX VENULE nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d venuleed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, venule_ok, vital);
    }
    bump(vm); return 1;
  }


  /* SMX ARTERIOLE|ARTERIGLIA|MESH_ARTERIOLE|ARTERI_WRAP|ARTERI_SHEATH|ARTERIOLE_GUIDE|RAISE_ARTERIOLE a b c ...
   * Life-force ARTERIOLE arteriole-membrane pressure scaffold mesh stability after venule: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete arteriole pressure scaffold mesh among live nodes, weaves a
   * arteriole wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks arteriole where life guides the arteriole pulse.
   * Latches SMX_ARTERIOLE when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = arteriole ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_ARTERIOLE_SUM = bonds+wraps+sheaths; SMX_ARTERIOLE|SMX_MESH_ARTERIOLE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"ARTERIOLE")||kw(&L->cur,"ARTERIGLIA")||kw(&L->cur,"MESH_ARTERIOLE")||kw(&L->cur,"ARTERI_WRAP")||kw(&L->cur,"ARTERI_WRAPS")||kw(&L->cur,"ARTERI_SHEATH")||kw(&L->cur,"ARTERI_SHEATHS")||kw(&L->cur,"ARTERIOLE_GUIDE")||kw(&L->cur,"RAISE_ARTERIOLE")||kw(&L->cur,"WE_ARTERIOLE")||kw(&L->cur,"LIFE_ARTERIOLE")||
      kw(&L->cur,"STABLE_ARTERIOLE")||kw(&L->cur,"MESH_ARTERIOLES")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_ARTERIOLE")||kw(&L->cur,"ARTERIOLE_LEAF")||
      kw(&L->cur,"SEEDARTERIOLE")||kw(&L->cur,"SEEDARTERISHEATH")||kw(&L->cur,"SEEDARTERIWRAP")||
      kw(&L->cur,"ARTERIOLE_RING")||kw(&L->cur,"ARTERI_SHEATH_HUB")||kw(&L->cur,"ARTERI_NODE")||
      kw(&L->cur,"LATTICE_ARTERIOLE")||kw(&L->cur,"WORLD_ARTERILE")||kw(&L->cur,"WORLD_ARTERI")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_ARTERIOLE")||kw(&L->cur,"HARDEN_ARTERIOLE")||
      kw(&L->cur,"ARTERIOLE_SCAFFOLD")||kw(&L->cur,"ARTERIOLE_MESH")){
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
      smx_fail_at(vm, aln, "ARTERIOLE needs >=2 cubes",
                  "SMX ARTERIOLE a b [c ...]  or  SMX ARTERI_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - arteriole needs clear channel */
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
    /* complete arteriole mesh among live */
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
    /* arteriole ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int arteriole_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (arteriole_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_ARTERIOLEED", (long)arteriole_ok);
      var_set_num(vm, "SMX_ARTERIOLE_LATCH", (long)arteriole_ok);
      var_set_num(vm, "SMX_ARTERIOLE", (long)(arteriole_ok ? 1 : 0));
      var_set_num(vm, "SMX_ARTERIOLE_SUM", (long)(arteriole_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_ARTERIOLE_ALIAS", (long)(arteriole_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_ARTERIOLE", (long)(arteriole_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(arteriole_ok ? 1 : 0));
      var_set_num(vm, "SMX_ARTERI_SHEATH_LATCH", (long)(arteriole_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(arteriole_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_ARTERIOLE", (long)(arteriole_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(arteriole_ok ? 1 : 0));
      var_set_num(vm, "SMX_ARTERI_WRAP_LATCH", (long)(arteriole_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(arteriole_ok ? wraps : 0));
      var_set_num(vm, "SMX_ARTERI_WRAP", (long)(arteriole_ok ? wraps : 0));
      var_set_num(vm, "SMX_ARTERI_WRAPS", (long)(arteriole_ok ? wraps : 0));
      var_set_num(vm, "SMX_ARTERI_SHEATHS", (long)(arteriole_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(arteriole_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(arteriole_ok ? wraps : 0));
      var_set_num(vm, "SMX_ARTERI_WRAP_LATCH", (long)(arteriole_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(arteriole_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(arteriole_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(arteriole_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(arteriole_ok ? 1 : 0));
      var_set_num(vm, "SMX_FO_BONDS", (long)(arteriole_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_MESH", (long)(arteriole_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_BONDS2", (long)(arteriole_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_STARS", (long)(arteriole_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_STAR", (long)(arteriole_ok ? wraps : 0));
      var_set_num(vm, "SMX_ARTERIOLE_RING", (long)(arteriole_ok ? wraps : 0));
      var_set_num(vm, "SMX_ARTERIOLE_LANE", (long)(arteriole_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(arteriole_ok ? sheaths : 0));
      var_set_num(vm, "SMX_ARTERI_SHEATH_HUB", (long)(arteriole_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(arteriole_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDARTERIOLE", (long)(arteriole_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDARTERISHEATH", (long)(arteriole_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(arteriole_ok ? live : 0));
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
      if (arteriole_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ARTERIOLE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ARTERIOLE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX ARTERIOLE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX ARTERIOLE nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d arterioleed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, arteriole_ok, vital);
    }
    bump(vm); return 1;
  }


  

  /* SMX ARTERY|ARTERGLIA|MESH_ARTERY|ARTER_WRAP|ARTER_SHEATH|ARTERY_GUIDE|RAISE_ARTERY a b c ...
   * Life-force ARTERY artery-membrane conduit scaffold mesh stability after arteriole: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete artery conduit scaffold mesh among live nodes, weaves a
   * artery wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks artery where life guides the artery pulse.
   * Latches SMX_ARTERY when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = artery ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_ARTERY_SUM = bonds+wraps+sheaths; SMX_ARTERY|SMX_MESH_ARTERY|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"ARTERY")||kw(&L->cur,"ARTERGLIA")||kw(&L->cur,"MESH_ARTERY")||kw(&L->cur,"ARTER_WRAP")||kw(&L->cur,"ARTER_WRAPS")||kw(&L->cur,"ARTER_SHEATH")||kw(&L->cur,"ARTER_SHEATHS")||kw(&L->cur,"ARTERY_GUIDE")||kw(&L->cur,"RAISE_ARTERY")||kw(&L->cur,"WE_ARTERY")||kw(&L->cur,"LIFE_ARTERY")||
      kw(&L->cur,"STABLE_ARTERY")||kw(&L->cur,"MESH_ARTERIES")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_ARTERY")||kw(&L->cur,"ARTERY_LEAF")||
      kw(&L->cur,"SEEDARTERY")||kw(&L->cur,"SEEDARTERSHEATH")||kw(&L->cur,"SEEDARTERWRAP")||
      kw(&L->cur,"ARTERY_RING")||kw(&L->cur,"ARTER_SHEATH_HUB")||kw(&L->cur,"ARTER_NODE")||
      kw(&L->cur,"LATTICE_ARTERY")||kw(&L->cur,"WORLD_ARTERY")||kw(&L->cur,"WORLD_ARTER")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_ARTERY")||kw(&L->cur,"HARDEN_ARTERY")||
      kw(&L->cur,"ARTERY_SCAFFOLD")||kw(&L->cur,"ARTERY_MESH")){
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
      smx_fail_at(vm, aln, "ARTERY needs >=2 cubes",
                  "SMX ARTERY a b [c ...]  or  SMX ARTER_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - artery needs clear channel */
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
    /* complete artery mesh among live */
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
    /* artery ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int artery_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (artery_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_ARTERYED", (long)artery_ok);
      var_set_num(vm, "SMX_ARTERY_LATCH", (long)artery_ok);
      var_set_num(vm, "SMX_ARTERY", (long)(artery_ok ? 1 : 0));
      var_set_num(vm, "SMX_ARTERY_SUM", (long)(artery_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_ARTERY_ALIAS", (long)(artery_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_ARTERY", (long)(artery_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(artery_ok ? 1 : 0));
      var_set_num(vm, "SMX_ARTER_SHEATH_LATCH", (long)(artery_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(artery_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_ARTERY", (long)(artery_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(artery_ok ? 1 : 0));
      var_set_num(vm, "SMX_ARTER_WRAP_LATCH", (long)(artery_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(artery_ok ? wraps : 0));
      var_set_num(vm, "SMX_ARTER_WRAP", (long)(artery_ok ? wraps : 0));
      var_set_num(vm, "SMX_ARTER_WRAPS", (long)(artery_ok ? wraps : 0));
      var_set_num(vm, "SMX_ARTER_SHEATHS", (long)(artery_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(artery_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(artery_ok ? wraps : 0));
      var_set_num(vm, "SMX_ARTER_WRAP_LATCH", (long)(artery_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(artery_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(artery_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(artery_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(artery_ok ? 1 : 0));
      var_set_num(vm, "SMX_FO_BONDS", (long)(artery_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_MESH", (long)(artery_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_BONDS2", (long)(artery_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_STARS", (long)(artery_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_STAR", (long)(artery_ok ? wraps : 0));
      var_set_num(vm, "SMX_ARTERY_RING", (long)(artery_ok ? wraps : 0));
      var_set_num(vm, "SMX_ARTERY_LANE", (long)(artery_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(artery_ok ? sheaths : 0));
      var_set_num(vm, "SMX_ARTER_SHEATH_HUB", (long)(artery_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(artery_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDARTERY", (long)(artery_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDARTERSHEATH", (long)(artery_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(artery_ok ? live : 0));
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
      if (artery_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ARTERY ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ARTERY partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX ARTERY soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX ARTERY nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d arteryed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, artery_ok, vital);
    }
    bump(vm); return 1;
  }


  



  /* SMX VEIN|VEINGLIA|MESH_VEIN|VEIN_WRAP|VEIN_SHEATH|VEIN_GUIDE|RAISE_VEIN a b c ...
   * Life-force VEIN vein-membrane return scaffold mesh stability after arteriole: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete vein return scaffold mesh among live nodes, weaves a
   * vein wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks vein where life guides the vein pulse.
   * Latches SMX_VEIN when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = vein ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_VEIN_SUM = bonds+wraps+sheaths; SMX_VEIN|SMX_MESH_VEIN|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"VEIN")||kw(&L->cur,"VEINGLIA")||kw(&L->cur,"MESH_VEIN")||kw(&L->cur,"VEIN_WRAP")||kw(&L->cur,"VEIN_WRAPS")||kw(&L->cur,"VEIN_SHEATH")||kw(&L->cur,"VEIN_SHEATHS")||kw(&L->cur,"VEIN_GUIDE")||kw(&L->cur,"RAISE_VEIN")||kw(&L->cur,"WE_VEIN")||kw(&L->cur,"LIFE_VEIN")||
      kw(&L->cur,"STABLE_VEIN")||kw(&L->cur,"MESH_VEINS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_VEIN")||kw(&L->cur,"VEIN_LEAF")||
      kw(&L->cur,"SEEDVEIN")||kw(&L->cur,"SEEDVEINSHEATH")||kw(&L->cur,"SEEDVEINWRAP")||
      kw(&L->cur,"VEIN_RING")||kw(&L->cur,"VEIN_SHEATH_HUB")||kw(&L->cur,"VEIN_NODE")||
      kw(&L->cur,"LATTICE_VEIN")||kw(&L->cur,"WORLD_VEIN")||kw(&L->cur,"WORLD_VEIN")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_VEIN")||kw(&L->cur,"HARDEN_VEIN")||
      kw(&L->cur,"VEIN_SCAFFOLD")||kw(&L->cur,"VEIN_MESH")){
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
      smx_fail_at(vm, aln, "VEIN needs >=2 cubes",
                  "SMX VEIN a b [c ...]  or  SMX VEIN_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - vein needs clear channel */
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
    /* complete vein mesh among live */
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
    /* vein ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int vein_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (vein_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_VEINED", (long)vein_ok);
      var_set_num(vm, "SMX_VEIN_LATCH", (long)vein_ok);
      var_set_num(vm, "SMX_VEIN", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_VEIN_SUM", (long)(vein_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_VEIN_ALIAS", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_VEIN", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_VEIN_SHEATH_LATCH", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_VEIN", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_VEIN_WRAP_LATCH", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(vein_ok ? wraps : 0));
      var_set_num(vm, "SMX_VEIN_WRAP", (long)(vein_ok ? wraps : 0));
      var_set_num(vm, "SMX_VEIN_WRAPS", (long)(vein_ok ? wraps : 0));
      var_set_num(vm, "SMX_VEIN_SHEATHS", (long)(vein_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(vein_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(vein_ok ? wraps : 0));
      var_set_num(vm, "SMX_VEIN_WRAP_LATCH", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_VE_BONDS", (long)(vein_ok ? bonds : 0));
      var_set_num(vm, "SMX_VE_MESH", (long)(vein_ok ? bonds : 0));
      var_set_num(vm, "SMX_VE_BONDS2", (long)(vein_ok ? bonds : 0));
      var_set_num(vm, "SMX_VE_STARS", (long)(vein_ok ? wraps : 0));
      var_set_num(vm, "SMX_VE_STAR", (long)(vein_ok ? wraps : 0));
      var_set_num(vm, "SMX_VEIN_RING", (long)(vein_ok ? wraps : 0));
      var_set_num(vm, "SMX_VEIN_LANE", (long)(vein_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(vein_ok ? sheaths : 0));
      var_set_num(vm, "SMX_VEIN_SHEATH_HUB", (long)(vein_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(vein_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDVEIN", (long)(vein_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDVEINSHEATH", (long)(vein_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(vein_ok ? live : 0));
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
      if (vein_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX VEIN ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX VEIN partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX VEIN soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX VEIN nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d veined=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, vein_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX AORTA|AORTAGLIA|MESH_AORTA|AORTA_WRAP|AORTA_SHEATH|AORTA_GUIDE|RAISE_AORTA a b c ...
   * Life-force AORTA aorta trunk conduit scaffold mesh stability after aorta: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete aorta trunk conduit scaffold mesh among live nodes, weaves a
   * aorta wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks aorta where life guides the aortic trunk pulse.
   * Latches SMX_AORTA when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = cava ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_AORTA_SUM = bonds+wraps+sheaths; SMX_AORTA|SMX_MESH_AORTA|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"AORTA")||kw(&L->cur,"AORTAGLIA")||kw(&L->cur,"MESH_AORTA")||kw(&L->cur,"AORTA_WRAP")||kw(&L->cur,"AORTA_WRAPS")||kw(&L->cur,"AORTA_SHEATH")||kw(&L->cur,"AORTA_SHEATHS")||kw(&L->cur,"AORTA_GUIDE")||kw(&L->cur,"RAISE_AORTA")||kw(&L->cur,"WE_AORTA")||kw(&L->cur,"LIFE_AORTA")||
      kw(&L->cur,"STABLE_AORTA")||kw(&L->cur,"MESH_AORTAS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_AORTA")||kw(&L->cur,"AORTA_LEAF")||
      kw(&L->cur,"SEEDAORTA")||kw(&L->cur,"SEEDAORTASHEATH")||kw(&L->cur,"SEEDAORTAWRAP")||
      kw(&L->cur,"AORTA_RING")||kw(&L->cur,"AORTA_SHEATH_HUB")||kw(&L->cur,"AORTA_NODE")||
      kw(&L->cur,"LATTICE_AORTA")||kw(&L->cur,"WORLD_AORTA")||kw(&L->cur,"WORLD_AORTA")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_AORTA")||kw(&L->cur,"HARDEN_AORTA")||
      kw(&L->cur,"AORTA_SCAFFOLD")||kw(&L->cur,"AORTA_MESH")){
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
      smx_fail_at(vm, aln, "AORTA needs >=2 cubes",
                  "SMX AORTA a b [c ...]  or  SMX AORTA_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - aorta needs clear channel */
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
    /* complete aorta mesh among live */
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
    /* aorta ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int aorta_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (aorta_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_AORTAED", (long)aorta_ok);
      var_set_num(vm, "SMX_AORTA_LATCH", (long)aorta_ok);
      var_set_num(vm, "SMX_AORTA", (long)(aorta_ok ? 1 : 0));
      var_set_num(vm, "SMX_AORTA_SUM", (long)(aorta_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_AORTA_ALIAS", (long)(aorta_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_AORTA", (long)(aorta_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(aorta_ok ? 1 : 0));
      var_set_num(vm, "SMX_AORTA_SHEATH_LATCH", (long)(aorta_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(aorta_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_AORTA", (long)(aorta_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(aorta_ok ? 1 : 0));
      var_set_num(vm, "SMX_AORTA_WRAP_LATCH", (long)(aorta_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(aorta_ok ? wraps : 0));
      var_set_num(vm, "SMX_AORTA_WRAP", (long)(aorta_ok ? wraps : 0));
      var_set_num(vm, "SMX_AORTA_WRAPS", (long)(aorta_ok ? wraps : 0));
      var_set_num(vm, "SMX_AORTA_SHEATHS", (long)(aorta_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(aorta_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(aorta_ok ? wraps : 0));
      var_set_num(vm, "SMX_AORTA_WRAP_LATCH", (long)(aorta_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(aorta_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(aorta_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(aorta_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(aorta_ok ? 1 : 0));
      var_set_num(vm, "SMX_AO_BONDS", (long)(aorta_ok ? bonds : 0));
      var_set_num(vm, "SMX_AO_MESH", (long)(aorta_ok ? bonds : 0));
      var_set_num(vm, "SMX_AO_BONDS2", (long)(aorta_ok ? bonds : 0));
      var_set_num(vm, "SMX_AO_STARS", (long)(aorta_ok ? wraps : 0));
      var_set_num(vm, "SMX_AO_STAR", (long)(aorta_ok ? wraps : 0));
      var_set_num(vm, "SMX_AORTA_RING", (long)(aorta_ok ? wraps : 0));
      var_set_num(vm, "SMX_AORTA_LANE", (long)(aorta_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(aorta_ok ? sheaths : 0));
      var_set_num(vm, "SMX_AORTA_SHEATH_HUB", (long)(aorta_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(aorta_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDAORTA", (long)(aorta_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDAORTASHEATH", (long)(aorta_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(aorta_ok ? live : 0));
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
      if (aorta_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AORTA ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AORTA partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX AORTA soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX AORTA nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d aortaed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, aorta_ok, vital);
    }
    bump(vm); return 1;
  }


  



  /* SMX VEIN|VEINGLIA|MESH_VEIN|VEIN_WRAP|VEIN_SHEATH|VEIN_GUIDE|RAISE_VEIN a b c ...
   * Life-force VEIN vein-membrane return scaffold mesh stability after arteriole: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete vein return scaffold mesh among live nodes, weaves a
   * vein wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks vein where life guides the vein pulse.
   * Latches SMX_VEIN when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = vein ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_VEIN_SUM = bonds+wraps+sheaths; SMX_VEIN|SMX_MESH_VEIN|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"VEIN")||kw(&L->cur,"VEINGLIA")||kw(&L->cur,"MESH_VEIN")||kw(&L->cur,"VEIN_WRAP")||kw(&L->cur,"VEIN_WRAPS")||kw(&L->cur,"VEIN_SHEATH")||kw(&L->cur,"VEIN_SHEATHS")||kw(&L->cur,"VEIN_GUIDE")||kw(&L->cur,"RAISE_VEIN")||kw(&L->cur,"WE_VEIN")||kw(&L->cur,"LIFE_VEIN")||
      kw(&L->cur,"STABLE_VEIN")||kw(&L->cur,"MESH_VEINS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_VEIN")||kw(&L->cur,"VEIN_LEAF")||
      kw(&L->cur,"SEEDVEIN")||kw(&L->cur,"SEEDVEINSHEATH")||kw(&L->cur,"SEEDVEINWRAP")||
      kw(&L->cur,"VEIN_RING")||kw(&L->cur,"VEIN_SHEATH_HUB")||kw(&L->cur,"VEIN_NODE")||
      kw(&L->cur,"LATTICE_VEIN")||kw(&L->cur,"WORLD_VEIN")||kw(&L->cur,"WORLD_VEIN")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_VEIN")||kw(&L->cur,"HARDEN_VEIN")||
      kw(&L->cur,"VEIN_SCAFFOLD")||kw(&L->cur,"VEIN_MESH")){
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
      smx_fail_at(vm, aln, "VEIN needs >=2 cubes",
                  "SMX VEIN a b [c ...]  or  SMX VEIN_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - vein needs clear channel */
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
    /* complete vein mesh among live */
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
    /* vein ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int vein_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (vein_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_VEINED", (long)vein_ok);
      var_set_num(vm, "SMX_VEIN_LATCH", (long)vein_ok);
      var_set_num(vm, "SMX_VEIN", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_VEIN_SUM", (long)(vein_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_VEIN_ALIAS", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_VEIN", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_VEIN_SHEATH_LATCH", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_VEIN", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_VEIN_WRAP_LATCH", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(vein_ok ? wraps : 0));
      var_set_num(vm, "SMX_VEIN_WRAP", (long)(vein_ok ? wraps : 0));
      var_set_num(vm, "SMX_VEIN_WRAPS", (long)(vein_ok ? wraps : 0));
      var_set_num(vm, "SMX_VEIN_SHEATHS", (long)(vein_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(vein_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(vein_ok ? wraps : 0));
      var_set_num(vm, "SMX_VEIN_WRAP_LATCH", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(vein_ok ? 1 : 0));
      var_set_num(vm, "SMX_VE_BONDS", (long)(vein_ok ? bonds : 0));
      var_set_num(vm, "SMX_VE_MESH", (long)(vein_ok ? bonds : 0));
      var_set_num(vm, "SMX_VE_BONDS2", (long)(vein_ok ? bonds : 0));
      var_set_num(vm, "SMX_VE_STARS", (long)(vein_ok ? wraps : 0));
      var_set_num(vm, "SMX_VE_STAR", (long)(vein_ok ? wraps : 0));
      var_set_num(vm, "SMX_VEIN_RING", (long)(vein_ok ? wraps : 0));
      var_set_num(vm, "SMX_VEIN_LANE", (long)(vein_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(vein_ok ? sheaths : 0));
      var_set_num(vm, "SMX_VEIN_SHEATH_HUB", (long)(vein_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(vein_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDVEIN", (long)(vein_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDVEINSHEATH", (long)(vein_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(vein_ok ? live : 0));
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
      if (vein_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX VEIN ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX VEIN partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX VEIN soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX VEIN nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d veined=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, vein_ok, vital);
    }
    bump(vm); return 1;
  }



  





  /* SMX CAVA|CAVAGLIA|MESH_CAVA|CAVA_WRAP|CAVA_SHEATH|CAVA_GUIDE|RAISE_CAVA a b c ...
   * Life-force CAVA cava great-return trunk scaffold mesh stability after aorta/vein: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete cava great-return trunk scaffold mesh among live nodes, weaves a
   * cava wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks cava where life guides the great venous return pulse.
   * Latches SMX_CAVA when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = aorta ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_CAVA_SUM = bonds+wraps+sheaths; SMX_CAVA|SMX_MESH_CAVA|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"CAVA")||kw(&L->cur,"CAVAGLIA")||kw(&L->cur,"MESH_CAVA")||kw(&L->cur,"CAVA_WRAP")||kw(&L->cur,"CAVA_WRAPS")||kw(&L->cur,"CAVA_SHEATH")||kw(&L->cur,"CAVA_SHEATHS")||kw(&L->cur,"CAVA_GUIDE")||kw(&L->cur,"RAISE_CAVA")||kw(&L->cur,"WE_CAVA")||kw(&L->cur,"LIFE_CAVA")||
      kw(&L->cur,"STABLE_CAVA")||kw(&L->cur,"MESH_CAVAS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_CAVA")||kw(&L->cur,"CAVA_LEAF")||
      kw(&L->cur,"SEEDCAVA")||kw(&L->cur,"SEEDCAVASHEATH")||kw(&L->cur,"SEEDCAVAWRAP")||
      kw(&L->cur,"CAVA_RING")||kw(&L->cur,"CAVA_SHEATH_HUB")||kw(&L->cur,"CAVA_NODE")||
      kw(&L->cur,"LATTICE_CAVA")||kw(&L->cur,"WORLD_CAVA")||kw(&L->cur,"WORLD_CAVA")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_CAVA")||kw(&L->cur,"HARDEN_CAVA")||
      kw(&L->cur,"CAVA_SCAFFOLD")||kw(&L->cur,"CAVA_MESH")){
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
      smx_fail_at(vm, aln, "CAVA needs >=2 cubes",
                  "SMX CAVA a b [c ...]  or  SMX CAVA_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - aorta needs clear channel */
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
    /* complete aorta mesh among live */
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
    /* aorta ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int cava_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (cava_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_CAVAED", (long)cava_ok);
      var_set_num(vm, "SMX_CAVA_LATCH", (long)cava_ok);
      var_set_num(vm, "SMX_CAVA", (long)(cava_ok ? 1 : 0));
      var_set_num(vm, "SMX_CAVA_SUM", (long)(cava_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_CAVA_ALIAS", (long)(cava_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_CAVA", (long)(cava_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(cava_ok ? 1 : 0));
      var_set_num(vm, "SMX_CAVA_SHEATH_LATCH", (long)(cava_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(cava_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_CAVA", (long)(cava_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(cava_ok ? 1 : 0));
      var_set_num(vm, "SMX_CAVA_WRAP_LATCH", (long)(cava_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(cava_ok ? wraps : 0));
      var_set_num(vm, "SMX_CAVA_WRAP", (long)(cava_ok ? wraps : 0));
      var_set_num(vm, "SMX_CAVA_WRAPS", (long)(cava_ok ? wraps : 0));
      var_set_num(vm, "SMX_CAVA_SHEATHS", (long)(cava_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(cava_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(cava_ok ? wraps : 0));
      var_set_num(vm, "SMX_CAVA_WRAP_LATCH", (long)(cava_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(cava_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(cava_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(cava_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(cava_ok ? 1 : 0));
      var_set_num(vm, "SMX_CV_BONDS", (long)(cava_ok ? bonds : 0));
      var_set_num(vm, "SMX_CV_MESH", (long)(cava_ok ? bonds : 0));
      var_set_num(vm, "SMX_CV_BONDS2", (long)(cava_ok ? bonds : 0));
      var_set_num(vm, "SMX_CV_STARS", (long)(cava_ok ? wraps : 0));
      var_set_num(vm, "SMX_CV_STAR", (long)(cava_ok ? wraps : 0));
      var_set_num(vm, "SMX_CAVA_RING", (long)(cava_ok ? wraps : 0));
      var_set_num(vm, "SMX_CAVA_LANE", (long)(cava_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(cava_ok ? sheaths : 0));
      var_set_num(vm, "SMX_CAVA_SHEATH_HUB", (long)(cava_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(cava_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDCAVA", (long)(cava_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDCAVASHEATH", (long)(cava_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(cava_ok ? live : 0));
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
      if (cava_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CAVA ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CAVA partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX CAVA soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX CAVA nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d cavaed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, cava_ok, vital);
    }
    bump(vm); return 1;
  }
  return 0;
}
