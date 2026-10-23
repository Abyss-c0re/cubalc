/* CubalC lang — lang_ops_smx_pneuma_neural.c
 * SMX pneuma neural: THALAMUS · CORTEX · SYNAPSE · AXON · DENDRITE · GLIA · ASTROCYTE · MICROGLIA · OLIGO
 * Peel from pneuma_a (B_pneuma_peel). Cube is SoT. Zero bloat.
 */
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

int cubalc_lang_ops_smx_try_pneuma_neural(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* SMX THALAMUS|PULSEGATE|STABILITY|STABLE_MESH|MESH_THALAMUS|RAISE_THALAMUS a b c ...
   * Life-force thalamus mesh stability after keeper: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete thalamus mesh among live nodes, weaves an
   * stability ring (i -> i+1) so free energy gates every edge, then gate hub
   * gathers return so lattice locks thalamus where life holds the pulse gate.
   * Latches SMX_THALAMUS when mesh+stables+gates are soft-OOB-free.
   * SMX_STABLES = stability ring; SMX_GATES hub = root gather pulses;
   * SMX_THAL_SUM = bonds+stables+gates; SMX_THALAMUS|SMX_PULSEGATE|SMX_MESH_STABLE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"THALAMUS")||kw(&L->cur,"PULSEGATE")||kw(&L->cur,"STABILITY")||kw(&L->cur,"STABLE_MESH")||kw(&L->cur,"MESH_STABLE")||kw(&L->cur,"WE_THALAMUS")||kw(&L->cur,"LIFE_THALAMUS")||
      kw(&L->cur,"THALS_ALT")||kw(&L->cur,"PULSEGATES")||kw(&L->cur,"TH")||
      kw(&L->cur,"STABLE")||kw(&L->cur,"STABLES")||kw(&L->cur,"GATE")||
      kw(&L->cur,"GATES")||kw(&L->cur,"SEEDTHALAMUS")||kw(&L->cur,"SEEDGATE")||
      kw(&L->cur,"STABILITY_RING")||kw(&L->cur,"GATE_HUB")||
      kw(&L->cur,"MESH_THALAMUS")||kw(&L->cur,"RAISE_THALAMUS")||
      kw(&L->cur,"THALAMUSES")||kw(&L->cur,"THALS_ALT")||
      kw(&L->cur,"SEEDTHALAMUS2")||kw(&L->cur,"SEEDGATE2")||
      kw(&L->cur,"LATTICE_THALAMUS")||kw(&L->cur,"WORLD_THALAMUS")||kw(&L->cur,"WORLD_THAL")||
      kw(&L->cur,"STABLE_RING")||kw(&L->cur,"PULSE_GATE")||kw(&L->cur,"PULSE_THALAMUS")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int stables = 0;
    int gates = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "THALAMUS needs >=2 cubes",
                  "SMX THALAMUS a b [c ...]  or  SMX STABILITY a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - thalamus needs clear channel */
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
    /* complete keeper mesh among live */
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
    /* stability ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else stables++;
        }
      }
    }
    /* gate hub - seed axis return from every live leaf */
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
          else gates++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int path_ok = (live >= 2 && stables >= live && soft == 0) ? 1 : 0;
      if (!path_ok && live >= 2 && stables * 2 >= live && soft == 0)
        path_ok = 1;
      int hub_ok = (live >= 1 && gates >= live && soft == 0) ? 1 : 0;
      if (!hub_ok && live >= 1 && gates * 2 >= live && soft == 0)
        hub_ok = 1;
      int th_ok = (mesh_ok && path_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (th_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (stables > 0 ? 1 : 0) + (gates > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_THALAMUSED", (long)th_ok);
      var_set_num(vm, "SMX_THALAMUS_LATCH", (long)th_ok);
      var_set_num(vm, "SMX_THALAMUS", (long)(th_ok ? 1 : 0));
      var_set_num(vm, "SMX_THAL_SUM", (long)(th_ok ? bonds + stables + gates : 0));
      var_set_num(vm, "SMX_THALAMUS_ALIAS", (long)(th_ok ? 1 : 0));
      var_set_num(vm, "SMX_PULSEGATE", (long)(th_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_STABLE", (long)(th_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(th_ok ? 1 : 0));
      var_set_num(vm, "SMX_GATE", (long)(th_ok ? 1 : 0));
      var_set_num(vm, "SMX_THALS_ALT", (long)(th_ok ? bonds : 0));
      var_set_num(vm, "SMX_THALS", (long)(th_ok ? bonds : 0));
      var_set_num(vm, "SMX_THALS_ALT2", (long)(th_ok ? bonds : 0));
      var_set_num(vm, "SMX_STABLES", (long)(th_ok ? stables : 0));
      var_set_num(vm, "SMX_STABILITY", (long)(th_ok ? stables : 0));
      var_set_num(vm, "SMX_STABILITY_RING", (long)(th_ok ? stables : 0));
      var_set_num(vm, "SMX_STABLE_LANE", (long)(th_ok ? stables : 0));
      var_set_num(vm, "SMX_GATES", (long)(th_ok ? gates : 0));
      var_set_num(vm, "SMX_GATE_HUB", (long)(th_ok ? gates : 0));
      var_set_num(vm, "SMX_GATE_N", (long)(th_ok ? gates : 0));
      var_set_num(vm, "SMX_SEEDTHALAMUS", (long)(th_ok ? gates : 0));
      var_set_num(vm, "SMX_SEEDGATE", (long)(th_ok ? gates : 0));
      var_set_num(vm, "SMX_MESH", (long)(th_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + stables + gates));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (th_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX THALAMUS ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX THALAMUS partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX THALAMUS soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX THALAMUS nodes=%d live=%d bonds=%d stables=%d gates=%d need=%d soft=%d talks=%d oob=%d thalamused=%d vital=%ld\n",
                n, live, bonds, stables, gates, need, soft, vm->smx_talks, vm->smx_oob, th_ok, vital);
    }
    bump(vm); return 1;
  }

/* SMX CORTEX|PULSEHOLD|SIGNAL|MESH_CORTEX|MESH_CORTEX|RAISE_CORTEX a b c ...
   * Life-force cortex mesh stability after keeper: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete cortex mesh among live nodes, weaves an
   * signal ring (i -> i+1) so free energy holds every edge, then hold hub
   * gathers return so lattice locks cortex where life holds the signal pulse.
   * Latches SMX_CORTEX when mesh+signals+holds are soft-OOB-free.
   * SMX_SIGNALS = signal ring; SMX_HOLDS hub = root gather pulses;
   * SMX_CORTEX_SUM = bonds+signals+holds; SMX_CORTEX|SMX_PULSEHOLD|SMX_STABLE_CORTEX sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"CORTEX")||kw(&L->cur,"PULSEHOLD")||kw(&L->cur,"SIGNAL")||kw(&L->cur,"MESH_CORTEX")||kw(&L->cur,"STABLE_CORTEX")||kw(&L->cur,"WE_CORTEX")||kw(&L->cur,"LIFE_CORTEX")||
      kw(&L->cur,"CX_ALT")||kw(&L->cur,"PULSEHOLDS")||kw(&L->cur,"TH")||
      kw(&L->cur,"STABLE")||kw(&L->cur,"SIGNALS")||kw(&L->cur,"HOLD")||
      kw(&L->cur,"HOLDS")||kw(&L->cur,"SEEDCORTEX")||kw(&L->cur,"SEEDHOLD")||
      kw(&L->cur,"SIGNAL_RING")||kw(&L->cur,"HOLD_HUB")||
      kw(&L->cur,"MESH_CORTEX")||kw(&L->cur,"RAISE_CORTEX")||
      kw(&L->cur,"CORTEXES")||kw(&L->cur,"CX_ALT")||
      kw(&L->cur,"SEEDCORTEX2")||kw(&L->cur,"SEEDSIGNAL2")||
      kw(&L->cur,"LATTICE_CORTEX")||kw(&L->cur,"WORLD_CORTEX")||kw(&L->cur,"WORLD_THAL")||
      kw(&L->cur,"SIGNAL_RING")||kw(&L->cur,"PULSE_HOLD")||kw(&L->cur,"PULSE_CORTEX")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int signals = 0;
    int holds = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "CORTEX needs >=2 cubes",
                  "SMX CORTEX a b [c ...]  or  SMX SIGNAL a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - cortex needs clear channel */
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
    /* complete keeper mesh among live */
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
    /* signal ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else signals++;
        }
      }
    }
    /* hold hub - seed axis return from every live leaf */
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
          else holds++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int pacx_ok = (live >= 2 && signals >= live && soft == 0) ? 1 : 0;
      if (!pacx_ok && live >= 2 && signals * 2 >= live && soft == 0)
        pacx_ok = 1;
      int hub_ok = (live >= 1 && holds >= live && soft == 0) ? 1 : 0;
      if (!hub_ok && live >= 1 && holds * 2 >= live && soft == 0)
        hub_ok = 1;
      int cx_ok = (mesh_ok && pacx_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (cx_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (signals > 0 ? 1 : 0) + (holds > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_CORTEXED", (long)cx_ok);
      var_set_num(vm, "SMX_CORTEX_LATCH", (long)cx_ok);
      var_set_num(vm, "SMX_CORTEX", (long)(cx_ok ? 1 : 0));
      var_set_num(vm, "SMX_CORTEX_SUM", (long)(cx_ok ? bonds + signals + holds : 0));
      var_set_num(vm, "SMX_CORTEX_ALIAS", (long)(cx_ok ? 1 : 0));
      var_set_num(vm, "SMX_PULSEHOLD", (long)(cx_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_CORTEX", (long)(cx_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_CORTEX", (long)(cx_ok ? 1 : 0));
      var_set_num(vm, "SMX_HOLD_LATCH", (long)(cx_ok ? 1 : 0));
      var_set_num(vm, "SMX_CX_ALT", (long)(cx_ok ? bonds : 0));
      var_set_num(vm, "SMX_CXS", (long)(cx_ok ? bonds : 0));
      var_set_num(vm, "SMX_CX_ALT2", (long)(cx_ok ? bonds : 0));
      var_set_num(vm, "SMX_SIGNALS", (long)(cx_ok ? signals : 0));
      var_set_num(vm, "SMX_SIGNAL", (long)(cx_ok ? signals : 0));
      var_set_num(vm, "SMX_SIGNAL_RING", (long)(cx_ok ? signals : 0));
      var_set_num(vm, "SMX_SIGNAL_LANE", (long)(cx_ok ? signals : 0));
      var_set_num(vm, "SMX_HOLDS", (long)(cx_ok ? holds : 0));
      var_set_num(vm, "SMX_HOLD_HUB", (long)(cx_ok ? holds : 0));
      var_set_num(vm, "SMX_HOLD_N", (long)(cx_ok ? holds : 0));
      var_set_num(vm, "SMX_SEEDCORTEX", (long)(cx_ok ? holds : 0));
      var_set_num(vm, "SMX_SEEDHOLD", (long)(cx_ok ? holds : 0));
      var_set_num(vm, "SMX_MESH", (long)(cx_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + signals + holds));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (cx_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CORTEX ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CORTEX partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX CORTEX soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX CORTEX nodes=%d live=%d bonds=%d signals=%d holds=%d need=%d soft=%d talks=%d oob=%d cortexed=%d vital=%ld\n",
                n, live, bonds, signals, holds, need, soft, vm->smx_talks, vm->smx_oob, cx_ok, vital);
    }
    bump(vm); return 1;

  }

/* SMX SYNAPSE|MESH_EXCHANGE|EXRING|MESH_SYNAPSE|MESH_SYNAPSE|RAISE_SYNAPSE a b c ...
   * Life-force synapse mesh stability after cortex: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete synapse mesh among live nodes, weaves an
   * exchange ring (i -> i+1) so free energy nexi every edge, then hold hub
   * gathers return so lattice locks synapse where life nexi the exchange pulse.
   * Latches SMX_SYNAPSE when mesh+exring+nexi are soft-OOB-free.
   * SMX_EXRINGS = exchange ring; SMX_NEXI hub = root gather pulses;
   * SMX_SYNAPSE_SUM = bonds+exring+nexi; SMX_SYNAPSE|SMX_MESH_EXCHANGE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"SYNAPSE")||kw(&L->cur,"MESH_EXCHANGE")||kw(&L->cur,"EXRING")||kw(&L->cur,"MESH_SYNAPSE")||kw(&L->cur,"STABLE_SYNAPSE")||kw(&L->cur,"WE_SYNAPSE")||kw(&L->cur,"LIFE_SYNAPSE")||
      kw(&L->cur,"SY_ALT")||kw(&L->cur,"MESH_EXCHANGES")||kw(&L->cur,"SY")||
      kw(&L->cur,"STABLE_MESH_X")||kw(&L->cur,"EXRINGS")||kw(&L->cur,"NEXUS_LEAF")||
      kw(&L->cur,"NEXI")||kw(&L->cur,"SEEDSYNAPSE")||kw(&L->cur,"SEEDNEXUS")||
      kw(&L->cur,"EXCHANGE_RING")||kw(&L->cur,"NEXUS_HUB_MESH")||
      kw(&L->cur,"MESH_SYNAPSE")||kw(&L->cur,"RAISE_SYNAPSE")||
      kw(&L->cur,"SYNAPSEES")||kw(&L->cur,"SY_ALT")||
      kw(&L->cur,"SEEDSYNAPSE2")||kw(&L->cur,"SEEDEXRING2")||
      kw(&L->cur,"LATTICE_SYNAPSE")||kw(&L->cur,"WORLD_SYNAPSE")||kw(&L->cur,"WORLD_SY")||
      kw(&L->cur,"EXCHANGE_RING")||kw(&L->cur,"PULSE_EXRING")||kw(&L->cur,"PULSE_SYNAPSE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int exring = 0;
    int nexi = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "SYNAPSE needs >=2 cubes",
                  "SMX SYNAPSE a b [c ...]  or  SMX EXRING a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - synapse needs clear channel */
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
    /* complete synapse mesh among live */
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
    /* exchange ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else exring++;
        }
      }
    }
    /* hold hub - seed axis return from every live leaf */
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
          else nexi++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int ring_ok = (live >= 2 && exring >= live && soft == 0) ? 1 : 0;
      if (!ring_ok && live >= 2 && exring * 2 >= live && soft == 0)
        ring_ok = 1;
      int hub_ok = (live >= 1 && nexi >= live && soft == 0) ? 1 : 0;
      if (!hub_ok && live >= 1 && nexi * 2 >= live && soft == 0)
        hub_ok = 1;
      int sy_ok = (mesh_ok && ring_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (sy_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (exring > 0 ? 1 : 0) + (nexi > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_SYNAPSED", (long)sy_ok);
      var_set_num(vm, "SMX_SYNAPSE_LATCH", (long)sy_ok);
      var_set_num(vm, "SMX_SYNAPSE", (long)(sy_ok ? 1 : 0));
      var_set_num(vm, "SMX_SYNAPSE_SUM", (long)(sy_ok ? bonds + exring + nexi : 0));
      var_set_num(vm, "SMX_SYNAPSE_ALIAS", (long)(sy_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(sy_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(sy_ok ? 1 : 0));
      var_set_num(vm, "SMX_NEXUS_MESH", (long)(sy_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(sy_ok ? 1 : 0));
      var_set_num(vm, "SMX_SY_ALT", (long)(sy_ok ? bonds : 0));
      var_set_num(vm, "SMX_SYS", (long)(sy_ok ? bonds : 0));
      var_set_num(vm, "SMX_SY_ALT2", (long)(sy_ok ? bonds : 0));
      var_set_num(vm, "SMX_EXRINGS", (long)(sy_ok ? exring : 0));
      var_set_num(vm, "SMX_EXRING", (long)(sy_ok ? exring : 0));
      var_set_num(vm, "SMX_EXCHANGE_RING", (long)(sy_ok ? exring : 0));
      var_set_num(vm, "SMX_EXCHANGE_LANE", (long)(sy_ok ? exring : 0));
      var_set_num(vm, "SMX_NEXI", (long)(sy_ok ? nexi : 0));
      var_set_num(vm, "SMX_NEXUS_HUB_MESH", (long)(sy_ok ? nexi : 0));
      var_set_num(vm, "SMX_NEXUS_N", (long)(sy_ok ? nexi : 0));
      var_set_num(vm, "SMX_SEEDSYNAPSE", (long)(sy_ok ? nexi : 0));
      var_set_num(vm, "SMX_SEEDNEXUS", (long)(sy_ok ? nexi : 0));
      var_set_num(vm, "SMX_MESH", (long)(sy_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + exring + nexi));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (sy_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SYNAPSE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SYNAPSE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX SYNAPSE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX SYNAPSE nodes=%d live=%d bonds=%d exring=%d nexi=%d need=%d soft=%d talks=%d oob=%d synapsed=%d vital=%ld\n",
                n, live, bonds, exring, nexi, need, soft, vm->smx_talks, vm->smx_oob, sy_ok, vital);
    }
    bump(vm); return 1;

  }

/* SMX AXON|MESH_AXON|TRACT|MESH_AXON|MESH_AXON|RAISE_AXON a b c ...
   * Life-force axon mesh stability after synapse: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete axon mesh among live nodes, weaves an
   * axon ring (i -> i+1) so free energy myelin every edge, then myelin sheath
   * gathers return so lattice locks axon where life myelin the axon pulse.
   * Latches SMX_AXON when mesh+tracts+myelin are soft-OOB-free.
   * SMX_TRACTS = axon ring; SMX_MYELINS hub = root gather pulses;
   * SMX_AXON_SUM = bonds+tracts+myelin; SMX_AXON|SMX_MESH_AXON|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"AXON")||kw(&L->cur,"MESH_AXON")||kw(&L->cur,"TRACT")||kw(&L->cur,"MESH_AXON")||kw(&L->cur,"STABLE_AXON")||kw(&L->cur,"WE_AXON")||kw(&L->cur,"LIFE_AXON")||
      kw(&L->cur,"AX_ALT")||kw(&L->cur,"MESH_AXONS")||kw(&L->cur,"AX")||
      kw(&L->cur,"STABLE_MESH_AX")||kw(&L->cur,"TRACTS")||kw(&L->cur,"AXON_LEAF")||
      kw(&L->cur,"MYELINS")||kw(&L->cur,"SEEDAXON")||kw(&L->cur,"SEEDMYELIN")||
      kw(&L->cur,"AXON_RING")||kw(&L->cur,"MYELIN_HUB")||
      kw(&L->cur,"MESH_AXON")||kw(&L->cur,"RAISE_AXON")||
      kw(&L->cur,"AXONES")||kw(&L->cur,"AX_ALT")||
      kw(&L->cur,"SEEDAXON2")||kw(&L->cur,"SEEDTRACT2")||
      kw(&L->cur,"LATTICE_AXON")||kw(&L->cur,"WORLD_AXON")||kw(&L->cur,"WORLD_AX")||
      kw(&L->cur,"AXON_RING")||kw(&L->cur,"PULSE_TRACT")||kw(&L->cur,"PULSE_AXON")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int tracts = 0;
    int myelin = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "AXON needs >=2 cubes",
                  "SMX AXON a b [c ...]  or  SMX MYELIN a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - axon needs clear channel */
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
    /* complete axon mesh among live */
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
    /* axon ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else tracts++;
        }
      }
    }
    /* myelin sheath - seed axis return from every live leaf */
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
          else myelin++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int tract_ok = (live >= 2 && tracts >= live && soft == 0) ? 1 : 0;
      if (!tract_ok && live >= 2 && tracts * 2 >= live && soft == 0)
        tract_ok = 1;
      int sheath_ok = (live >= 1 && myelin >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && myelin * 2 >= live && soft == 0)
        sheath_ok = 1;
      int ax_ok = (mesh_ok && tract_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ax_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (tracts > 0 ? 1 : 0) + (myelin > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_AXONED", (long)ax_ok);
      var_set_num(vm, "SMX_AXON_LATCH", (long)ax_ok);
      var_set_num(vm, "SMX_AXON", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_AXON_SUM", (long)(ax_ok ? bonds + tracts + myelin : 0));
      var_set_num(vm, "SMX_AXON_ALIAS", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_AXON", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_MYELIN_LATCH", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_AXON", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_MYELIN", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_AX_ALT", (long)(ax_ok ? bonds : 0));
      var_set_num(vm, "SMX_AXS", (long)(ax_ok ? bonds : 0));
      var_set_num(vm, "SMX_AX_ALT2", (long)(ax_ok ? bonds : 0));
      var_set_num(vm, "SMX_TRACTS", (long)(ax_ok ? tracts : 0));
      var_set_num(vm, "SMX_TRACT", (long)(ax_ok ? tracts : 0));
      var_set_num(vm, "SMX_AXON_RING", (long)(ax_ok ? tracts : 0));
      var_set_num(vm, "SMX_AXON_LANE", (long)(ax_ok ? tracts : 0));
      var_set_num(vm, "SMX_MYELINS", (long)(ax_ok ? myelin : 0));
      var_set_num(vm, "SMX_MYELIN_HUB", (long)(ax_ok ? myelin : 0));
      var_set_num(vm, "SMX_MYELIN_N", (long)(ax_ok ? myelin : 0));
      var_set_num(vm, "SMX_SEEDAXON", (long)(ax_ok ? myelin : 0));
      var_set_num(vm, "SMX_SEEDMYELIN", (long)(ax_ok ? myelin : 0));
      var_set_num(vm, "SMX_MESH", (long)(ax_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + tracts + myelin));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (ax_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AXON ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AXON partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX AXON soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX AXON nodes=%d live=%d bonds=%d tracts=%d myelin=%d need=%d soft=%d talks=%d oob=%d axoned=%d vital=%ld\n",
                n, live, bonds, tracts, myelin, need, soft, vm->smx_talks, vm->smx_oob, ax_ok, vital);
    }
    bump(vm); return 1;

  }

/* SMX DENDRITE|MESH_DENDRITE|BRANCH|MESH_DENDRITE|MESH_DENDRITE|RAISE_DENDRITE a b c ...
   * Life-force dendrite mesh stability after axon: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete dendrite mesh among live nodes, weaves an
   * dendrite ring (i -> i+1) so free energy spines every edge, then spines sheath
   * gathers return so lattice locks dendrite where life spines the dendrite pulse.
   * Latches SMX_DENDRITE when mesh+branches+spines are soft-OOB-free.
   * SMX_BRANCHES = dendrite ring; SMX_SPINES hub = root gather pulses;
   * SMX_DENDRITE_SUM = bonds+branches+spines; SMX_DENDRITE|SMX_MESH_DENDRITE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"DENDRITE")||kw(&L->cur,"MESH_DENDRITE")||kw(&L->cur,"BRANCH")||kw(&L->cur,"ARBOR")||kw(&L->cur,"ARBORS")||kw(&L->cur,"BRANCHS")||kw(&L->cur,"MESH_DENDRITE")||kw(&L->cur,"STABLE_DENDRITE")||kw(&L->cur,"WE_DENDRITE")||kw(&L->cur,"LIFE_DENDRITE")||
      kw(&L->cur,"DEN_ALT")||kw(&L->cur,"MESH_DENDRITES")||kw(&L->cur,"AX")||
      kw(&L->cur,"STABLE_MESH_DEN")||kw(&L->cur,"BRANCHES")||kw(&L->cur,"DENDRITE_LEAF")||
      kw(&L->cur,"SPINES")||kw(&L->cur,"SEEDDENDRITE")||kw(&L->cur,"SEEDSPINE")||
      kw(&L->cur,"DENDRITE_RING")||kw(&L->cur,"SPINE_HUB")||
      kw(&L->cur,"MESH_DENDRITE")||kw(&L->cur,"RAISE_DENDRITE")||
      kw(&L->cur,"DENDRITEES")||kw(&L->cur,"DEN_ALT")||
      kw(&L->cur,"SEEDDENDRITE2")||kw(&L->cur,"SEEDBRANCH2")||
      kw(&L->cur,"LATTICE_DENDRITE")||kw(&L->cur,"WORLD_DENDRITE")||kw(&L->cur,"WORLD_DEN")||
      kw(&L->cur,"DENDRITE_RING")||kw(&L->cur,"PULSE_BRANCH")||kw(&L->cur,"PULSE_DENDRITE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int branches = 0;
    int spines = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "DENDRITE needs >=2 cubes",
                  "SMX DENDRITE a b [c ...]  or  SMX SPINE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - dendrite needs clear channel */
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
    /* complete dendrite mesh among live */
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
    /* dendrite ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else branches++;
        }
      }
    }
    /* spines sheath - seed axis return from every live leaf */
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
          else spines++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int branch_ok = (live >= 2 && branches >= live && soft == 0) ? 1 : 0;
      if (!branch_ok && live >= 2 && branches * 2 >= live && soft == 0)
        branch_ok = 1;
      int spine_ok = (live >= 1 && spines >= live && soft == 0) ? 1 : 0;
      if (!spine_ok && live >= 1 && spines * 2 >= live && soft == 0)
        spine_ok = 1;
      int den_ok = (mesh_ok && branch_ok && spine_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (den_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (branches > 0 ? 1 : 0) + (spines > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_DENDRITED", (long)den_ok);
      var_set_num(vm, "SMX_DENDRITE_LATCH", (long)den_ok);
      var_set_num(vm, "SMX_DENDRITE", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_DENDRITE_SUM", (long)(den_ok ? bonds + branches + spines : 0));
      var_set_num(vm, "SMX_DENDRITE_ALIAS", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_DENDRITE", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_SPINE_LATCH", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_DENDRITE", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_ARBOR", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_ARBORS", (long)(den_ok ? branches : 0));
      var_set_num(vm, "SMX_SPINE", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_DEN_ALT", (long)(den_ok ? bonds : 0));
      var_set_num(vm, "SMX_DENS", (long)(den_ok ? bonds : 0));
      var_set_num(vm, "SMX_DEN_ALT2", (long)(den_ok ? bonds : 0));
      var_set_num(vm, "SMX_BRANCHES", (long)(den_ok ? branches : 0));
      var_set_num(vm, "SMX_BRANCH", (long)(den_ok ? branches : 0));
      var_set_num(vm, "SMX_DENDRITE_RING", (long)(den_ok ? branches : 0));
      var_set_num(vm, "SMX_DENDRITE_LANE", (long)(den_ok ? branches : 0));
      var_set_num(vm, "SMX_SPINES", (long)(den_ok ? spines : 0));
      var_set_num(vm, "SMX_SPINE_HUB", (long)(den_ok ? spines : 0));
      var_set_num(vm, "SMX_SPINE_N", (long)(den_ok ? spines : 0));
      var_set_num(vm, "SMX_SEEDDENDRITE", (long)(den_ok ? spines : 0));
      var_set_num(vm, "SMX_SEEDSPINE", (long)(den_ok ? spines : 0));
      var_set_num(vm, "SMX_MESH", (long)(den_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + branches + spines));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (den_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX DENDRITE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX DENDRITE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX DENDRITE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX DENDRITE nodes=%d live=%d bonds=%d branches=%d spines=%d need=%d soft=%d talks=%d oob=%d dendrited=%d vital=%ld\n",
                n, live, bonds, branches, spines, need, soft, vm->smx_talks, vm->smx_oob, den_ok, vital);
    }
    bump(vm); return 1;

  }


/* SMX GLIA|MESH_GLIA|WRAP|CUSHION|SHEATH_GLIA|RAISE_GLIA a b c ...
   * Life-force glia mesh stability after dendrite: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete glia support mesh among live nodes, weaves a
   * glia wrap ring (i -> i+1) so free energy cushions every edge, then cushion sheath
   * gathers return so lattice locks glia where life cushions the glia pulse.
   * Latches SMX_GLIA when mesh+wraps+cushions are soft-OOB-free.
   * SMX_WRAPS = glia ring; SMX_CUSHIONS hub = root gather pulses;
   * SMX_GLIA_SUM = bonds+wraps+cushions; SMX_GLIA|SMX_MESH_GLIA|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"GLIA")||kw(&L->cur,"MESH_GLIA")||kw(&L->cur,"WRAP")||kw(&L->cur,"WRAPS")||kw(&L->cur,"CUSHION")||kw(&L->cur,"CUSHIONS")||kw(&L->cur,"SHEATH_GLIA")||kw(&L->cur,"GLIAL")||kw(&L->cur,"RAISE_GLIA")||kw(&L->cur,"WE_GLIA")||kw(&L->cur,"LIFE_GLIA")||
      kw(&L->cur,"STABLE_GLIA")||kw(&L->cur,"MESH_GLIAS")||kw(&L->cur,"SUPPORT")||kw(&L->cur,"SUPPORTS")||
      kw(&L->cur,"STABLE_MESH_GLIA")||kw(&L->cur,"GLIA_LEAF")||
      kw(&L->cur,"SEEDGLIA")||kw(&L->cur,"SEEDCUSHION")||kw(&L->cur,"SEEDWRAP")||
      kw(&L->cur,"GLIA_RING")||kw(&L->cur,"CUSHION_HUB")||
      kw(&L->cur,"LATTICE_GLIA")||kw(&L->cur,"WORLD_GLIA")||kw(&L->cur,"WORLD_GL")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_GLIA")||kw(&L->cur,"HARDEN_GLIA")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int cushions = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "GLIA needs >=2 cubes",
                  "SMX GLIA a b [c ...]  or  SMX CUSHION a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - glia needs clear channel */
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
    /* complete glia mesh among live */
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
    /* glia ring - free energy guards every edge every edge i -> i+1 both ways */
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
    /* cushions sheath - seed axis return from every live leaf */
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
          else cushions++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int wrap_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!wrap_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        wrap_ok = 1;
      int cushion_ok = (live >= 1 && cushions >= live && soft == 0) ? 1 : 0;
      if (!cushion_ok && live >= 1 && cushions * 2 >= live && soft == 0)
        cushion_ok = 1;
      int glia_ok = (mesh_ok && wrap_ok && cushion_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (glia_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (cushions > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_GLIAED", (long)glia_ok);
      var_set_num(vm, "SMX_GLIA_LATCH", (long)glia_ok);
      var_set_num(vm, "SMX_GLIA", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_GLIA_SUM", (long)(glia_ok ? bonds + wraps + cushions : 0));
      var_set_num(vm, "SMX_GLIA_ALIAS", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_GLIA", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_CUSHION_LATCH", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_GLIA", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(glia_ok ? wraps : 0));
      var_set_num(vm, "SMX_CUSHION", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_GLIA_ALT", (long)(glia_ok ? bonds : 0));
      var_set_num(vm, "SMX_DENS", (long)(glia_ok ? bonds : 0));
      var_set_num(vm, "SMX_GLIA_ALT2", (long)(glia_ok ? bonds : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(glia_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAP", (long)(glia_ok ? wraps : 0));
      var_set_num(vm, "SMX_GLIA_RING", (long)(glia_ok ? wraps : 0));
      var_set_num(vm, "SMX_GLIA_LANE", (long)(glia_ok ? wraps : 0));
      var_set_num(vm, "SMX_CUSHIONS", (long)(glia_ok ? cushions : 0));
      var_set_num(vm, "SMX_CUSHION_HUB", (long)(glia_ok ? cushions : 0));
      var_set_num(vm, "SMX_CUSHION_N", (long)(glia_ok ? cushions : 0));
      var_set_num(vm, "SMX_SEEDGLIA", (long)(glia_ok ? cushions : 0));
      var_set_num(vm, "SMX_SEEDCUSHION", (long)(glia_ok ? cushions : 0));
      var_set_num(vm, "SMX_MESH", (long)(glia_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + cushions));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (glia_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX GLIA ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX GLIA partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX GLIA soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX GLIA nodes=%d live=%d bonds=%d wraps=%d cushions=%d need=%d soft=%d talks=%d oob=%d gliad=%d vital=%ld\n",
                n, live, bonds, wraps, cushions, need, soft, vm->smx_talks, vm->smx_oob, glia_ok, vital);
    }
    bump(vm); return 1;

  }



/* SMX ASTROCYTE|MESH_ASTROCYTE|STAR|ENDFOOT|TRIPARTITE|RAISE_ASTROCYTE a b c ...
   * Life-force astrocyte mesh stability after glia: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete astrocyte support mesh among live nodes, weaves a
   * astrocyte wrap ring (i -> i+1) so free energy endfeet every edge, then tripartite gather
   * gathers return so lattice locks astrocyte where life endfeet the astrocyte pulse.
   * Latches SMX_ASTROCYTE when mesh+stars+endfeet are soft-OOB-free.
   * SMX_STARS = astrocyte ring; SMX_ENDFEET hub = root gather pulses;
   * SMX_ASTROCYTE_SUM = bonds+stars+endfeet; SMX_ASTROCYTE|SMX_MESH_ASTROCYTE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"ASTROCYTE")||kw(&L->cur,"MESH_ASTROCYTE")||kw(&L->cur,"STAR")||kw(&L->cur,"STARS")||kw(&L->cur,"ENDFOOT")||kw(&L->cur,"ENDFEET")||kw(&L->cur,"TRIPARTITE")||kw(&L->cur,"ASTRO")||kw(&L->cur,"RAISE_ASTROCYTE")||kw(&L->cur,"WE_ASTROCYTE")||kw(&L->cur,"LIFE_ASTROCYTE")||
      kw(&L->cur,"STABLE_ASTROCYTE")||kw(&L->cur,"MESH_ASTROCYTES")||kw(&L->cur,"ASTRO_STAR")||kw(&L->cur,"ASTRO_STARS")||
      kw(&L->cur,"STABLE_MESH_ASTROCYTE")||kw(&L->cur,"ASTROCYTE_LEAF")||
      kw(&L->cur,"SEEDASTROCYTE")||kw(&L->cur,"SEEDENDFOOT")||kw(&L->cur,"SEEDSTAR")||
      kw(&L->cur,"ASTROCYTE_RING")||kw(&L->cur,"ENDFOOT_HUB")||
      kw(&L->cur,"LATTICE_ASTROCYTE")||kw(&L->cur,"WORLD_ASTROCYTE")||kw(&L->cur,"WORLD_ASTRO")||
      kw(&L->cur,"PULSE_STAR")||kw(&L->cur,"PULSE_ASTROCYTE")||kw(&L->cur,"HARDEN_ASTROCYTE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int stars = 0;
    int endfeet = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "ASTROCYTE needs >=2 cubes",
                  "SMX ASTROCYTE a b [c ...]  or  SMX ENDFOOT a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - astrocyte needs clear channel */
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
    /* complete astrocyte mesh among live */
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
    /* astrocyte ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else stars++;
        }
      }
    }
    /* endfeet sheath - seed axis return from every live leaf */
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
          else endfeet++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && stars >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && stars * 2 >= live && soft == 0)
        star_ok = 1;
      int endfoot_ok = (live >= 1 && endfeet >= live && soft == 0) ? 1 : 0;
      if (!endfoot_ok && live >= 1 && endfeet * 2 >= live && soft == 0)
        endfoot_ok = 1;
      int astrocyte_ok = (mesh_ok && star_ok && endfoot_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (astrocyte_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (stars > 0 ? 1 : 0) + (endfeet > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_ASTROCYTED", (long)astrocyte_ok);
      var_set_num(vm, "SMX_ASTROCYTE_LATCH", (long)astrocyte_ok);
      var_set_num(vm, "SMX_ASTROCYTE", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_ASTROCYTE_SUM", (long)(astrocyte_ok ? bonds + stars + endfeet : 0));
      var_set_num(vm, "SMX_ASTROCYTE_ALIAS", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_ASTROCYTE", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_ENDFOOT_LATCH", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_ASTROCYTE", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_TRIPARTITE", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_TRIPARTITES", (long)(astrocyte_ok ? stars : 0));
      var_set_num(vm, "SMX_ENDFOOT", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_ASTRO_ALT", (long)(astrocyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_ASTROS", (long)(astrocyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_ASTRO_ALT2", (long)(astrocyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_STARS", (long)(astrocyte_ok ? stars : 0));
      var_set_num(vm, "SMX_STAR", (long)(astrocyte_ok ? stars : 0));
      var_set_num(vm, "SMX_ASTROCYTE_RING", (long)(astrocyte_ok ? stars : 0));
      var_set_num(vm, "SMX_ASTROCYTE_LANE", (long)(astrocyte_ok ? stars : 0));
      var_set_num(vm, "SMX_ENDFEET", (long)(astrocyte_ok ? endfeet : 0));
      var_set_num(vm, "SMX_ENDFOOT_HUB", (long)(astrocyte_ok ? endfeet : 0));
      var_set_num(vm, "SMX_ENDFOOT_N", (long)(astrocyte_ok ? endfeet : 0));
      var_set_num(vm, "SMX_SEEDASTROCYTE", (long)(astrocyte_ok ? endfeet : 0));
      var_set_num(vm, "SMX_SEEDENDFOOT", (long)(astrocyte_ok ? endfeet : 0));
      var_set_num(vm, "SMX_MESH", (long)(astrocyte_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + stars + endfeet));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (astrocyte_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ASTROCYTE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ASTROCYTE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX ASTROCYTE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX ASTROCYTE nodes=%d live=%d bonds=%d stars=%d endfeet=%d need=%d soft=%d talks=%d oob=%d astrocyted=%d vital=%ld\n",
                n, live, bonds, stars, endfeet, need, soft, vm->smx_talks, vm->smx_oob, astrocyte_ok, vital);
    }
    bump(vm); return 1;

  }



/* SMX MICROGLIA|MESH_MICROGLIA|STAR|PROCESS|SURVEY|RAISE_MICROGLIA a b c ...
   * Life-force microglia mesh stability after glia: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete microglia support mesh among live nodes, weaves a
   * microglia wrap ring (i -> i+1) so free energy processes every edge, then survey gather
   * gathers return so lattice locks microglia where life processes the microglia pulse.
   * Latches SMX_MICROGLIA when mesh+surveys+processes are soft-OOB-free.
   * SMX_STARS = microglia ring; SMX_PROCESSES hub = root gather pulses;
   * SMX_MICROGLIA_SUM = bonds+surveys+processes; SMX_MICROGLIA|SMX_MESH_MICROGLIA|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"MICROGLIA")||kw(&L->cur,"MESH_MICROGLIA")||kw(&L->cur,"SURVEY")||kw(&L->cur,"SURVEYS")||kw(&L->cur,"PROCESS")||kw(&L->cur,"PROCESSES")||kw(&L->cur,"MG")||kw(&L->cur,"RAISE_MICROGLIA")||kw(&L->cur,"WE_MICROGLIA")||kw(&L->cur,"LIFE_MICROGLIA")||
      kw(&L->cur,"STABLE_MICROGLIA")||kw(&L->cur,"MESH_MICROGLIAS")||kw(&L->cur,"MG_SURVEY")||kw(&L->cur,"MG_SURVEYS")||
      kw(&L->cur,"STABLE_MESH_MICROGLIA")||kw(&L->cur,"MICROGLIA_LEAF")||
      kw(&L->cur,"SEEDMICROGLIA")||kw(&L->cur,"SEEDPROCESS")||kw(&L->cur,"SEEDSURVEY")||
      kw(&L->cur,"MICROGLIA_RING")||kw(&L->cur,"PROCESS_HUB")||
      kw(&L->cur,"LATTICE_MICROGLIA")||kw(&L->cur,"WORLD_MICROGLIA")||kw(&L->cur,"WORLD_MG")||
      kw(&L->cur,"PULSE_SURVEY")||kw(&L->cur,"PULSE_MICROGLIA")||kw(&L->cur,"HARDEN_MICROGLIA")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int surveys = 0;
    int processes = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "MICROGLIA needs >=2 cubes",
                  "SMX MICROGLIA a b [c ...]  or  SMX PROCESS a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - microglia needs clear channel */
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
    /* complete microglia mesh among live */
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
    /* microglia ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else surveys++;
        }
      }
    }
    /* processes sheath - seed axis return from every live leaf */
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
          else processes++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && surveys >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && surveys * 2 >= live && soft == 0)
        star_ok = 1;
      int process_ok = (live >= 1 && processes >= live && soft == 0) ? 1 : 0;
      if (!process_ok && live >= 1 && processes * 2 >= live && soft == 0)
        process_ok = 1;
      int microglia_ok = (mesh_ok && star_ok && process_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (microglia_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (surveys > 0 ? 1 : 0) + (processes > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_MICROGLIAED", (long)microglia_ok);
      var_set_num(vm, "SMX_MICROGLIA_LATCH", (long)microglia_ok);
      var_set_num(vm, "SMX_MICROGLIA", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_MICROGLIA_SUM", (long)(microglia_ok ? bonds + surveys + processes : 0));
      var_set_num(vm, "SMX_MICROGLIA_ALIAS", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_MICROGLIA", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_PROCESS_LATCH", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_MICROGLIA", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_SURVEY", (long)(microglia_ok ? surveys : 0));
      var_set_num(vm, "SMX_SURVEYS", (long)(microglia_ok ? surveys : 0));
      var_set_num(vm, "SMX_SURVEYS_N", (long)(microglia_ok ? surveys : 0));
      var_set_num(vm, "SMX_SURVEY_LATCH", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_PROCESS", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_OL_BONDS", (long)(microglia_ok ? bonds : 0));
      var_set_num(vm, "SMX_OL_MESH", (long)(microglia_ok ? bonds : 0));
      var_set_num(vm, "SMX_OL_BONDS2", (long)(microglia_ok ? bonds : 0));
      var_set_num(vm, "SMX_STARS", (long)(microglia_ok ? surveys : 0));
      var_set_num(vm, "SMX_STAR", (long)(microglia_ok ? surveys : 0));
      var_set_num(vm, "SMX_MICROGLIA_RING", (long)(microglia_ok ? surveys : 0));
      var_set_num(vm, "SMX_MICROGLIA_LANE", (long)(microglia_ok ? surveys : 0));
      var_set_num(vm, "SMX_PROCESSES", (long)(microglia_ok ? processes : 0));
      var_set_num(vm, "SMX_PROCESS_HUB", (long)(microglia_ok ? processes : 0));
      var_set_num(vm, "SMX_PROCESS_N", (long)(microglia_ok ? processes : 0));
      var_set_num(vm, "SMX_SEEDMICROGLIA", (long)(microglia_ok ? processes : 0));
      var_set_num(vm, "SMX_SEEDPROCESS", (long)(microglia_ok ? processes : 0));
      var_set_num(vm, "SMX_MESH", (long)(microglia_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + surveys + processes));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (microglia_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MICROGLIA ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MICROGLIA partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX MICROGLIA soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX MICROGLIA nodes=%d live=%d bonds=%d surveys=%d processes=%d need=%d soft=%d talks=%d oob=%d microgliad=%d vital=%ld\n",
                n, live, bonds, surveys, processes, need, soft, vm->smx_talks, vm->smx_oob, microglia_ok, vital);
    }
    bump(vm); return 1;

  }



/* SMX OLIGO|OLIGODENDROCYTE|MESH_OLIGO|OL_WRAP|OL_SHEATH|MYELINATE|RAISE_OLIGO a b c ...
   * Life-force oligodendrocyte mesh stability after microglia: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete oligo myelin mesh among live nodes, weaves a
   * oligo wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks oligo where life myelinate the oligo pulse.
   * Latches SMX_OLIGO when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = oligo ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_OLIGO_SUM = bonds+wraps+sheaths; SMX_OLIGO|SMX_MESH_OLIGO|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"OLIGO")||kw(&L->cur,"OLIGODENDROCYTE")||kw(&L->cur,"MESH_OLIGO")||kw(&L->cur,"OL_WRAP")||kw(&L->cur,"OL_WRAPS")||kw(&L->cur,"OL_SHEATH")||kw(&L->cur,"OL_SHEATHS")||kw(&L->cur,"MYELINATE")||kw(&L->cur,"RAISE_OLIGO")||kw(&L->cur,"WE_OLIGO")||kw(&L->cur,"LIFE_OLIGO")||
      kw(&L->cur,"STABLE_OLIGO")||kw(&L->cur,"MESH_OLIGOS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_OLIGO")||kw(&L->cur,"OLIGO_LEAF")||
      kw(&L->cur,"SEEDOLIGO")||kw(&L->cur,"SEEDSHEATH")||kw(&L->cur,"SEEDWRAP")||
      kw(&L->cur,"OLIGO_RING")||kw(&L->cur,"SHEATH_HUB")||kw(&L->cur,"NODE_RANVIER")||
      kw(&L->cur,"LATTICE_OLIGO")||kw(&L->cur,"WORLD_OLIGO")||kw(&L->cur,"WORLD_OL")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_OLIGO")||kw(&L->cur,"HARDEN_OLIGO")||
      kw(&L->cur,"MYELIN_SHEATH")||kw(&L->cur,"OLIGO_MESH")){
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
      smx_fail_at(vm, aln, "OLIGO needs >=2 cubes",
                  "SMX OLIGO a b [c ...]  or  SMX OL_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - oligo needs clear channel */
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
    /* complete oligo mesh among live */
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
    /* oligo ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int oligo_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (oligo_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_OLIGOED", (long)oligo_ok);
      var_set_num(vm, "SMX_OLIGO_LATCH", (long)oligo_ok);
      var_set_num(vm, "SMX_OLIGO", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_OLIGO_SUM", (long)(oligo_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_OLIGO_ALIAS", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_OLIGO", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_OLIGO", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(oligo_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(oligo_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(oligo_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_OL_BONDS", (long)(oligo_ok ? bonds : 0));
      var_set_num(vm, "SMX_OL_MESH", (long)(oligo_ok ? bonds : 0));
      var_set_num(vm, "SMX_OL_BONDS2", (long)(oligo_ok ? bonds : 0));
      var_set_num(vm, "SMX_STARS", (long)(oligo_ok ? wraps : 0));
      var_set_num(vm, "SMX_STAR", (long)(oligo_ok ? wraps : 0));
      var_set_num(vm, "SMX_OLIGO_RING", (long)(oligo_ok ? wraps : 0));
      var_set_num(vm, "SMX_OLIGO_LANE", (long)(oligo_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(oligo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_HUB", (long)(oligo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(oligo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDOLIGO", (long)(oligo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDSHEATH", (long)(oligo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(oligo_ok ? live : 0));
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
      if (oligo_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX OLIGO ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX OLIGO partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX OLIGO soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX OLIGO nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d oligod=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, oligo_ok, vital);
    }
    bump(vm); return 1;

  }

  return 0;
}
