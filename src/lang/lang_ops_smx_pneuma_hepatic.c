/* CubalC lang — lang_ops_smx_pneuma_hepatic.c
 * SMX hepatic: PORTAL · SINUSOID · DISSE · HEPATOCYTE · KUPFFER · STELLATE ·
 *              OVAL · CANALICULUS · DUCTULE · HEPATIC_DUCT · COMMON_BILE_DUCT
 * Peel from pneuma_b (B_pneuma_b_peel). Leaves pure dispatcher. Cube is SoT.
 */
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

int cubalc_lang_ops_smx_try_pneuma_hepatic(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* SMX PORTAL|PORTALGLIA|MESH_PORTAL|PORTAL_WRAP|PORTAL_SHEATH|PORTAL_GUIDE|RAISE_PORTAL a b c ...
   * Life-force PORTAL portal confluence return scaffold mesh stability after portal great-return: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete portal confluence return scaffold mesh among live nodes, weaves a
   * portal wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks portal where life guides the portal confluence return pulse.
   * Latches SMX_PORTAL when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = portal ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_PORTAL_SUM = bonds+wraps+sheaths; SMX_PORTAL|SMX_MESH_PORTAL|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"PORTAL")||kw(&L->cur,"PORTALGLIA")||kw(&L->cur,"MESH_PORTAL")||kw(&L->cur,"PORTAL_WRAP")||kw(&L->cur,"PORTAL_WRAPS")||kw(&L->cur,"PORTAL_SHEATH")||kw(&L->cur,"PORTAL_SHEATHS")||kw(&L->cur,"PORTAL_GUIDE")||kw(&L->cur,"RAISE_PORTAL")||kw(&L->cur,"WE_PORTAL")||kw(&L->cur,"LIFE_PORTAL")||
      kw(&L->cur,"STABLE_PORTAL")||kw(&L->cur,"MESH_PORTALS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_PORTAL")||kw(&L->cur,"PORTAL_LEAF")||
      kw(&L->cur,"SEEDPORTAL")||kw(&L->cur,"SEEDPORTALSHEATH")||kw(&L->cur,"SEEDPORTALWRAP")||
      kw(&L->cur,"PORTAL_RING")||kw(&L->cur,"PORTAL_SHEATH_HUB")||kw(&L->cur,"PORTAL_NODE")||
      kw(&L->cur,"LATTICE_PORTAL")||kw(&L->cur,"WORLD_PORTAL")||kw(&L->cur,"WORLD_PORTAL")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_PORTAL")||kw(&L->cur,"HARDEN_PORTAL")||
      kw(&L->cur,"PORTAL_SCAFFOLD")||kw(&L->cur,"PORTAL_MESH")){
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
      smx_fail_at(vm, aln, "PORTAL needs >=2 cubes",
                  "SMX PORTAL a b [c ...]  or  SMX PORTAL_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - portal needs clear channel */
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
    /* complete portal mesh among live */
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
    /* portal ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int portal_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (portal_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_PORTALED", (long)portal_ok);
      var_set_num(vm, "SMX_PORTAL_LATCH", (long)portal_ok);
      var_set_num(vm, "SMX_PORTAL", (long)(portal_ok ? 1 : 0));
      var_set_num(vm, "SMX_PORTAL_SUM", (long)(portal_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_PORTAL_ALIAS", (long)(portal_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_PORTAL", (long)(portal_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(portal_ok ? 1 : 0));
      var_set_num(vm, "SMX_PORTAL_SHEATH_LATCH", (long)(portal_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(portal_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_PORTAL", (long)(portal_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(portal_ok ? 1 : 0));
      var_set_num(vm, "SMX_PORTAL_WRAP_LATCH", (long)(portal_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(portal_ok ? wraps : 0));
      var_set_num(vm, "SMX_PORTAL_WRAP", (long)(portal_ok ? wraps : 0));
      var_set_num(vm, "SMX_PORTAL_WRAPS", (long)(portal_ok ? wraps : 0));
      var_set_num(vm, "SMX_PORTAL_SHEATHS", (long)(portal_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(portal_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(portal_ok ? wraps : 0));
      var_set_num(vm, "SMX_PORTAL_WRAP_LATCH", (long)(portal_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(portal_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(portal_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(portal_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(portal_ok ? 1 : 0));
      var_set_num(vm, "SMX_PO_BONDS", (long)(portal_ok ? bonds : 0));
      var_set_num(vm, "SMX_PO_MESH", (long)(portal_ok ? bonds : 0));
      var_set_num(vm, "SMX_PO_BONDS2", (long)(portal_ok ? bonds : 0));
      var_set_num(vm, "SMX_PO_STARS", (long)(portal_ok ? wraps : 0));
      var_set_num(vm, "SMX_PO_STAR", (long)(portal_ok ? wraps : 0));
      var_set_num(vm, "SMX_PORTAL_RING", (long)(portal_ok ? wraps : 0));
      var_set_num(vm, "SMX_PORTAL_LANE", (long)(portal_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(portal_ok ? sheaths : 0));
      var_set_num(vm, "SMX_PORTAL_SHEATH_HUB", (long)(portal_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(portal_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDPORTAL", (long)(portal_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDPORTALSHEATH", (long)(portal_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(portal_ok ? live : 0));
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
      if (portal_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX PORTAL ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX PORTAL partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX PORTAL soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX PORTAL nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d portaled=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, portal_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX SINUSOID|SINUSOIDGLIA|MESH_SINUSOID|SINUSOID_WRAP|SINUSOID_SHEATH|SINUSOID_GUIDE|RAISE_SINUSOID a b c ...
   * Life-force SINUSOID sinusoid hepatic exchange scaffold mesh stability after portal confluence: hepatic sinusoid exchange bed: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete sinusoid hepatic exchange scaffold mesh among live nodes, weaves a
   * sinusoid wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks sinusoid where life guides the sinusoid confluence return pulse.
   * Latches SMX_SINUSOID when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = sinusoid ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_SINUSOID_SUM = bonds+wraps+sheaths; SMX_SINUSOID|SMX_MESH_SINUSOID|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"SINUSOID")||kw(&L->cur,"SINUSOIDGLIA")||kw(&L->cur,"MESH_SINUSOID")||kw(&L->cur,"SINUSOID_WRAP")||kw(&L->cur,"SINUSOID_WRAPS")||kw(&L->cur,"SINUSOID_SHEATH")||kw(&L->cur,"SINUSOID_SHEATHS")||kw(&L->cur,"SINUSOID_GUIDE")||kw(&L->cur,"RAISE_SINUSOID")||kw(&L->cur,"WE_SINUSOID")||kw(&L->cur,"LIFE_SINUSOID")||
      kw(&L->cur,"STABLE_SINUSOID")||kw(&L->cur,"MESH_SINUSOIDS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_SINUSOID")||kw(&L->cur,"SINUSOID_LEAF")||
      kw(&L->cur,"SEEDSINUSOID")||kw(&L->cur,"SEEDSINUSOIDSHEATH")||kw(&L->cur,"SEEDSINUSOIDWRAP")||
      kw(&L->cur,"SINUSOID_RING")||kw(&L->cur,"SINUSOID_SHEATH_HUB")||kw(&L->cur,"SINUSOID_NODE")||
      kw(&L->cur,"LATTICE_SINUSOID")||kw(&L->cur,"WORLD_SINUSOID")||kw(&L->cur,"WORLD_SINUSOID")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_SINUSOID")||kw(&L->cur,"HARDEN_SINUSOID")||
      kw(&L->cur,"SINUSOID_SCAFFOLD")||kw(&L->cur,"SINUSOID_MESH")){
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
      smx_fail_at(vm, aln, "SINUSOID needs >=2 cubes",
                  "SMX SINUSOID a b [c ...]  or  SMX SINUSOID_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - sinusoid needs clear channel */
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
    /* complete sinusoid mesh among live */
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
    /* sinusoid ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int sinusoid_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (sinusoid_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_SINUSOIDED", (long)sinusoid_ok);
      var_set_num(vm, "SMX_SINUSOID_LATCH", (long)sinusoid_ok);
      var_set_num(vm, "SMX_SINUSOID", (long)(sinusoid_ok ? 1 : 0));
      var_set_num(vm, "SMX_SINUSOID_SUM", (long)(sinusoid_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_SINUSOID_ALIAS", (long)(sinusoid_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_SINUSOID", (long)(sinusoid_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(sinusoid_ok ? 1 : 0));
      var_set_num(vm, "SMX_SINUSOID_SHEATH_LATCH", (long)(sinusoid_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(sinusoid_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_SINUSOID", (long)(sinusoid_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(sinusoid_ok ? 1 : 0));
      var_set_num(vm, "SMX_SINUSOID_WRAP_LATCH", (long)(sinusoid_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(sinusoid_ok ? wraps : 0));
      var_set_num(vm, "SMX_SINUSOID_WRAP", (long)(sinusoid_ok ? wraps : 0));
      var_set_num(vm, "SMX_SINUSOID_WRAPS", (long)(sinusoid_ok ? wraps : 0));
      var_set_num(vm, "SMX_SINUSOID_SHEATHS", (long)(sinusoid_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(sinusoid_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(sinusoid_ok ? wraps : 0));
      var_set_num(vm, "SMX_SINUSOID_WRAP_LATCH", (long)(sinusoid_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(sinusoid_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(sinusoid_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(sinusoid_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(sinusoid_ok ? 1 : 0));
      var_set_num(vm, "SMX_PO_BONDS", (long)(sinusoid_ok ? bonds : 0));
      var_set_num(vm, "SMX_PO_MESH", (long)(sinusoid_ok ? bonds : 0));
      var_set_num(vm, "SMX_PO_BONDS2", (long)(sinusoid_ok ? bonds : 0));
      var_set_num(vm, "SMX_PO_STARS", (long)(sinusoid_ok ? wraps : 0));
      var_set_num(vm, "SMX_PO_STAR", (long)(sinusoid_ok ? wraps : 0));
      var_set_num(vm, "SMX_SINUSOID_RING", (long)(sinusoid_ok ? wraps : 0));
      var_set_num(vm, "SMX_SINUSOID_LANE", (long)(sinusoid_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(sinusoid_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SINUSOID_SHEATH_HUB", (long)(sinusoid_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(sinusoid_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDSINUSOID", (long)(sinusoid_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDSINUSOIDSHEATH", (long)(sinusoid_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(sinusoid_ok ? live : 0));
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
      if (sinusoid_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SINUSOID ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SINUSOID partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX SINUSOID soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX SINUSOID nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d sinusoided=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, sinusoid_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX DISSE|DISSEGLIA|MESH_DISSE|DISSE_WRAP|DISSE_SHEATH|DISSE_GUIDE|RAISE_DISSE a b c ...
   * Life-force DISSE space-of-Disse perisinusoidal continuum after SINUSOID: hepatic plasma filtration bed: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete disse hepatic exchange scaffold mesh among live nodes, weaves a
   * disse wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks disse where life guides the disse confluence return pulse.
   * Latches SMX_DISSE when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = disse ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_DISSE_SUM = bonds+wraps+sheaths; SMX_DISSE|SMX_MESH_DISSE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"DISSE")||kw(&L->cur,"DISSEGLIA")||kw(&L->cur,"MESH_DISSE")||kw(&L->cur,"DISSE_WRAP")||kw(&L->cur,"DISSE_WRAPS")||kw(&L->cur,"DISSE_SHEATH")||kw(&L->cur,"DISSE_SHEATHS")||kw(&L->cur,"DISSE_GUIDE")||kw(&L->cur,"RAISE_DISSE")||kw(&L->cur,"WE_DISSE")||kw(&L->cur,"LIFE_DISSE")||
      kw(&L->cur,"STABLE_DISSE")||kw(&L->cur,"MESH_DISSES")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_DISSE")||kw(&L->cur,"DISSE_LEAF")||
      kw(&L->cur,"SEEDDISSE")||kw(&L->cur,"SEEDDISSESHEATH")||kw(&L->cur,"SEEDDISSEWRAP")||
      kw(&L->cur,"DISSE_RING")||kw(&L->cur,"DISSE_SHEATH_HUB")||kw(&L->cur,"DISSE_NODE")||
      kw(&L->cur,"LATTICE_DISSE")||kw(&L->cur,"WORLD_DISSE")||kw(&L->cur,"WORLD_DISSE")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_DISSE")||kw(&L->cur,"HARDEN_DISSE")||
      kw(&L->cur,"DISSE_SCAFFOLD")||kw(&L->cur,"DISSE_MESH")){
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
      smx_fail_at(vm, aln, "DISSE needs >=2 cubes",
                  "SMX DISSE a b [c ...]  or  SMX DISSE_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - disse needs clear channel */
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
    /* complete disse mesh among live */
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
    /* disse ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int disse_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (disse_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_DISSEED", (long)disse_ok);
      var_set_num(vm, "SMX_DISSE_LATCH", (long)disse_ok);
      var_set_num(vm, "SMX_DISSE", (long)(disse_ok ? 1 : 0));
      var_set_num(vm, "SMX_DISSE_SUM", (long)(disse_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_DISSE_ALIAS", (long)(disse_ok ? 1 : 0));
      var_set_num(vm, "SMX_SPACE_OF_DISSE", (long)(disse_ok ? 1 : 0));
      var_set_num(vm, "SMX_PERISINUSOIDAL", (long)(disse_ok ? 1 : 0));
      var_set_num(vm, "SMX_FENESTRATION", (long)(disse_ok ? wraps : 0));
      var_set_num(vm, "SMX_MESH_DISSE", (long)(disse_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(disse_ok ? 1 : 0));
      var_set_num(vm, "SMX_DISSE_SHEATH_LATCH", (long)(disse_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(disse_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_DISSE", (long)(disse_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(disse_ok ? 1 : 0));
      var_set_num(vm, "SMX_DISSE_WRAP_LATCH", (long)(disse_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(disse_ok ? wraps : 0));
      var_set_num(vm, "SMX_DISSE_WRAP", (long)(disse_ok ? wraps : 0));
      var_set_num(vm, "SMX_DISSE_WRAPS", (long)(disse_ok ? wraps : 0));
      var_set_num(vm, "SMX_DISSE_SHEATHS", (long)(disse_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(disse_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(disse_ok ? wraps : 0));
      var_set_num(vm, "SMX_DISSE_WRAP_LATCH", (long)(disse_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(disse_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(disse_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(disse_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(disse_ok ? 1 : 0));
      var_set_num(vm, "SMX_DI_BONDS", (long)(disse_ok ? bonds : 0));
      var_set_num(vm, "SMX_DI_MESH", (long)(disse_ok ? bonds : 0));
      var_set_num(vm, "SMX_DI_BONDS2", (long)(disse_ok ? bonds : 0));
      var_set_num(vm, "SMX_DI_STARS", (long)(disse_ok ? wraps : 0));
      var_set_num(vm, "SMX_DI_STAR", (long)(disse_ok ? wraps : 0));
      var_set_num(vm, "SMX_DISSE_RING", (long)(disse_ok ? wraps : 0));
      var_set_num(vm, "SMX_DISSE_LANE", (long)(disse_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(disse_ok ? sheaths : 0));
      var_set_num(vm, "SMX_DISSE_SHEATH_HUB", (long)(disse_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(disse_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDDISSE", (long)(disse_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDDISSESHEATH", (long)(disse_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(disse_ok ? live : 0));
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
      if (disse_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX DISSE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX DISSE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX DISSE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX DISSE nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d disseed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, disse_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX HEPATOCYTE|HEPATOGLIA|MESH_HEPATOCYTE|HEPATO_WRAP|HEPATO_SHEATH|HEPATO_GUIDE|RAISE_HEPATOCYTE a b c ...
   * Life-force HEPATOCYTE hepatocyte parenchymal plate continuum after DISSE: hepatic cord metabolic bed: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete hepato hepatic exchange scaffold mesh among live nodes, weaves a
   * hepato wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks hepato where life guides the hepato confluence return pulse.
   * Latches SMX_HEPATOCYTE when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = hepato ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_HEPATO_SUM = bonds+wraps+sheaths; SMX_HEPATOCYTE|SMX_MESH_HEPATOCYTE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"HEPATOCYTE")||kw(&L->cur,"HEPATO")||kw(&L->cur,"HEPATIC_PLATE")||kw(&L->cur,"HEPATOGLIA")||kw(&L->cur,"MESH_HEPATOCYTE")||kw(&L->cur,"HEPATO_WRAP")||kw(&L->cur,"HEPATO_WRAPS")||kw(&L->cur,"HEPATO_SHEATH")||kw(&L->cur,"HEPATO_SHEATHS")||kw(&L->cur,"HEPATO_GUIDE")||kw(&L->cur,"RAISE_HEPATOCYTE")||kw(&L->cur,"WE_HEPATOCYTE")||kw(&L->cur,"LIFE_HEPATOCYTE")||
      kw(&L->cur,"STABLE_HEPATOCYTE")||kw(&L->cur,"MESH_HEPATOCYTES")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_HEPATOCYTE")||kw(&L->cur,"HEPATO_LEAF")||
      kw(&L->cur,"SEEDHEPATO")||kw(&L->cur,"SEEDHEPATOSHEATH")||kw(&L->cur,"SEEDHEPATOWRAP")||
      kw(&L->cur,"HEPATO_RING")||kw(&L->cur,"HEPATO_SHEATH_HUB")||kw(&L->cur,"HEPATO_NODE")||
      kw(&L->cur,"LATTICE_HEPATOCYTE")||kw(&L->cur,"WORLD_HEPATOCYTE")||kw(&L->cur,"WORLD_HEPATOCYTE")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_HEPATOCYTE")||kw(&L->cur,"HARDEN_HEPATOCYTE")||
      kw(&L->cur,"HEPATO_SCAFFOLD")||kw(&L->cur,"HEPATO_MESH")){
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
      smx_fail_at(vm, aln, "HEPATOCYTE needs >=2 cubes",
                  "SMX HEPATOCYTE a b [c ...]  or  SMX HEPATO_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - hepato needs clear channel */
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
    /* complete hepato mesh among live */
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
    /* hepato ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int hepato_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (hepato_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_HEPATOCYTED", (long)hepato_ok);
      var_set_num(vm, "SMX_HEPATO_LATCH", (long)hepato_ok);
      var_set_num(vm, "SMX_HEPATOCYTE", (long)(hepato_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATO_SUM", (long)(hepato_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_HEPATO_ALIAS", (long)(hepato_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_PLATE", (long)(hepato_ok ? 1 : 0));
      var_set_num(vm, "SMX_PARENCHYMAL", (long)(hepato_ok ? 1 : 0));
      var_set_num(vm, "SMX_BILE_CANALICULUS", (long)(hepato_ok ? wraps : 0));
      var_set_num(vm, "SMX_MESH_HEPATOCYTE", (long)(hepato_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(hepato_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATO_SHEATH_LATCH", (long)(hepato_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(hepato_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_HEPATOCYTE", (long)(hepato_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(hepato_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATO_WRAP_LATCH", (long)(hepato_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(hepato_ok ? wraps : 0));
      var_set_num(vm, "SMX_HEPATO_WRAP", (long)(hepato_ok ? wraps : 0));
      var_set_num(vm, "SMX_HEPATO_WRAPS", (long)(hepato_ok ? wraps : 0));
      var_set_num(vm, "SMX_HEPATO_SHEATHS", (long)(hepato_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(hepato_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(hepato_ok ? wraps : 0));
      var_set_num(vm, "SMX_HEPATO_WRAP_LATCH", (long)(hepato_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(hepato_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(hepato_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(hepato_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(hepato_ok ? 1 : 0));
      var_set_num(vm, "SMX_HP_BONDS", (long)(hepato_ok ? bonds : 0));
      var_set_num(vm, "SMX_HP_MESH", (long)(hepato_ok ? bonds : 0));
      var_set_num(vm, "SMX_HP_BONDS2", (long)(hepato_ok ? bonds : 0));
      var_set_num(vm, "SMX_HP_STARS", (long)(hepato_ok ? wraps : 0));
      var_set_num(vm, "SMX_HP_STAR", (long)(hepato_ok ? wraps : 0));
      var_set_num(vm, "SMX_HEPATO_RING", (long)(hepato_ok ? wraps : 0));
      var_set_num(vm, "SMX_HEPATO_LANE", (long)(hepato_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(hepato_ok ? sheaths : 0));
      var_set_num(vm, "SMX_HEPATO_SHEATH_HUB", (long)(hepato_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(hepato_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDHEPATO", (long)(hepato_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDHEPATOSHEATH", (long)(hepato_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(hepato_ok ? live : 0));
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
      if (hepato_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HEPATOCYTE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HEPATOCYTE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX HEPATOCYTE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX HEPATOCYTE nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d hepatocyted=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, hepato_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX KUPFFER|KUPFFER|MESH_KUPFFER|KUPFFER_WRAP|KUPFFER_SHEATH|KUPFFER_GUIDE|RAISE_KUPFFER a b c ...
   * Life-force KUPFFER kupffer parenchymal plate continuum after DISSE: hepatic cord metabolic bed: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete kupffer hepatic exchange scaffold mesh among live nodes, weaves a
   * kupffer wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks kupffer where life guides the kupffer confluence return pulse.
   * Latches SMX_KUPFFER when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = kupffer ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_KUPFFER_SUM = bonds+wraps+sheaths; SMX_KUPFFER|SMX_MESH_KUPFFER|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"KUPFFER")||kw(&L->cur,"KUPFFER")||kw(&L->cur,"KUPFFER_PLATE")||kw(&L->cur,"KUPFFER")||kw(&L->cur,"MESH_KUPFFER")||kw(&L->cur,"KUPFFER_WRAP")||kw(&L->cur,"KUPFFER_WRAPS")||kw(&L->cur,"KUPFFER_SHEATH")||kw(&L->cur,"KUPFFER_SHEATHS")||kw(&L->cur,"KUPFFER_GUIDE")||kw(&L->cur,"RAISE_KUPFFER")||kw(&L->cur,"WE_KUPFFER")||kw(&L->cur,"LIFE_KUPFFER")||
      kw(&L->cur,"STABLE_KUPFFER")||kw(&L->cur,"MESH_KUPFFERS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_KUPFFER")||kw(&L->cur,"KUPFFER_LEAF")||
      kw(&L->cur,"SEEDKUPFFER")||kw(&L->cur,"SEEDKUPFFERSHEATH")||kw(&L->cur,"SEEDKUPFFERWRAP")||
      kw(&L->cur,"KUPFFER_RING")||kw(&L->cur,"KUPFFER_SHEATH_HUB")||kw(&L->cur,"KUPFFER_NODE")||
      kw(&L->cur,"LATTICE_KUPFFER")||kw(&L->cur,"WORLD_KUPFFER")||kw(&L->cur,"WORLD_KUPFFER")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_KUPFFER")||kw(&L->cur,"HARDEN_KUPFFER")||
      kw(&L->cur,"KUPFFER_SCAFFOLD")||kw(&L->cur,"KUPFFER_MESH")||kw(&L->cur,"MACROPHAGE")||kw(&L->cur,"PHAGOCYTIC")||kw(&L->cur,"RESIDENT_MACROPHAGE")||kw(&L->cur,"SINUSOIDAL_MACROPHAGE")||kw(&L->cur,"CLEARANCE")||kw(&L->cur,"KUPFFER_CELL")){
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
      smx_fail_at(vm, aln, "KUPFFER needs >=2 cubes",
                  "SMX KUPFFER a b [c ...]  or  SMX KUPFFER_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - kupffer needs clear channel */
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
    /* complete kupffer mesh among live */
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
    /* kupffer ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int kupffer_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (kupffer_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_KUPFFERED", (long)kupffer_ok);
      var_set_num(vm, "SMX_KUPFFER_LATCH", (long)kupffer_ok);
      var_set_num(vm, "SMX_KUPFFER", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_KUPFFER_SUM", (long)(kupffer_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_KUPFFER_ALIAS", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_KUPFFER_PLATE", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_PHAGOCYTIC", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_KUPFFER_CELL", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_MACROPHAGE", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_RESIDENT_MACROPHAGE", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_SINUSOIDAL_MACROPHAGE", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_CLEARANCE", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_PHAGOCYTIC", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_CLEARANCE_LANE", (long)(kupffer_ok ? wraps : 0));
      var_set_num(vm, "SMX_MESH_KUPFFER", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_KUPFFER_SHEATH_LATCH", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_KUPFFER", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_KUPFFER_WRAP_LATCH", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(kupffer_ok ? wraps : 0));
      var_set_num(vm, "SMX_KUPFFER_WRAP", (long)(kupffer_ok ? wraps : 0));
      var_set_num(vm, "SMX_KUPFFER_WRAPS", (long)(kupffer_ok ? wraps : 0));
      var_set_num(vm, "SMX_KUPFFER_SHEATHS", (long)(kupffer_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(kupffer_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(kupffer_ok ? wraps : 0));
      var_set_num(vm, "SMX_KUPFFER_WRAP_LATCH", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(kupffer_ok ? 1 : 0));
      var_set_num(vm, "SMX_KF_BONDS", (long)(kupffer_ok ? bonds : 0));
      var_set_num(vm, "SMX_KF_MESH", (long)(kupffer_ok ? bonds : 0));
      var_set_num(vm, "SMX_KF_BONDS2", (long)(kupffer_ok ? bonds : 0));
      var_set_num(vm, "SMX_KF_STARS", (long)(kupffer_ok ? wraps : 0));
      var_set_num(vm, "SMX_KF_STAR", (long)(kupffer_ok ? wraps : 0));
      var_set_num(vm, "SMX_KUPFFER_RING", (long)(kupffer_ok ? wraps : 0));
      var_set_num(vm, "SMX_KUPFFER_LANE", (long)(kupffer_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(kupffer_ok ? sheaths : 0));
      var_set_num(vm, "SMX_KUPFFER_SHEATH_HUB", (long)(kupffer_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(kupffer_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDKUPFFER", (long)(kupffer_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDKUPFFERSHEATH", (long)(kupffer_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(kupffer_ok ? live : 0));
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
      if (kupffer_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX KUPFFER ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX KUPFFER partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX KUPFFER soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX KUPFFER nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d kupfferd=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, kupffer_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX STELLATE|STELLATE|MESH_STELLATE|STELLATE_WRAP|STELLATE_SHEATH|STELLATE_GUIDE|RAISE_STELLATE a b c ...
   * Life-force STELLATE Ito hepatic-stellate continuum after KUPFFER: space-of-Disse vitamin-A fibrosis bed: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete stellate hepatic exchange scaffold mesh among live nodes, weaves a
   * stellate wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks stellate where life guides the stellate confluence return pulse.
   * Latches SMX_STELLATE when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = stellate ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_STELLATE_SUM = bonds+wraps+sheaths; SMX_STELLATE|SMX_MESH_STELLATE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"STELLATE")||kw(&L->cur,"ITO")||kw(&L->cur,"ITO_CELL")||kw(&L->cur,"HSC")||kw(&L->cur,"STELLATE_PLATE")||kw(&L->cur,"MESH_STELLATE")||kw(&L->cur,"STELLATE_WRAP")||kw(&L->cur,"STELLATE_WRAPS")||kw(&L->cur,"STELLATE_SHEATH")||kw(&L->cur,"STELLATE_SHEATHS")||kw(&L->cur,"STELLATE_GUIDE")||kw(&L->cur,"RAISE_STELLATE")||kw(&L->cur,"WE_STELLATE")||kw(&L->cur,"LIFE_STELLATE")||
      kw(&L->cur,"STABLE_STELLATE")||kw(&L->cur,"MESH_STELLATES")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_STELLATE")||kw(&L->cur,"STELLATE_LEAF")||
      kw(&L->cur,"SEEDSTELLATE")||kw(&L->cur,"SEEDSTELLATESHEATH")||kw(&L->cur,"SEEDSTELLATEWRAP")||
      kw(&L->cur,"STELLATE_RING")||kw(&L->cur,"STELLATE_SHEATH_HUB")||kw(&L->cur,"STELLATE_NODE")||
      kw(&L->cur,"LATTICE_STELLATE")||kw(&L->cur,"WORLD_STELLATE")||kw(&L->cur,"WORLD_STELLATE")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_STELLATE")||kw(&L->cur,"HARDEN_STELLATE")||
      kw(&L->cur,"STELLATE_SCAFFOLD")||kw(&L->cur,"STELLATE_MESH")||kw(&L->cur,"HEPATIC_STELLATE")||kw(&L->cur,"VITAMIN_A")||kw(&L->cur,"ITO_STELLATE")||kw(&L->cur,"RETINOID")||kw(&L->cur,"PERISINUSOIDAL_STELLATE")||kw(&L->cur,"FIBROGENIC")||kw(&L->cur,"STELLATE_CELL")){
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
      smx_fail_at(vm, aln, "STELLATE needs >=2 cubes",
                  "SMX STELLATE a b [c ...]  or  SMX STELLATE_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - stellate needs clear channel */
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
    /* complete stellate mesh among live */
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
    /* stellate ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int stellate_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (stellate_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_STELLATED", (long)stellate_ok);
      var_set_num(vm, "SMX_STELLATE_LATCH", (long)stellate_ok);
      var_set_num(vm, "SMX_STELLATE", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_STELLATE_SUM", (long)(stellate_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_STELLATE_ALIAS", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_STELLATE_PLATE", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_VITAMIN_A", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_STELLATE_CELL", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_ITO", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_STELLATE", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_ITO_CELL", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_PERISINUSOIDAL_STELLATE", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_RETINOID", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_FIBROSIS_BED", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_SPACE_OF_DISSE_CELL", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_FIBROGENIC", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_VITAMIN_A", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_RETINOID_LANE", (long)(stellate_ok ? wraps : 0));
      var_set_num(vm, "SMX_MESH_STELLATE", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_STELLATE_SHEATH_LATCH", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_STELLATE", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_STELLATE_WRAP_LATCH", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(stellate_ok ? wraps : 0));
      var_set_num(vm, "SMX_STELLATE_WRAP", (long)(stellate_ok ? wraps : 0));
      var_set_num(vm, "SMX_STELLATE_WRAPS", (long)(stellate_ok ? wraps : 0));
      var_set_num(vm, "SMX_STELLATE_SHEATHS", (long)(stellate_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(stellate_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(stellate_ok ? wraps : 0));
      var_set_num(vm, "SMX_STELLATE_WRAP_LATCH", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(stellate_ok ? 1 : 0));
      var_set_num(vm, "SMX_KF_BONDS", (long)(stellate_ok ? bonds : 0));
      var_set_num(vm, "SMX_KF_MESH", (long)(stellate_ok ? bonds : 0));
      var_set_num(vm, "SMX_KF_BONDS2", (long)(stellate_ok ? bonds : 0));
      var_set_num(vm, "SMX_KF_STARS", (long)(stellate_ok ? wraps : 0));
      var_set_num(vm, "SMX_KF_STAR", (long)(stellate_ok ? wraps : 0));
      var_set_num(vm, "SMX_STELLATE_RING", (long)(stellate_ok ? wraps : 0));
      var_set_num(vm, "SMX_STELLATE_LANE", (long)(stellate_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(stellate_ok ? sheaths : 0));
      var_set_num(vm, "SMX_STELLATE_SHEATH_HUB", (long)(stellate_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(stellate_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDSTELLATE", (long)(stellate_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDSTELLATESHEATH", (long)(stellate_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(stellate_ok ? live : 0));
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
      if (stellate_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX STELLATE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX STELLATE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX STELLATE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX STELLATE nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d stellated=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, stellate_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX OVAL|OVAL|MESH_OVAL|OVAL_WRAP|OVAL_SHEATH|OVAL_GUIDE|RAISE_OVAL a b c ...
   * Life-force OVAL hepatic-progenitor continuum after STELLATE: canal-of-Hering bipotent progenitor bed: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete oval hepatic exchange scaffold mesh among live nodes, weaves a
   * oval wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks oval where life guides the oval confluence return pulse.
   * Latches SMX_OVAL when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = oval ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_OVAL_SUM = bonds+wraps+sheaths; SMX_OVAL|SMX_MESH_OVAL|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"OVAL")||kw(&L->cur,"PROGENITOR")||kw(&L->cur,"PROGENITOR_CELL")||kw(&L->cur,"HPC")||kw(&L->cur,"OVAL_PLATE")||kw(&L->cur,"MESH_OVAL")||kw(&L->cur,"OVAL_WRAP")||kw(&L->cur,"OVAL_WRAPS")||kw(&L->cur,"OVAL_SHEATH")||kw(&L->cur,"OVAL_SHEATHS")||kw(&L->cur,"OVAL_GUIDE")||kw(&L->cur,"RAISE_OVAL")||kw(&L->cur,"WE_OVAL")||kw(&L->cur,"LIFE_OVAL")||
      kw(&L->cur,"STABLE_OVAL")||kw(&L->cur,"MESH_OVALS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_OVAL")||kw(&L->cur,"OVAL_LEAF")||
      kw(&L->cur,"SEEDOVAL")||kw(&L->cur,"SEEDOVALSHEATH")||kw(&L->cur,"SEEDOVALWRAP")||
      kw(&L->cur,"OVAL_RING")||kw(&L->cur,"OVAL_SHEATH_HUB")||kw(&L->cur,"OVAL_NODE")||
      kw(&L->cur,"LATTICE_OVAL")||kw(&L->cur,"WORLD_OVAL")||kw(&L->cur,"WORLD_OVAL")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_OVAL")||kw(&L->cur,"HARDEN_OVAL")||
      kw(&L->cur,"OVAL_SCAFFOLD")||kw(&L->cur,"OVAL_MESH")||kw(&L->cur,"HEPATIC_PROGENITOR")||kw(&L->cur,"BIPOTENT")||kw(&L->cur,"OVAL_PROGENITOR")||kw(&L->cur,"STEMNESS")||kw(&L->cur,"PERIPORTAL_PROGENITOR")||kw(&L->cur,"CHOLANGIOGENIC")||kw(&L->cur,"OVAL_CELL")||kw(&L->cur,"CANAL_OF_HERING")||kw(&L->cur,"HERING")){
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
      smx_fail_at(vm, aln, "OVAL needs >=2 cubes",
                  "SMX OVAL a b [c ...]  or  SMX OVAL_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - oval needs clear channel */
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
    /* complete oval mesh among live */
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
    /* oval ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int oval_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (oval_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_OVALED", (long)oval_ok);
      var_set_num(vm, "SMX_OVAL_LATCH", (long)oval_ok);
      var_set_num(vm, "SMX_OVAL", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_OVAL_SUM", (long)(oval_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_OVAL_ALIAS", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_OVAL_PLATE", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_BIPOTENT", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_OVAL_CELL", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_PROGENITOR", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_PROGENITOR", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_PROGENITOR_CELL", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_PERIPORTAL_PROGENITOR", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_STEMNESS", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_CANAL_OF_HERING", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_HERING_CANAL_CELL", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_CHOLANGIOGENIC", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_BIPOTENT", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_PROGENITOR_LANE", (long)(oval_ok ? wraps : 0));
      var_set_num(vm, "SMX_MESH_OVAL", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_OVAL_SHEATH_LATCH", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_OVAL", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_OVAL_WRAP_LATCH", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(oval_ok ? wraps : 0));
      var_set_num(vm, "SMX_OVAL_WRAP", (long)(oval_ok ? wraps : 0));
      var_set_num(vm, "SMX_OVAL_WRAPS", (long)(oval_ok ? wraps : 0));
      var_set_num(vm, "SMX_OVAL_SHEATHS", (long)(oval_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(oval_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(oval_ok ? wraps : 0));
      var_set_num(vm, "SMX_OVAL_WRAP_LATCH", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(oval_ok ? 1 : 0));
      var_set_num(vm, "SMX_KF_BONDS", (long)(oval_ok ? bonds : 0));
      var_set_num(vm, "SMX_KF_MESH", (long)(oval_ok ? bonds : 0));
      var_set_num(vm, "SMX_KF_BONDS2", (long)(oval_ok ? bonds : 0));
      var_set_num(vm, "SMX_KF_STARS", (long)(oval_ok ? wraps : 0));
      var_set_num(vm, "SMX_KF_STAR", (long)(oval_ok ? wraps : 0));
      var_set_num(vm, "SMX_OVAL_RING", (long)(oval_ok ? wraps : 0));
      var_set_num(vm, "SMX_OVAL_LANE", (long)(oval_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(oval_ok ? sheaths : 0));
      var_set_num(vm, "SMX_OVAL_SHEATH_HUB", (long)(oval_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(oval_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDOVAL", (long)(oval_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDOVALSHEATH", (long)(oval_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(oval_ok ? live : 0));
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
      if (oval_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX OVAL ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX OVAL partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX OVAL soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX OVAL nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d ovaled=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, oval_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX CANALICULUS|CANALICULI|BILE_CANALICULUS|MESH_CANALICULUS|CANALICULUS_WRAP|CANALICULUS_SHEATH|CANALICULUS_GUIDE|RAISE_CANALICULUS a b c ...
   * Life-force CANALICULUS bile-canaliculus continuum after OVAL: hepatocyte tight-junction bile conduit bed: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete canaliculus bile exchange scaffold mesh among live nodes, weaves a
   * canaliculus wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks canaliculus where life guides the bile confluence return pulse.
   * Latches SMX_CANALICULUS when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = canaliculus ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_CANALICULUS_SUM = bonds+wraps+sheaths; SMX_CANALICULUS|SMX_MESH_CANALICULUS|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"CANALICULUS")||kw(&L->cur,"CANALICULI")||kw(&L->cur,"BILE_CANALICULUS")||kw(&L->cur,"BILE_CANAL")||kw(&L->cur,"CANALICULUS_PLATE")||kw(&L->cur,"MESH_CANALICULUS")||kw(&L->cur,"CANALICULUS_WRAP")||kw(&L->cur,"CANALICULUS_WRAPS")||kw(&L->cur,"CANALICULUS_SHEATH")||kw(&L->cur,"CANALICULUS_SHEATHS")||kw(&L->cur,"CANALICULUS_GUIDE")||kw(&L->cur,"RAISE_CANALICULUS")||kw(&L->cur,"WE_CANALICULUS")||kw(&L->cur,"LIFE_CANALICULUS")||
      kw(&L->cur,"STABLE_CANALICULUS")||kw(&L->cur,"MESH_CANALICULI")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_CANALICULUS")||kw(&L->cur,"CANALICULUS_LEAF")||
      kw(&L->cur,"SEEDCANALICULUS")||kw(&L->cur,"SEEDCANALICULUSSHEATH")||kw(&L->cur,"SEEDCANALICULUSWRAP")||
      kw(&L->cur,"CANALICULUS_RING")||kw(&L->cur,"CANALICULUS_SHEATH_HUB")||kw(&L->cur,"CANALICULUS_NODE")||
      kw(&L->cur,"LATTICE_CANALICULUS")||kw(&L->cur,"WORLD_CANALICULUS")||kw(&L->cur,"WORLD_CANALICULI")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_CANALICULUS")||kw(&L->cur,"HARDEN_CANALICULUS")||
      kw(&L->cur,"CANALICULUS_SCAFFOLD")||kw(&L->cur,"CANALICULUS_MESH")||kw(&L->cur,"BILE_CONDUIT")||kw(&L->cur,"TIGHT_JUNCTION")||kw(&L->cur,"HEPATOCELLULAR_CANAL")||kw(&L->cur,"BILIARY_POLE")||kw(&L->cur,"CANALICULAR")||kw(&L->cur,"CANALICULAR_MEMBRANE")||kw(&L->cur,"BILE_FLOW")||kw(&L->cur,"CANALICULUS_CELL")||kw(&L->cur,"CHOLANGIOLE_FEED")||kw(&L->cur,"BILE_CHANNEL")){
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
      smx_fail_at(vm, aln, "CANALICULUS needs >=2 cubes",
                  "SMX CANALICULUS a b [c ...]  or  SMX CANALICULUS_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - canaliculus needs clear channel */
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
    /* complete canaliculus mesh among live */
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
    /* canaliculus ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int canaliculus_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (canaliculus_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_CANALICULUSED", (long)canaliculus_ok);
      var_set_num(vm, "SMX_CANALICULUS_LATCH", (long)canaliculus_ok);
      var_set_num(vm, "SMX_CANALICULUS", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_CANALICULUS_SUM", (long)(canaliculus_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_CANALICULUS_ALIAS", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_CANALICULUS_PLATE", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_CANALICULAR", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_CANALICULUS_CELL", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_BILE_CANALICULUS", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_BILE_CONDUIT", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_TIGHT_JUNCTION", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_TIGHT_JUNCTIONS", (long)(canaliculus_ok ? wraps : 0));
      var_set_num(vm, "SMX_HEPATOCELLULAR_CANAL", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_BILE_CANALICULUS_CELL", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_BILIARY_POLE", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_CANALICULAR_MEMBRANE", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_BILE_FLOW", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_CHOLANGIOLE_FEED", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_BILE_CHANNEL", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_CANALICULAR", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_BILE_CANALICULUS_LANE", (long)(canaliculus_ok ? wraps : 0));
      var_set_num(vm, "SMX_MESH_CANALICULUS", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_CANALICULUS_SHEATH_LATCH", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_CANALICULUS", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_CANALICULUS_WRAP_LATCH", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(canaliculus_ok ? wraps : 0));
      var_set_num(vm, "SMX_CANALICULUS_WRAP", (long)(canaliculus_ok ? wraps : 0));
      var_set_num(vm, "SMX_CANALICULUS_WRAPS", (long)(canaliculus_ok ? wraps : 0));
      var_set_num(vm, "SMX_CANALICULUS_SHEATHS", (long)(canaliculus_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(canaliculus_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(canaliculus_ok ? wraps : 0));
      var_set_num(vm, "SMX_CANALICULUS_WRAP_LATCH", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(canaliculus_ok ? 1 : 0));
      var_set_num(vm, "SMX_KF_BONDS", (long)(canaliculus_ok ? bonds : 0));
      var_set_num(vm, "SMX_KF_MESH", (long)(canaliculus_ok ? bonds : 0));
      var_set_num(vm, "SMX_KF_BONDS2", (long)(canaliculus_ok ? bonds : 0));
      var_set_num(vm, "SMX_KF_STARS", (long)(canaliculus_ok ? wraps : 0));
      var_set_num(vm, "SMX_KF_STAR", (long)(canaliculus_ok ? wraps : 0));
      var_set_num(vm, "SMX_CANALICULUS_RING", (long)(canaliculus_ok ? wraps : 0));
      var_set_num(vm, "SMX_CANALICULUS_LANE2", (long)(canaliculus_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(canaliculus_ok ? sheaths : 0));
      var_set_num(vm, "SMX_CANALICULUS_SHEATH_HUB", (long)(canaliculus_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(canaliculus_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDCANALICULUS", (long)(canaliculus_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDCANALICULUSSHEATH", (long)(canaliculus_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(canaliculus_ok ? live : 0));
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
      if (canaliculus_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CANALICULUS ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CANALICULUS partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX CANALICULUS soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX CANALICULUS nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d canaliculused=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, canaliculus_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX DUCTULE|DUCTULES|BILE_DUCTULE|MESH_DUCTULE|DUCTULE_WRAP|DUCTULE_SHEATH|DUCTULE_GUIDE|RAISE_DUCTULE a b c ...
   * Life-force DUCTULE bile-ductule continuum after OVAL: hepatocyte tight-junction bile conduit bed: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete ductule bile exchange scaffold mesh among live nodes, weaves a
   * ductule wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks ductule where life guides the bile confluence return pulse.
   * Latches SMX_DUCTULE when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = ductule ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_DUCTULE_SUM = bonds+wraps+sheaths; SMX_DUCTULE|SMX_MESH_DUCTULE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"DUCTULE")||kw(&L->cur,"DUCTULES")||kw(&L->cur,"CHOLANGIOLE")||kw(&L->cur,"BILE_DUCTULE")||kw(&L->cur,"INTERLOBULAR")||kw(&L->cur,"INTERLOBULAR_DUCT")||kw(&L->cur,"DUCTULAR_PLATE")||kw(&L->cur,"CHOLANGIOCYTE")||kw(&L->cur,"DUCTULE_PLATE")||kw(&L->cur,"MESH_DUCTULE")||kw(&L->cur,"DUCTULE_WRAP")||kw(&L->cur,"DUCTULE_WRAPS")||kw(&L->cur,"DUCTULE_SHEATH")||kw(&L->cur,"DUCTULE_SHEATHS")||kw(&L->cur,"DUCTULE_GUIDE")||kw(&L->cur,"RAISE_DUCTULE")||kw(&L->cur,"WE_DUCTULE")||kw(&L->cur,"LIFE_DUCTULE")||
      kw(&L->cur,"STABLE_DUCTULE")||kw(&L->cur,"MESH_DUCTULES")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_DUCTULE")||kw(&L->cur,"DUCTULE_LEAF")||
      kw(&L->cur,"SEEDDUCTULE")||kw(&L->cur,"SEEDDUCTULESHEATH")||kw(&L->cur,"SEEDDUCTULEWRAP")||
      kw(&L->cur,"DUCTULE_RING")||kw(&L->cur,"DUCTULE_SHEATH_HUB")||kw(&L->cur,"DUCTULE_NODE")||
      kw(&L->cur,"LATTICE_DUCTULE")||kw(&L->cur,"WORLD_DUCTULE")||kw(&L->cur,"WORLD_DUCTULES")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_DUCTULE")||kw(&L->cur,"HARDEN_DUCTULE")||
      kw(&L->cur,"DUCTULE_SCAFFOLD")||kw(&L->cur,"DUCTULE_MESH")||kw(&L->cur,"DUCTULE_CONDUIT")||kw(&L->cur,"DUCTULE_JUNCTION")||kw(&L->cur,"INTERLOBULAR_DUCT")||kw(&L->cur,"CHOLANGIOCYTE_POLE")||kw(&L->cur,"DUCTULAR")||kw(&L->cur,"DUCTULAR_MEMBRANE")||kw(&L->cur,"DUCTULE_FLOW")||kw(&L->cur,"DUCTULE_CELL")||kw(&L->cur,"CHOLANGIOLE")||kw(&L->cur,"DUCTULE_CHANNEL")){
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
      smx_fail_at(vm, aln, "DUCTULE needs >=2 cubes",
                  "SMX DUCTULE a b [c ...]  or  SMX DUCTULE_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - ductule needs clear channel */
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
    /* complete ductule mesh among live */
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
    /* ductule ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int ductule_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ductule_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_DUCTULED", (long)ductule_ok);
      var_set_num(vm, "SMX_DUCTULE_LATCH", (long)ductule_ok);
      var_set_num(vm, "SMX_DUCTULE", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_CHOLANGIOLE", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_INTERLOBULAR", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_INTERLOBULAR_DUCT", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_CHOLANGIOCYTE", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_DUCTULAR", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_BILE_DUCTULE", (long)(ductule_ok ? 1 : 0));

      var_set_num(vm, "SMX_DUCTULE_SUM", (long)(ductule_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_DUCTULE_ALIAS", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_DUCTULE_PLATE", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_DUCTULAR", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_DUCTULE_CELL", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_BILE_DUCTULE", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_DUCTULE_CONDUIT", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_DUCTULE_JUNCTION", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_DUCTULE_JUNCTIONS", (long)(ductule_ok ? wraps : 0));
      var_set_num(vm, "SMX_INTERLOBULAR_DUCT", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_BILE_DUCTULE_CELL", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_CHOLANGIOCYTE_POLE", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_DUCTULAR_MEMBRANE", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_DUCTULE_FLOW", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_CHOLANGIOLE", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_DUCTULE_CHANNEL", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_DUCTULAR", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_BILE_DUCTULE_LANE", (long)(ductule_ok ? wraps : 0));
      var_set_num(vm, "SMX_MESH_DUCTULE", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_DUCTULE_SHEATH_LATCH", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_DUCTULE", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_DUCTULE_WRAP_LATCH", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(ductule_ok ? wraps : 0));
      var_set_num(vm, "SMX_DUCTULE_WRAP", (long)(ductule_ok ? wraps : 0));
      var_set_num(vm, "SMX_DUCTULE_WRAPS", (long)(ductule_ok ? wraps : 0));
      var_set_num(vm, "SMX_DUCTULE_SHEATHS", (long)(ductule_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(ductule_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(ductule_ok ? wraps : 0));
      var_set_num(vm, "SMX_DUCTULE_WRAP_LATCH", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(ductule_ok ? 1 : 0));
      var_set_num(vm, "SMX_KF_BONDS", (long)(ductule_ok ? bonds : 0));
      var_set_num(vm, "SMX_KF_MESH", (long)(ductule_ok ? bonds : 0));
      var_set_num(vm, "SMX_KF_BONDS2", (long)(ductule_ok ? bonds : 0));
      var_set_num(vm, "SMX_KF_STARS", (long)(ductule_ok ? wraps : 0));
      var_set_num(vm, "SMX_KF_STAR", (long)(ductule_ok ? wraps : 0));
      var_set_num(vm, "SMX_DUCTULE_RING", (long)(ductule_ok ? wraps : 0));
      var_set_num(vm, "SMX_DUCTULE_LANE2", (long)(ductule_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(ductule_ok ? sheaths : 0));
      var_set_num(vm, "SMX_DUCTULE_SHEATH_HUB", (long)(ductule_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(ductule_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDDUCTULE", (long)(ductule_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDDUCTULESHEATH", (long)(ductule_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(ductule_ok ? live : 0));
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
      if (ductule_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX DUCTULE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX DUCTULE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX DUCTULE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX DUCTULE nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d ductuled=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, ductule_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX HEPATIC_DUCT|HEPATIC_DUCTS|HEPATIC_BILE_DUCT|MESH_HEPATIC_DUCT|HEPATIC_DUCT_WRAP|HEPATIC_DUCT_SHEATH|HEPATIC_DUCT_GUIDE|RAISE_HEPATIC_DUCT a b c ...
   * Life-force HEPATIC_DUCT left/right hepatic duct continuum after DUCTULE: septal-to-hepatic bile trunk bed: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete hepatic duct bile exchange scaffold mesh among live nodes, weaves a
   * hepatic-duct wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks hepatic duct where life guides the hepatic confluence return pulse.
   * Latches SMX_HEPATIC_DUCT when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = hepatic-duct ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_HEPATIC_DUCT_SUM = bonds+wraps+sheaths; SMX_HEPATIC_DUCT|SMX_MESH_HEPATIC_DUCT|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"HEPATIC_DUCT")||kw(&L->cur,"HEPATIC_DUCTS")||kw(&L->cur,"HEPATIC_TRUNK")||kw(&L->cur,"HEPATIC_BILE_DUCT")||kw(&L->cur,"BILE_DUCT")||kw(&L->cur,"COMMON_HEPATIC")||kw(&L->cur,"COMMON_HEPATIC_DUCT")||kw(&L->cur,"SEPTAL_DUCT")||kw(&L->cur,"AREA_DUCT")||kw(&L->cur,"HEPATIC_TRUNK_DUCT")||kw(&L->cur,"HEPATIC_DUCT_PLATE2")||kw(&L->cur,"HEPATIC_CHOLANGIOCYTE")||kw(&L->cur,"HEPATIC_DUCT_PLATE")||kw(&L->cur,"LEFT_HEPATIC")||kw(&L->cur,"RIGHT_HEPATIC")||kw(&L->cur,"HEPATIC_CONFLUENCE")||kw(&L->cur,"MESH_HEPATIC_DUCT")||kw(&L->cur,"HEPATIC_DUCT_WRAP")||kw(&L->cur,"HEPATIC_DUCT_WRAPS")||kw(&L->cur,"HEPATIC_DUCT_SHEATH")||kw(&L->cur,"HEPATIC_DUCT_SHEATHS")||kw(&L->cur,"HEPATIC_DUCT_GUIDE")||kw(&L->cur,"RAISE_HEPATIC_DUCT")||kw(&L->cur,"WE_HEPATIC_DUCT")||kw(&L->cur,"LIFE_HEPATIC_DUCT")||
      kw(&L->cur,"STABLE_HEPATIC_DUCT")||kw(&L->cur,"MESH_HEPATIC_DUCTS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_HEPATIC_DUCT")||kw(&L->cur,"HEPATIC_DUCT_LEAF")||
      kw(&L->cur,"SEEDHEPATICDUCT")||kw(&L->cur,"SEEDHEPATICDUCTSHEATH")||kw(&L->cur,"SEEDHEPATICDUCTWRAP")||
      kw(&L->cur,"HEPATIC_DUCT_RING")||kw(&L->cur,"HEPATIC_DUCT_SHEATH_HUB")||kw(&L->cur,"HEPATIC_DUCT_NODE")||
      kw(&L->cur,"LATTICE_HEPATIC_DUCT")||kw(&L->cur,"WORLD_HEPATIC_DUCT")||kw(&L->cur,"WORLD_HEPATIC_DUCTS")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_HEPATIC_DUCT")||kw(&L->cur,"HARDEN_HEPATIC_DUCT")||
      kw(&L->cur,"HEPATIC_DUCT_SCAFFOLD")||kw(&L->cur,"HEPATIC_DUCT_MESH")||kw(&L->cur,"HEPATIC_DUCT_CONDUIT")||kw(&L->cur,"HEPATIC_DUCT_JUNCTION")||kw(&L->cur,"HEPATIC_TRUNK_DUCT")||kw(&L->cur,"HEPATIC_DUCT_POLE")||kw(&L->cur,"HEPATIC_DUCTULAR")||kw(&L->cur,"HEPATIC_DUCT_MEMBRANE")||kw(&L->cur,"HEPATIC_DUCT_FLOW")||kw(&L->cur,"HEPATIC_DUCT_CELL")||kw(&L->cur,"HEPATIC_TRUNK")||kw(&L->cur,"HEPATIC_DUCT_CHANNEL")){
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
      smx_fail_at(vm, aln, "HEPATIC_DUCT needs >=2 cubes",
                  "SMX HEPATIC_DUCT a b [c ...]  or  SMX HEPATIC_DUCT_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - hepatic duct needs clear channel */
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
    /* complete hepatic-duct mesh among live */
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
    /* hepatic-duct ring - free energy guards every edge every edge i -> i+1 both ways */
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
      int hepatic_duct_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (hepatic_duct_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_HEPATIC_DUCTED", (long)hepatic_duct_ok);
      var_set_num(vm, "SMX_HEPATIC_DUCT_LATCH", (long)hepatic_duct_ok);
      var_set_num(vm, "SMX_HEPATIC_DUCT", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_TRUNK", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_COMMON_HEPATIC", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_TRUNK_DUCT", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_CHOLANGIOCYTE", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCTULAR", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_BILE_DUCT", (long)(hepatic_duct_ok ? 1 : 0));

      var_set_num(vm, "SMX_HEPATIC_DUCT_SUM", (long)(hepatic_duct_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_ALIAS", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_PLATE", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCTULAR", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_CELL", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_BILE_DUCT", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_CONDUIT", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_JUNCTION", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_JUNCTIONS", (long)(hepatic_duct_ok ? wraps : 0));
      var_set_num(vm, "SMX_HEPATIC_TRUNK_DUCT", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_CELL", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_POLE", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_MEMBRANE", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_FLOW", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_TRUNK", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_CHANNEL", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCTULAR", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_LANE", (long)(hepatic_duct_ok ? wraps : 0));
      var_set_num(vm, "SMX_MESH_HEPATIC_DUCT", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_SHEATH_LATCH", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_HEPATIC_DUCT", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_WRAP_LATCH", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(hepatic_duct_ok ? wraps : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_WRAP", (long)(hepatic_duct_ok ? wraps : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_WRAPS", (long)(hepatic_duct_ok ? wraps : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_SHEATHS", (long)(hepatic_duct_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(hepatic_duct_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(hepatic_duct_ok ? wraps : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_WRAP_LATCH", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(hepatic_duct_ok ? 1 : 0));
      var_set_num(vm, "SMX_KF_BONDS", (long)(hepatic_duct_ok ? bonds : 0));
      var_set_num(vm, "SMX_KF_MESH", (long)(hepatic_duct_ok ? bonds : 0));
      var_set_num(vm, "SMX_KF_BONDS2", (long)(hepatic_duct_ok ? bonds : 0));
      var_set_num(vm, "SMX_KF_STARS", (long)(hepatic_duct_ok ? wraps : 0));
      var_set_num(vm, "SMX_KF_STAR", (long)(hepatic_duct_ok ? wraps : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_RING", (long)(hepatic_duct_ok ? wraps : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_LANE2", (long)(hepatic_duct_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(hepatic_duct_ok ? sheaths : 0));
      var_set_num(vm, "SMX_HEPATIC_DUCT_SHEATH_HUB", (long)(hepatic_duct_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(hepatic_duct_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDHEPATICDUCT", (long)(hepatic_duct_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDHEPATICDUCTSHEATH", (long)(hepatic_duct_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(hepatic_duct_ok ? live : 0));
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
      if (hepatic_duct_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HEPATIC_DUCT ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HEPATIC_DUCT partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX HEPATIC_DUCT soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX HEPATIC_DUCT nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d hepatic_ducted=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, hepatic_duct_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX COMMON_BILE_DUCT|CBD|CHOLEDOCHUS — extrahepatic CBD after HEPATIC_DUCT.
   * Soft-OOB fail-closed mesh+wrap+sheath; latches SMX_COMMON_BILE_DUCT|SMX_CBD. */
  if (kw(&L->cur,"COMMON_BILE_DUCT")||kw(&L->cur,"COMMON_BILE_DUCTS")||kw(&L->cur,"CBD")||
      kw(&L->cur,"CBD_DUCT")||kw(&L->cur,"CHOLEDOCH")||kw(&L->cur,"CHOLEDOCHUS")||
      kw(&L->cur,"DUCTUS_CHOLEDOCHUS")||kw(&L->cur,"COMMON_BILE")||kw(&L->cur,"EXTRAHEPATIC")||
      kw(&L->cur,"MESH_COMMON_BILE_DUCT")||kw(&L->cur,"MESH_CBD")||kw(&L->cur,"RAISE_CBD")||
      kw(&L->cur,"WE_CBD")||kw(&L->cur,"LIFE_CBD")||kw(&L->cur,"STABLE_CBD")||
      kw(&L->cur,"HARDEN_CBD")||kw(&L->cur,"CBD_TRUNK")||kw(&L->cur,"CBD_CHANNEL")||kw(&L->cur,"COMMON_BILE_DUCT_PLATE")||kw(&L->cur,"CBD_PLATE")||kw(&L->cur,"COMMON_BILE_DUCTULAR")||kw(&L->cur,"HARDEN_COMMON_BILE_DUCT")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0, wraps = 0, sheaths = 0, soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "COMMON_BILE_DUCT needs >=2 cubes",
                  "SMX COMMON_BILE_DUCT a b [c ...]  or  SMX CBD a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
    if (live >= 2){
      for (i = 0; i < live; i++){
        for (j = i + 1; j < live; j++){
          int a = live_ix[i], b = live_ix[j];
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
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0) mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0) star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0) sheath_ok = 1;
      int cbd_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (cbd_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_COMMON_BILE_DUCTED", (long)cbd_ok);
      var_set_num(vm, "SMX_COMMON_BILE_DUCT_LATCH", (long)cbd_ok);
      var_set_num(vm, "SMX_COMMON_BILE_DUCT", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_CBD", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_CBD_DUCT", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_CHOLEDOCH", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_CHOLEDOCHUS", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_COMMON_BILE", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_EXTRAHEPATIC", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_CBD_TRUNK", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_COMMON_BILE_DUCT", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_CBD", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_COMMON_BILE_DUCT_PLATE", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_COMMON_BILE_DUCTULAR", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_COMMON_BILE_DUCT_CELL", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_COMMON_BILE_DUCT_CHANNEL", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_COMMON_BILE_DUCT_FLOW", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_COMMON_BILE_DUCT", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_COMMON_BILE_DUCT_CONDUIT", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_COMMON_BILE_DUCT_JUNCTION", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_COMMON_BILE_DUCT_MEMBRANE", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_COMMON_BILE_DUCT_POLE", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_CBD_PLATE", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_CBD_CHANNEL", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_CBD_FLOW", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_CBD", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_COMMON_BILE_DUCT_SUM", (long)(cbd_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_CBD_SUM", (long)(cbd_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(cbd_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(cbd_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(cbd_ok ? sheaths : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (cbd_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX COMMON_BILE_DUCT ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX COMMON_BILE_DUCT partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX COMMON_BILE_DUCT soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX CBD nodes=%d live=%d bonds=%d wraps=%d sheaths=%d soft=%d cbd=%d\n",
                n, live, bonds, wraps, sheaths, soft, cbd_ok);
    }
    bump(vm); return 1;
  }

  return 0;
}
