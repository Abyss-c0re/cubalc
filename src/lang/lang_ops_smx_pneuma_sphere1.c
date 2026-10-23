/* CubalC lang — lang_ops_smx_pneuma_sphere1.c
 * SMX pneuma spheres LITH..MESO. Peel from pneuma_a. Cube is SoT.
 */
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
int cubalc_lang_ops_smx_try_pneuma_sphere1(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* SMX LITHOSPHERE|CRUST|TECTONIC|ROCKWEB|GEOSPHERE|PLATE|MESH_LITHOSPHERE|RAISE_LITHOSPHERE a b c ...
   * Life-force lithosphere after atmosphere: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete crust mesh among live nodes, weaves a
   * fault ring (i -> i+1) so free energy locks the plates, then bedrock hub
   * gathers return so lattice raises a lithosphere lock where life holds the hive.
   * Latches SMX_LITHOSPHERED when mesh+faults+strata are soft-OOB-free.
   * SMX_FAULTS = chain bonds; SMX_BEDROCK hub = root gather pulses;
   * SMX_LITHOSPHERE sum = bonds+faults+strata; SMX_CRUST|SMX_TECTONIC sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"LITHOSPHERE")||kw(&L->cur,"CRUST")||kw(&L->cur,"TECTONIC")||
      kw(&L->cur,"ROCKWEB")||kw(&L->cur,"GEOSPHERE")||kw(&L->cur,"PLATE")||
      kw(&L->cur,"MESH_LITHOSPHERE")||kw(&L->cur,"RAISE_LITHOSPHERE")||
      kw(&L->cur,"LITHOSPHERES")||kw(&L->cur,"CRUSTS")||kw(&L->cur,"TECTONICS")||
      kw(&L->cur,"FAULTS")||kw(&L->cur,"PLATES")||kw(&L->cur,"SEEDLITH")||
      kw(&L->cur,"LATTICE_LITHOSPHERE")||kw(&L->cur,"WORLD_ROCK")||
      kw(&L->cur,"FAULT_RING")||kw(&L->cur,"PULSE_LITHOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int faults = 0;
    int strata = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "LITHOSPHERE needs >=2 cubes",
                  "SMX LITHOSPHERE a b [c ...]  or  SMX CRUST a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - lithosphere needs clear channel */
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
    /* complete crust mesh among live */
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
    /* fault ring - free energy holds every edge i -> i+1 both ways */
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
          else faults++;
        }
      }
    }
    /* bedrock hub - seed axis return from every live leaf */
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
          else strata++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int fault_ok = (live >= 2 && faults >= live && soft == 0) ? 1 : 0;
      if (!fault_ok && live >= 2 && faults * 2 >= live && soft == 0)
        fault_ok = 1;
      int strata_ok = (live >= 1 && strata >= live && soft == 0) ? 1 : 0;
      if (!strata_ok && live >= 1 && strata * 2 >= live && soft == 0)
        strata_ok = 1;
      int lt_ok = (mesh_ok && fault_ok && strata_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (lt_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (faults > 0 ? 1 : 0) + (strata > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_LITHOSPHERED", (long)lt_ok);
      var_set_num(vm, "SMX_LITHOSPHERED_LATCH", (long)lt_ok);
      var_set_num(vm, "SMX_LITHOSPHERE", (long)(lt_ok ? bonds + faults + strata : 0));
      var_set_num(vm, "SMX_CRUST", (long)(lt_ok ? 1 : 0));
      var_set_num(vm, "SMX_TECTONIC", (long)(lt_ok ? 1 : 0));
      var_set_num(vm, "SMX_FAULTS", (long)(lt_ok ? bonds : 0));
      var_set_num(vm, "SMX_FAULT", (long)(lt_ok ? bonds : 0));
      var_set_num(vm, "SMX_PLATES", (long)(lt_ok ? faults : 0));
      var_set_num(vm, "SMX_PLATE", (long)(lt_ok ? faults : 0));
      var_set_num(vm, "SMX_FAULTRING", (long)(lt_ok ? faults : 0));
      var_set_num(vm, "SMX_STRATA", (long)(lt_ok ? strata : 0));
      var_set_num(vm, "SMX_BEDROCK", (long)(lt_ok ? strata : 0));
      var_set_num(vm, "SMX_SEEDLITH", (long)(lt_ok ? strata : 0));
      var_set_num(vm, "SMX_MESH", (long)(lt_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + faults + strata));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (lt_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX LITHOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX LITHOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX LITHOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX LITHOSPHERE nodes=%d live=%d bonds=%d faults=%d strata=%d need=%d soft=%d talks=%d oob=%d lithosphered=%d vital=%ld\n",
                n, live, bonds, faults, strata, need, soft, vm->smx_talks, vm->smx_oob, lt_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX MAGNETOSPHERE|MAGFIELD|MAGNETO|FIELDWEB|MAGNETIC|AURORAL|MESH_MAGNETOSPHERE|RAISE_MAGNETOSPHERE a b c ...
   * Life-force magnetosphere after lithosphere: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete field mesh among live nodes, weaves a
   * fieldline ring (i -> i+1) so free energy locks the poles, then pole hub
   * gathers return so lattice raises a magnetosphere lock where life holds the hive.
   * Latches SMX_MAGNETOSPHERED when mesh+fieldlines+poles are soft-OOB-free.
   * SMX_FIELDLINES = chain bonds; SMX_POLE hub = root gather pulses;
   * SMX_MAGNETOSPHERE sum = bonds+fieldlines+poles; SMX_MAGFIELD|SMX_MAGNETO sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"MAGNETOSPHERE")||kw(&L->cur,"MAGFIELD")||kw(&L->cur,"MAGNETO")||
      kw(&L->cur,"FIELDWEB")||kw(&L->cur,"MAGNETIC")||kw(&L->cur,"AURORAL")||
      kw(&L->cur,"MESH_MAGNETOSPHERE")||kw(&L->cur,"RAISE_MAGNETOSPHERE")||
      kw(&L->cur,"MAGNETOSPHERES")||kw(&L->cur,"MAGFIELDS")||kw(&L->cur,"MAGNETOS")||
      kw(&L->cur,"FIELDLINES")||kw(&L->cur,"FIELDS")||kw(&L->cur,"FIELD")||kw(&L->cur,"SEEDMAG")||
      kw(&L->cur,"LATTICE_MAGNETOSPHERE")||kw(&L->cur,"WORLD_FIELD")||
      kw(&L->cur,"FIELD_RING")||kw(&L->cur,"PULSE_MAGNETOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int fieldlines = 0;
    int poles = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "MAGNETOSPHERE needs >=2 cubes",
                  "SMX MAGNETOSPHERE a b [c ...]  or  SMX MAGFIELD a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - magnetosphere needs clear channel */
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
    /* complete field mesh among live */
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
    /* fieldline ring - free energy holds every edge i -> i+1 both ways */
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
          else fieldlines++;
        }
      }
    }
    /* pole hub - seed axis return from every live leaf */
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
          else poles++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int fl_ok = (live >= 2 && fieldlines >= live && soft == 0) ? 1 : 0;
      if (!fl_ok && live >= 2 && fieldlines * 2 >= live && soft == 0)
        fl_ok = 1;
      int pole_ok = (live >= 1 && poles >= live && soft == 0) ? 1 : 0;
      if (!pole_ok && live >= 1 && poles * 2 >= live && soft == 0)
        pole_ok = 1;
      int mg_ok = (mesh_ok && fl_ok && pole_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (mg_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (fieldlines > 0 ? 1 : 0) + (poles > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_MAGNETOSPHERED", (long)mg_ok);
      var_set_num(vm, "SMX_MAGNETOSPHERED_LATCH", (long)mg_ok);
      var_set_num(vm, "SMX_MAGNETOSPHERE", (long)(mg_ok ? bonds + fieldlines + poles : 0));
      var_set_num(vm, "SMX_MAGFIELD", (long)(mg_ok ? 1 : 0));
      var_set_num(vm, "SMX_MAGNETO", (long)(mg_ok ? 1 : 0));
      var_set_num(vm, "SMX_FIELDLINES", (long)(mg_ok ? bonds : 0));
      var_set_num(vm, "SMX_FIELDLINE", (long)(mg_ok ? bonds : 0));
      var_set_num(vm, "SMX_FIELDS", (long)(mg_ok ? fieldlines : 0));
      var_set_num(vm, "SMX_FIELD", (long)(mg_ok ? fieldlines : 0));
      var_set_num(vm, "SMX_FIELDRING", (long)(mg_ok ? fieldlines : 0));
      var_set_num(vm, "SMX_POLES", (long)(mg_ok ? poles : 0));
      var_set_num(vm, "SMX_POLE", (long)(mg_ok ? poles : 0));
      var_set_num(vm, "SMX_SEEDMAG", (long)(mg_ok ? poles : 0));
      var_set_num(vm, "SMX_MESH", (long)(mg_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + fieldlines + poles));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (mg_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MAGNETOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MAGNETOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX MAGNETOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX MAGNETOSPHERE nodes=%d live=%d bonds=%d fieldlines=%d poles=%d need=%d soft=%d talks=%d oob=%d magnetosphered=%d vital=%ld\n",
                n, live, bonds, fieldlines, poles, need, soft, vm->smx_talks, vm->smx_oob, mg_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX HOMEOSTASIS|BALANCE|SETPOINT|FEEDBACK|REGULATE|EQUILIBRIUM|MESH_HOMEOSTASIS|RAISE_HOMEOSTASIS a b c ...
   * Life-force mesh stability after biosphere: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete balance mesh among live nodes, weaves a
   * feedback ring (i -> i+1) so free energy self-regulates, then setpoint hub
   * gathers return so lattice locks homeostasis where life holds the hive.
   * Latches SMX_HOMEOSTATIC when mesh+feedback+setpoints are soft-OOB-free.
   * SMX_BALANCES = chain bonds; SMX_SETPOINT hub = root gather pulses;
   * SMX_HOMEOSTASIS sum = bonds+feedback+setpoints; SMX_BALANCE|SMX_REGULATE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"HOMEOSTASIS")||kw(&L->cur,"BALANCE")||kw(&L->cur,"SETPOINT")||
      kw(&L->cur,"FEEDBACK")||kw(&L->cur,"REGULATE")||kw(&L->cur,"EQUILIBRIUM")||
      kw(&L->cur,"MESH_HOMEOSTASIS")||kw(&L->cur,"RAISE_HOMEOSTASIS")||
      kw(&L->cur,"HOMEOSTASES")||kw(&L->cur,"BALANCES")||kw(&L->cur,"SETPOINTS")||
      kw(&L->cur,"FEEDBACKS")||kw(&L->cur,"REGULATES")||kw(&L->cur,"SEEDHOME")||
      kw(&L->cur,"LATTICE_HOMEOSTASIS")||kw(&L->cur,"WORLD_BALANCE")||
      kw(&L->cur,"FEEDBACK_RING")||kw(&L->cur,"PULSE_HOMEOSTASIS")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int feedback = 0;
    int setpoints = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "HOMEOSTASIS needs >=2 cubes",
                  "SMX HOMEOSTASIS a b [c ...]  or  SMX BALANCE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - homeostasis needs clear channel */
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
    /* complete balance mesh among live */
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
    /* feedback ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else feedback++;
        }
      }
    }
    /* setpoint hub - seed axis return from every live leaf */
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
          else setpoints++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int fb_ok = (live >= 2 && feedback >= live && soft == 0) ? 1 : 0;
      if (!fb_ok && live >= 2 && feedback * 2 >= live && soft == 0)
        fb_ok = 1;
      int sp_ok = (live >= 1 && setpoints >= live && soft == 0) ? 1 : 0;
      if (!sp_ok && live >= 1 && setpoints * 2 >= live && soft == 0)
        sp_ok = 1;
      int hs_ok = (mesh_ok && fb_ok && sp_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (hs_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (feedback > 0 ? 1 : 0) + (setpoints > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_HOMEOSTATIC", (long)hs_ok);
      var_set_num(vm, "SMX_HOMEOSTATIC_LATCH", (long)hs_ok);
      var_set_num(vm, "SMX_HOMEOSTASIS", (long)(hs_ok ? bonds + feedback + setpoints : 0));
      var_set_num(vm, "SMX_BALANCE", (long)(hs_ok ? 1 : 0));
      var_set_num(vm, "SMX_REGULATE", (long)(hs_ok ? 1 : 0));
      var_set_num(vm, "SMX_BALANCES", (long)(hs_ok ? bonds : 0));
      var_set_num(vm, "SMX_FEEDBACKS", (long)(hs_ok ? feedback : 0));
      var_set_num(vm, "SMX_FEEDBACK", (long)(hs_ok ? feedback : 0));
      var_set_num(vm, "SMX_FEEDBACKRING", (long)(hs_ok ? feedback : 0));
      var_set_num(vm, "SMX_SETPOINTS", (long)(hs_ok ? setpoints : 0));
      var_set_num(vm, "SMX_SETPOINT", (long)(hs_ok ? setpoints : 0));
      var_set_num(vm, "SMX_EQUILIBRIUM", (long)(hs_ok ? 1 : 0));
      var_set_num(vm, "SMX_SEEDHOME", (long)(hs_ok ? setpoints : 0));
      var_set_num(vm, "SMX_MESH", (long)(hs_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + feedback + setpoints));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (hs_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HOMEOSTASIS ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HOMEOSTASIS partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX HOMEOSTASIS soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX HOMEOSTASIS nodes=%d live=%d bonds=%d feedback=%d setpoints=%d need=%d soft=%d talks=%d oob=%d homeostatic=%d vital=%ld\n",
                n, live, bonds, feedback, setpoints, need, soft, vm->smx_talks, vm->smx_oob, hs_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX IONOSPHERE|IONO|LAYER|BEAM|IONIZE|PLASMA|MESH_IONOSPHERE|RAISE_IONOSPHERE a b c ...
   * Life-force mesh stability after homeostasis: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete iono mesh among live nodes, weaves a
   * beam ring (i -> i+1) so free energy self-regulates, then layer hub
   * gathers return so lattice locks ionosphere where life holds the hive.
   * Latches SMX_IONOSPHERED when mesh+beam+layers are soft-OOB-free.
   * SMX_IONOS = chain bonds; SMX_LAYER hub = root gather pulses;
   * SMX_IONOSPHERE sum = bonds+beam+layers; SMX_IONO|SMX_IONIZE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"IONOSPHERE")||kw(&L->cur,"IONO")||kw(&L->cur,"LAYER")||
      kw(&L->cur,"BEAM")||kw(&L->cur,"IONIZE")||kw(&L->cur,"PLASMA")||
      kw(&L->cur,"MESH_IONOSPHERE")||kw(&L->cur,"RAISE_IONOSPHERE")||
      kw(&L->cur,"IONOSPHERES")||kw(&L->cur,"IONOS")||kw(&L->cur,"LAYERS")||
      kw(&L->cur,"BEAMS")||kw(&L->cur,"IONIZES")||kw(&L->cur,"SEEDIONO")||
      kw(&L->cur,"LATTICE_IONOSPHERE")||kw(&L->cur,"WORLD_IONO")||
      kw(&L->cur,"BEAM_RING")||kw(&L->cur,"PULSE_IONOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int beam = 0;
    int layers = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "IONOSPHERE needs >=2 cubes",
                  "SMX IONOSPHERE a b [c ...]  or  SMX IONO a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - ionosphere needs clear channel */
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
    /* complete iono mesh among live */
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
    /* beam ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else beam++;
        }
      }
    }
    /* layer hub - seed axis return from every live leaf */
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
          else layers++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int bm_ok = (live >= 2 && beam >= live && soft == 0) ? 1 : 0;
      if (!bm_ok && live >= 2 && beam * 2 >= live && soft == 0)
        bm_ok = 1;
      int ly_ok = (live >= 1 && layers >= live && soft == 0) ? 1 : 0;
      if (!ly_ok && live >= 1 && layers * 2 >= live && soft == 0)
        ly_ok = 1;
      int io_ok = (mesh_ok && bm_ok && ly_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (io_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (beam > 0 ? 1 : 0) + (layers > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_IONOSPHERED", (long)io_ok);
      var_set_num(vm, "SMX_IONOSPHERED_LATCH", (long)io_ok);
      var_set_num(vm, "SMX_IONOSPHERE", (long)(io_ok ? bonds + beam + layers : 0));
      var_set_num(vm, "SMX_IONO", (long)(io_ok ? 1 : 0));
      var_set_num(vm, "SMX_IONIZE", (long)(io_ok ? 1 : 0));
      var_set_num(vm, "SMX_IONOS", (long)(io_ok ? bonds : 0));
      var_set_num(vm, "SMX_BEAMS", (long)(io_ok ? beam : 0));
      var_set_num(vm, "SMX_BEAM", (long)(io_ok ? beam : 0));
      var_set_num(vm, "SMX_BEAMRING", (long)(io_ok ? beam : 0));
      var_set_num(vm, "SMX_LAYERS", (long)(io_ok ? layers : 0));
      var_set_num(vm, "SMX_LAYER", (long)(io_ok ? layers : 0));
      var_set_num(vm, "SMX_PLASMA", (long)(io_ok ? 1 : 0));
      var_set_num(vm, "SMX_SEEDIONO", (long)(io_ok ? layers : 0));
      var_set_num(vm, "SMX_MESH", (long)(io_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + beam + layers));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (io_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX IONOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX IONOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX IONOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX IONOSPHERE nodes=%d live=%d bonds=%d beam=%d layers=%d need=%d soft=%d talks=%d oob=%d ionosphered=%d vital=%ld\n",
                n, live, bonds, beam, layers, need, soft, vm->smx_talks, vm->smx_oob, io_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX THERMOSPHERE|THERMO|THERMAL|HEAT|HEAT|WARM|MESH_THERMOSPHERE|RAISE_THERMOSPHERE a b c ...
   * Life-force mesh stability after homeostasis: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete iono mesh among live nodes, weaves a
   * heat ring (i -> i+1) so free energy self-regulates, then thermal hub
   * gathers return so lattice locks thermosphere where life holds the hive.
   * Latches SMX_THERMOSPHERED when mesh+heat+thermals are soft-OOB-free.
   * SMX_THERMOS = chain bonds; SMX_THERMAL hub = root gather pulses;
   * SMX_THERMOSPHERE sum = bonds+heat+thermals; SMX_THERMO|SMX_HEAT sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"THERMOSPHERE")||kw(&L->cur,"THERMO")||kw(&L->cur,"THERMAL")||
      kw(&L->cur,"HEAT")||kw(&L->cur,"HEAT")||kw(&L->cur,"WARM")||
      kw(&L->cur,"MESH_THERMOSPHERE")||kw(&L->cur,"RAISE_THERMOSPHERE")||
      kw(&L->cur,"THERMOSPHERES")||kw(&L->cur,"THERMOS")||kw(&L->cur,"THERMALS")||
      kw(&L->cur,"HEATS")||kw(&L->cur,"HEATS")||kw(&L->cur,"SEEDTHERMO")||
      kw(&L->cur,"LATTICE_THERMOSPHERE")||kw(&L->cur,"WORLD_THERMO")||
      kw(&L->cur,"HEAT_RING")||kw(&L->cur,"PULSE_THERMOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int heat = 0;
    int thermals = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "THERMOSPHERE needs >=2 cubes",
                  "SMX THERMOSPHERE a b [c ...]  or  SMX THERMO a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - thermosphere needs clear channel */
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
    /* complete iono mesh among live */
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
    /* heat ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else heat++;
        }
      }
    }
    /* thermal hub - seed axis return from every live leaf */
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
          else thermals++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int ht_ok = (live >= 2 && heat >= live && soft == 0) ? 1 : 0;
      if (!ht_ok && live >= 2 && heat * 2 >= live && soft == 0)
        ht_ok = 1;
      int th_ok = (live >= 1 && thermals >= live && soft == 0) ? 1 : 0;
      if (!th_ok && live >= 1 && thermals * 2 >= live && soft == 0)
        th_ok = 1;
      int tm_ok = (mesh_ok && ht_ok && th_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (tm_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (heat > 0 ? 1 : 0) + (thermals > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_THERMOSPHERED", (long)tm_ok);
      var_set_num(vm, "SMX_THERMOSPHERED_LATCH", (long)tm_ok);
      var_set_num(vm, "SMX_THERMOSPHERE", (long)(tm_ok ? bonds + heat + thermals : 0));
      var_set_num(vm, "SMX_THERMO", (long)(tm_ok ? 1 : 0));
      var_set_num(vm, "SMX_WARM", (long)(tm_ok ? 1 : 0));
      var_set_num(vm, "SMX_THERMOS", (long)(tm_ok ? bonds : 0));
      var_set_num(vm, "SMX_HEATS", (long)(tm_ok ? heat : 0));
      var_set_num(vm, "SMX_HEAT", (long)(tm_ok ? heat : 0));
      var_set_num(vm, "SMX_HEATRING", (long)(tm_ok ? heat : 0));
      var_set_num(vm, "SMX_THERMALS", (long)(tm_ok ? thermals : 0));
      var_set_num(vm, "SMX_THERMAL", (long)(tm_ok ? thermals : 0));
      var_set_num(vm, "SMX_SEEDTHERMO", (long)(tm_ok ? thermals : 0));
      var_set_num(vm, "SMX_MESH", (long)(tm_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + heat + thermals));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (tm_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX THERMOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX THERMOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX THERMOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX THERMOSPHERE nodes=%d live=%d bonds=%d heat=%d thermals=%d need=%d soft=%d talks=%d oob=%d thermosphered=%d vital=%ld\n",
                n, live, bonds, heat, thermals, need, soft, vm->smx_talks, vm->smx_oob, tm_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX EXOSPHERE|EXO|FRINGE|ESCAPE|ESCAPE|OUTER|MESH_EXOSPHERE|RAISE_EXOSPHERE a b c ...
   * Life-force mesh stability after homeostasis: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete exo mesh among live nodes, weaves a
   * escape ring (i -> i+1) so free energy self-regulates, then fringe hub
   * gathers return so lattice locks exosphere where life holds the hive.
   * Latches SMX_EXOSPHERED when mesh+escape+fringes are soft-OOB-free.
   * SMX_EXOS = chain bonds; SMX_FRINGE hub = root gather pulses;
   * SMX_EXOSPHERE sum = bonds+escape+fringes; SMX_EXO|SMX_ESCAPE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"EXOSPHERE")||kw(&L->cur,"EXO")||kw(&L->cur,"FRINGE")||
      kw(&L->cur,"ESCAPE")||kw(&L->cur,"ESCAPE")||kw(&L->cur,"OUTER")||
      kw(&L->cur,"MESH_EXOSPHERE")||kw(&L->cur,"RAISE_EXOSPHERE")||
      kw(&L->cur,"EXOSPHERES")||kw(&L->cur,"EXOS")||kw(&L->cur,"FRINGES")||
      kw(&L->cur,"ESCAPES")||kw(&L->cur,"ESCAPES")||kw(&L->cur,"SEEDEXO")||
      kw(&L->cur,"LATTICE_EXOSPHERE")||kw(&L->cur,"WORLD_EXO")||
      kw(&L->cur,"ESCAPE_RING")||kw(&L->cur,"PULSE_EXOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int escape = 0;
    int fringes = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "EXOSPHERE needs >=2 cubes",
                  "SMX EXOSPHERE a b [c ...]  or  SMX EXO a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - exosphere needs clear channel */
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
    /* complete exo mesh among live */
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
    /* escape ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else escape++;
        }
      }
    }
    /* fringe hub - seed axis return from every live leaf */
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
          else fringes++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int es_ok = (live >= 2 && escape >= live && soft == 0) ? 1 : 0;
      if (!es_ok && live >= 2 && escape * 2 >= live && soft == 0)
        es_ok = 1;
      int fr_ok = (live >= 1 && fringes >= live && soft == 0) ? 1 : 0;
      if (!fr_ok && live >= 1 && fringes * 2 >= live && soft == 0)
        fr_ok = 1;
      int ex_ok = (mesh_ok && es_ok && fr_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ex_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (escape > 0 ? 1 : 0) + (fringes > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_EXOSPHERED", (long)ex_ok);
      var_set_num(vm, "SMX_EXOSPHERED_LATCH", (long)ex_ok);
      var_set_num(vm, "SMX_EXOSPHERE", (long)(ex_ok ? bonds + escape + fringes : 0));
      var_set_num(vm, "SMX_EXO", (long)(ex_ok ? 1 : 0));
      var_set_num(vm, "SMX_OUTER", (long)(ex_ok ? 1 : 0));
      var_set_num(vm, "SMX_EXOS", (long)(ex_ok ? bonds : 0));
      var_set_num(vm, "SMX_ESCAPES", (long)(ex_ok ? escape : 0));
      var_set_num(vm, "SMX_ESCAPE", (long)(ex_ok ? escape : 0));
      var_set_num(vm, "SMX_ESCAPERING", (long)(ex_ok ? escape : 0));
      var_set_num(vm, "SMX_FRINGES", (long)(ex_ok ? fringes : 0));
      var_set_num(vm, "SMX_FRINGE", (long)(ex_ok ? fringes : 0));
      var_set_num(vm, "SMX_SEEDEXO", (long)(ex_ok ? fringes : 0));
      var_set_num(vm, "SMX_MESH", (long)(ex_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + escape + fringes));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (ex_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX EXOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX EXOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX EXOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX EXOSPHERE nodes=%d live=%d bonds=%d escape=%d fringes=%d need=%d soft=%d talks=%d oob=%d exosphered=%d vital=%ld\n",
                n, live, bonds, escape, fringes, need, soft, vm->smx_talks, vm->smx_oob, ex_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX HELIOSPHERE|HS|HELIOPAUSE|SOLARWIND|TERMINATION|HELIOSHEATH|MESH_HELIOSPHERE|RAISE_HELIOSPHERE a b c ...
   * Life-force mesh stability after homeostasis: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete heliosphere mesh among live nodes, weaves a
   * wind ring (i -> i+1) so free energy self-regulates, then pause hub
   * gathers return so lattice locks heliosphere where life holds the hive.
   * Latches SMX_HELIOSPHERED when mesh+wind+pauses are soft-OOB-free.
   * SMX_HELIOSHEATH = chain bonds; SMX_HELIOPAUSE hub = root gather pulses;
   * SMX_HELIOSPHERE sum = bonds+wind+pauses; SMX_HS|SMX_SOLARWIND sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"HELIOSPHERE")||kw(&L->cur,"HS")||kw(&L->cur,"HELIOPAUSE")||
      kw(&L->cur,"HELIO")||kw(&L->cur,"SOLARWIND")||kw(&L->cur,"TERMINATION")||
      kw(&L->cur,"HELIOSHEATH")||kw(&L->cur,"SOLAR")||kw(&L->cur,"BOW")||
      kw(&L->cur,"BOWS")||kw(&L->cur,"HELIOSHEATHS")||
      kw(&L->cur,"MESH_HELIOSPHERE")||kw(&L->cur,"RAISE_HELIOSPHERE")||
      kw(&L->cur,"HELIOSPHERES")||kw(&L->cur,"HELIOPAUSES")||
      kw(&L->cur,"SOLARWINDS")||kw(&L->cur,"SEEDHS")||kw(&L->cur,"SEEDHELIO")||
      kw(&L->cur,"LATTICE_HELIOSPHERE")||kw(&L->cur,"WORLD_HS")||kw(&L->cur,"WORLD_HELIO")||
      kw(&L->cur,"SOLARWIND_RING")||kw(&L->cur,"PULSE_HELIOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wind = 0;
    int pauses = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "HELIOSPHERE needs >=2 cubes",
                  "SMX HELIOSPHERE a b [c ...]  or  SMX HS a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - heliosphere needs clear channel */
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
    /* complete heliosphere mesh among live */
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
    /* wind ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else wind++;
        }
      }
    }
    /* pause hub - seed axis return from every live leaf */
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
          else pauses++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int es_ok = (live >= 2 && wind >= live && soft == 0) ? 1 : 0;
      if (!es_ok && live >= 2 && wind * 2 >= live && soft == 0)
        es_ok = 1;
      int fr_ok = (live >= 1 && pauses >= live && soft == 0) ? 1 : 0;
      if (!fr_ok && live >= 1 && pauses * 2 >= live && soft == 0)
        fr_ok = 1;
      int hs_ok = (mesh_ok && es_ok && fr_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (hs_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wind > 0 ? 1 : 0) + (pauses > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_HELIOSPHERED", (long)hs_ok);
      var_set_num(vm, "SMX_HELIOSPHERED_LATCH", (long)hs_ok);
      var_set_num(vm, "SMX_HELIOSPHERE", (long)(hs_ok ? bonds + wind + pauses : 0));
      var_set_num(vm, "SMX_HS", (long)(hs_ok ? 1 : 0));
      var_set_num(vm, "SMX_HELIO", (long)(hs_ok ? 1 : 0));
      var_set_num(vm, "SMX_SOLAR", (long)(hs_ok ? 1 : 0));
      var_set_num(vm, "SMX_TERMINATION", (long)(hs_ok ? 1 : 0));
      var_set_num(vm, "SMX_HELIOSHEATH", (long)(hs_ok ? bonds : 0));
      var_set_num(vm, "SMX_HELIOS", (long)(hs_ok ? bonds : 0));
      var_set_num(vm, "SMX_SOLARWINDS", (long)(hs_ok ? wind : 0));
      var_set_num(vm, "SMX_SOLARWIND", (long)(hs_ok ? wind : 0));
      var_set_num(vm, "SMX_SOLARWINDRING", (long)(hs_ok ? wind : 0));
      var_set_num(vm, "SMX_WINDS", (long)(hs_ok ? wind : 0));
      var_set_num(vm, "SMX_WIND", (long)(hs_ok ? wind : 0));
      var_set_num(vm, "SMX_HELIOPAUSES", (long)(hs_ok ? pauses : 0));
      var_set_num(vm, "SMX_HELIOPAUSE", (long)(hs_ok ? pauses : 0));
      var_set_num(vm, "SMX_BOWS", (long)(hs_ok ? pauses : 0));
      var_set_num(vm, "SMX_BOW", (long)(hs_ok ? pauses : 0));
      var_set_num(vm, "SMX_SEEDHS", (long)(hs_ok ? pauses : 0));
      var_set_num(vm, "SMX_SEEDHELIO", (long)(hs_ok ? pauses : 0));
      var_set_num(vm, "SMX_MESH", (long)(hs_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wind + pauses));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (hs_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HELIOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HELIOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX HELIOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX HELIOSPHERE nodes=%d live=%d bonds=%d wind=%d pauses=%d need=%d soft=%d talks=%d oob=%d heliosphered=%d vital=%ld\n",
                n, live, bonds, wind, pauses, need, soft, vm->smx_talks, vm->smx_oob, hs_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX MESOSPHERE|HS|GLOW|CHILL|COLD_TOP|MESOS|MESH_MESOSPHERE|RAISE_MESOSPHERE a b c ...
   * Life-force mesh stability after heliosphere: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete mesosphere mesh among live nodes, weaves a
   * chill ring (i -> i+1) so free energy self-regulates, then glow hub
   * gathers return so lattice locks mesosphere where life holds the hive.
   * Latches SMX_MESOSPHERED when mesh+chill+glows are soft-OOB-free.
   * SMX_MESOS = chain bonds; SMX_GLOW hub = root gather pulses;
   * SMX_MESOSPHERE sum = bonds+wind+pauses; SMX_LAYERED|SMX_CHILL sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"MESOSPHERE")||kw(&L->cur,"MESO")||kw(&L->cur,"GLOW")||
      kw(&L->cur,"MESO")||kw(&L->cur,"CHILL")||kw(&L->cur,"COLD_TOP")||
      kw(&L->cur,"MESOS")||kw(&L->cur,"AIRGLOW")||kw(&L->cur,"GLOW")||
      kw(&L->cur,"GLOWS")||kw(&L->cur,"MESOSS")||
      kw(&L->cur,"MESH_MESOSPHERE")||kw(&L->cur,"RAISE_MESOSPHERE")||
      kw(&L->cur,"MESOSPHERES")||kw(&L->cur,"GLOWS")||
      kw(&L->cur,"CHILLS")||kw(&L->cur,"SEEDGLOW")||kw(&L->cur,"SEEDMESO")||
      kw(&L->cur,"LATTICE_MESOSPHERE")||kw(&L->cur,"WORLD_MESO")||kw(&L->cur,"WORLD_MESO")||
      kw(&L->cur,"CHILL_RING")||kw(&L->cur,"PULSE_MESOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int chill = 0;
    int glows = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "MESOSPHERE needs >=2 cubes",
                  "SMX MESOSPHERE a b [c ...]  or  SMX MESO a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - mesosphere needs clear channel */
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
    /* complete mesosphere mesh among live */
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
    /* chill ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else chill++;
        }
      }
    }
    /* glow hub - seed axis return from every live leaf */
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
          else glows++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int ch_ok = (live >= 2 && chill >= live && soft == 0) ? 1 : 0;
      if (!ch_ok && live >= 2 && chill * 2 >= live && soft == 0)
        ch_ok = 1;
      int gl_ok = (live >= 1 && glows >= live && soft == 0) ? 1 : 0;
      if (!gl_ok && live >= 1 && glows * 2 >= live && soft == 0)
        gl_ok = 1;
      int ms_ok = (mesh_ok && ch_ok && gl_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ms_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (chill > 0 ? 1 : 0) + (glows > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_MESOSPHERED", (long)ms_ok);
      var_set_num(vm, "SMX_MESOSPHERED_LATCH", (long)ms_ok);
      var_set_num(vm, "SMX_MESOSPHERE", (long)(ms_ok ? bonds + chill + glows : 0));
      var_set_num(vm, "SMX_LAYERED", (long)(ms_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESO", (long)(ms_ok ? 1 : 0));
      var_set_num(vm, "SMX_AIRGLOW", (long)(ms_ok ? 1 : 0));
      var_set_num(vm, "SMX_COLD_TOP", (long)(ms_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESOS", (long)(ms_ok ? bonds : 0));
      var_set_num(vm, "SMX_MESOS", (long)(ms_ok ? bonds : 0));
      var_set_num(vm, "SMX_CHILLS", (long)(ms_ok ? chill : 0));
      var_set_num(vm, "SMX_CHILL", (long)(ms_ok ? chill : 0));
      var_set_num(vm, "SMX_CHILLRING", (long)(ms_ok ? chill : 0));
      var_set_num(vm, "SMX_CHILLS", (long)(ms_ok ? chill : 0));
      var_set_num(vm, "SMX_CHILL", (long)(ms_ok ? chill : 0));
      var_set_num(vm, "SMX_GLOWS", (long)(ms_ok ? glows : 0));
      var_set_num(vm, "SMX_GLOW", (long)(ms_ok ? glows : 0));
      var_set_num(vm, "SMX_GLOWS", (long)(ms_ok ? glows : 0));
      var_set_num(vm, "SMX_GLOW", (long)(ms_ok ? glows : 0));
      var_set_num(vm, "SMX_SEEDGLOW", (long)(ms_ok ? glows : 0));
      var_set_num(vm, "SMX_SEEDMESO", (long)(ms_ok ? glows : 0));
      var_set_num(vm, "SMX_MESH", (long)(ms_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + chill + glows));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (ms_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MESOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MESOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX MESOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX MESOSPHERE nodes=%d live=%d bonds=%d chill=%d glows=%d need=%d soft=%d talks=%d oob=%d mesosphered=%d vital=%ld\n",
                n, live, bonds, chill, glows, need, soft, vm->smx_talks, vm->smx_oob, ms_ok, vital);
    }
    bump(vm); return 1;
  }
  return 0;
}
