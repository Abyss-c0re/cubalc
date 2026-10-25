/* CubalC lang — lang_ops_smx_pneuma_ward.c
 * SMX ward plane: AUTOHEAL · RESILIENCE · OMNIVERSE · SEER · FORGED · KEEPER
 * Peel from pneuma_a. Leaves pure dispatcher. Cube is SoT.
 */
#include "lang/cubalc_lang_ops_smx_pneuma_ward.h"
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
int cubalc_lang_ops_smx_try_pneuma_ward(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* SMX AUTOHEAL|AH|MULTI|LIFE_BEACON|WE_AUTOHEAL|MESH_AUTOHEAL|RAISE_AUTOHEAL a b c ...
   * Life-force autoheal mesh stability after multiverse: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete autoheal mesh among live nodes, weaves a
   * mend ring (i -> i+1) so free energy self-regulates, then beacon hub
   * gathers return so lattice locks autoheal where life holds the hive.
   * Latches SMX_AUTOHEALED when mesh+versepaths+hubs are soft-OOB-free.
   * SMX_AHS = chain bonds; SMX_BEACON hub = root gather pulses;
   * SMX_AUTOHEAL sum = bonds+versepaths+hubs; SMX_HEAL|SMX_REGEN sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"AUTOHEAL")||kw(&L->cur,"AH")||kw(&L->cur,"HEAL")||kw(&L->cur,"MEND")||kw(&L->cur,"MENDS")||kw(&L->cur,"REGEN")||kw(&L->cur,"WE_AUTOHEAL")||
      kw(&L->cur,"LIFE_BEACON")||kw(&L->cur,"REGEN")||kw(&L->cur,"WE_AUTOHEAL")||
      kw(&L->cur,"MENDLANE")||kw(&L->cur,"MENDLANES")||kw(&L->cur,"HEALSPACE")||
      kw(&L->cur,"BEACON")||kw(&L->cur,"BEACONS")||kw(&L->cur,"MENDS")||
      kw(&L->cur,"MEND")||kw(&L->cur,"HEALWEB")||kw(&L->cur,"AHS")||
      kw(&L->cur,"MESH_AUTOHEAL")||kw(&L->cur,"RAISE_AUTOHEAL")||
      kw(&L->cur,"AUTOHEALS")||kw(&L->cur,"AHS")||
      kw(&L->cur,"SEEDAH")||kw(&L->cur,"SEEDHEAL")||
      kw(&L->cur,"LATTICE_AUTOHEAL")||kw(&L->cur,"WORLD_AH")||kw(&L->cur,"WORLD_AUTOHEAL")||
      kw(&L->cur,"MEND_RING")||kw(&L->cur,"BEACON_HUB")||kw(&L->cur,"PULSE_AUTOHEAL")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int mends = 0;
    int beacons = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "AUTOHEAL needs >=2 cubes",
                  "SMX AUTOHEAL a b [c ...]  or  SMX AH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - autoheal needs clear channel */
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
    /* complete autoheal mesh among live */
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
    /* starpath ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else mends++;
        }
      }
    }
    /* beacon hub - seed axis return from every live leaf */
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
          else beacons++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int mend_ok = (live >= 2 && mends >= live && soft == 0) ? 1 : 0;
      if (!mend_ok && live >= 2 && mends * 2 >= live && soft == 0)
        mend_ok = 1;
      int beacon_ok = (live >= 1 && beacons >= live && soft == 0) ? 1 : 0;
      if (!beacon_ok && live >= 1 && beacons * 2 >= live && soft == 0)
        beacon_ok = 1;
      int ah_ok = (mesh_ok && mend_ok && beacon_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ah_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (mends > 0 ? 1 : 0) + (beacons > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_AUTOHEALED", (long)ah_ok);
      var_set_num(vm, "SMX_AUTOHEALED_LATCH", (long)ah_ok);
      var_set_num(vm, "SMX_AUTOHEAL", (long)(ah_ok ? bonds + mends + beacons : 0));
      var_set_num(vm, "SMX_AH", (long)(ah_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEAL", (long)(ah_ok ? 1 : 0));
      var_set_num(vm, "SMX_REGEN", (long)(ah_ok ? 1 : 0));
      var_set_num(vm, "SMX_AHS", (long)(ah_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEALLANE", (long)(ah_ok ? bonds : 0));
      var_set_num(vm, "SMX_AHS", (long)(ah_ok ? bonds : 0));
      var_set_num(vm, "SMX_HEALS", (long)(ah_ok ? bonds : 0));
      var_set_num(vm, "SMX_MENDS", (long)(ah_ok ? mends : 0));
      var_set_num(vm, "SMX_MEND", (long)(ah_ok ? mends : 0));
      var_set_num(vm, "SMX_MENDRING", (long)(ah_ok ? mends : 0));
      var_set_num(vm, "SMX_MENDLANE", (long)(ah_ok ? mends : 0));
      var_set_num(vm, "SMX_BEACONS", (long)(ah_ok ? beacons : 0));
      var_set_num(vm, "SMX_BEACON_HUB", (long)(ah_ok ? beacons : 0));
      var_set_num(vm, "SMX_BEACON", (long)(ah_ok ? beacons : 0));
      var_set_num(vm, "SMX_SEEDHEAL", (long)(ah_ok ? beacons : 0));
      var_set_num(vm, "SMX_LIFE_BEACON", (long)(ah_ok ? beacons : 0));
      var_set_num(vm, "SMX_MESH", (long)(ah_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + mends + beacons));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (ah_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AUTOHEAL ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AUTOHEAL partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX AUTOHEAL soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX AUTOHEAL nodes=%d live=%d bonds=%d mends=%d beacons=%d need=%d soft=%d talks=%d oob=%d autohealed=%d vital=%ld\n",
                n, live, bonds, mends, beacons, need, soft, vm->smx_talks, vm->smx_oob, ah_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX RESILIENCE|AH|MULTI|LIFE_ANCHOR|WE_RESILIENCE|MESH_RESILIENCE|RAISE_RESILIENCE a b c ...
   * Life-force resilience mesh stability after autoheal: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete resilience mesh among live nodes, weaves a
   * guard ring (i -> i+1) so free energy self-regulates, then anchor hub
   * gathers return so lattice locks resilience where life holds the hive.
   * Latches SMX_RESILIENT when mesh+versepaths+hubs are soft-OOB-free.
   * SMX_RSS = chain bonds; SMX_ANCHOR hub = root gather pulses;
   * SMX_RESILIENCE sum = bonds+versepaths+hubs; SMX_GUARD|SMX_RESIL sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  /* bare GUARD reserved for TRY…FINALLY alias (flow_ctrl) — not SMX RESILIENCE */
  if (kw(&L->cur,"RESILIENCE")||kw(&L->cur,"RS")||kw(&L->cur,"RESIL")||kw(&L->cur,"GUARDS")||kw(&L->cur,"RESILIENT")||kw(&L->cur,"WE_RESILIENCE")||
      kw(&L->cur,"LIFE_ANCHOR")||kw(&L->cur,"WE_RESILIENCE")||
      kw(&L->cur,"MESH_RESILIENCE")||kw(&L->cur,"RAISE_RESILIENCE")||
      kw(&L->cur,"RESILIENCES")||
      kw(&L->cur,"SEEDRESIL")||kw(&L->cur,"SEEDGUARD")||
      kw(&L->cur,"LATTICE_RESILIENCE")||kw(&L->cur,"WORLD_RS")||kw(&L->cur,"WORLD_RESILIENCE")||
      kw(&L->cur,"GUARD_RING")||kw(&L->cur,"ANCHOR_HUB")||kw(&L->cur,"PULSE_RESILIENCE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int guards = 0;
    int anchors = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "RESILIENCE needs >=2 cubes",
                  "SMX RESILIENCE a b [c ...]  or  SMX RS a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - resilience needs clear channel */
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
    /* complete resilience mesh among live */
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
    /* starpath ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else guards++;
        }
      }
    }
    /* anchor hub - seed axis return from every live leaf */
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
          else anchors++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int guard_ok = (live >= 2 && guards >= live && soft == 0) ? 1 : 0;
      if (!guard_ok && live >= 2 && guards * 2 >= live && soft == 0)
        guard_ok = 1;
      int anchor_ok = (live >= 1 && anchors >= live && soft == 0) ? 1 : 0;
      if (!anchor_ok && live >= 1 && anchors * 2 >= live && soft == 0)
        anchor_ok = 1;
      int rs_ok = (mesh_ok && guard_ok && anchor_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (rs_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (guards > 0 ? 1 : 0) + (anchors > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_RESILIENT", (long)rs_ok);
      var_set_num(vm, "SMX_RESILIENT_LATCH", (long)rs_ok);
      var_set_num(vm, "SMX_RESILIENCE", (long)(rs_ok ? bonds + guards + anchors : 0));
      var_set_num(vm, "SMX_RS", (long)(rs_ok ? 1 : 0));
      var_set_num(vm, "SMX_RESIL", (long)(rs_ok ? 1 : 0));
      var_set_num(vm, "SMX_RSS", (long)(rs_ok ? bonds : 0));
      var_set_num(vm, "SMX_GUARDLANE", (long)(rs_ok ? guards : 0));
      var_set_num(vm, "SMX_GUARDS", (long)(rs_ok ? guards : 0));
      var_set_num(vm, "SMX_GUARD", (long)(rs_ok ? guards : 0));
      var_set_num(vm, "SMX_GUARDRING", (long)(rs_ok ? guards : 0));
      var_set_num(vm, "SMX_ANCHORS", (long)(rs_ok ? anchors : 0));
      var_set_num(vm, "SMX_ANCHOR_HUB", (long)(rs_ok ? anchors : 0));
      var_set_num(vm, "SMX_ANCHOR", (long)(rs_ok ? anchors : 0));
      var_set_num(vm, "SMX_SEEDRESIL", (long)(rs_ok ? anchors : 0));
      var_set_num(vm, "SMX_LIFE_ANCHOR", (long)(rs_ok ? anchors : 0));
      var_set_num(vm, "SMX_MESH", (long)(rs_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + guards + anchors));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (rs_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX RESILIENCE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX RESILIENCE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX RESILIENCE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX RESILIENCE nodes=%d live=%d bonds=%d guards=%d anchors=%d need=%d soft=%d talks=%d oob=%d resilient=%d vital=%ld\n",
                n, live, bonds, guards, anchors, need, soft, vm->smx_talks, vm->smx_oob, rs_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX OMNIVERSE|OMNI|ALLVERSE|TOTALVERSE|MESH_OMNIVERSE|RAISE_OMNIVERSE a b c ...
   * Life-force omniverse mesh stability after autoheal: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete omniverse mesh among live nodes, weaves an
   * allpath ring (i -> i+1) so free energy spans every realm, then nexus hub
   * gathers return so lattice locks omniverse where life holds the hive.
   * Latches SMX_OMNIVERSED when mesh+allpaths+hubs are soft-OOB-free.
   * SMX_ALLPATHS = chain bonds; SMX_NEXUS hub = root gather pulses;
   * SMX_OMNIVERSE sum = bonds+allpaths+hubs; SMX_OMNI|SMX_TOTAL sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"OMNIVERSE")||kw(&L->cur,"OMNI")||kw(&L->cur,"ALLVERSE")||kw(&L->cur,"TOTALVERSE")||kw(&L->cur,"TOTAL")||kw(&L->cur,"WE_OMNI")||kw(&L->cur,"LIFE_NEXUS")||
      kw(&L->cur,"OMNIS")||kw(&L->cur,"ALLVERSES")||kw(&L->cur,"TOTALVERSES")||
      kw(&L->cur,"ALLPATH")||kw(&L->cur,"ALLPATHS")||kw(&L->cur,"NEXUS")||
      kw(&L->cur,"NEXUSES")||kw(&L->cur,"SEEDOMNI")||kw(&L->cur,"SEEDALL")||
      kw(&L->cur,"ALLPATH_RING")||kw(&L->cur,"NEXUS_HUB")||
      kw(&L->cur,"MESH_OMNIVERSE")||kw(&L->cur,"RAISE_OMNIVERSE")||
      kw(&L->cur,"OMNIVERSES")||kw(&L->cur,"OMNIS")||
      kw(&L->cur,"SEEDOMNI")||kw(&L->cur,"SEEDTOTAL")||
      kw(&L->cur,"LATTICE_OMNIVERSE")||kw(&L->cur,"WORLD_OMNI")||kw(&L->cur,"WORLD_OMNIVERSE")||
      kw(&L->cur,"ALLPATH_RING")||kw(&L->cur,"NEXUS_HUB")||kw(&L->cur,"PULSE_OMNIVERSE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int paths = 0;
    int hubs = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "OMNIVERSE needs >=2 cubes",
                  "SMX OMNIVERSE a b [c ...]  or  SMX OMNI a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - omniverse needs clear channel */
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
    /* complete omniverse mesh among live */
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
    /* allpath ring - free energy spans every realm every edge i -> i+1 both ways */
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
          else paths++;
        }
      }
    }
    /* nexus hub - seed axis return from every live leaf */
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
          else hubs++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int path_ok = (live >= 2 && paths >= live && soft == 0) ? 1 : 0;
      if (!path_ok && live >= 2 && paths * 2 >= live && soft == 0)
        path_ok = 1;
      int hub_ok = (live >= 1 && hubs >= live && soft == 0) ? 1 : 0;
      if (!hub_ok && live >= 1 && hubs * 2 >= live && soft == 0)
        hub_ok = 1;
      int om_ok = (mesh_ok && path_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (om_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (paths > 0 ? 1 : 0) + (hubs > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_OMNIVERSED", (long)om_ok);
      var_set_num(vm, "SMX_OMNIVERSED_LATCH", (long)om_ok);
      var_set_num(vm, "SMX_OMNIVERSE", (long)(om_ok ? bonds + paths + hubs : 0));
      var_set_num(vm, "SMX_OMNI", (long)(om_ok ? 1 : 0));
      var_set_num(vm, "SMX_TOTAL", (long)(om_ok ? 1 : 0));
      var_set_num(vm, "SMX_TOTALVERSE", (long)(om_ok ? 1 : 0));
      var_set_num(vm, "SMX_ALLVERSE", (long)(om_ok ? 1 : 0));
      var_set_num(vm, "SMX_OMNIS", (long)(om_ok ? bonds : 0));
      var_set_num(vm, "SMX_ALLVERSE", (long)(om_ok ? bonds : 0));
      var_set_num(vm, "SMX_OMNIS2", (long)(om_ok ? bonds : 0));
      var_set_num(vm, "SMX_ALLPATHS", (long)(om_ok ? paths : 0));
      var_set_num(vm, "SMX_ALLPATH", (long)(om_ok ? paths : 0));
      var_set_num(vm, "SMX_ALLPATH_RING", (long)(om_ok ? paths : 0));
      var_set_num(vm, "SMX_ALLPATH_LANE", (long)(om_ok ? paths : 0));
      var_set_num(vm, "SMX_NEXUSES", (long)(om_ok ? hubs : 0));
      var_set_num(vm, "SMX_NEXUS_HUB", (long)(om_ok ? hubs : 0));
      var_set_num(vm, "SMX_NEXUS", (long)(om_ok ? hubs : 0));
      var_set_num(vm, "SMX_SEEDOMNI", (long)(om_ok ? hubs : 0));
      var_set_num(vm, "SMX_SEEDALL", (long)(om_ok ? hubs : 0));
      var_set_num(vm, "SMX_MESH", (long)(om_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + paths + hubs));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (om_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX OMNIVERSE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX OMNIVERSE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX OMNIVERSE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX OMNIVERSE nodes=%d live=%d bonds=%d paths=%d hubs=%d need=%d soft=%d talks=%d oob=%d omniversed=%d vital=%ld\n",
                n, live, bonds, paths, hubs, need, soft, vm->smx_talks, vm->smx_oob, om_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX SEER|SEER_ALIAS|VISION|ORACLEVERSE|MESH_SEER|RAISE_SEER a b c ...
   * Life-force omniverse mesh stability after autoheal: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete omniverse mesh among live nodes, weaves an
   * allpath ring (i -> i+1) so free energy spans every realm, then nexus hub
   * gathers return so lattice locks omniverse where life holds the hive.
   * Latches SMX_SEERED when mesh+allpaths+hubs are soft-OOB-free.
   * SMX_SIGHTS = chain bonds; SMX_ORACLE hub = root gather pulses;
   * SMX_SEER sum = bonds+allpaths+hubs; SMX_SEER_ALIAS|SMX_ORACLE_LATCH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"SEER")||kw(&L->cur,"SEER_ALIAS")||kw(&L->cur,"VISION")||kw(&L->cur,"ORACLEVERSE")||kw(&L->cur,"ORACLE_LATCH")||kw(&L->cur,"WE_SEER")||kw(&L->cur,"LIFE_SEER")||
      kw(&L->cur,"SEERS_ALT")||kw(&L->cur,"VISIONS")||kw(&L->cur,"ORACLES_KW")||
      kw(&L->cur,"SIGHT")||kw(&L->cur,"SIGHTS")||kw(&L->cur,"ORACLE")||
      kw(&L->cur,"ORACLES")||kw(&L->cur,"SEEDSEER")||kw(&L->cur,"SEEDVISION")||
      kw(&L->cur,"SIGHT_RING")||kw(&L->cur,"ORACLE_HUB")||
      kw(&L->cur,"MESH_SEER")||kw(&L->cur,"RAISE_SEER")||
      kw(&L->cur,"SEERS")||kw(&L->cur,"SEERS_ALT")||
      kw(&L->cur,"SEEDSEER")||kw(&L->cur,"SEEDORACLE")||
      kw(&L->cur,"LATTICE_SEER")||kw(&L->cur,"WORLD_SEER")||kw(&L->cur,"WORLD_SEER")||
      kw(&L->cur,"SIGHT_RING")||kw(&L->cur,"ORACLE_HUB")||kw(&L->cur,"PULSE_SEER")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int sights = 0;
    int oracles = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "SEER needs >=2 cubes",
                  "SMX SEER a b [c ...]  or  SMX SEER_ALIAS a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - omniverse needs clear channel */
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
    /* complete omniverse mesh among live */
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
    /* allpath ring - free energy spans every realm every edge i -> i+1 both ways */
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
          else sights++;
        }
      }
    }
    /* nexus hub - seed axis return from every live leaf */
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
          else oracles++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int path_ok = (live >= 2 && sights >= live && soft == 0) ? 1 : 0;
      if (!path_ok && live >= 2 && sights * 2 >= live && soft == 0)
        path_ok = 1;
      int hub_ok = (live >= 1 && oracles >= live && soft == 0) ? 1 : 0;
      if (!hub_ok && live >= 1 && oracles * 2 >= live && soft == 0)
        hub_ok = 1;
      int seer_ok = (mesh_ok && path_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (seer_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (sights > 0 ? 1 : 0) + (oracles > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_SEERED", (long)seer_ok);
      var_set_num(vm, "SMX_SEERED_LATCH", (long)seer_ok);
      var_set_num(vm, "SMX_SEER", (long)(seer_ok ? bonds + sights + oracles : 0));
      var_set_num(vm, "SMX_SEER_ALIAS", (long)(seer_ok ? 1 : 0));
      var_set_num(vm, "SMX_ORACLE_LATCH", (long)(seer_ok ? 1 : 0));
      var_set_num(vm, "SMX_ORACLEVERSE", (long)(seer_ok ? 1 : 0));
      var_set_num(vm, "SMX_VISION", (long)(seer_ok ? 1 : 0));
      var_set_num(vm, "SMX_SEERS_ALT", (long)(seer_ok ? bonds : 0));
      var_set_num(vm, "SMX_SEERS", (long)(seer_ok ? bonds : 0));
      var_set_num(vm, "SMX_SEERS_ALT2", (long)(seer_ok ? bonds : 0));
      var_set_num(vm, "SMX_SIGHTS", (long)(seer_ok ? sights : 0));
      var_set_num(vm, "SMX_SIGHT", (long)(seer_ok ? sights : 0));
      var_set_num(vm, "SMX_SIGHT_RING", (long)(seer_ok ? sights : 0));
      var_set_num(vm, "SMX_SIGHT_LANE", (long)(seer_ok ? sights : 0));
      var_set_num(vm, "SMX_ORACLES", (long)(seer_ok ? oracles : 0));
      var_set_num(vm, "SMX_ORACLE_HUB", (long)(seer_ok ? oracles : 0));
      var_set_num(vm, "SMX_ORACLE", (long)(seer_ok ? oracles : 0));
      var_set_num(vm, "SMX_SEEDSEER", (long)(seer_ok ? oracles : 0));
      var_set_num(vm, "SMX_SEEDVISION", (long)(seer_ok ? oracles : 0));
      var_set_num(vm, "SMX_MESH", (long)(seer_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + sights + oracles));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (seer_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SEER ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SEER partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX SEER soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX SEER nodes=%d live=%d bonds=%d sights=%d oracles=%d need=%d soft=%d talks=%d oob=%d seered=%d vital=%ld\n",
                n, live, bonds, sights, oracles, need, soft, vm->smx_talks, vm->smx_oob, seer_ok, vital);
    }
    bump(vm); return 1;
  }


  /* SMX FORGED|TEMPERED|TEMPER|ANVILVERSE|MESH_FORGED|RAISE_FORGED a b c ...
   * Life-force forge mesh stability after seer: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete forge mesh among live nodes, weaves an
   * temper ring (i -> i+1) so free energy hardens every edge, then anvil hub
   * gathers return so lattice locks forged where life holds the hive.
   * Latches SMX_FORGEDED when mesh+strikes+anvils are soft-OOB-free.
   * SMX_STRIKES = temper ring; SMX_ANVILS hub = root gather pulses;
   * SMX_FORGE_SUM = bonds+strikes+anvils; SMX_FORGED|SMX_TEMPERED|SMX_FORGED_ALIAS sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"FORGED")||kw(&L->cur,"TEMPERED")||kw(&L->cur,"TEMPER")||kw(&L->cur,"ANVILVERSE")||kw(&L->cur,"ANVIL_LATCH")||kw(&L->cur,"WE_FORGED")||kw(&L->cur,"LIFE_ANVIL")||
      kw(&L->cur,"FORGES_ALT")||kw(&L->cur,"TEMPERS")||kw(&L->cur,"FG")||
      kw(&L->cur,"STRIKE")||kw(&L->cur,"STRIKES")||kw(&L->cur,"ANVIL")||
      kw(&L->cur,"ANVILS")||kw(&L->cur,"SEEDFORGE")||kw(&L->cur,"SEEDANVIL")||
      kw(&L->cur,"TEMPER_RING")||kw(&L->cur,"ANVIL_HUB")||
      kw(&L->cur,"MESH_FORGED")||kw(&L->cur,"RAISE_FORGED")||
      kw(&L->cur,"FORGEDS")||kw(&L->cur,"FORGES_ALT")||
      kw(&L->cur,"SEEDFORGE")||kw(&L->cur,"SEEDANVIL2")||
      kw(&L->cur,"LATTICE_FORGED")||kw(&L->cur,"WORLD_FORGED")||kw(&L->cur,"WORLD_FORGED")||
      kw(&L->cur,"TEMPER_RING")||kw(&L->cur,"ANVIL_HUB")||kw(&L->cur,"PULSE_FORGED")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int strikes = 0;
    int anvils = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "FORGED needs >=2 cubes",
                  "SMX FORGED a b [c ...]  or  SMX TEMPER a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - forge needs clear channel */
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
    /* complete forge mesh among live */
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
    /* temper ring - free energy hardens every edge every edge i -> i+1 both ways */
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
          else strikes++;
        }
      }
    }
    /* anvil hub - seed axis return from every live leaf */
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
          else anvils++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int path_ok = (live >= 2 && strikes >= live && soft == 0) ? 1 : 0;
      if (!path_ok && live >= 2 && strikes * 2 >= live && soft == 0)
        path_ok = 1;
      int hub_ok = (live >= 1 && anvils >= live && soft == 0) ? 1 : 0;
      if (!hub_ok && live >= 1 && anvils * 2 >= live && soft == 0)
        hub_ok = 1;
      int fg_ok = (mesh_ok && path_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (fg_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (strikes > 0 ? 1 : 0) + (anvils > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_FORGEDED", (long)fg_ok);
      var_set_num(vm, "SMX_FORGED_LATCH", (long)fg_ok);
      var_set_num(vm, "SMX_FORGED", (long)(fg_ok ? 1 : 0));
      var_set_num(vm, "SMX_FORGE_SUM", (long)(fg_ok ? bonds + strikes + anvils : 0));
      var_set_num(vm, "SMX_FORGED_ALIAS", (long)(fg_ok ? 1 : 0));
      var_set_num(vm, "SMX_TEMPERED", (long)(fg_ok ? 1 : 0));
      var_set_num(vm, "SMX_ANVIL_LATCH", (long)(fg_ok ? 1 : 0));
      var_set_num(vm, "SMX_ANVILVERSE", (long)(fg_ok ? 1 : 0));
      var_set_num(vm, "SMX_TEMPER", (long)(fg_ok ? 1 : 0));
      var_set_num(vm, "SMX_FORGES_ALT", (long)(fg_ok ? bonds : 0));
      var_set_num(vm, "SMX_FORGES", (long)(fg_ok ? bonds : 0));
      var_set_num(vm, "SMX_FORGES_ALT2", (long)(fg_ok ? bonds : 0));
      var_set_num(vm, "SMX_STRIKES", (long)(fg_ok ? strikes : 0));
      var_set_num(vm, "SMX_STRIKE", (long)(fg_ok ? strikes : 0));
      var_set_num(vm, "SMX_TEMPER_RING", (long)(fg_ok ? strikes : 0));
      var_set_num(vm, "SMX_STRIKE_LANE", (long)(fg_ok ? strikes : 0));
      var_set_num(vm, "SMX_ANVILS", (long)(fg_ok ? anvils : 0));
      var_set_num(vm, "SMX_ANVIL_HUB", (long)(fg_ok ? anvils : 0));
      var_set_num(vm, "SMX_ANVIL", (long)(fg_ok ? anvils : 0));
      var_set_num(vm, "SMX_SEEDFORGE", (long)(fg_ok ? anvils : 0));
      var_set_num(vm, "SMX_SEEDANVIL", (long)(fg_ok ? anvils : 0));
      var_set_num(vm, "SMX_MESH", (long)(fg_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + strikes + anvils));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (fg_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX FORGED ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX FORGED partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX FORGED soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX FORGED nodes=%d live=%d bonds=%d strikes=%d anvils=%d need=%d soft=%d talks=%d oob=%d forged=%d vital=%ld\n",
                n, live, bonds, strikes, anvils, need, soft, vm->smx_talks, vm->smx_oob, fg_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX KEEPER|WARDEN|WATCH|SENTINEL|MESH_KEEPER|RAISE_KEEPER a b c ...
   * Life-force keeper mesh stability after forged: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete keeper mesh among live nodes, weaves an
   * watch ring (i -> i+1) so free energy guards every edge, then bastion hub
   * gathers return so lattice locks keepered where life holds the hive.
   * Latches SMX_KEEPERED when mesh+watches+bastions are soft-OOB-free.
   * SMX_WATCHES = watch ring; SMX_BASTIONS hub = root gather pulses;
   * SMX_KEEP_SUM = bonds+watches+bastions; SMX_KEEPER|SMX_WARDEN|SMX_KEEPER_ALIAS sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"KEEPER")||kw(&L->cur,"WARDEN")||kw(&L->cur,"WATCH")||kw(&L->cur,"SENTINEL")||kw(&L->cur,"BASTION_LATCH")||kw(&L->cur,"WE_KEEPER")||kw(&L->cur,"LIFE_KEEPER")||
      kw(&L->cur,"KEEPS_ALT")||kw(&L->cur,"WARDENS")||kw(&L->cur,"KP")||
      kw(&L->cur,"WATCH")||kw(&L->cur,"WATCHES")||kw(&L->cur,"BASTION")||
      kw(&L->cur,"BASTIONS")||kw(&L->cur,"SEEDKEEPER")||kw(&L->cur,"SEEDBASTION")||
      kw(&L->cur,"WATCH_RING")||kw(&L->cur,"BASTION_HUB")||
      kw(&L->cur,"MESH_KEEPER")||kw(&L->cur,"RAISE_KEEPER")||
      kw(&L->cur,"KEEPERS")||kw(&L->cur,"KEEPS_ALT")||
      kw(&L->cur,"SEEDKEEPER")||kw(&L->cur,"SEEDBASTION2")||
      kw(&L->cur,"LATTICE_KEEPER")||kw(&L->cur,"WORLD_KEEPER")||kw(&L->cur,"WORLD_KEEPER")||
      kw(&L->cur,"WATCH_RING")||kw(&L->cur,"BASTION_HUB")||kw(&L->cur,"PULSE_KEEPER")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int watches = 0;
    int bastions = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "KEEPER needs >=2 cubes",
                  "SMX KEEPER a b [c ...]  or  SMX WATCH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - forge needs clear channel */
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
    /* watch ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else watches++;
        }
      }
    }
    /* bastion hub - seed axis return from every live leaf */
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
          else bastions++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int path_ok = (live >= 2 && watches >= live && soft == 0) ? 1 : 0;
      if (!path_ok && live >= 2 && watches * 2 >= live && soft == 0)
        path_ok = 1;
      int hub_ok = (live >= 1 && bastions >= live && soft == 0) ? 1 : 0;
      if (!hub_ok && live >= 1 && bastions * 2 >= live && soft == 0)
        hub_ok = 1;
      int kp_ok = (mesh_ok && path_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (kp_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (watches > 0 ? 1 : 0) + (bastions > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_KEEPERED", (long)kp_ok);
      var_set_num(vm, "SMX_KEEPER_LATCH", (long)kp_ok);
      var_set_num(vm, "SMX_KEEPER", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_KEEP_SUM", (long)(kp_ok ? bonds + watches + bastions : 0));
      var_set_num(vm, "SMX_KEEPER_ALIAS", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_WARDEN", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_BASTION_LATCH", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_SENTINEL", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_BASTION", (long)(kp_ok ? 1 : 0));
      /* usability latches — mesh keeper life cascade lock */
      var_set_num(vm, "SMX_MESH_KEEPER", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_KEEPER", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_KEEPER", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_LIFE_KEEPER", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_WE_KEEPER", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_KEEPER_OK", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_WATCH_OK", (long)(kp_ok && watches > 0 ? 1 : 0));
      var_set_num(vm, "SMX_BASTION_OK", (long)(kp_ok && bastions > 0 ? 1 : 0));
      var_set_num(vm, "SMX_KEEPS_ALT", (long)(kp_ok ? bonds : 0));
      var_set_num(vm, "SMX_KEEPS", (long)(kp_ok ? bonds : 0));
      var_set_num(vm, "SMX_KEEPS_ALT2", (long)(kp_ok ? bonds : 0));
      var_set_num(vm, "SMX_WATCHES", (long)(kp_ok ? watches : 0));
      var_set_num(vm, "SMX_WATCH", (long)(kp_ok ? watches : 0));
      var_set_num(vm, "SMX_WATCH_RING", (long)(kp_ok ? watches : 0));
      var_set_num(vm, "SMX_WATCH_LANE", (long)(kp_ok ? watches : 0));
      var_set_num(vm, "SMX_BASTIONS", (long)(kp_ok ? bastions : 0));
      var_set_num(vm, "SMX_BASTION_HUB", (long)(kp_ok ? bastions : 0));
      var_set_num(vm, "SMX_BASTION_N", (long)(kp_ok ? bastions : 0));
      var_set_num(vm, "SMX_SEEDKEEPER", (long)(kp_ok ? bastions : 0));
      var_set_num(vm, "SMX_SEEDBASTION", (long)(kp_ok ? bastions : 0));
      var_set_num(vm, "SMX_MESH", (long)(kp_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + watches + bastions));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (kp_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX KEEPER ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX KEEPER partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX KEEPER soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX KEEPER nodes=%d live=%d bonds=%d watches=%d bastions=%d need=%d soft=%d talks=%d oob=%d keepered=%d vital=%ld\n",
                n, live, bonds, watches, bastions, need, soft, vm->smx_talks, vm->smx_oob, kp_ok, vital);
    }
    bump(vm); return 1;
  }
  return 0;
}
