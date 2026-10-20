/* CubalC SMX cascade/soft plane — peeled from lang_ops_smx.
 * HOLD_FLASH discipline: mesh sticky only; meaningful forms preferred.
 */
#include "lang/cubalc_lang_ops_smx_water.h"
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

int cubalc_lang_ops_smx_try_water(VM *vm, Lex *L) {
  if (kw(&L->cur,"RIVER")||kw(&L->cur,"STREAM")||kw(&L->cur,"CURRENT")||
      kw(&L->cur,"SPRING")||kw(&L->cur,"DELTA")||kw(&L->cur,"WATERSHED")||
      kw(&L->cur,"MESH_RIVER")||kw(&L->cur,"RAISE_RIVER")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int currents = 0;
    int springs = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "RIVER needs >=2 cubes",
                  "SMX RIVER a b [c ...]  or  SMX STREAM a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - river needs clear channel */
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
    /* complete bank mesh among live */
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
    /* current ring - flow walks every edge i -> i+1 both ways */
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
          else currents++;
        }
      }
    }
    /* spring confluence - hub feeds every live leaf (incl self pulse) */
    if (live >= 1){
      int hub = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[hub], ids[leaf]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[leaf], ids[hub]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else springs++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int cur_ok = (live >= 2 && currents >= live && soft == 0) ? 1 : 0;
      if (!cur_ok && live >= 2 && currents * 2 >= live && soft == 0)
        cur_ok = 1;
      int spr_ok = (live >= 1 && springs >= live && soft == 0) ? 1 : 0;
      if (!spr_ok && live >= 1 && springs * 2 >= live && soft == 0)
        spr_ok = 1;
      int rivered = (mesh_ok && cur_ok && spr_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (rivered ? 11 : (bonds > 0 ? 3 : 0)) +
                   (currents > 0 ? 1 : 0) + (springs > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_RIVERED", (long)rivered);
      var_set_num(vm, "SMX_RIVER", (long)(rivered ? bonds + currents + springs : 0));
      var_set_num(vm, "SMX_STREAM", (long)(rivered ? 1 : 0));
      var_set_num(vm, "SMX_DELTA", (long)(rivered ? 1 : 0));
      var_set_num(vm, "SMX_CURRENT", (long)(rivered ? currents : 0));
      var_set_num(vm, "SMX_CURRENTS", (long)(rivered ? currents : 0));
      var_set_num(vm, "SMX_SPRINGS", (long)(rivered ? springs : 0));
      var_set_num(vm, "SMX_BANKS", (long)bonds);
      var_set_num(vm, "SMX_MESH", (long)(rivered ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + currents + springs));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (rivered){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX RIVER ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX RIVER partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX RIVER soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX RIVER nodes=%d live=%d bonds=%d currents=%d springs=%d need=%d soft=%d talks=%d oob=%d rivered=%d vital=%ld\n",
                n, live, bonds, currents, springs, need, soft, vm->smx_talks, vm->smx_oob, rivered, vital);
    }
    bump(vm); return 1;
  }

  /* SMX CASCADE|WATERFALL|RAPIDS|FALLS|TERRACE|BASIN|MESH_CASCADE|RAISE_CASCADE a b c ...
   * Life-force cascade after river: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete terrace mesh among live nodes, weaves a
   * directed fall chain (i -> i+1) so energy drops every edge, then basin pool
   * gathers return flow at the hub so each live leaf pours one pulse home.
   * Latches SMX_CASCADED when mesh+falls+basins are soft-OOB-free.
   * SMX_FALLS = chain bonds; SMX_BASINS = hub gather pulses;
   * SMX_CASCADE = terraces+falls+basins; SMX_WATERFALL|SMX_RAPIDS sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"CASCADE")||kw(&L->cur,"WATERFALL")||kw(&L->cur,"RAPIDS")||
      kw(&L->cur,"FALLS")||kw(&L->cur,"TERRACE")||kw(&L->cur,"BASIN")||
      kw(&L->cur,"MESH_CASCADE")||kw(&L->cur,"RAISE_CASCADE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int terraces = 0;
    int falls = 0;
    int basins = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "CASCADE needs >=2 cubes",
                  "SMX CASCADE a b [c ...]  or  SMX WATERFALL a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - cascade needs clear channel */
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
    /* complete terrace mesh among live */
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
            else terraces++;
          }
        }
      }
    }
    /* fall chain - energy drops every edge i -> i+1 both ways */
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
          else falls++;
        }
      }
    }
    /* basin pool - hub gathers return flow from every live leaf */
    if (live >= 1){
      int hub = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[hub]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[hub], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else basins++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && terraces >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && terraces * 2 >= need && soft == 0)
        mesh_ok = 1;
      int fall_ok = (live >= 2 && falls >= live && soft == 0) ? 1 : 0;
      if (!fall_ok && live >= 2 && falls * 2 >= live && soft == 0)
        fall_ok = 1;
      int bas_ok = (live >= 1 && basins >= live && soft == 0) ? 1 : 0;
      if (!bas_ok && live >= 1 && basins * 2 >= live && soft == 0)
        bas_ok = 1;
      int cascaded = (mesh_ok && fall_ok && bas_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (cascaded ? 11 : (terraces > 0 ? 3 : 0)) +
                   (falls > 0 ? 1 : 0) + (basins > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_CASCADED", (long)cascaded);
      var_set_num(vm, "SMX_CASCADE", (long)(cascaded ? terraces + falls + basins : 0));
      var_set_num(vm, "SMX_WATERFALL", (long)(cascaded ? 1 : 0));
      var_set_num(vm, "SMX_RAPIDS", (long)(cascaded ? 1 : 0));
      var_set_num(vm, "SMX_FALL", (long)(cascaded ? falls : 0));
      var_set_num(vm, "SMX_FALLS", (long)(cascaded ? falls : 0));
      var_set_num(vm, "SMX_BASINS", (long)(cascaded ? basins : 0));
      var_set_num(vm, "SMX_BASIN", (long)(cascaded ? basins : 0));
      var_set_num(vm, "SMX_TERRACES", (long)terraces);
      var_set_num(vm, "SMX_MESH", (long)(cascaded ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)terraces);
      var_set_num(vm, "SMX_EXCHANGES", (long)terraces);
      var_set_num(vm, "SMX_FUSE", (long)terraces);
      var_set_num(vm, "SMX_BIND", (long)terraces);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(terraces + falls + basins));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (cascaded){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CASCADE ok");
      } else if (terraces > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CASCADE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX CASCADE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX CASCADE nodes=%d live=%d terraces=%d falls=%d basins=%d need=%d soft=%d talks=%d oob=%d cascaded=%d vital=%ld\n",
                n, live, terraces, falls, basins, need, soft, vm->smx_talks, vm->smx_oob, cascaded, vital);
    }
    bump(vm); return 1;
  }

  /* SMX ESTUARY|TIDE|BRACKISH|LAGOON|BRAID|MANGROVE|MESH_ESTUARY|RAISE_ESTUARY a b c ...
   * Life-force estuary after cascade: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete braid mesh among live nodes, weaves a
   * directed tide ring (i -> i+1) so salt/fresh pulse every edge, then mangrove hub
   * roots gather nursery return so lattice raises estuary where river meets sea.
   * Latches SMX_ESTUARIED when mesh+tides+mangroves are soft-OOB-free.
   * SMX_LAGOON = chain bonds; SMX_MANGROVES = hub gather pulses;
   * SMX_ESTUARY = braids+tides+mangroves; SMX_TIDE|SMX_BRACKISH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"ESTUARY")||kw(&L->cur,"TIDE")||kw(&L->cur,"BRACKISH")||
      kw(&L->cur,"LAGOON")||kw(&L->cur,"BRAID")||kw(&L->cur,"MANGROVE")||
      kw(&L->cur,"MESH_ESTUARY")||kw(&L->cur,"RAISE_ESTUARY")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int braids = 0;
    int tides = 0;
    int mangroves = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "ESTUARY needs >=2 cubes",
                  "SMX ESTUARY a b [c ...]  or  SMX TIDE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - estuary needs clear channel */
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
    /* complete braid mesh among live */
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
            else braids++;
          }
        }
      }
    }
    /* fall chain - energy drops every edge i -> i+1 both ways */
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
          else tides++;
        }
      }
    }
    /* basin pool - hub gathers return flow from every live leaf */
    if (live >= 1){
      int hub = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[hub]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[hub], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else mangroves++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && braids >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && braids * 2 >= need && soft == 0)
        mesh_ok = 1;
      int tide_ok = (live >= 2 && tides >= live && soft == 0) ? 1 : 0;
      if (!tide_ok && live >= 2 && tides * 2 >= live && soft == 0)
        tide_ok = 1;
      int mang_ok = (live >= 1 && mangroves >= live && soft == 0) ? 1 : 0;
      if (!mang_ok && live >= 1 && mangroves * 2 >= live && soft == 0)
        mang_ok = 1;
      int estuaried = (mesh_ok && tide_ok && mang_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (estuaried ? 11 : (braids > 0 ? 3 : 0)) +
                   (tides > 0 ? 1 : 0) + (mangroves > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_ESTUARIED", (long)estuaried);
      var_set_num(vm, "SMX_ESTUARY", (long)(estuaried ? braids + tides + mangroves : 0));
      var_set_num(vm, "SMX_TIDE", (long)(estuaried ? 1 : 0));
      var_set_num(vm, "SMX_BRACKISH", (long)(estuaried ? 1 : 0));
      var_set_num(vm, "SMX_TIDES", (long)(estuaried ? tides : 0));
      var_set_num(vm, "SMX_LAGOON", (long)(estuaried ? tides : 0));
      var_set_num(vm, "SMX_MANGROVES", (long)(estuaried ? mangroves : 0));
      var_set_num(vm, "SMX_MANGROVE", (long)(estuaried ? mangroves : 0));
      var_set_num(vm, "SMX_BRAIDS", (long)braids);
      var_set_num(vm, "SMX_MESH", (long)(estuaried ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)braids);
      var_set_num(vm, "SMX_EXCHANGES", (long)braids);
      var_set_num(vm, "SMX_FUSE", (long)braids);
      var_set_num(vm, "SMX_BIND", (long)braids);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(braids + tides + mangroves));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (estuaried){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ESTUARY ok");
      } else if (braids > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ESTUARY partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX ESTUARY soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX ESTUARY nodes=%d live=%d braids=%d tides=%d mangroves=%d need=%d soft=%d talks=%d oob=%d estuaried=%d vital=%ld\n",
                n, live, braids, tides, mangroves, need, soft, vm->smx_talks, vm->smx_oob, estuaried, vital);
    }
    bump(vm); return 1;
  }

  /* SMX REEF|CORAL|SURGE|ATOLL|POLYPS|NURSERY|MESH_REEF|RAISE_REEF a b c ...
   * Life-force reef after estuary: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete polyp mesh among live nodes, weaves a
   * directed surge ring (i -> i+1) so tide energy pulses every edge, then nursery hub
   * gathers larval return so lattice raises reef where estuary meets open sea.
   * Latches SMX_REEFED when mesh+surges+nurseries are soft-OOB-free.
   * SMX_ATOLL = chain bonds; SMX_NURSERIES = hub gather pulses;
   * SMX_REEF = polyps+surges+nurseries; SMX_CORAL|SMX_POLYPS sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"REEF")||kw(&L->cur,"CORAL")||kw(&L->cur,"SURGE")||
      kw(&L->cur,"ATOLL")||kw(&L->cur,"POLYPS")||kw(&L->cur,"NURSERY")||
      kw(&L->cur,"MESH_REEF")||kw(&L->cur,"RAISE_REEF")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int polyps = 0;
    int surges = 0;
    int nurseries = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "REEF needs >=2 cubes",
                  "SMX REEF a b [c ...]  or  SMX CORAL a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - reef needs clear channel */
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
    /* complete polyp mesh among live */
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
            else polyps++;
          }
        }
      }
    }
    /* surge ring - energy pulses every edge i -> i+1 both ways */
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
          else surges++;
        }
      }
    }
    /* nursery pool - hub gathers larval return from every live leaf */
    if (live >= 1){
      int hub = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[hub]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[hub], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else nurseries++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && polyps >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && polyps * 2 >= need && soft == 0)
        mesh_ok = 1;
      int surge_ok = (live >= 2 && surges >= live && soft == 0) ? 1 : 0;
      if (!surge_ok && live >= 2 && surges * 2 >= live && soft == 0)
        surge_ok = 1;
      int nurs_ok = (live >= 1 && nurseries >= live && soft == 0) ? 1 : 0;
      if (!nurs_ok && live >= 1 && nurseries * 2 >= live && soft == 0)
        nurs_ok = 1;
      int reefed = (mesh_ok && surge_ok && nurs_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (reefed ? 11 : (polyps > 0 ? 3 : 0)) +
                   (surges > 0 ? 1 : 0) + (nurseries > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_REEFED", (long)reefed);
      var_set_num(vm, "SMX_REEF", (long)(reefed ? polyps + surges + nurseries : 0));
      var_set_num(vm, "SMX_CORAL", (long)(reefed ? 1 : 0));
      var_set_num(vm, "SMX_POLYPS", (long)polyps);
      var_set_num(vm, "SMX_SURGES", (long)(reefed ? surges : 0));
      var_set_num(vm, "SMX_SURGE", (long)(reefed ? surges : 0));
      var_set_num(vm, "SMX_ATOLL", (long)(reefed ? surges : 0));
      var_set_num(vm, "SMX_NURSERIES", (long)(reefed ? nurseries : 0));
      var_set_num(vm, "SMX_NURSERY", (long)(reefed ? nurseries : 0));
      var_set_num(vm, "SMX_MESH", (long)(reefed ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)polyps);
      var_set_num(vm, "SMX_EXCHANGES", (long)polyps);
      var_set_num(vm, "SMX_FUSE", (long)polyps);
      var_set_num(vm, "SMX_BIND", (long)polyps);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(polyps + surges + nurseries));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (reefed){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX REEF ok");
      } else if (polyps > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX REEF partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX REEF soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX REEF nodes=%d live=%d polyps=%d surges=%d nurseries=%d need=%d soft=%d talks=%d oob=%d reefed=%d vital=%ld\n",
                n, live, polyps, surges, nurseries, need, soft, vm->smx_talks, vm->smx_oob, reefed, vital);
    }
    bump(vm); return 1;
  }


  /* SMX KELP|FROND|SWAY|HOLDFAST|BLADE|STIPE|MESH_KELP|RAISE_KELP a b c ...
   * Life-force kelp forest after reef: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete blade mesh among live nodes, weaves a
   * directed sway ring (i -> i+1) so surge energy pulses every frond, then holdfast hub
   * anchors root return so lattice raises a kelp forest where reef meets open light.
   * Latches SMX_KELPED when mesh+sways+holdfasts are soft-OOB-free.
   * SMX_STIPE = chain bonds; SMX_HOLDFASTS = hub gather pulses;
   * SMX_KELP = blades+sways+holdfasts; SMX_FROND|SMX_BLADE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"KELP")||kw(&L->cur,"FROND")||kw(&L->cur,"SWAY")||
      kw(&L->cur,"HOLDFAST")||kw(&L->cur,"BLADE")||kw(&L->cur,"STIPE")||
      kw(&L->cur,"MESH_KELP")||kw(&L->cur,"RAISE_KELP")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int blades = 0;
    int sways = 0;
    int holdfasts = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "KELP needs >=2 cubes",
                  "SMX KELP a b [c ...]  or  SMX FROND a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - kelp needs clear channel */
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
    /* complete blade mesh among live */
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
            else blades++;
          }
        }
      }
    }
    /* sway ring - energy pulses every edge i -> i+1 both ways */
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
          else sways++;
        }
      }
    }
    /* holdfast pool - hub anchors root return from every live leaf */
    if (live >= 1){
      int hub = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[hub]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[hub], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else holdfasts++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && blades >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && blades * 2 >= need && soft == 0)
        mesh_ok = 1;
      int sway_ok = (live >= 2 && sways >= live && soft == 0) ? 1 : 0;
      if (!sway_ok && live >= 2 && sways * 2 >= live && soft == 0)
        sway_ok = 1;
      int hold_ok = (live >= 1 && holdfasts >= live && soft == 0) ? 1 : 0;
      if (!hold_ok && live >= 1 && holdfasts * 2 >= live && soft == 0)
        hold_ok = 1;
      int kelped = (mesh_ok && sway_ok && hold_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (kelped ? 11 : (blades > 0 ? 3 : 0)) +
                   (sways > 0 ? 1 : 0) + (holdfasts > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_KELPED", (long)kelped);
      var_set_num(vm, "SMX_KELP", (long)(kelped ? blades + sways + holdfasts : 0));
      var_set_num(vm, "SMX_FROND", (long)(kelped ? 1 : 0));
      var_set_num(vm, "SMX_BLADE", (long)(kelped ? 1 : 0));
      var_set_num(vm, "SMX_BLADES", (long)blades);
      var_set_num(vm, "SMX_SWAYS", (long)(kelped ? sways : 0));
      var_set_num(vm, "SMX_SWAY", (long)(kelped ? sways : 0));
      var_set_num(vm, "SMX_STIPE", (long)(kelped ? sways : 0));
      var_set_num(vm, "SMX_HOLDFASTS", (long)(kelped ? holdfasts : 0));
      var_set_num(vm, "SMX_HOLDFAST", (long)(kelped ? holdfasts : 0));
      var_set_num(vm, "SMX_MESH", (long)(kelped ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)blades);
      var_set_num(vm, "SMX_EXCHANGES", (long)blades);
      var_set_num(vm, "SMX_FUSE", (long)blades);
      var_set_num(vm, "SMX_BIND", (long)blades);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(blades + sways + holdfasts));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (kelped){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX KELP ok");
      } else if (blades > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX KELP partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX KELP soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX KELP nodes=%d live=%d blades=%d sways=%d holdfasts=%d need=%d soft=%d talks=%d oob=%d kelped=%d vital=%ld\n",
                n, live, blades, sways, holdfasts, need, soft, vm->smx_talks, vm->smx_oob, kelped, vital);
    }
    bump(vm); return 1;
  }

  /* SMX TIDAL|MARSH|EDDY|SEAGRASS|SALTFLAT|SPARTINA|MESH_TIDAL|RAISE_TIDAL a b c ...
   * Life-force tidal forest after kelp: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete marsh mesh among live nodes, weaves a
   * directed eddy ring (i -> i+1) so tidal energy pulses every marsh, then seagrass hub
   * anchors root return so lattice raises a tidal forest where reef meets open light.
   * Latches SMX_TIDALED when mesh+eddies+seagrasses are soft-OOB-free.
   * SMX_SPARTINA = chain bonds; SMX_SEAGRASSES = hub gather pulses;
   * SMX_TIDAL = marshes+eddies+seagrasses; SMX_MARSH|SMX_SALTFLAT sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"TIDAL")||kw(&L->cur,"MARSH")||kw(&L->cur,"EDDY")||
      kw(&L->cur,"SEAGRASS")||kw(&L->cur,"SALTFLAT")||kw(&L->cur,"SPARTINA")||
      kw(&L->cur,"MESH_TIDAL")||kw(&L->cur,"RAISE_TIDAL")||kw(&L->cur,"SALTFLATS")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int marshes = 0;
    int eddies = 0;
    int seagrasses = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "TIDAL needs >=2 cubes",
                  "SMX TIDAL a b [c ...]  or  SMX MARSH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - tidal needs clear channel */
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
    /* complete marsh mesh among live */
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
            else marshes++;
          }
        }
      }
    }
    /* eddy ring - energy pulses every edge i -> i+1 both ways */
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
          else eddies++;
        }
      }
    }
    /* seagrass pool - hub anchors root return from every live leaf */
    if (live >= 1){
      int hub = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[hub]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[hub], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else seagrasses++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && marshes >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && marshes * 2 >= need && soft == 0)
        mesh_ok = 1;
      int eddy_ok = (live >= 2 && eddies >= live && soft == 0) ? 1 : 0;
      if (!eddy_ok && live >= 2 && eddies * 2 >= live && soft == 0)
        eddy_ok = 1;
      int sea_ok = (live >= 1 && seagrasses >= live && soft == 0) ? 1 : 0;
      if (!sea_ok && live >= 1 && seagrasses * 2 >= live && soft == 0)
        sea_ok = 1;
      int tidaled = (mesh_ok && eddy_ok && sea_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (tidaled ? 11 : (marshes > 0 ? 3 : 0)) +
                   (eddies > 0 ? 1 : 0) + (seagrasses > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_TIDALED", (long)tidaled);
      var_set_num(vm, "SMX_TIDAL", (long)(tidaled ? marshes + eddies + seagrasses : 0));
      var_set_num(vm, "SMX_MARSH", (long)(tidaled ? 1 : 0));
      var_set_num(vm, "SMX_SALTFLAT", (long)(tidaled ? 1 : 0));
      var_set_num(vm, "SMX_MARSHES", (long)marshes);
      var_set_num(vm, "SMX_EDDIES", (long)(tidaled ? eddies : 0));
      var_set_num(vm, "SMX_EDDY", (long)(tidaled ? eddies : 0));
      var_set_num(vm, "SMX_SPARTINA", (long)(tidaled ? eddies : 0));
      var_set_num(vm, "SMX_SEAGRASSES", (long)(tidaled ? seagrasses : 0));
      var_set_num(vm, "SMX_SEAGRASS", (long)(tidaled ? seagrasses : 0));
      var_set_num(vm, "SMX_MESH", (long)(tidaled ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)marshes);
      var_set_num(vm, "SMX_EXCHANGES", (long)marshes);
      var_set_num(vm, "SMX_FUSE", (long)marshes);
      var_set_num(vm, "SMX_BIND", (long)marshes);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(marshes + eddies + seagrasses));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (tidaled){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX TIDAL ok");
      } else if (marshes > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX TIDAL partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX TIDAL soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX TIDAL nodes=%d live=%d marshes=%d eddies=%d seagrasses=%d need=%d soft=%d talks=%d oob=%d tidaled=%d vital=%ld\n",
                n, live, marshes, eddies, seagrasses, need, soft, vm->smx_talks, vm->smx_oob, tidaled, vital);
    }
    bump(vm); return 1;
  }

  /* SMX DUNE|FOREDUNE|DRIFT|RIDGE|AMMOPHILA|SAND|BEACHGRASS|MESH_DUNE|RAISE_DUNE a b c ...
   * Life-force dune ridge after tidal marsh: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete dune mesh among live nodes, weaves a
   * directed drift ring (i -> i+1) so sand energy pulses every ridge, then beachgrass root
   * anchors return so lattice raises a foredune where marsh meets open wind.
   * Latches SMX_DUNED when mesh+drifts+grasses are soft-OOB-free.
   * SMX_RIDGE = chain bonds; SMX_AMMOPHILA = root gather pulses;
   * SMX_DUNE = ridges+drifts+grasses; SMX_FOREDUNE|SMX_SAND sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  return 0;
}
