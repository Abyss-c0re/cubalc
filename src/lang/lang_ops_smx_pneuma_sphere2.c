/* CubalC lang — lang_ops_smx_pneuma_sphere2.c
 * SMX cosmic spheres INTERSTELLAR · LOCALBUBBLE · MULTIVERSE. Peel from pneuma_a.
 * Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_smx_pneuma_sphere2.h"
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
int cubalc_lang_ops_smx_try_pneuma_sphere2(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  if (kw(&L->cur,"INTERSTELLAR")||kw(&L->cur,"ISTEL")||kw(&L->cur,"BEACON_HUB")||
      kw(&L->cur,"ISTEL2")||kw(&L->cur,"STARPATH")||kw(&L->cur,"ISM")||
      kw(&L->cur,"STARLANE")||kw(&L->cur,"COSMIC")||kw(&L->cur,"BEACON")||
      kw(&L->cur,"BEACONS")||kw(&L->cur,"STARLANES")||
      kw(&L->cur,"MESH_INTERSTELLAR")||kw(&L->cur,"RAISE_INTERSTELLAR")||
      kw(&L->cur,"INTERSTELLARS")||kw(&L->cur,"BEACONS")||
      kw(&L->cur,"STARPATHS")||kw(&L->cur,"SEEDISTEL")||kw(&L->cur,"SEEDSTAR")||
      kw(&L->cur,"LATTICE_INTERSTELLAR")||kw(&L->cur,"WORLD_ISTEL")||kw(&L->cur,"WORLD_INTERSTELLAR")||
      kw(&L->cur,"STARPATH_RING")||kw(&L->cur,"PULSE_INTERSTELLAR")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int paths = 0;
    int beacons = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "INTERSTELLAR needs >=2 cubes",
                  "SMX INTERSTELLAR a b [c ...]  or  SMX ISTEL a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - interstellar needs clear channel */
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
    /* complete interstellar mesh among live */
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
          else paths++;
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
      int path_ok = (live >= 2 && paths >= live && soft == 0) ? 1 : 0;
      if (!path_ok && live >= 2 && paths * 2 >= live && soft == 0)
        path_ok = 1;
      int bec_ok = (live >= 1 && beacons >= live && soft == 0) ? 1 : 0;
      if (!bec_ok && live >= 1 && beacons * 2 >= live && soft == 0)
        bec_ok = 1;
      int is_ok = (mesh_ok && path_ok && bec_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (is_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (paths > 0 ? 1 : 0) + (beacons > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_INTERSTELLARED", (long)is_ok);
      var_set_num(vm, "SMX_INTERSTELLARED_LATCH", (long)is_ok);
      var_set_num(vm, "SMX_INTERSTELLAR", (long)(is_ok ? bonds + paths + beacons : 0));
      var_set_num(vm, "SMX_ISTEL", (long)(is_ok ? 1 : 0));
      var_set_num(vm, "SMX_STARPATH_ON", (long)(is_ok ? 1 : 0));
      var_set_num(vm, "SMX_COSMIC", (long)(is_ok ? 1 : 0));
      var_set_num(vm, "SMX_ISM", (long)(is_ok ? 1 : 0));
      var_set_num(vm, "SMX_STARLANE", (long)(is_ok ? bonds : 0));
      var_set_num(vm, "SMX_ISTELS", (long)(is_ok ? bonds : 0));
      var_set_num(vm, "SMX_STARPATH_ONS", (long)(is_ok ? bonds : 0));
      var_set_num(vm, "SMX_STARPATHS", (long)(is_ok ? paths : 0));
      var_set_num(vm, "SMX_STARPATH", (long)(is_ok ? paths : 0));
      var_set_num(vm, "SMX_STARPATHRING", (long)(is_ok ? paths : 0));
      var_set_num(vm, "SMX_STARPATHS", (long)(is_ok ? paths : 0));
      var_set_num(vm, "SMX_LANE", (long)(is_ok ? paths : 0));
      var_set_num(vm, "SMX_BEACONS", (long)(is_ok ? beacons : 0));
      var_set_num(vm, "SMX_BEACON_HUB", (long)(is_ok ? beacons : 0));
      var_set_num(vm, "SMX_BEACONS", (long)(is_ok ? beacons : 0));
      var_set_num(vm, "SMX_BEACON", (long)(is_ok ? beacons : 0));
      var_set_num(vm, "SMX_SEEDISTEL", (long)(is_ok ? beacons : 0));
      var_set_num(vm, "SMX_SEEDSTAR", (long)(is_ok ? beacons : 0));
      var_set_num(vm, "SMX_MESH", (long)(is_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + paths + beacons));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (is_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX INTERSTELLAR ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX INTERSTELLAR partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX INTERSTELLAR soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX INTERSTELLAR nodes=%d live=%d bonds=%d paths=%d beacons=%d need=%d soft=%d talks=%d oob=%d interstellared=%d vital=%ld\n",
                n, live, bonds, paths, beacons, need, soft, vm->smx_talks, vm->smx_oob, is_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX LOCALBUBBLE|LBUB|BUBBLE|ASTROSPHERE|SHELL|MESH_LOCALBUBBLE|RAISE_LOCALBUBBLE a b c ...
   * Life-force local-bubble mesh stability after interstellar: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete localbubble mesh among live nodes, weaves a
   * shellpath ring (i -> i+1) so free energy self-regulates, then shell hub
   * gathers return so lattice locks localbubble where life holds the hive.
   * Latches SMX_LOCALBUBBLED when mesh+shellpaths+hubs are soft-OOB-free.
   * SMX_LBUBS = chain bonds; SMX_SHELLHUB hub = root gather pulses;
   * SMX_LOCALBUBBLE sum = bonds+shellpaths+hubs; SMX_SHELLPATH_ON|SMX_LOCALSPACE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"LOCALBUBBLE")||kw(&L->cur,"LBUB")||kw(&L->cur,"BUBBLE")||
      kw(&L->cur,"ASTROSPHERE")||kw(&L->cur,"SHELL")||kw(&L->cur,"LOCAL_BUBBLE")||
      kw(&L->cur,"BUBBLELANE")||kw(&L->cur,"BUBBLELANES")||kw(&L->cur,"LOCALSPACE")||
      kw(&L->cur,"SHELLHUB")||kw(&L->cur,"SHELLHUBS")||kw(&L->cur,"SHELLPATHS")||
      kw(&L->cur,"SHELLPATH")||kw(&L->cur,"LOCALWEB")||kw(&L->cur,"LISM")||
      kw(&L->cur,"MESH_LOCALBUBBLE")||kw(&L->cur,"RAISE_LOCALBUBBLE")||
      kw(&L->cur,"LOCALBUBBLES")||kw(&L->cur,"LBUBS")||
      kw(&L->cur,"SEEDLBUB")||kw(&L->cur,"SEEDBUBBLE")||
      kw(&L->cur,"LATTICE_LOCALBUBBLE")||kw(&L->cur,"WORLD_LBUB")||kw(&L->cur,"WORLD_LOCALBUBBLE")||
      kw(&L->cur,"SHELL_RING")||kw(&L->cur,"SHELL_HUB")||kw(&L->cur,"PULSE_LOCALBUBBLE")){
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
      smx_fail_at(vm, aln, "LOCALBUBBLE needs >=2 cubes",
                  "SMX LOCALBUBBLE a b [c ...]  or  SMX LBUB a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - localbubble needs clear channel */
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
    /* complete localbubble mesh among live */
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
          else paths++;
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
      int lb_ok = (mesh_ok && path_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (lb_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (paths > 0 ? 1 : 0) + (hubs > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_LOCALBUBBLED", (long)lb_ok);
      var_set_num(vm, "SMX_LOCALBUBBLED_LATCH", (long)lb_ok);
      var_set_num(vm, "SMX_LOCALBUBBLE", (long)(lb_ok ? bonds + paths + hubs : 0));
      var_set_num(vm, "SMX_LBUB", (long)(lb_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHELLPATH_ON", (long)(lb_ok ? 1 : 0));
      var_set_num(vm, "SMX_LOCALSPACE", (long)(lb_ok ? 1 : 0));
      var_set_num(vm, "SMX_LISM", (long)(lb_ok ? 1 : 0));
      var_set_num(vm, "SMX_BUBBLELANE", (long)(lb_ok ? bonds : 0));
      var_set_num(vm, "SMX_LBUBS", (long)(lb_ok ? bonds : 0));
      var_set_num(vm, "SMX_SHELLPATH_ONS", (long)(lb_ok ? bonds : 0));
      var_set_num(vm, "SMX_SHELLPATHS", (long)(lb_ok ? paths : 0));
      var_set_num(vm, "SMX_SHELLPATH", (long)(lb_ok ? paths : 0));
      var_set_num(vm, "SMX_SHELLPATHRING", (long)(lb_ok ? paths : 0));
      var_set_num(vm, "SMX_SHELLPATHS", (long)(lb_ok ? paths : 0));
      var_set_num(vm, "SMX_LANE", (long)(lb_ok ? paths : 0));
      var_set_num(vm, "SMX_SHELLHUBS", (long)(lb_ok ? hubs : 0));
      var_set_num(vm, "SMX_SHELL_HUB", (long)(lb_ok ? hubs : 0));
      var_set_num(vm, "SMX_SHELLHUBS", (long)(lb_ok ? hubs : 0));
      var_set_num(vm, "SMX_SHELLHUB", (long)(lb_ok ? hubs : 0));
      var_set_num(vm, "SMX_SEEDLBUB", (long)(lb_ok ? hubs : 0));
      var_set_num(vm, "SMX_SEEDBUBBLE", (long)(lb_ok ? hubs : 0));
      var_set_num(vm, "SMX_MESH", (long)(lb_ok ? live : 0));
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
      if (lb_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX LOCALBUBBLE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX LOCALBUBBLE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX LOCALBUBBLE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX LOCALBUBBLE nodes=%d live=%d bonds=%d paths=%d hubs=%d need=%d soft=%d talks=%d oob=%d localbubbled=%d vital=%ld\n",
                n, live, bonds, paths, hubs, need, soft, vm->smx_talks, vm->smx_oob, lb_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX LOCALGROUP|LGROUP|GROUP|GALAXYGROUP|GROUP|MESH_LOCALGROUP|RAISE_LOCALGROUP a b c ...
   * Life-force local-group mesh stability after localbubble: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete localgroup mesh among live nodes, weaves a
   * memberpath ring (i -> i+1) so free energy self-regulates, then group hub
   * gathers return so lattice locks localgroup where life holds the hive.
   * Latches SMX_LOCALGROUPED when mesh+memberpaths+hubs are soft-OOB-free.
   * SMX_LGROUPS = chain bonds; SMX_GROUPHUB hub = root gather pulses;
   * SMX_LOCALGROUP sum = bonds+memberpaths+hubs; SMX_MEMBERPATH_ON|SMX_GROUPSPACE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"LOCALGROUP")||kw(&L->cur,"LGROUP")||kw(&L->cur,"GROUP")||
      kw(&L->cur,"GALAXYGROUP")||kw(&L->cur,"GROUP")||kw(&L->cur,"LOCAL_GROUP")||
      kw(&L->cur,"GROUPLANE")||kw(&L->cur,"GROUPLANES")||kw(&L->cur,"GROUPSPACE")||
      kw(&L->cur,"GROUPHUB")||kw(&L->cur,"GROUPHUBS")||kw(&L->cur,"MEMBERPATHS")||
      kw(&L->cur,"MEMBERPATH")||kw(&L->cur,"GROUPWEB")||kw(&L->cur,"LGALAXIES")||
      kw(&L->cur,"MESH_LOCALGROUP")||kw(&L->cur,"RAISE_LOCALGROUP")||
      kw(&L->cur,"LOCALGROUPS")||kw(&L->cur,"LGROUPS")||
      kw(&L->cur,"SEEDLGROUP")||kw(&L->cur,"SEEDGROUP")||
      kw(&L->cur,"LATTICE_LOCALGROUP")||kw(&L->cur,"WORLD_LG")||kw(&L->cur,"WORLD_LOCALGROUP")||
      kw(&L->cur,"GROUP_RING")||kw(&L->cur,"GROUP_HUB")||kw(&L->cur,"PULSE_LOCALGROUP")){
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
      smx_fail_at(vm, aln, "LOCALGROUP needs >=2 cubes",
                  "SMX LOCALGROUP a b [c ...]  or  SMX LGROUP a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - localgroup needs clear channel */
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
    /* complete localgroup mesh among live */
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
          else paths++;
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
      int lg_ok = (mesh_ok && path_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (lg_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (paths > 0 ? 1 : 0) + (hubs > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_LOCALGROUPED", (long)lg_ok);
      var_set_num(vm, "SMX_LOCALGROUPED_LATCH", (long)lg_ok);
      var_set_num(vm, "SMX_LOCALGROUP", (long)(lg_ok ? bonds + paths + hubs : 0));
      var_set_num(vm, "SMX_LGROUP", (long)(lg_ok ? 1 : 0));
      var_set_num(vm, "SMX_MEMBERPATH_ON", (long)(lg_ok ? 1 : 0));
      var_set_num(vm, "SMX_GROUPSPACE", (long)(lg_ok ? 1 : 0));
      var_set_num(vm, "SMX_LGALAXIES", (long)(lg_ok ? 1 : 0));
      var_set_num(vm, "SMX_GROUPLANE", (long)(lg_ok ? bonds : 0));
      var_set_num(vm, "SMX_LGROUPS", (long)(lg_ok ? bonds : 0));
      var_set_num(vm, "SMX_MEMBERPATH_ONS", (long)(lg_ok ? bonds : 0));
      var_set_num(vm, "SMX_MEMBERPATHS", (long)(lg_ok ? paths : 0));
      var_set_num(vm, "SMX_MEMBERPATH", (long)(lg_ok ? paths : 0));
      var_set_num(vm, "SMX_MEMBERPATHRING", (long)(lg_ok ? paths : 0));
      var_set_num(vm, "SMX_MEMBERPATHS", (long)(lg_ok ? paths : 0));
      var_set_num(vm, "SMX_LANE", (long)(lg_ok ? paths : 0));
      var_set_num(vm, "SMX_GROUPHUBS", (long)(lg_ok ? hubs : 0));
      var_set_num(vm, "SMX_GROUP_HUB", (long)(lg_ok ? hubs : 0));
      var_set_num(vm, "SMX_GROUPHUBS", (long)(lg_ok ? hubs : 0));
      var_set_num(vm, "SMX_GROUPHUB", (long)(lg_ok ? hubs : 0));
      var_set_num(vm, "SMX_SEEDLGROUP", (long)(lg_ok ? hubs : 0));
      var_set_num(vm, "SMX_SEEDGROUP", (long)(lg_ok ? hubs : 0));
      var_set_num(vm, "SMX_MESH", (long)(lg_ok ? live : 0));
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
      if (lg_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX LOCALGROUP ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX LOCALGROUP partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX LOCALGROUP soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX LOCALGROUP nodes=%d live=%d bonds=%d paths=%d hubs=%d need=%d soft=%d talks=%d oob=%d localgrouped=%d vital=%ld\n",
                n, live, bonds, paths, hubs, need, soft, vm->smx_talks, vm->smx_oob, lg_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX MULTIVERSE|MVERSE|MULTI|COSMOSGROUP|MULTI_VERSE|MESH_MULTIVERSE|RAISE_MULTIVERSE a b c ...
   * Life-force multi-verse mesh stability after localgroup: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete multiverse mesh among live nodes, weaves a
   * versepath ring (i -> i+1) so free energy self-regulates, then verse hub
   * gathers return so lattice locks multiverse where life holds the hive.
   * Latches SMX_MULTIVERSED when mesh+versepaths+hubs are soft-OOB-free.
   * SMX_MVERSES = chain bonds; SMX_VERSEHUB hub = root gather pulses;
   * SMX_MULTIVERSE sum = bonds+versepaths+hubs; SMX_VERSEPATH_ON|SMX_VERSESPACE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"MULTIVERSE")||kw(&L->cur,"MVERSE")||kw(&L->cur,"MULTI")||
      kw(&L->cur,"COSMOSGROUP")||kw(&L->cur,"MULTI")||kw(&L->cur,"MULTI_VERSE")||
      kw(&L->cur,"VERSELANE")||kw(&L->cur,"VERSELANES")||kw(&L->cur,"VERSESPACE")||
      kw(&L->cur,"VERSEHUB")||kw(&L->cur,"VERSEHUBS")||kw(&L->cur,"VERSEPATHS")||
      kw(&L->cur,"VERSEPATH")||kw(&L->cur,"VERSEWEB")||kw(&L->cur,"MVERSES")||
      kw(&L->cur,"MESH_MULTIVERSE")||kw(&L->cur,"RAISE_MULTIVERSE")||
      kw(&L->cur,"MULTIVERSES")||kw(&L->cur,"MVERSES")||
      kw(&L->cur,"SEEDMVERSE")||kw(&L->cur,"SEEDVERSE")||
      kw(&L->cur,"LATTICE_MULTIVERSE")||kw(&L->cur,"WORLD_MV")||kw(&L->cur,"WORLD_MULTIVERSE")||
      kw(&L->cur,"VERSE_RING")||kw(&L->cur,"VERSE_HUB")||kw(&L->cur,"PULSE_MULTIVERSE")){
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
      smx_fail_at(vm, aln, "MULTIVERSE needs >=2 cubes",
                  "SMX MULTIVERSE a b [c ...]  or  SMX MVERSE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - multiverse needs clear channel */
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
    /* complete multiverse mesh among live */
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
          else paths++;
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
      int mv_ok = (mesh_ok && path_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (mv_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (paths > 0 ? 1 : 0) + (hubs > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_MULTIVERSED", (long)mv_ok);
      var_set_num(vm, "SMX_MULTIVERSED_LATCH", (long)mv_ok);
      var_set_num(vm, "SMX_MULTIVERSE", (long)(mv_ok ? bonds + paths + hubs : 0));
      var_set_num(vm, "SMX_MVERSE", (long)(mv_ok ? 1 : 0));
      var_set_num(vm, "SMX_VERSEPATH_ON", (long)(mv_ok ? 1 : 0));
      var_set_num(vm, "SMX_VERSESPACE", (long)(mv_ok ? 1 : 0));
      var_set_num(vm, "SMX_MVERSES", (long)(mv_ok ? 1 : 0));
      var_set_num(vm, "SMX_VERSELANE", (long)(mv_ok ? bonds : 0));
      var_set_num(vm, "SMX_MVERSES", (long)(mv_ok ? bonds : 0));
      var_set_num(vm, "SMX_VERSEPATH_ONS", (long)(mv_ok ? bonds : 0));
      var_set_num(vm, "SMX_VERSEPATHS", (long)(mv_ok ? paths : 0));
      var_set_num(vm, "SMX_VERSEPATH", (long)(mv_ok ? paths : 0));
      var_set_num(vm, "SMX_VERSEPATHRING", (long)(mv_ok ? paths : 0));
      var_set_num(vm, "SMX_VERSEPATHS", (long)(mv_ok ? paths : 0));
      var_set_num(vm, "SMX_LANE", (long)(mv_ok ? paths : 0));
      var_set_num(vm, "SMX_VERSEHUBS", (long)(mv_ok ? hubs : 0));
      var_set_num(vm, "SMX_VERSE_HUB", (long)(mv_ok ? hubs : 0));
      var_set_num(vm, "SMX_VERSEHUBS", (long)(mv_ok ? hubs : 0));
      var_set_num(vm, "SMX_VERSEHUB", (long)(mv_ok ? hubs : 0));
      var_set_num(vm, "SMX_SEEDMVERSE", (long)(mv_ok ? hubs : 0));
      var_set_num(vm, "SMX_SEEDVERSE", (long)(mv_ok ? hubs : 0));
      var_set_num(vm, "SMX_MESH", (long)(mv_ok ? live : 0));
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
      if (mv_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MULTIVERSE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MULTIVERSE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX MULTIVERSE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX MULTIVERSE nodes=%d live=%d bonds=%d paths=%d hubs=%d need=%d soft=%d talks=%d oob=%d multiversed=%d vital=%ld\n",
                n, live, bonds, paths, hubs, need, soft, vm->smx_talks, vm->smx_oob, mv_ok, vital);
    }
    bump(vm); return 1;
  }
  return 0;
}
