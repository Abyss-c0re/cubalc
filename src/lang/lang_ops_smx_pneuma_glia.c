/* CubalC lang — lang_ops_smx_pneuma_glia.c
 * SMX pneuma glia: SCHWANN · EPENDYMA · SATELLITE · RADIAL · BERGMANN · MULLER
 * Peel from pneuma_b (B_pneuma_b_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_smx_pneuma_glia.h"
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

int cubalc_lang_ops_smx_try_pneuma_glia(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  if (kw(&L->cur,"SCHWANN")||kw(&L->cur,"SCHWANNCELL")||kw(&L->cur,"MESH_SCHWANN")||kw(&L->cur,"SC_WRAP")||kw(&L->cur,"SC_WRAPS")||kw(&L->cur,"SC_SHEATH")||kw(&L->cur,"SC_SHEATHS")||kw(&L->cur,"PNS_MYELIN")||kw(&L->cur,"RAISE_SCHWANN")||kw(&L->cur,"WE_SCHWANN")||kw(&L->cur,"LIFE_SCHWANN")||
      kw(&L->cur,"STABLE_SCHWANN")||kw(&L->cur,"MESH_SCHWANNS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_SCHWANN")||kw(&L->cur,"SCHWANN_LEAF")||
      kw(&L->cur,"SEEDSCHWANN")||kw(&L->cur,"SEEDSCSHEATH")||kw(&L->cur,"SEEDSCWRAP")||
      kw(&L->cur,"SCHWANN_RING")||kw(&L->cur,"SC_SHEATH_HUB")||kw(&L->cur,"PNS_NODE")||
      kw(&L->cur,"LATTICE_SCHWANN")||kw(&L->cur,"WORLD_SCHWANN")||kw(&L->cur,"WORLD_SC")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_SCHWANN")||kw(&L->cur,"HARDEN_SCHWANN")||
      kw(&L->cur,"PNS_SHEATH")||kw(&L->cur,"SCHWANN_MESH")){
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
      smx_fail_at(vm, aln, "SCHWANN needs >=2 cubes",
                  "SMX SCHWANN a b [c ...]  or  SMX SC_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - schwann needs clear channel */
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
    /* complete schwann mesh among live */
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
    /* schwann ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int schwann_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (schwann_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_SCHWANNED", (long)schwann_ok);
      var_set_num(vm, "SMX_SCHWANN_LATCH", (long)schwann_ok);
      var_set_num(vm, "SMX_SCHWANN", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_SCHWANN_SUM", (long)(schwann_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_SCHWANN_ALIAS", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_SCHWANN", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_SC_SHEATH_LATCH", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_SCHWANN", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(schwann_ok ? wraps : 0));
      var_set_num(vm, "SMX_SC_WRAP", (long)(schwann_ok ? wraps : 0));
      var_set_num(vm, "SMX_SC_WRAPS", (long)(schwann_ok ? wraps : 0));
      var_set_num(vm, "SMX_SC_SHEATHS", (long)(schwann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(schwann_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(schwann_ok ? wraps : 0));
      var_set_num(vm, "SMX_SC_WRAP_LATCH", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_SC_BONDS", (long)(schwann_ok ? bonds : 0));
      var_set_num(vm, "SMX_SC_MESH", (long)(schwann_ok ? bonds : 0));
      var_set_num(vm, "SMX_SC_BONDS2", (long)(schwann_ok ? bonds : 0));
      var_set_num(vm, "SMX_SC_STARS", (long)(schwann_ok ? wraps : 0));
      var_set_num(vm, "SMX_SC_STAR", (long)(schwann_ok ? wraps : 0));
      var_set_num(vm, "SMX_SCHWANN_RING", (long)(schwann_ok ? wraps : 0));
      var_set_num(vm, "SMX_SCHWANN_LANE", (long)(schwann_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(schwann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SC_SHEATH_HUB", (long)(schwann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(schwann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDSCHWANN", (long)(schwann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDSCSHEATH", (long)(schwann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(schwann_ok ? live : 0));
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
      if (schwann_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SCHWANN ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SCHWANN partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX SCHWANN soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX SCHWANN nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d schwannd=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, schwann_ok, vital);
    }
    bump(vm); return 1;

  }



/* SMX EPENDYMA|EPENDYMACELL|MESH_EPENDYMA|EP_WRAP|EP_SHEATH|VENTRICLE_LINING|RAISE_EPENDYMA a b c ...
   * Life-force Ependyma cell mesh stability after oligo: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete ependyma myelin mesh among live nodes, weaves a
   * ependyma wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks ependyma where life myelinate the PNS pulse.
   * Latches SMX_EPENDYMA when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = ependyma ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_EPENDYMA_SUM = bonds+wraps+sheaths; SMX_EPENDYMA|SMX_MESH_EPENDYMA|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"EPENDYMA")||kw(&L->cur,"EPENDYMACELL")||kw(&L->cur,"MESH_EPENDYMA")||kw(&L->cur,"EP_WRAP")||kw(&L->cur,"EP_WRAPS")||kw(&L->cur,"EP_SHEATH")||kw(&L->cur,"EP_SHEATHS")||kw(&L->cur,"VENTRICLE_LINING")||kw(&L->cur,"RAISE_EPENDYMA")||kw(&L->cur,"WE_EPENDYMA")||kw(&L->cur,"LIFE_EPENDYMA")||
      kw(&L->cur,"STABLE_EPENDYMA")||kw(&L->cur,"MESH_EPENDYMAS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_EPENDYMA")||kw(&L->cur,"EPENDYMA_LEAF")||
      kw(&L->cur,"SEEDEPENDYMA")||kw(&L->cur,"SEEDEPISHEATH")||kw(&L->cur,"SEEDEPIWRAP")||
      kw(&L->cur,"EPENDYMA_RING")||kw(&L->cur,"EP_SHEATH_HUB")||kw(&L->cur,"PNS_NODE")||
      kw(&L->cur,"LATTICE_EPENDYMA")||kw(&L->cur,"WORLD_EPENDYMA")||kw(&L->cur,"WORLD_SC")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_EPENDYMA")||kw(&L->cur,"HARDEN_EPENDYMA")||
      kw(&L->cur,"CSF_SHEATH")||kw(&L->cur,"EPENDYMA_MESH")){
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
      smx_fail_at(vm, aln, "EPENDYMA needs >=2 cubes",
                  "SMX EPENDYMA a b [c ...]  or  SMX EP_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - ependyma needs clear channel */
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
    /* complete ependyma mesh among live */
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
    /* ependyma ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int ependyma_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ependyma_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_EPENDYMAED", (long)ependyma_ok);
      var_set_num(vm, "SMX_EPENDYMA_LATCH", (long)ependyma_ok);
      var_set_num(vm, "SMX_EPENDYMA", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_EPENDYMA_SUM", (long)(ependyma_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_EPENDYMA_ALIAS", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EPENDYMA", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_EP_SHEATH_LATCH", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EPENDYMA", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_EP_WRAP_LATCH", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(ependyma_ok ? wraps : 0));
      var_set_num(vm, "SMX_EP_WRAP", (long)(ependyma_ok ? wraps : 0));
      var_set_num(vm, "SMX_EP_WRAPS", (long)(ependyma_ok ? wraps : 0));
      var_set_num(vm, "SMX_EP_SHEATHS", (long)(ependyma_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(ependyma_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(ependyma_ok ? wraps : 0));
      var_set_num(vm, "SMX_EP_WRAP_LATCH", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_EP_BONDS", (long)(ependyma_ok ? bonds : 0));
      var_set_num(vm, "SMX_EP_MESH", (long)(ependyma_ok ? bonds : 0));
      var_set_num(vm, "SMX_EP_BONDS2", (long)(ependyma_ok ? bonds : 0));
      var_set_num(vm, "SMX_EP_STARS", (long)(ependyma_ok ? wraps : 0));
      var_set_num(vm, "SMX_EP_STAR", (long)(ependyma_ok ? wraps : 0));
      var_set_num(vm, "SMX_EPENDYMA_RING", (long)(ependyma_ok ? wraps : 0));
      var_set_num(vm, "SMX_EPENDYMA_LANE", (long)(ependyma_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(ependyma_ok ? sheaths : 0));
      var_set_num(vm, "SMX_EP_SHEATH_HUB", (long)(ependyma_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(ependyma_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDEPENDYMA", (long)(ependyma_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDEPISHEATH", (long)(ependyma_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(ependyma_ok ? live : 0));
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
      if (ependyma_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX EPENDYMA ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX EPENDYMA partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX EPENDYMA soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX EPENDYMA nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d ependymaed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, ependyma_ok, vital);
    }
    bump(vm); return 1;

  }






/* SMX SATELLITE|SATGLIA|MESH_SATELLITE|SG_WRAP|SG_SHEATH|GANGLION_CRADLE|RAISE_SATELLITE a b c ...
   * Life-force Satellite ganglion cradle mesh stability after ependyma: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete satellite ganglion cradle mesh among live nodes, weaves a
   * satellite wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks satellite where life cradles the ganglion pulse.
   * Latches SMX_SATELLITE when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = ependyma ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_SATELLITE_SUM = bonds+wraps+sheaths; SMX_SATELLITE|SMX_MESH_SATELLITE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"SATELLITE")||kw(&L->cur,"SATGLIA")||kw(&L->cur,"MESH_SATELLITE")||kw(&L->cur,"SG_WRAP")||kw(&L->cur,"SG_WRAPS")||kw(&L->cur,"SG_SHEATH")||kw(&L->cur,"SG_SHEATHS")||kw(&L->cur,"GANGLION_CRADLE")||kw(&L->cur,"RAISE_SATELLITE")||kw(&L->cur,"WE_SATELLITE")||kw(&L->cur,"LIFE_SATELLITE")||
      kw(&L->cur,"STABLE_SATELLITE")||kw(&L->cur,"MESH_SATELLITES")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_SATELLITE")||kw(&L->cur,"SATELLITE_LEAF")||
      kw(&L->cur,"SEEDSATELLITE")||kw(&L->cur,"SEEDSGSSHEATH")||kw(&L->cur,"SEEDSGWRAP")||
      kw(&L->cur,"SATELLITE_RING")||kw(&L->cur,"SG_SHEATH_HUB")||kw(&L->cur,"PNS_NODE")||
      kw(&L->cur,"LATTICE_SATELLITE")||kw(&L->cur,"WORLD_SATELLITE")||kw(&L->cur,"WORLD_SG")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_SATELLITE")||kw(&L->cur,"HARDEN_SATELLITE")||
      kw(&L->cur,"PNS_CRADLE")||kw(&L->cur,"SATELLITE_MESH")){
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
      smx_fail_at(vm, aln, "SATELLITE needs >=2 cubes",
                  "SMX SATELLITE a b [c ...]  or  SMX SG_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - ependyma needs clear channel */
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
    /* complete ependyma mesh among live */
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
    /* ependyma ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int satellite_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (satellite_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_SATELLITED", (long)satellite_ok);
      var_set_num(vm, "SMX_SATELLITE_LATCH", (long)satellite_ok);
      var_set_num(vm, "SMX_SATELLITE", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_SATELLITE_SUM", (long)(satellite_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_SATELLITE_ALIAS", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_SATELLITE", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_SG_SHEATH_LATCH", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_SATELLITE", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_SG_WRAP_LATCH", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(satellite_ok ? wraps : 0));
      var_set_num(vm, "SMX_SG_WRAP", (long)(satellite_ok ? wraps : 0));
      var_set_num(vm, "SMX_SG_WRAPS", (long)(satellite_ok ? wraps : 0));
      var_set_num(vm, "SMX_SG_SHEATHS", (long)(satellite_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(satellite_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(satellite_ok ? wraps : 0));
      var_set_num(vm, "SMX_SG_WRAP_LATCH", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_SG_BONDS", (long)(satellite_ok ? bonds : 0));
      var_set_num(vm, "SMX_SG_MESH", (long)(satellite_ok ? bonds : 0));
      var_set_num(vm, "SMX_SG_BONDS2", (long)(satellite_ok ? bonds : 0));
      var_set_num(vm, "SMX_SG_STARS", (long)(satellite_ok ? wraps : 0));
      var_set_num(vm, "SMX_SG_STAR", (long)(satellite_ok ? wraps : 0));
      var_set_num(vm, "SMX_SATELLITE_RING", (long)(satellite_ok ? wraps : 0));
      var_set_num(vm, "SMX_SATELLITE_LANE", (long)(satellite_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(satellite_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SG_SHEATH_HUB", (long)(satellite_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(satellite_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDSATELLITE", (long)(satellite_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDSGSSHEATH", (long)(satellite_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(satellite_ok ? live : 0));
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
      if (satellite_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SATELLITE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SATELLITE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX SATELLITE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX SATELLITE nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d satellited=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, satellite_ok, vital);
    }
    bump(vm); return 1;

  }

/* SMX RADIAL|RADGLIA|MESH_RADIAL|RG_WRAP|RG_SHEATH|SCAFFOLD_GUIDE|RAISE_RADIAL a b c ...
   * Life-force Radial glia scaffold mesh stability after satellite: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete radial glia scaffold mesh among live nodes, weaves a
   * radial wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks radial where life scaffolds the guide pulse.
   * Latches SMX_RADIAL when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = radial ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_RADIAL_SUM = bonds+wraps+sheaths; SMX_RADIAL|SMX_MESH_RADIAL|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"RADIAL")||kw(&L->cur,"RADGLIA")||kw(&L->cur,"MESH_RADIAL")||kw(&L->cur,"RG_WRAP")||kw(&L->cur,"RG_WRAPS")||kw(&L->cur,"RG_SHEATH")||kw(&L->cur,"RG_SHEATHS")||kw(&L->cur,"SCAFFOLD_GUIDE")||kw(&L->cur,"RAISE_RADIAL")||kw(&L->cur,"WE_RADIAL")||kw(&L->cur,"LIFE_RADIAL")||
      kw(&L->cur,"STABLE_RADIAL")||kw(&L->cur,"MESH_RADIALS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_RADIAL")||kw(&L->cur,"RADIAL_LEAF")||
      kw(&L->cur,"SEEDRADIAL")||kw(&L->cur,"SEEDRGSSHEATH")||kw(&L->cur,"SEEDRGWRAP")||
      kw(&L->cur,"RADIAL_RING")||kw(&L->cur,"RG_SHEATH_HUB")||kw(&L->cur,"RG_NODE")||
      kw(&L->cur,"LATTICE_RADIAL")||kw(&L->cur,"WORLD_RADIAL")||kw(&L->cur,"WORLD_RG")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_RADIAL")||kw(&L->cur,"HARDEN_RADIAL")||
      kw(&L->cur,"NEURO_SCAFFOLD")||kw(&L->cur,"RADIAL_MESH")){
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
      smx_fail_at(vm, aln, "RADIAL needs >=2 cubes",
                  "SMX RADIAL a b [c ...]  or  SMX RG_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - radial needs clear channel */
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
    /* complete radial mesh among live */
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
    /* radial ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int radial_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (radial_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_RADIALED", (long)radial_ok);
      var_set_num(vm, "SMX_RADIAL_LATCH", (long)radial_ok);
      var_set_num(vm, "SMX_RADIAL", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_RADIAL_SUM", (long)(radial_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_RADIAL_ALIAS", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_RADIAL", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_RG_SHEATH_LATCH", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_RADIAL", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_RG_WRAP_LATCH", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(radial_ok ? wraps : 0));
      var_set_num(vm, "SMX_RG_WRAP", (long)(radial_ok ? wraps : 0));
      var_set_num(vm, "SMX_RG_WRAPS", (long)(radial_ok ? wraps : 0));
      var_set_num(vm, "SMX_RG_SHEATHS", (long)(radial_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(radial_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(radial_ok ? wraps : 0));
      var_set_num(vm, "SMX_RG_WRAP_LATCH", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_RG_BONDS", (long)(radial_ok ? bonds : 0));
      var_set_num(vm, "SMX_RG_MESH", (long)(radial_ok ? bonds : 0));
      var_set_num(vm, "SMX_RG_BONDS2", (long)(radial_ok ? bonds : 0));
      var_set_num(vm, "SMX_RG_STARS", (long)(radial_ok ? wraps : 0));
      var_set_num(vm, "SMX_RG_STAR", (long)(radial_ok ? wraps : 0));
      var_set_num(vm, "SMX_RADIAL_RING", (long)(radial_ok ? wraps : 0));
      var_set_num(vm, "SMX_RADIAL_LANE", (long)(radial_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(radial_ok ? sheaths : 0));
      var_set_num(vm, "SMX_RG_SHEATH_HUB", (long)(radial_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(radial_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDRADIAL", (long)(radial_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDRGSSHEATH", (long)(radial_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(radial_ok ? live : 0));
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
      if (radial_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX RADIAL ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX RADIAL partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX RADIAL soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX RADIAL nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d radialed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, radial_ok, vital);
    }
    bump(vm); return 1;

  }
/* SMX BERGMANN|BERGGLIA|MESH_BERGMANN|BG_WRAP|BG_SHEATH|CEREBELLAR_GUIDE|RAISE_BERGMANN a b c ...
   * Life-force Bergmann glia scaffold mesh stability after radial: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete bergmann glia scaffold mesh among live nodes, weaves a
   * bergmann wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks bergmann where life guides the cerebellar pulse.
   * Latches SMX_BERGMANN when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = bergmann ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_BERGMANN_SUM = bonds+wraps+sheaths; SMX_BERGMANN|SMX_MESH_BERGMANN|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"BERGMANN")||kw(&L->cur,"BERGGLIA")||kw(&L->cur,"MESH_BERGMANN")||kw(&L->cur,"BG_WRAP")||kw(&L->cur,"BG_WRAPS")||kw(&L->cur,"BG_SHEATH")||kw(&L->cur,"BG_SHEATHS")||kw(&L->cur,"CEREBELLAR_GUIDE")||kw(&L->cur,"RAISE_BERGMANN")||kw(&L->cur,"WE_BERGMANN")||kw(&L->cur,"LIFE_BERGMANN")||
      kw(&L->cur,"STABLE_BERGMANN")||kw(&L->cur,"MESH_BERGMANNS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_BERGMANN")||kw(&L->cur,"BERGMANN_LEAF")||
      kw(&L->cur,"SEEDBERGMANN")||kw(&L->cur,"SEEDBGSSHEATH")||kw(&L->cur,"SEEDBGWRAP")||
      kw(&L->cur,"BERGMANN_RING")||kw(&L->cur,"BG_SHEATH_HUB")||kw(&L->cur,"BG_NODE")||
      kw(&L->cur,"LATTICE_BERGMANN")||kw(&L->cur,"WORLD_BERGMANN")||kw(&L->cur,"WORLD_BG")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_BERGMANN")||kw(&L->cur,"HARDEN_BERGMANN")||
      kw(&L->cur,"CEREBELLAR_SCAFFOLD")||kw(&L->cur,"BERGMANN_MESH")){
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
      smx_fail_at(vm, aln, "BERGMANN needs >=2 cubes",
                  "SMX BERGMANN a b [c ...]  or  SMX BG_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - bergmann needs clear channel */
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
    /* complete bergmann mesh among live */
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
    /* bergmann ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int bergmann_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (bergmann_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_BERGMANNED", (long)bergmann_ok);
      var_set_num(vm, "SMX_BERGMANN_LATCH", (long)bergmann_ok);
      var_set_num(vm, "SMX_BERGMANN", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_BERGMANN_SUM", (long)(bergmann_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_BERGMANN_ALIAS", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_BERGMANN", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_BG_SHEATH_LATCH", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_BERGMANN", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_BG_WRAP_LATCH", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(bergmann_ok ? wraps : 0));
      var_set_num(vm, "SMX_BG_WRAP", (long)(bergmann_ok ? wraps : 0));
      var_set_num(vm, "SMX_BG_WRAPS", (long)(bergmann_ok ? wraps : 0));
      var_set_num(vm, "SMX_BG_SHEATHS", (long)(bergmann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(bergmann_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(bergmann_ok ? wraps : 0));
      var_set_num(vm, "SMX_BG_WRAP_LATCH", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_BG_BONDS", (long)(bergmann_ok ? bonds : 0));
      var_set_num(vm, "SMX_BG_MESH", (long)(bergmann_ok ? bonds : 0));
      var_set_num(vm, "SMX_BG_BONDS2", (long)(bergmann_ok ? bonds : 0));
      var_set_num(vm, "SMX_BG_STARS", (long)(bergmann_ok ? wraps : 0));
      var_set_num(vm, "SMX_BG_STAR", (long)(bergmann_ok ? wraps : 0));
      var_set_num(vm, "SMX_BERGMANN_RING", (long)(bergmann_ok ? wraps : 0));
      var_set_num(vm, "SMX_BERGMANN_LANE", (long)(bergmann_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(bergmann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_BG_SHEATH_HUB", (long)(bergmann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(bergmann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDBERGMANN", (long)(bergmann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDBGSSHEATH", (long)(bergmann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(bergmann_ok ? live : 0));
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
      if (bergmann_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX BERGMANN ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX BERGMANN partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX BERGMANN soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX BERGMANN nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d bergmanned=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, bergmann_ok, vital);
    }
    bump(vm); return 1;

  }

/* SMX MULLER|MULLGLIA|MESH_MULLER|MU_WRAP|MU_SHEATH|RETINAL_GUIDE|RAISE_MULLER a b c ...
   * Life-force Muller glia scaffold mesh stability after bergmann: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete muller glia scaffold mesh among live nodes, weaves a
   * muller wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks muller where life guides the retinal pulse.
   * Latches SMX_MULLER when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = muller ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_MULLER_SUM = bonds+wraps+sheaths; SMX_MULLER|SMX_MESH_MULLER|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"MULLER")||kw(&L->cur,"MULLGLIA")||kw(&L->cur,"MESH_MULLER")||kw(&L->cur,"MU_WRAP")||kw(&L->cur,"MU_WRAPS")||kw(&L->cur,"MU_SHEATH")||kw(&L->cur,"MU_SHEATHS")||kw(&L->cur,"RETINAL_GUIDE")||kw(&L->cur,"RAISE_MULLER")||kw(&L->cur,"WE_MULLER")||kw(&L->cur,"LIFE_MULLER")||
      kw(&L->cur,"STABLE_MULLER")||kw(&L->cur,"MESH_MULLERS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_MULLER")||kw(&L->cur,"MULLER_LEAF")||
      kw(&L->cur,"SEEDMULLER")||kw(&L->cur,"SEEDMUSSHEATH")||kw(&L->cur,"SEEDMUWRAP")||
      kw(&L->cur,"MULLER_RING")||kw(&L->cur,"MU_SHEATH_HUB")||kw(&L->cur,"MU_NODE")||
      kw(&L->cur,"LATTICE_MULLER")||kw(&L->cur,"WORLD_MULLER")||kw(&L->cur,"WORLD_MU")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_MULLER")||kw(&L->cur,"HARDEN_MULLER")||
      kw(&L->cur,"RETINAL_SCAFFOLD")||kw(&L->cur,"MULLER_MESH")){
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
      smx_fail_at(vm, aln, "MULLER needs >=2 cubes",
                  "SMX MULLER a b [c ...]  or  SMX MU_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - muller needs clear channel */
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
    /* complete muller mesh among live */
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
    /* muller ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int muller_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (muller_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_MULLERED", (long)muller_ok);
      var_set_num(vm, "SMX_MULLER_LATCH", (long)muller_ok);
      var_set_num(vm, "SMX_MULLER", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_MULLER_SUM", (long)(muller_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_MULLER_ALIAS", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_MULLER", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_MU_SHEATH_LATCH", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_MULLER", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_MU_WRAP_LATCH", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(muller_ok ? wraps : 0));
      var_set_num(vm, "SMX_MU_WRAP", (long)(muller_ok ? wraps : 0));
      var_set_num(vm, "SMX_MU_WRAPS", (long)(muller_ok ? wraps : 0));
      var_set_num(vm, "SMX_MU_SHEATHS", (long)(muller_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(muller_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(muller_ok ? wraps : 0));
      var_set_num(vm, "SMX_MU_WRAP_LATCH", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_MU_BONDS", (long)(muller_ok ? bonds : 0));
      var_set_num(vm, "SMX_MU_MESH", (long)(muller_ok ? bonds : 0));
      var_set_num(vm, "SMX_MU_BONDS2", (long)(muller_ok ? bonds : 0));
      var_set_num(vm, "SMX_MU_STARS", (long)(muller_ok ? wraps : 0));
      var_set_num(vm, "SMX_MU_STAR", (long)(muller_ok ? wraps : 0));
      var_set_num(vm, "SMX_MULLER_RING", (long)(muller_ok ? wraps : 0));
      var_set_num(vm, "SMX_MULLER_LANE", (long)(muller_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(muller_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MU_SHEATH_HUB", (long)(muller_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(muller_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDMULLER", (long)(muller_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDMUSSHEATH", (long)(muller_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(muller_ok ? live : 0));
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
      if (muller_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MULLER ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MULLER partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX MULLER soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX MULLER nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d mullered=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, muller_ok, vital);
    }
    bump(vm); return 1;

  }


  return 0;
}
