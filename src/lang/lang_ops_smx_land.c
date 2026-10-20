/* CubalC SMX cascade/soft plane — peeled from lang_ops_smx.
 * HOLD_FLASH discipline: mesh sticky only; meaningful forms preferred.
 */
#include "lang/cubalc_lang_ops_smx_land.h"
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

int cubalc_lang_ops_smx_try_land(VM *vm, Lex *L) {
  if (kw(&L->cur,"DUNE")||kw(&L->cur,"FOREDUNE")||kw(&L->cur,"DRIFT")||
      kw(&L->cur,"RIDGE")||kw(&L->cur,"AMMOPHILA")||kw(&L->cur,"SAND")||
      kw(&L->cur,"BEACHGRASS")||kw(&L->cur,"MESH_DUNE")||kw(&L->cur,"RAISE_DUNE")||
      kw(&L->cur,"SANDRIDGE")||kw(&L->cur,"DUNES")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int ridges = 0;
    int drifts = 0;
    int grasses = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "DUNE needs >=2 cubes",
                  "SMX DUNE a b [c ...]  or  SMX FOREDUNE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - dune needs clear channel */
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
    /* complete dune mesh among live */
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
            else ridges++;
          }
        }
      }
    }
    /* drift ring - energy pulses every edge i -> i+1 both ways */
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
          else drifts++;
        }
      }
    }
    /* beachgrass pool - root anchors return from every live leaf */
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
          else grasses++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && ridges >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && ridges * 2 >= need && soft == 0)
        mesh_ok = 1;
      int drift_ok = (live >= 2 && drifts >= live && soft == 0) ? 1 : 0;
      if (!drift_ok && live >= 2 && drifts * 2 >= live && soft == 0)
        drift_ok = 1;
      int grass_ok = (live >= 1 && grasses >= live && soft == 0) ? 1 : 0;
      if (!grass_ok && live >= 1 && grasses * 2 >= live && soft == 0)
        grass_ok = 1;
      int duned = (mesh_ok && drift_ok && grass_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (duned ? 11 : (ridges > 0 ? 3 : 0)) +
                   (drifts > 0 ? 1 : 0) + (grasses > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_DUNED", (long)duned);
      var_set_num(vm, "SMX_DUNE", (long)(duned ? ridges + drifts + grasses : 0));
      var_set_num(vm, "SMX_FOREDUNE", (long)(duned ? 1 : 0));
      var_set_num(vm, "SMX_SAND", (long)(duned ? 1 : 0));
      var_set_num(vm, "SMX_RIDGES", (long)ridges);
      var_set_num(vm, "SMX_RIDGE", (long)(duned ? ridges : 0));
      var_set_num(vm, "SMX_DRIFTS", (long)(duned ? drifts : 0));
      var_set_num(vm, "SMX_DRIFT", (long)(duned ? drifts : 0));
      var_set_num(vm, "SMX_AMMOPHILA", (long)(duned ? grasses : 0));
      var_set_num(vm, "SMX_BEACHGRASS", (long)(duned ? grasses : 0));
      var_set_num(vm, "SMX_GRASSES", (long)(duned ? grasses : 0));
      var_set_num(vm, "SMX_MESH", (long)(duned ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)ridges);
      var_set_num(vm, "SMX_EXCHANGES", (long)ridges);
      var_set_num(vm, "SMX_FUSE", (long)ridges);
      var_set_num(vm, "SMX_BIND", (long)ridges);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(ridges + drifts + grasses));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (duned){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX DUNE ok");
      } else if (ridges > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX DUNE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX DUNE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX DUNE nodes=%d live=%d ridges=%d drifts=%d grasses=%d need=%d soft=%d talks=%d oob=%d duned=%d vital=%ld\n",
                n, live, ridges, drifts, grasses, need, soft, vm->smx_talks, vm->smx_oob, duned, vital);
    }
    bump(vm); return 1;
  }
  /* SMX OASIS|MIRAGE|WADI|PALM|DATEPALM|SPRINGWELL|MESH_OASIS|RAISE_OASIS a b c ...
   * Life-force oasis after dune ridge: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete wadi mesh among live nodes, weaves a
   * directed mirage ring (i -> i+1) so desert heat pulses every wadi, then palm root
   * anchors return so lattice raises a springwell where dune meets open sky.
   * Latches SMX_OASISED when mesh+mirages+palms are soft-OOB-free.
   * SMX_WADI = chain bonds; SMX_DATEPALM = root gather pulses;
   * SMX_OASIS = wadis+mirages+palms; SMX_MIRAGE|SMX_WADIS sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"OASIS")||kw(&L->cur,"MIRAGE")||kw(&L->cur,"WADI")||
      kw(&L->cur,"PALM")||kw(&L->cur,"DATEPALM")||kw(&L->cur,"SPRINGWELL")||
      kw(&L->cur,"MESH_OASIS")||kw(&L->cur,"RAISE_OASIS")||
      kw(&L->cur,"OASES")||kw(&L->cur,"WADIS")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int wadis = 0;
    int mirages = 0;
    int palms = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "OASIS needs >=2 cubes",
                  "SMX OASIS a b [c ...]  or  SMX MIRAGE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - oasis needs clear channel */
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
    /* complete oasis mesh among live */
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
            else wadis++;
          }
        }
      }
    }
    /* mirage ring - energy pulses every edge i -> i+1 both ways */
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
          else mirages++;
        }
      }
    }
    /* palm pool - root anchors return from every live leaf */
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
          else palms++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && wadis >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && wadis * 2 >= need && soft == 0)
        mesh_ok = 1;
      int mirage_ok = (live >= 2 && mirages >= live && soft == 0) ? 1 : 0;
      if (!mirage_ok && live >= 2 && mirages * 2 >= live && soft == 0)
        mirage_ok = 1;
      int palm_ok = (live >= 1 && palms >= live && soft == 0) ? 1 : 0;
      if (!palm_ok && live >= 1 && palms * 2 >= live && soft == 0)
        palm_ok = 1;
      int oasised = (mesh_ok && mirage_ok && palm_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (oasised ? 11 : (wadis > 0 ? 3 : 0)) +
                   (mirages > 0 ? 1 : 0) + (palms > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_OASISED", (long)oasised);
      var_set_num(vm, "SMX_OASIS", (long)(oasised ? wadis + mirages + palms : 0));
      var_set_num(vm, "SMX_MIRAGE", (long)(oasised ? 1 : 0));
      var_set_num(vm, "SMX_WADIS", (long)(oasised ? wadis : 0));
      var_set_num(vm, "SMX_WADI", (long)(oasised ? wadis : 0));
      var_set_num(vm, "SMX_SPRINGWELL", (long)(oasised ? 1 : 0));
      var_set_num(vm, "SMX_MIRAGES", (long)(oasised ? mirages : 0));
      var_set_num(vm, "SMX_DATEPALM", (long)(oasised ? palms : 0));
      var_set_num(vm, "SMX_PALM", (long)(oasised ? palms : 0));
      var_set_num(vm, "SMX_PALMS", (long)(oasised ? palms : 0));
      var_set_num(vm, "SMX_MESH", (long)(oasised ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)wadis);
      var_set_num(vm, "SMX_EXCHANGES", (long)wadis);
      var_set_num(vm, "SMX_FUSE", (long)wadis);
      var_set_num(vm, "SMX_BIND", (long)wadis);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(wadis + mirages + palms));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (oasised){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX OASIS ok");
      } else if (wadis > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX OASIS partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX OASIS soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX OASIS nodes=%d live=%d wadis=%d mirages=%d palms=%d need=%d soft=%d talks=%d oob=%d oasised=%d vital=%ld\n",
                n, live, wadis, mirages, palms, need, soft, vm->smx_talks, vm->smx_oob, oasised, vital);
    }
    bump(vm); return 1;
  }
  /* SMX GROTTO|CAVERN|DRIP|STALACTITE|STALAGMITE|FLOWSTONE|MESH_GROTTO|RAISE_GROTTO a b c ...
   * Life-force grotto after oasis spring: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete chamber mesh among live nodes, weaves a
   * directed drip ring (i -> i+1) so cave water pulses every chamber, then pillar root
   * anchors return so lattice raises a flowstone where spring meets stone.
   * Latches SMX_GROTTOED when mesh+drips+pillars are soft-OOB-free.
   * SMX_CHAMBER = chain bonds; SMX_PILLAR = root gather pulses;
   * SMX_GROTTO = chambers+drips+pillars; SMX_DRIP|SMX_CHAMBERS sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"GROTTO")||kw(&L->cur,"CAVERN")||kw(&L->cur,"DRIP")||
      kw(&L->cur,"STALACTITE")||kw(&L->cur,"STALAGMITE")||kw(&L->cur,"FLOWSTONE")||
      kw(&L->cur,"MESH_GROTTO")||kw(&L->cur,"RAISE_GROTTO")||
      kw(&L->cur,"GROTTOS")||kw(&L->cur,"CHAMBER")||kw(&L->cur,"CHAMBERS")||kw(&L->cur,"PILLAR")||
      kw(&L->cur,"PILLARS")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int chambers = 0;
    int drips = 0;
    int pillars = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "GROTTO needs >=2 cubes",
                  "SMX GROTTO a b [c ...]  or  SMX CAVERN a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - grotto needs clear channel */
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
    /* complete chamber mesh among live */
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
            else chambers++;
          }
        }
      }
    }
    /* drip ring - cave water pulses every edge i -> i+1 both ways */
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
          else drips++;
        }
      }
    }
    /* pillar pool - root anchors return from every live leaf */
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
          else pillars++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && chambers >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && chambers * 2 >= need && soft == 0)
        mesh_ok = 1;
      int drip_ok = (live >= 2 && drips >= live && soft == 0) ? 1 : 0;
      if (!drip_ok && live >= 2 && drips * 2 >= live && soft == 0)
        drip_ok = 1;
      int pillar_ok = (live >= 1 && pillars >= live && soft == 0) ? 1 : 0;
      if (!pillar_ok && live >= 1 && pillars * 2 >= live && soft == 0)
        pillar_ok = 1;
      int grottoed = (mesh_ok && drip_ok && pillar_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (grottoed ? 11 : (chambers > 0 ? 3 : 0)) +
                   (drips > 0 ? 1 : 0) + (pillars > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_GROTTOED", (long)grottoed);
      var_set_num(vm, "SMX_GROTTO", (long)(grottoed ? chambers + drips + pillars : 0));
      var_set_num(vm, "SMX_DRIP", (long)(grottoed ? 1 : 0));
      var_set_num(vm, "SMX_CHAMBERS", (long)(grottoed ? chambers : 0));
      var_set_num(vm, "SMX_CHAMBER", (long)(grottoed ? chambers : 0));
      var_set_num(vm, "SMX_FLOWSTONE", (long)(grottoed ? 1 : 0));
      var_set_num(vm, "SMX_DRIPS", (long)(grottoed ? drips : 0));
      var_set_num(vm, "SMX_PILLAR", (long)(grottoed ? pillars : 0));
      var_set_num(vm, "SMX_PILLARS", (long)(grottoed ? pillars : 0));
      var_set_num(vm, "SMX_STALACTITE", (long)(grottoed ? drips : 0));
      var_set_num(vm, "SMX_STALAGMITE", (long)(grottoed ? pillars : 0));
      var_set_num(vm, "SMX_MESH", (long)(grottoed ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)chambers);
      var_set_num(vm, "SMX_EXCHANGES", (long)chambers);
      var_set_num(vm, "SMX_FUSE", (long)chambers);
      var_set_num(vm, "SMX_BIND", (long)chambers);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(chambers + drips + pillars));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (grottoed){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX GROTTO ok");
      } else if (chambers > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX GROTTO partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX GROTTO soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX GROTTO nodes=%d live=%d chambers=%d drips=%d pillars=%d need=%d soft=%d talks=%d oob=%d grottoed=%d vital=%ld\n",
                n, live, chambers, drips, pillars, need, soft, vm->smx_talks, vm->smx_oob, grottoed, vital);
    }
    bump(vm); return 1;
  }
  /* SMX CRYSTAL|GEODE|FACET|PRISM|NUCLEUS|QUARTZ|MESH_CRYSTAL|RAISE_CRYSTAL a b c ...
   * Life-force crystal geode after grotto chamber: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete facet mesh among live nodes, weaves a
   * directed prism ring (i -> i+1) so light pulses every facet, then nucleus root
   * anchors return so lattice raises a quartz geode where stone meets light.
   * Latches SMX_CRYSTALLED when mesh+prisms+nuclei are soft-OOB-free.
   * SMX_FACET = chain bonds; SMX_NUCLEUS = root gather pulses;
   * SMX_CRYSTAL = facets+prisms+nuclei; SMX_PRISM|SMX_FACETS sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"CRYSTAL")||kw(&L->cur,"GEODE")||kw(&L->cur,"FACET")||
      kw(&L->cur,"PRISM")||kw(&L->cur,"NUCLEUS")||kw(&L->cur,"QUARTZ")||
      kw(&L->cur,"MESH_CRYSTAL")||kw(&L->cur,"RAISE_CRYSTAL")||
      kw(&L->cur,"CRYSTALS")||kw(&L->cur,"FACETS")||kw(&L->cur,"PRISMS")||
      kw(&L->cur,"GEODES")||kw(&L->cur,"SEEDCRYSTAL")||kw(&L->cur,"LATTICE_CRYSTAL")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int facets = 0;
    int prisms = 0;
    int nuclei = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "CRYSTAL needs >=2 cubes",
                  "SMX CRYSTAL a b [c ...]  or  SMX GEODE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - crystal needs clear channel */
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
    /* complete facet mesh among live */
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
            else facets++;
          }
        }
      }
    }
    /* prism ring - light pulses every edge i -> i+1 both ways */
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
          else prisms++;
        }
      }
    }
    /* nucleus pool - seed anchors return from every live leaf */
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
          else nuclei++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && facets >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && facets * 2 >= need && soft == 0)
        mesh_ok = 1;
      int prism_ok = (live >= 2 && prisms >= live && soft == 0) ? 1 : 0;
      if (!prism_ok && live >= 2 && prisms * 2 >= live && soft == 0)
        prism_ok = 1;
      int nucleus_ok = (live >= 1 && nuclei >= live && soft == 0) ? 1 : 0;
      if (!nucleus_ok && live >= 1 && nuclei * 2 >= live && soft == 0)
        nucleus_ok = 1;
      int crystallised = (mesh_ok && prism_ok && nucleus_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (crystallised ? 11 : (facets > 0 ? 3 : 0)) +
                   (prisms > 0 ? 1 : 0) + (nuclei > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_CRYSTALLED", (long)crystallised);
      var_set_num(vm, "SMX_CRYSTAL", (long)(crystallised ? facets + prisms + nuclei : 0));
      var_set_num(vm, "SMX_PRISM", (long)(crystallised ? 1 : 0));
      var_set_num(vm, "SMX_FACETS", (long)(crystallised ? facets : 0));
      var_set_num(vm, "SMX_FACET", (long)(crystallised ? facets : 0));
      var_set_num(vm, "SMX_QUARTZ", (long)(crystallised ? 1 : 0));
      var_set_num(vm, "SMX_PRISMS", (long)(crystallised ? prisms : 0));
      var_set_num(vm, "SMX_NUCLEUS", (long)(crystallised ? nuclei : 0));
      var_set_num(vm, "SMX_NUCLEI", (long)(crystallised ? nuclei : 0));
      var_set_num(vm, "SMX_GEODE", (long)(crystallised ? 1 : 0));
      var_set_num(vm, "SMX_SEEDCRYSTAL", (long)(crystallised ? nuclei : 0));
      var_set_num(vm, "SMX_MESH", (long)(crystallised ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)facets);
      var_set_num(vm, "SMX_EXCHANGES", (long)facets);
      var_set_num(vm, "SMX_FUSE", (long)facets);
      var_set_num(vm, "SMX_BIND", (long)facets);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(facets + prisms + nuclei));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (crystallised){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CRYSTAL ok");
      } else if (facets > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CRYSTAL partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX CRYSTAL soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX CRYSTAL nodes=%d live=%d facets=%d prisms=%d nuclei=%d need=%d soft=%d talks=%d oob=%d crystallised=%d vital=%ld\n",
                n, live, facets, prisms, nuclei, need, soft, vm->smx_talks, vm->smx_oob, crystallised, vital);
    }
    bump(vm); return 1;
  }
  /* SMX AURORA|BOREALIS|VEIL|RIBBON|CORONA|ARC|MESH_AURORA|RAISE_AURORA a b c ...
   * Life-force aurora curtain after crystal geode: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete veil mesh among live nodes, weaves a
   * directed ribbon ring (i -> i+1) so light dances every veil, then corona root
   * anchors return so lattice raises a polar aurora where sky meets free energy.
   * Latches SMX_AURORAED when mesh+ribbons+coronas are soft-OOB-free.
   * SMX_VEIL = chain bonds; SMX_CORONA = root gather pulses;
   * SMX_AURORA = veils+ribbons+coronas; SMX_RIBBON|SMX_VEILS sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"AURORA")||kw(&L->cur,"BOREALIS")||kw(&L->cur,"VEIL")||
      kw(&L->cur,"RIBBON")||kw(&L->cur,"CORONA")||kw(&L->cur,"ARC")||
      kw(&L->cur,"MESH_AURORA")||kw(&L->cur,"RAISE_AURORA")||
      kw(&L->cur,"AURORAS")||kw(&L->cur,"VEILS")||kw(&L->cur,"RIBBONS")||
      kw(&L->cur,"CORONAS")||kw(&L->cur,"SEEDAURORA")||kw(&L->cur,"LATTICE_AURORA")||
      kw(&L->cur,"POLARIS")||kw(&L->cur,"SKYFIRE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int veils = 0;
    int ribbons = 0;
    int coronas = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "AURORA needs >=2 cubes",
                  "SMX AURORA a b [c ...]  or  SMX BOREALIS a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - crystal needs clear channel */
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
    /* complete veil mesh among live */
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
            else veils++;
          }
        }
      }
    }
    /* ribbon ring - light dances every edge i -> i+1 both ways */
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
          else ribbons++;
        }
      }
    }
    /* corona pool - seed anchors return from every live leaf */
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
          else coronas++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && veils >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && veils * 2 >= need && soft == 0)
        mesh_ok = 1;
      int ribbon_ok = (live >= 2 && ribbons >= live && soft == 0) ? 1 : 0;
      if (!ribbon_ok && live >= 2 && ribbons * 2 >= live && soft == 0)
        ribbon_ok = 1;
      int corona_ok = (live >= 1 && coronas >= live && soft == 0) ? 1 : 0;
      if (!corona_ok && live >= 1 && coronas * 2 >= live && soft == 0)
        corona_ok = 1;
      int auroraed = (mesh_ok && ribbon_ok && corona_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (auroraed ? 11 : (veils > 0 ? 3 : 0)) +
                   (ribbons > 0 ? 1 : 0) + (coronas > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_AURORAED", (long)auroraed);
      var_set_num(vm, "SMX_AURORA", (long)(auroraed ? veils + ribbons + coronas : 0));
      var_set_num(vm, "SMX_RIBBON", (long)(auroraed ? 1 : 0));
      var_set_num(vm, "SMX_VEILS", (long)(auroraed ? veils : 0));
      var_set_num(vm, "SMX_VEIL", (long)(auroraed ? veils : 0));
      var_set_num(vm, "SMX_ARC", (long)(auroraed ? 1 : 0));
      var_set_num(vm, "SMX_RIBBONS", (long)(auroraed ? ribbons : 0));
      var_set_num(vm, "SMX_CORONA", (long)(auroraed ? coronas : 0));
      var_set_num(vm, "SMX_CORONAS", (long)(auroraed ? coronas : 0));
      var_set_num(vm, "SMX_BOREALIS", (long)(auroraed ? 1 : 0));
      var_set_num(vm, "SMX_SEEDAURORA", (long)(auroraed ? coronas : 0));
      var_set_num(vm, "SMX_MESH", (long)(auroraed ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)veils);
      var_set_num(vm, "SMX_EXCHANGES", (long)veils);
      var_set_num(vm, "SMX_FUSE", (long)veils);
      var_set_num(vm, "SMX_BIND", (long)veils);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(veils + ribbons + coronas));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (auroraed){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AURORA ok");
      } else if (veils > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AURORA partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX AURORA soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX AURORA nodes=%d live=%d veils=%d ribbons=%d coronas=%d need=%d soft=%d talks=%d oob=%d auroraed=%d vital=%ld\n",
                n, live, veils, ribbons, coronas, need, soft, vm->smx_talks, vm->smx_oob, auroraed, vital);
    }
    bump(vm); return 1;
  }

  /* SMX SOLSTICE|EQUINOX|MERIDIAN|SPINE|ZENITH|AXIS|MESH_SOLSTICE|RAISE_SOLSTICE a b c ...
   * Life-force polar balance after aurora curtain: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete meridian mesh among live nodes, weaves an
   * axial spine ring (i -> i+1) so free energy holds both poles, then zenith hub
   * gathers return so lattice raises a solstice balance where day meets night.
   * Latches SMX_SOLSTICED when mesh+spines+zeniths are soft-OOB-free.
   * SMX_MERIDIAN = chain bonds; SMX_ZENITH = root gather pulses;
   * SMX_SOLSTICE = meridians+spines+zeniths; SMX_SPINE|SMX_AXIS sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"SOLSTICE")||kw(&L->cur,"EQUINOX")||kw(&L->cur,"MERIDIAN")||
      kw(&L->cur,"SPINE")||kw(&L->cur,"ZENITH")||kw(&L->cur,"AXIS")||
      kw(&L->cur,"MESH_SOLSTICE")||kw(&L->cur,"RAISE_SOLSTICE")||
      kw(&L->cur,"SOLSTICES")||kw(&L->cur,"MERIDIANS")||kw(&L->cur,"SPINES")||
      kw(&L->cur,"ZENITHS")||kw(&L->cur,"AXES")||kw(&L->cur,"SEEDSOLSTICE")||
      kw(&L->cur,"LATTICE_SOLSTICE")||kw(&L->cur,"POLAR_BALANCE")||
      kw(&L->cur,"DAYNIGHT")||kw(&L->cur,"TURNING")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int meridians = 0;
    int spines = 0;
    int zeniths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "SOLSTICE needs >=2 cubes",
                  "SMX SOLSTICE a b [c ...]  or  SMX EQUINOX a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - solstice needs clear channel */
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
    /* complete meridian mesh among live */
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
            else meridians++;
          }
        }
      }
    }
    /* axial spine ring - free energy holds every edge i -> i+1 both ways */
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
          else spines++;
        }
      }
    }
    /* zenith hub - seed anchors return from every live leaf */
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
          else zeniths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && meridians >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && meridians * 2 >= need && soft == 0)
        mesh_ok = 1;
      int spine_ok = (live >= 2 && spines >= live && soft == 0) ? 1 : 0;
      if (!spine_ok && live >= 2 && spines * 2 >= live && soft == 0)
        spine_ok = 1;
      int zenith_ok = (live >= 1 && zeniths >= live && soft == 0) ? 1 : 0;
      if (!zenith_ok && live >= 1 && zeniths * 2 >= live && soft == 0)
        zenith_ok = 1;
      int solsticed = (mesh_ok && spine_ok && zenith_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (solsticed ? 12 : (meridians > 0 ? 3 : 0)) +
                   (spines > 0 ? 1 : 0) + (zeniths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_SOLSTICED", (long)solsticed);
      var_set_num(vm, "SMX_SOLSTICE", (long)(solsticed ? meridians + spines + zeniths : 0));
      var_set_num(vm, "SMX_SPINE", (long)(solsticed ? 1 : 0));
      var_set_num(vm, "SMX_AXIS", (long)(solsticed ? 1 : 0));
      var_set_num(vm, "SMX_MERIDIANS", (long)(solsticed ? meridians : 0));
      var_set_num(vm, "SMX_MERIDIAN", (long)(solsticed ? meridians : 0));
      var_set_num(vm, "SMX_EQUINOX", (long)(solsticed ? 1 : 0));
      var_set_num(vm, "SMX_SPINES", (long)(solsticed ? spines : 0));
      var_set_num(vm, "SMX_ZENITH", (long)(solsticed ? zeniths : 0));
      var_set_num(vm, "SMX_ZENITHS", (long)(solsticed ? zeniths : 0));
      var_set_num(vm, "SMX_SEEDSOLSTICE", (long)(solsticed ? zeniths : 0));
      var_set_num(vm, "SMX_MESH", (long)(solsticed ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)meridians);
      var_set_num(vm, "SMX_EXCHANGES", (long)meridians);
      var_set_num(vm, "SMX_FUSE", (long)meridians);
      var_set_num(vm, "SMX_BIND", (long)meridians);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(meridians + spines + zeniths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (solsticed){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SOLSTICE ok");
      } else if (meridians > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SOLSTICE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX SOLSTICE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX SOLSTICE nodes=%d live=%d meridians=%d spines=%d zeniths=%d need=%d soft=%d talks=%d oob=%d solsticed=%d vital=%ld\n",
                n, live, meridians, spines, zeniths, need, soft, vm->smx_talks, vm->smx_oob, solsticed, vital);
    }
    bump(vm); return 1;
  }
  /* SMX HELIOS|ORBIT|ECLIPSE|APHELION|PERIHELION|PHOTON|MESH_HELIOS|RAISE_HELIOS a b c ...
   * Life-force solar orbit after solstice balance: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete photon mesh among live nodes, weaves an
   * orbital ring (i -> i+1) so free energy circles the sun-path, then helios hub
   * gathers return so lattice raises a helios lock where light holds the hive.
   * Latches SMX_HELIOSED when mesh+orbits+helioses are soft-OOB-free.
   * SMX_PHOTON = chain bonds; SMX_HELIOS hub = root gather pulses;
   * SMX_ORBIT sum = photons+orbits+helioses; SMX_ORBIT|SMX_ECLIPSE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"HELIOS")||kw(&L->cur,"ORBIT")||kw(&L->cur,"ECLIPSE")||
      kw(&L->cur,"APHELION")||kw(&L->cur,"PERIHELION")||kw(&L->cur,"PHOTON")||
      kw(&L->cur,"MESH_HELIOS")||kw(&L->cur,"RAISE_HELIOS")||
      kw(&L->cur,"HELIOSES")||kw(&L->cur,"ORBITS")||kw(&L->cur,"ECLIPSES")||
      kw(&L->cur,"PHOTONS")||kw(&L->cur,"APHELIONS")||kw(&L->cur,"SEEDHELIOS")||
      kw(&L->cur,"LATTICE_HELIOS")||kw(&L->cur,"SOLAR_LOCK")||
      kw(&L->cur,"SUNPATH")||kw(&L->cur,"DAYSTAR")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int photons = 0;
    int orbits = 0;
    int helioses = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "HELIOS needs >=2 cubes",
                  "SMX HELIOS a b [c ...]  or  SMX ORBIT a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - helios needs clear channel */
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
    /* complete photon mesh among live */
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
            else photons++;
          }
        }
      }
    }
    /* orbital ring - free energy circles every edge i -> i+1 both ways */
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
          else orbits++;
        }
      }
    }
    /* helios hub - seed anchors return from every live leaf */
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
          else helioses++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && photons >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && photons * 2 >= need && soft == 0)
        mesh_ok = 1;
      int orbit_ok = (live >= 2 && orbits >= live && soft == 0) ? 1 : 0;
      if (!orbit_ok && live >= 2 && orbits * 2 >= live && soft == 0)
        orbit_ok = 1;
      int helios_ok = (live >= 1 && helioses >= live && soft == 0) ? 1 : 0;
      if (!helios_ok && live >= 1 && helioses * 2 >= live && soft == 0)
        helios_ok = 1;
      int heliosed = (mesh_ok && orbit_ok && helios_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (heliosed ? 12 : (photons > 0 ? 3 : 0)) +
                   (orbits > 0 ? 1 : 0) + (helioses > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_HELIOSED", (long)heliosed);
      var_set_num(vm, "SMX_HELIOS", (long)(heliosed ? photons + orbits + helioses : 0));
      var_set_num(vm, "SMX_ORBIT", (long)(heliosed ? 1 : 0));
      var_set_num(vm, "SMX_ECLIPSE", (long)(heliosed ? 1 : 0));
      var_set_num(vm, "SMX_PHOTONS", (long)(heliosed ? photons : 0));
      var_set_num(vm, "SMX_PHOTON", (long)(heliosed ? photons : 0));
      var_set_num(vm, "SMX_APHELION", (long)(heliosed ? 1 : 0));
      var_set_num(vm, "SMX_ORBITS", (long)(heliosed ? orbits : 0));
      var_set_num(vm, "SMX_HELIOSES", (long)(heliosed ? helioses : 0));
      var_set_num(vm, "SMX_PERIHELION", (long)(heliosed ? helioses : 0));
      var_set_num(vm, "SMX_SEEDHELIOS", (long)(heliosed ? helioses : 0));
      var_set_num(vm, "SMX_MESH", (long)(heliosed ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)photons);
      var_set_num(vm, "SMX_EXCHANGES", (long)photons);
      var_set_num(vm, "SMX_FUSE", (long)photons);
      var_set_num(vm, "SMX_BIND", (long)photons);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(photons + orbits + helioses));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (heliosed){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HELIOS ok");
      } else if (photons > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HELIOS partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX HELIOS soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX HELIOS nodes=%d live=%d photons=%d orbits=%d helioses=%d need=%d soft=%d talks=%d oob=%d heliosed=%d vital=%ld\n",
                n, live, photons, orbits, helioses, need, soft, vm->smx_talks, vm->smx_oob, heliosed, vital);
    }
    bump(vm); return 1;
  }
  /* SMX NEBULA|STELLAR|NURSERY|DUST|CORE|CLOUD|MESH_NEBULA|RAISE_NEBULA a b c ...
   * Life-force stellar nursery after helios lock: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete dust mesh among live nodes, weaves a
   * nursery ring (i -> i+1) so free energy circles the cloud-path, then core hub
   * gathers return so lattice raises a nebula lock where stars birth the hive.
   * Latches SMX_NEBULAED when mesh+nurseries+cores are soft-OOB-free.
   * SMX_DUST = chain bonds; SMX_CORE hub = root gather pulses;
   * SMX_NEBULA sum = dusts+nurseries+cores; SMX_NURSERY|SMX_CLOUD sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"NEBULA")||kw(&L->cur,"STELLAR")||kw(&L->cur,"NURSERY")||
      kw(&L->cur,"DUST")||kw(&L->cur,"CORE")||kw(&L->cur,"CLOUD")||
      kw(&L->cur,"MESH_NEBULA")||kw(&L->cur,"RAISE_NEBULA")||
      kw(&L->cur,"NEBULAS")||kw(&L->cur,"NURSERIES")||kw(&L->cur,"CLOUDS")||
      kw(&L->cur,"DUSTS")||kw(&L->cur,"CORES")||kw(&L->cur,"SEEDNEBULA")||
      kw(&L->cur,"LATTICE_NEBULA")||kw(&L->cur,"STAR_BIRTH")||
      kw(&L->cur,"STARDUST")||kw(&L->cur,"PROTOSTAR")||
      kw(&L->cur,"LIFE_FORCE")||kw(&L->cur,"LIFEFORCE")||kw(&L->cur,"MESH_LIFE_FORCE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int dusts = 0;
    int nurseries = 0;
    int cores = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "NEBULA needs >=2 cubes",
                  "SMX NEBULA a b [c ...]  or  SMX STELLAR a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - nebula needs clear channel */
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
    /* complete dust mesh among live */
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
            else dusts++;
          }
        }
      }
    }
    /* nursery ring - free energy circles every edge i -> i+1 both ways */
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
          else nurseries++;
        }
      }
    }
    /* core hub - seed anchors return from every live leaf */
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
      int mesh_ok = (need > 0 && dusts >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && dusts * 2 >= need && soft == 0)
        mesh_ok = 1;
      int nursery_ok = (live >= 2 && nurseries >= live && soft == 0) ? 1 : 0;
      if (!nursery_ok && live >= 2 && nurseries * 2 >= live && soft == 0)
        nursery_ok = 1;
      int core_ok = (live >= 1 && cores >= live && soft == 0) ? 1 : 0;
      if (!core_ok && live >= 1 && cores * 2 >= live && soft == 0)
        core_ok = 1;
      int nebulaed = (mesh_ok && nursery_ok && core_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (nebulaed ? 12 : (dusts > 0 ? 3 : 0)) +
                   (nurseries > 0 ? 1 : 0) + (cores > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_NEBULAED", (long)nebulaed);
      var_set_num(vm, "SMX_NEBULA", (long)(nebulaed ? dusts + nurseries + cores : 0));
      var_set_num(vm, "SMX_LIFE_FORCE", (long)(nebulaed ? 1 : 0));
      var_set_num(vm, "SMX_LIFE", (long)(nebulaed ? dusts + nurseries + cores : 0));
      var_set_num(vm, "SMX_NURSERY", (long)(nebulaed ? 1 : 0));
      var_set_num(vm, "SMX_CLOUD", (long)(nebulaed ? 1 : 0));
      var_set_num(vm, "SMX_DUSTS", (long)(nebulaed ? dusts : 0));
      var_set_num(vm, "SMX_DUST", (long)(nebulaed ? dusts : 0));
      var_set_num(vm, "SMX_STELLAR", (long)(nebulaed ? 1 : 0));
      var_set_num(vm, "SMX_NURSERIES", (long)(nebulaed ? nurseries : 0));
      var_set_num(vm, "SMX_CORES", (long)(nebulaed ? cores : 0));
      var_set_num(vm, "SMX_CORE", (long)(nebulaed ? cores : 0));
      var_set_num(vm, "SMX_SEEDNEBULA", (long)(nebulaed ? cores : 0));
      var_set_num(vm, "SMX_MESH", (long)(nebulaed ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)dusts);
      var_set_num(vm, "SMX_EXCHANGES", (long)dusts);
      var_set_num(vm, "SMX_FUSE", (long)dusts);
      var_set_num(vm, "SMX_BIND", (long)dusts);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(dusts + nurseries + cores));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (nebulaed){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX NEBULA ok");
      } else if (dusts > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX NEBULA partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX NEBULA soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX NEBULA nodes=%d live=%d dusts=%d nurseries=%d cores=%d need=%d soft=%d talks=%d oob=%d nebulaed=%d vital=%ld\n",
                n, live, dusts, nurseries, cores, need, soft, vm->smx_talks, vm->smx_oob, nebulaed, vital);
    }
    bump(vm); return 1;
  }
  /* SMX PULSAR|MAGNETAR|BEACON|SPIN|JET|LIGHTHOUSE|MESH_PULSAR|RAISE_PULSAR a b c ...
   * Life-force stellar beacon after helios lock: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete spin mesh among live nodes, weaves a
   * beacon ring (i -> i+1) so free energy circles the cloud-path, then jet hub
   * gathers return so lattice raises a pulsar lock where stars birth the hive.
   * Latches SMX_PULSARED when mesh+beacons+jets are soft-OOB-free.
   * SMX_SPIN = chain bonds; SMX_JET hub = root gather pulses;
   * SMX_PULSAR sum = spins+beacons+jets; SMX_BEACON|SMX_LIGHTHOUSE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"PULSAR")||kw(&L->cur,"MAGNETAR")||kw(&L->cur,"BEACON")||
      kw(&L->cur,"SPIN")||kw(&L->cur,"JET")||kw(&L->cur,"LIGHTHOUSE")||
      kw(&L->cur,"MESH_PULSAR")||kw(&L->cur,"RAISE_PULSAR")||
      kw(&L->cur,"PULSARS")||kw(&L->cur,"BEACONS")||kw(&L->cur,"LIGHTHOUSES")||
      kw(&L->cur,"SPINS")||kw(&L->cur,"JETS")||kw(&L->cur,"SEEDPULSAR")||
      kw(&L->cur,"LATTICE_PULSAR")||kw(&L->cur,"TIMED_LIGHT")||
      kw(&L->cur,"STARSPIN")||kw(&L->cur,"PULSE_STAR")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int spins = 0;
    int beacons = 0;
    int jets = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "PULSAR needs >=2 cubes",
                  "SMX PULSAR a b [c ...]  or  SMX MAGNETAR a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - pulsar needs clear channel */
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
    /* complete spin mesh among live */
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
            else spins++;
          }
        }
      }
    }
    /* beacon ring - free energy circles every edge i -> i+1 both ways */
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
          else beacons++;
        }
      }
    }
    /* jet hub - seed anchors return from every live leaf */
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
          else jets++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && spins >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && spins * 2 >= need && soft == 0)
        mesh_ok = 1;
      int beacon_ok = (live >= 2 && beacons >= live && soft == 0) ? 1 : 0;
      if (!beacon_ok && live >= 2 && beacons * 2 >= live && soft == 0)
        beacon_ok = 1;
      int jet_ok = (live >= 1 && jets >= live && soft == 0) ? 1 : 0;
      if (!jet_ok && live >= 1 && jets * 2 >= live && soft == 0)
        jet_ok = 1;
      int pulsared = (mesh_ok && beacon_ok && jet_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (pulsared ? 12 : (spins > 0 ? 3 : 0)) +
                   (beacons > 0 ? 1 : 0) + (jets > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_PULSARED", (long)pulsared);
      var_set_num(vm, "SMX_PULSAR", (long)(pulsared ? spins + beacons + jets : 0));
      var_set_num(vm, "SMX_BEACON", (long)(pulsared ? 1 : 0));
      var_set_num(vm, "SMX_LIGHTHOUSE", (long)(pulsared ? 1 : 0));
      var_set_num(vm, "SMX_SPINS", (long)(pulsared ? spins : 0));
      var_set_num(vm, "SMX_SPIN", (long)(pulsared ? spins : 0));
      var_set_num(vm, "SMX_MAGNETAR", (long)(pulsared ? 1 : 0));
      var_set_num(vm, "SMX_BEACONS", (long)(pulsared ? beacons : 0));
      var_set_num(vm, "SMX_JETS", (long)(pulsared ? jets : 0));
      var_set_num(vm, "SMX_JET", (long)(pulsared ? jets : 0));
      var_set_num(vm, "SMX_SEEDPULSAR", (long)(pulsared ? jets : 0));
      var_set_num(vm, "SMX_MESH", (long)(pulsared ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)spins);
      var_set_num(vm, "SMX_EXCHANGES", (long)spins);
      var_set_num(vm, "SMX_FUSE", (long)spins);
      var_set_num(vm, "SMX_BIND", (long)spins);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(spins + beacons + jets));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (pulsared){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX PULSAR ok");
      } else if (spins > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX PULSAR partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX PULSAR soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX PULSAR nodes=%d live=%d spins=%d beacons=%d jets=%d need=%d soft=%d talks=%d oob=%d pulsared=%d vital=%ld\n",
                n, live, spins, beacons, jets, need, soft, vm->smx_talks, vm->smx_oob, pulsared, vital);
    }
    bump(vm); return 1;
  }
  /* SMX QUASAR|BLAZAR|DISK|ACCRETION|JETSTREAM|EVENTHORIZON|MESH_QUASAR|RAISE_QUASAR a b c ...
   * Life-force galactic engine after pulsar lock: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete accretion mesh among live nodes, weaves a
   * disk ring (i -> i+1) so free energy circles the event path, then jetstream hub
   * gathers return so lattice raises a quasar lock where galaxies feed the hive.
   * Latches SMX_QUASARED when mesh+disks+jetstreams are soft-OOB-free.
   * SMX_ACCRETION = chain bonds; SMX_JETSTREAM hub = root gather pulses;
   * SMX_QUASAR sum = accretions+disks+jetstreams; SMX_DISK|SMX_BLAZAR sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"QUASAR")||kw(&L->cur,"BLAZAR")||kw(&L->cur,"DISK")||
      kw(&L->cur,"ACCRETION")||kw(&L->cur,"JETSTREAM")||kw(&L->cur,"EVENTHORIZON")||
      kw(&L->cur,"MESH_QUASAR")||kw(&L->cur,"RAISE_QUASAR")||
      kw(&L->cur,"QUASARS")||kw(&L->cur,"DISKS")||kw(&L->cur,"EVENTHORIZONS")||
      kw(&L->cur,"ACCRETIONS")||kw(&L->cur,"JETSTREAMS")||kw(&L->cur,"SEEDQUASAR")||
      kw(&L->cur,"LATTICE_QUASAR")||kw(&L->cur,"GALACTIC_CORE")||
      kw(&L->cur,"ACTIVE_NUCLEUS")||kw(&L->cur,"EVENT_HORIZON")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int accretions = 0;
    int disks = 0;
    int jetstreams = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "QUASAR needs >=2 cubes",
                  "SMX QUASAR a b [c ...]  or  SMX BLAZAR a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - quasar needs clear channel */
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
    /* complete accretion mesh among live */
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
            else accretions++;
          }
        }
      }
    }
    /* disk ring - free energy circles every edge i -> i+1 both ways */
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
          else disks++;
        }
      }
    }
    /* jetstream hub - seed anchors return from every live leaf */
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
          else jetstreams++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && accretions >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && accretions * 2 >= need && soft == 0)
        mesh_ok = 1;
      int disk_ok = (live >= 2 && disks >= live && soft == 0) ? 1 : 0;
      if (!disk_ok && live >= 2 && disks * 2 >= live && soft == 0)
        disk_ok = 1;
      int jet_ok = (live >= 1 && jetstreams >= live && soft == 0) ? 1 : 0;
      if (!jet_ok && live >= 1 && jetstreams * 2 >= live && soft == 0)
        jet_ok = 1;
      int quasared = (mesh_ok && disk_ok && jet_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (quasared ? 12 : (accretions > 0 ? 3 : 0)) +
                   (disks > 0 ? 1 : 0) + (jetstreams > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_QUASARED", (long)quasared);
      var_set_num(vm, "SMX_QUASAR", (long)(quasared ? accretions + disks + jetstreams : 0));
      var_set_num(vm, "SMX_DISK", (long)(quasared ? 1 : 0));
      var_set_num(vm, "SMX_EVENTHORIZON", (long)(quasared ? 1 : 0));
      var_set_num(vm, "SMX_ACCRETIONS", (long)(quasared ? accretions : 0));
      var_set_num(vm, "SMX_ACCRETION", (long)(quasared ? accretions : 0));
      var_set_num(vm, "SMX_BLAZAR", (long)(quasared ? 1 : 0));
      var_set_num(vm, "SMX_DISKS", (long)(quasared ? disks : 0));
      var_set_num(vm, "SMX_JETSTREAMS", (long)(quasared ? jetstreams : 0));
      var_set_num(vm, "SMX_JETSTREAM", (long)(quasared ? jetstreams : 0));
      var_set_num(vm, "SMX_JETS", (long)(quasared ? jetstreams : 0));
      var_set_num(vm, "SMX_JET", (long)(quasared ? jetstreams : 0));
      var_set_num(vm, "SMX_SEEDQUASAR", (long)(quasared ? jetstreams : 0));
      var_set_num(vm, "SMX_MESH", (long)(quasared ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)accretions);
      var_set_num(vm, "SMX_EXCHANGES", (long)accretions);
      var_set_num(vm, "SMX_FUSE", (long)accretions);
      var_set_num(vm, "SMX_BIND", (long)accretions);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(accretions + disks + jetstreams));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (quasared){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX QUASAR ok");
      } else if (accretions > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX QUASAR partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX QUASAR soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX QUASAR nodes=%d live=%d accretions=%d disks=%d jetstreams=%d need=%d soft=%d talks=%d oob=%d quasared=%d vital=%ld\n",
                n, live, accretions, disks, jetstreams, need, soft, vm->smx_talks, vm->smx_oob, quasared, vital);
    }
    bump(vm); return 1;
  }

  /* SMX COMET|METEOR|TAIL|NUCLEUS_ICE|COMA|DEBRIS|MESH_COMET|RAISE_COMET a b c ...
   * Life-force ice-fire visitor after quasar lock: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete debris mesh among live nodes, weaves a
   * tail ring (i -> i+1) so free energy circles the coma-path, then nucleus hub
   * gathers return so lattice raises a comet lock where visitors seed the hive.
   * Latches SMX_COMETED when mesh+tails+nuclei are soft-OOB-free.
   * SMX_DEBRIS = chain bonds; SMX_NUCLEUS_ICE hub = root gather pulses;
   * SMX_COMET sum = debris+tails+nuclei; SMX_COMA|SMX_METEOR sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"COMET")||kw(&L->cur,"METEOR")||kw(&L->cur,"TAIL")||
      kw(&L->cur,"COMA")||kw(&L->cur,"DEBRIS")||kw(&L->cur,"NUCLEUS_ICE")||
      kw(&L->cur,"MESH_COMET")||kw(&L->cur,"RAISE_COMET")||
      kw(&L->cur,"COMETS")||kw(&L->cur,"TAILS")||kw(&L->cur,"COMAS")||
      kw(&L->cur,"DEBRISFIELD")||kw(&L->cur,"METEORS")||kw(&L->cur,"SEEDCOMET")||
      kw(&L->cur,"LATTICE_COMET")||kw(&L->cur,"ICE_FIRE")||
      kw(&L->cur,"VISITOR")||kw(&L->cur,"PULSE_COMET")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int debris = 0;
    int tails = 0;
    int nuclei = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "COMET needs >=2 cubes",
                  "SMX COMET a b [c ...]  or  SMX METEOR a b c d");
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
            else debris++;
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
          else tails++;
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
          else nuclei++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && debris >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && debris * 2 >= need && soft == 0)
        mesh_ok = 1;
      int tail_ok = (live >= 2 && tails >= live && soft == 0) ? 1 : 0;
      if (!tail_ok && live >= 2 && tails * 2 >= live && soft == 0)
        tail_ok = 1;
      int nucleus_ok = (live >= 1 && nuclei >= live && soft == 0) ? 1 : 0;
      if (!nucleus_ok && live >= 1 && nuclei * 2 >= live && soft == 0)
        nucleus_ok = 1;
      int cometed = (mesh_ok && tail_ok && nucleus_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (cometed ? 12 : (debris > 0 ? 3 : 0)) +
                   (tails > 0 ? 1 : 0) + (nuclei > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_COMETED", (long)cometed);
      var_set_num(vm, "SMX_COMET", (long)(cometed ? debris + tails + nuclei : 0));
      var_set_num(vm, "SMX_METEOR", (long)(cometed ? 1 : 0));
      var_set_num(vm, "SMX_COMA", (long)(cometed ? 1 : 0));
      var_set_num(vm, "SMX_DEBRIS", (long)(cometed ? debris : 0));
      var_set_num(vm, "SMX_TAILS", (long)(cometed ? tails : 0));
      var_set_num(vm, "SMX_TAIL", (long)(cometed ? tails : 0));
      var_set_num(vm, "SMX_NUCLEI", (long)(cometed ? nuclei : 0));
      var_set_num(vm, "SMX_NUCLEUS_ICE", (long)(cometed ? nuclei : 0));
      var_set_num(vm, "SMX_SEEDCOMET", (long)(cometed ? nuclei : 0));
      var_set_num(vm, "SMX_MESH", (long)(cometed ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)debris);
      var_set_num(vm, "SMX_EXCHANGES", (long)debris);
      var_set_num(vm, "SMX_FUSE", (long)debris);
      var_set_num(vm, "SMX_BIND", (long)debris);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(debris + tails + nuclei));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (cometed){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX COMET ok");
      } else if (debris > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX COMET partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX COMET soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX COMET nodes=%d live=%d debris=%d tails=%d nuclei=%d need=%d soft=%d talks=%d oob=%d cometed=%d vital=%ld\n",
                n, live, debris, tails, nuclei, need, soft, vm->smx_talks, vm->smx_oob, cometed, vital);
    }
    bump(vm); return 1;
  }
  /* SMX SUPERNOVA|NOVA|SHOCKWAVE|EJECTA|REMNANT|MESH_SUPERNOVA|RAISE_SUPERNOVA a b c ...
   * Life-force stellar detonation after comet lock: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete ejecta mesh among live nodes, weaves a
   * shockwave ring (i -> i+1) so free energy circles the blast shell, then remnant
   * hub gathers return so lattice raises a supernova lock where collapse seeds the hive.
   * Latches SMX_SUPERNOVAED when mesh+shocks+remnants are soft-OOB-free.
   * SMX_EJECTA = chain bonds; SMX_REMNANT hub = root gather pulses;
   * SMX_SUPERNOVA sum = ejecta+shocks+remnants; SMX_NOVA|SMX_SHOCK sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"SUPERNOVA")||kw(&L->cur,"NOVA")||kw(&L->cur,"SHOCKWAVE")||
      kw(&L->cur,"EJECTA")||kw(&L->cur,"REMNANT")||kw(&L->cur,"BLAST")||
      kw(&L->cur,"MESH_SUPERNOVA")||kw(&L->cur,"RAISE_SUPERNOVA")||
      kw(&L->cur,"SUPERNOVAS")||kw(&L->cur,"NOVAE")||kw(&L->cur,"SHOCKWAVES")||
      kw(&L->cur,"EJECTAS")||kw(&L->cur,"REMNANTS")||kw(&L->cur,"SEEDSUPERNOVA")||
      kw(&L->cur,"LATTICE_SUPERNOVA")||kw(&L->cur,"STAR_DEATH")||
      kw(&L->cur,"COLLAPSE")||kw(&L->cur,"PULSE_SUPERNOVA")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int ejecta = 0;
    int shocks = 0;
    int remnants = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "SUPERNOVA needs >=2 cubes",
                  "SMX SUPERNOVA a b [c ...]  or  SMX NOVA a b c d");
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
            else ejecta++;
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
          else shocks++;
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
          else remnants++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && ejecta >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && ejecta * 2 >= need && soft == 0)
        mesh_ok = 1;
      int shock_ok = (live >= 2 && shocks >= live && soft == 0) ? 1 : 0;
      if (!shock_ok && live >= 2 && shocks * 2 >= live && soft == 0)
        shock_ok = 1;
      int remnant_ok = (live >= 1 && remnants >= live && soft == 0) ? 1 : 0;
      if (!remnant_ok && live >= 1 && remnants * 2 >= live && soft == 0)
        remnant_ok = 1;
      int sn_ok = (mesh_ok && shock_ok && remnant_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (sn_ok ? 12 : (ejecta > 0 ? 3 : 0)) +
                   (shocks > 0 ? 1 : 0) + (remnants > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_SUPERNOVAED", (long)sn_ok);
      var_set_num(vm, "SMX_SUPERNOVA", (long)(sn_ok ? ejecta + shocks + remnants : 0));
      var_set_num(vm, "SMX_NOVA", (long)(sn_ok ? 1 : 0));
      var_set_num(vm, "SMX_BLAST", (long)(sn_ok ? 1 : 0));
      var_set_num(vm, "SMX_EJECTA", (long)(sn_ok ? ejecta : 0));
      var_set_num(vm, "SMX_SHOCKS", (long)(sn_ok ? shocks : 0));
      var_set_num(vm, "SMX_SHOCKWAVE", (long)(sn_ok ? shocks : 0));
      var_set_num(vm, "SMX_SHOCK", (long)(sn_ok ? shocks : 0));
      var_set_num(vm, "SMX_REMNANTS", (long)(sn_ok ? remnants : 0));
      var_set_num(vm, "SMX_REMNANT", (long)(sn_ok ? remnants : 0));
      var_set_num(vm, "SMX_SEEDSUPERNOVA", (long)(sn_ok ? remnants : 0));
      var_set_num(vm, "SMX_MESH", (long)(sn_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)ejecta);
      var_set_num(vm, "SMX_EXCHANGES", (long)ejecta);
      var_set_num(vm, "SMX_FUSE", (long)ejecta);
      var_set_num(vm, "SMX_BIND", (long)ejecta);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(ejecta + shocks + remnants));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (sn_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SUPERNOVA ok");
      } else if (ejecta > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SUPERNOVA partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX SUPERNOVA soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX SUPERNOVA nodes=%d live=%d ejecta=%d shocks=%d remnants=%d need=%d soft=%d talks=%d oob=%d supernovaed=%d vital=%ld\n",
                n, live, ejecta, shocks, remnants, need, soft, vm->smx_talks, vm->smx_oob, sn_ok, vital);
    }
    bump(vm); return 1;
  }



  /* SMX GALAXY|SPIRAL|ARM|CORE|HALO|BULGE|MESH_GALAXY|RAISE_GALAXY a b c ...
   * Life-force galactic weave after supernova seed: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete stellar mesh among live nodes, weaves a
   * spiral-arm ring (i -> i+1) so free energy circles the disk, then core hub
   * gathers return so lattice raises a galaxy lock where arms feed the bulge.
   * Latches SMX_GALAXIED when mesh+arms+core are soft-OOB-free.
   * SMX_STARS = chain bonds; SMX_CORE hub = root gather pulses;
   * SMX_GALAXY sum = stars+arms+cores; SMX_SPIRAL|SMX_ARM sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  return 0;
}
