/* CubalC SMX cascade/soft plane — peeled from lang_ops_smx.
 * HOLD_FLASH discipline: mesh sticky only; meaningful forms preferred.
 */
#include "lang/cubalc_lang_ops_smx_pneuma_a.h"
#include "lang/cubalc_lang_ops_smx_pneuma_sphere0.h"
#include "lang/cubalc_lang_ops_smx_pneuma_sphere1.h"
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
  if (kw(&L->cur,"RESILIENCE")||kw(&L->cur,"RS")||kw(&L->cur,"RESIL")||kw(&L->cur,"GUARD")||kw(&L->cur,"GUARDS")||kw(&L->cur,"RESILIENT")||kw(&L->cur,"WE_RESILIENCE")||
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





/* SMX SCHWANN|SCHWANNCELL|MESH_SCHWANN|SC_WRAP|SC_SHEATH|PNS_MYELIN|RAISE_SCHWANN a b c ...
   * Life-force Schwann cell mesh stability after oligo: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete schwann myelin mesh among live nodes, weaves a
   * schwann wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks schwann where life myelinate the PNS pulse.
   * Latches SMX_SCHWANN when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = schwann ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_SCHWANN_SUM = bonds+wraps+sheaths; SMX_SCHWANN|SMX_MESH_SCHWANN|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  return 0;
}
