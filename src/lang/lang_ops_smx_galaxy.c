/* CubalC SMX cascade/soft plane — peeled from lang_ops_smx.
 * HOLD_FLASH discipline: mesh sticky only; meaningful forms preferred.
 */
#include "lang/cubalc_lang_ops_smx_galaxy.h"
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

int cubalc_lang_ops_smx_try_galaxy(VM *vm, Lex *L) {
  if (kw(&L->cur,"GALAXY")||kw(&L->cur,"SPIRAL")||kw(&L->cur,"ARM")||
      kw(&L->cur,"CORE")||kw(&L->cur,"HALO")||kw(&L->cur,"BULGE")||
      kw(&L->cur,"MESH_GALAXY")||kw(&L->cur,"RAISE_GALAXY")||
      kw(&L->cur,"GALAXIES")||kw(&L->cur,"SPIRALS")||kw(&L->cur,"ARMS")||
      kw(&L->cur,"CORES")||kw(&L->cur,"HALOS")||kw(&L->cur,"SEEDGALAXY")||
      kw(&L->cur,"LATTICE_GALAXY")||kw(&L->cur,"STAR_DISK")||
      kw(&L->cur,"DISK_ARM")||kw(&L->cur,"PULSE_GALAXY")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int stars = 0;
    int arms = 0;
    int cores = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "GALAXY needs >=2 cubes",
                  "SMX GALAXY a b [c ...]  or  SMX SPIRAL a b c d");
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
          int a = live_ix[i];
          int b = live_ix[j];
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
          else arms++;
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
          else cores++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && stars >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && stars * 2 >= need && soft == 0)
        mesh_ok = 1;
      int arm_ok = (live >= 2 && arms >= live && soft == 0) ? 1 : 0;
      if (!arm_ok && live >= 2 && arms * 2 >= live && soft == 0)
        arm_ok = 1;
      int core_ok = (live >= 1 && cores >= live && soft == 0) ? 1 : 0;
      if (!core_ok && live >= 1 && cores * 2 >= live && soft == 0)
        core_ok = 1;
      int gx_ok = (mesh_ok && arm_ok && core_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (gx_ok ? 12 : (stars > 0 ? 3 : 0)) +
                   (arms > 0 ? 1 : 0) + (cores > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_GALAXIED", (long)gx_ok);
      var_set_num(vm, "SMX_GALAXY", (long)(gx_ok ? stars + arms + cores : 0));
      var_set_num(vm, "SMX_SPIRAL", (long)(gx_ok ? 1 : 0));
      var_set_num(vm, "SMX_BULGE", (long)(gx_ok ? 1 : 0));
      var_set_num(vm, "SMX_STARS", (long)(gx_ok ? stars : 0));
      var_set_num(vm, "SMX_ARMS", (long)(gx_ok ? arms : 0));
      var_set_num(vm, "SMX_ARM", (long)(gx_ok ? arms : 0));
      var_set_num(vm, "SMX_HALO", (long)(gx_ok ? arms : 0));
      var_set_num(vm, "SMX_CORES", (long)(gx_ok ? cores : 0));
      var_set_num(vm, "SMX_CORE", (long)(gx_ok ? cores : 0));
      var_set_num(vm, "SMX_SEEDGALAXY", (long)(gx_ok ? cores : 0));
      var_set_num(vm, "SMX_MESH", (long)(gx_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)stars);
      var_set_num(vm, "SMX_EXCHANGES", (long)stars);
      var_set_num(vm, "SMX_FUSE", (long)stars);
      var_set_num(vm, "SMX_BIND", (long)stars);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(stars + arms + cores));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (gx_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX GALAXY ok");
      } else if (stars > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX GALAXY partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX GALAXY soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX GALAXY nodes=%d live=%d stars=%d arms=%d cores=%d need=%d soft=%d talks=%d oob=%d galaxied=%d vital=%ld\n",
                n, live, stars, arms, cores, need, soft, vm->smx_talks, vm->smx_oob, gx_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX CONSTELLATION|ASTERISM|STARFIELD|GUIDESTAR|NAVSTAR|LODGE|MESH_CONSTELLATION|RAISE_CONSTELLATION a b c ...
   * Life-force sky map after galaxy weave: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete link mesh among live nodes, weaves a
   * guide-arc ring (i -> i+1) so free energy traces the sky path, then anchor hub
   * gathers return so lattice raises a constellation lock where guides hold the hive.
   * Latches SMX_CONSTELLATED when mesh+arcs+anchors are soft-OOB-free.
   * SMX_LINKS = chain bonds; SMX_ANCHOR hub = root gather pulses;
   * SMX_CONSTELLATION sum = links+arcs+anchors; SMX_ASTERISM|SMX_STARFIELD sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"CONSTELLATION")||kw(&L->cur,"ASTERISM")||kw(&L->cur,"STARFIELD")||
      kw(&L->cur,"GUIDESTAR")||kw(&L->cur,"NAVSTAR")||kw(&L->cur,"LODGE")||
      kw(&L->cur,"MESH_CONSTELLATION")||kw(&L->cur,"RAISE_CONSTELLATION")||
      kw(&L->cur,"CONSTELLATIONS")||kw(&L->cur,"ASTERISMS")||kw(&L->cur,"STARFIELDS")||
      kw(&L->cur,"GUIDESTARS")||kw(&L->cur,"NAVSTARS")||kw(&L->cur,"SEEDCONSTELLATION")||
      kw(&L->cur,"LATTICE_CONSTELLATION")||kw(&L->cur,"SKY_MAP")||
      kw(&L->cur,"GUIDE_MESH")||kw(&L->cur,"PULSE_CONSTELLATION")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int links = 0;
    int arcs = 0;
    int anchors = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "CONSTELLATION needs >=2 cubes",
                  "SMX CONSTELLATION a b [c ...]  or  SMX ASTERISM a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - constellation needs clear channel */
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
    /* complete link mesh among live */
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
            else links++;
          }
        }
      }
    }
    /* guide-arc ring - free energy holds every edge i -> i+1 both ways */
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
          else arcs++;
        }
      }
    }
    /* anchor hub - seed polestar return from every live leaf */
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
      int mesh_ok = (need > 0 && links >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && links * 2 >= need && soft == 0)
        mesh_ok = 1;
      int arc_ok = (live >= 2 && arcs >= live && soft == 0) ? 1 : 0;
      if (!arc_ok && live >= 2 && arcs * 2 >= live && soft == 0)
        arc_ok = 1;
      int anchor_ok = (live >= 1 && anchors >= live && soft == 0) ? 1 : 0;
      if (!anchor_ok && live >= 1 && anchors * 2 >= live && soft == 0)
        anchor_ok = 1;
      int cst_ok = (mesh_ok && arc_ok && anchor_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (cst_ok ? 12 : (links > 0 ? 3 : 0)) +
                   (arcs > 0 ? 1 : 0) + (anchors > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_CONSTELLATED", (long)cst_ok);
      var_set_num(vm, "SMX_CONSTELLATION", (long)(cst_ok ? links + arcs + anchors : 0));
      var_set_num(vm, "SMX_ASTERISM", (long)(cst_ok ? 1 : 0));
      var_set_num(vm, "SMX_STARFIELD", (long)(cst_ok ? 1 : 0));
      var_set_num(vm, "SMX_LINKS", (long)(cst_ok ? links : 0));
      var_set_num(vm, "SMX_LINK", (long)(cst_ok ? links : 0));
      var_set_num(vm, "SMX_ARCS", (long)(cst_ok ? arcs : 0));
      var_set_num(vm, "SMX_ARC", (long)(cst_ok ? arcs : 0));
      var_set_num(vm, "SMX_GUIDESTAR", (long)(cst_ok ? 1 : 0));
      var_set_num(vm, "SMX_NAVSTAR", (long)(cst_ok ? 1 : 0));
      var_set_num(vm, "SMX_ANCHORS", (long)(cst_ok ? anchors : 0));
      var_set_num(vm, "SMX_ANCHOR", (long)(cst_ok ? anchors : 0));
      var_set_num(vm, "SMX_LODGE", (long)(cst_ok ? anchors : 0));
      var_set_num(vm, "SMX_SEEDCONSTELLATION", (long)(cst_ok ? anchors : 0));
      var_set_num(vm, "SMX_MESH", (long)(cst_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)links);
      var_set_num(vm, "SMX_EXCHANGES", (long)links);
      var_set_num(vm, "SMX_FUSE", (long)links);
      var_set_num(vm, "SMX_BIND", (long)links);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(links + arcs + anchors));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (cst_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CONSTELLATION ok");
      } else if (links > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CONSTELLATION partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX CONSTELLATION soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX CONSTELLATION nodes=%d live=%d links=%d arcs=%d anchors=%d need=%d soft=%d talks=%d oob=%d constellated=%d vital=%ld\n",
                n, live, links, arcs, anchors, need, soft, vm->smx_talks, vm->smx_oob, cst_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX ZODIAC|ECLIPTIC|PATH|HOUSE|SIGN|POLE|MESH_ZODIAC|RAISE_ZODIAC a b c ...
   * Life-force year-path after constellation: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete sign mesh among live nodes, weaves a
   * ecliptic-house ring (i -> i+1) so free energy traces the year-path, then pole hub
   * gathers return so lattice raises a zodiac lock where houses hold the hive.
   * Latches SMX_ZODIACED when mesh+houses+poles are soft-OOB-free.
   * SMX_SIGNS = chain bonds; SMX_POLE hub = root gather pulses;
   * SMX_ZODIAC sum = signs+houses+poles; SMX_ECLIPTIC|SMX_PATH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  return 0;
}
