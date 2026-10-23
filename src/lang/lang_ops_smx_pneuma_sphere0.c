/* CubalC lang — lang_ops_smx_pneuma_sphere0.c
 * SMX pneuma soft spheres: PNEUMA · AEGIS · BIOSPHERE · HYDROSPHERE · ATMOSPHERE
 * Peel from pneuma_a (B_pneuma_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_smx_pneuma_sphere0.h"
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

int cubalc_lang_ops_smx_try_pneuma_sphere0(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  if (kw(&L->cur,"PNEUMA")||kw(&L->cur,"PRANA")||kw(&L->cur,"BREATHWORLD")||
      kw(&L->cur,"SOULFIRE")||kw(&L->cur,"SPIRITUS")||kw(&L->cur,"WORLDSOUL")||
      kw(&L->cur,"MESH_PNEUMA")||kw(&L->cur,"RAISE_PNEUMA")||
      kw(&L->cur,"PNEUMAS")||kw(&L->cur,"PRANAE")||kw(&L->cur,"SPIRITI")||
      kw(&L->cur,"BREATHS")||kw(&L->cur,"BREATHLINK")||kw(&L->cur,"SEEDPNEUMA")||kw(&L->cur,"SOULFIRES")||
      kw(&L->cur,"LATTICE_PNEUMA")||kw(&L->cur,"LIFE_BREATH")||
      kw(&L->cur,"PRANA_RING")||kw(&L->cur,"PULSE_PNEUMA")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int pranae = 0;
    int spiriti = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "PNEUMA needs >=2 cubes",
                  "SMX PNEUMA a b [c ...]  or  SMX PRANA a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - pneuma needs clear channel */
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
    /* complete breath mesh among live */
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
    /* prana ring - free energy holds every edge i -> i+1 both ways */
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
          else pranae++;
        }
      }
    }
    /* spirit hub - seed axis return from every live leaf */
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
          else spiriti++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int prana_ok = (live >= 2 && pranae >= live && soft == 0) ? 1 : 0;
      if (!prana_ok && live >= 2 && pranae * 2 >= live && soft == 0)
        prana_ok = 1;
      int spirit_ok = (live >= 1 && spiriti >= live && soft == 0) ? 1 : 0;
      if (!spirit_ok && live >= 1 && spiriti * 2 >= live && soft == 0)
        spirit_ok = 1;
      int pn_ok = (mesh_ok && prana_ok && spirit_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (pn_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (pranae > 0 ? 1 : 0) + (spiriti > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_PNEUMATED", (long)pn_ok);
      var_set_num(vm, "SMX_PNEUMATED_LATCH", (long)pn_ok);
      var_set_num(vm, "SMX_PNEUMA", (long)(pn_ok ? bonds + pranae + spiriti : 0));
      var_set_num(vm, "SMX_PRANA", (long)(pn_ok ? 1 : 0));
      var_set_num(vm, "SMX_WORLDSOUL", (long)(pn_ok ? 1 : 0));
      var_set_num(vm, "SMX_SOULFIRE", (long)(pn_ok ? 1 : 0));
      var_set_num(vm, "SMX_BREATHS", (long)(pn_ok ? bonds : 0));
      var_set_num(vm, "SMX_BREATHLINK", (long)(pn_ok ? bonds : 0));
      var_set_num(vm, "SMX_PRANAE", (long)(pn_ok ? pranae : 0));
      var_set_num(vm, "SMX_PRANARING", (long)(pn_ok ? pranae : 0));
      var_set_num(vm, "SMX_SPIRITI", (long)(pn_ok ? spiriti : 0));
      var_set_num(vm, "SMX_SPIRIT", (long)(pn_ok ? spiriti : 0));
      var_set_num(vm, "SMX_SEEDPNEUMA", (long)(pn_ok ? spiriti : 0));
      var_set_num(vm, "SMX_MESH", (long)(pn_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + pranae + spiriti));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (pn_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX PNEUMA ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX PNEUMA partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX PNEUMA soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX PNEUMA nodes=%d live=%d bonds=%d pranae=%d spiriti=%d need=%d soft=%d talks=%d oob=%d pneumated=%d vital=%ld\n",
                n, live, bonds, pranae, spiriti, need, soft, vm->smx_talks, vm->smx_oob, pn_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX AEGIS|SHIELD|WARD|BULWARK|PAVIS|MESH_AEGIS|RAISE_AEGIS a b c ...
   * Mesh stability after pneuma: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete aegis mesh among live nodes, weaves a
   * shield ring (i -> i+1) so free energy holds the ward, then hub gathers
   * return so lattice raises an aegis lock where the shield holds the hive.
   * Latches SMX_AEGISED when mesh+shields+wards are soft-OOB-free.
   * SMX_SHIELDS = chain bonds; SMX_WARD hub = root gather pulses;
   * SMX_AEGIS sum = bonds+shields+wards; SMX_SHIELD|SMX_BULWARK sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"AEGIS")||kw(&L->cur,"SHIELD")||kw(&L->cur,"WARD")||
      kw(&L->cur,"BULWARK")||kw(&L->cur,"PAVIS")||kw(&L->cur,"RAMPART")||
      kw(&L->cur,"MESH_AEGIS")||kw(&L->cur,"RAISE_AEGIS")||
      kw(&L->cur,"AEGISES")||kw(&L->cur,"SHIELDS")||kw(&L->cur,"WARDS")||
      kw(&L->cur,"SHIELDRING")||kw(&L->cur,"WARDHUB")||kw(&L->cur,"SEEDAEGIS")||
      kw(&L->cur,"LATTICE_AEGIS")||kw(&L->cur,"LIFE_AEGIS")||
      kw(&L->cur,"SHIELD_RING")||kw(&L->cur,"PULSE_AEGIS")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int shields = 0;
    int wards = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "AEGIS needs >=2 cubes",
                  "SMX AEGIS a b [c ...]  or  SMX SHIELD a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - aegis needs clear channel */
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
    /* complete aegis mesh among live */
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
    /* shield ring - free energy holds every edge i -> i+1 both ways */
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
          else shields++;
        }
      }
    }
    /* ward hub - seed axis return from every live leaf */
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
          else wards++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int shield_ok = (live >= 2 && shields >= live && soft == 0) ? 1 : 0;
      if (!shield_ok && live >= 2 && shields * 2 >= live && soft == 0)
        shield_ok = 1;
      int ward_ok = (live >= 1 && wards >= live && soft == 0) ? 1 : 0;
      if (!ward_ok && live >= 1 && wards * 2 >= live && soft == 0)
        ward_ok = 1;
      int ag_ok = (mesh_ok && shield_ok && ward_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ag_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (shields > 0 ? 1 : 0) + (wards > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_AEGISED", (long)ag_ok);
      var_set_num(vm, "SMX_AEGISED_LATCH", (long)ag_ok);
      var_set_num(vm, "SMX_AEGIS", (long)(ag_ok ? bonds + shields + wards : 0));
      var_set_num(vm, "SMX_SHIELD", (long)(ag_ok ? 1 : 0));
      var_set_num(vm, "SMX_BULWARK", (long)(ag_ok ? 1 : 0));
      var_set_num(vm, "SMX_WARD", (long)(ag_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHIELDS", (long)(ag_ok ? bonds : 0));
      var_set_num(vm, "SMX_SHIELDRING", (long)(ag_ok ? shields : 0));
      var_set_num(vm, "SMX_SHIELDE", (long)(ag_ok ? shields : 0));
      var_set_num(vm, "SMX_SHIELDAE", (long)(ag_ok ? shields : 0));
      var_set_num(vm, "SMX_WARDS", (long)(ag_ok ? wards : 0));
      var_set_num(vm, "SMX_WARDHUB", (long)(ag_ok ? wards : 0));
      var_set_num(vm, "SMX_SEEDAEGIS", (long)(ag_ok ? wards : 0));
      var_set_num(vm, "SMX_MESH", (long)(ag_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + shields + wards));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (ag_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AEGIS ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AEGIS partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX AEGIS soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX AEGIS nodes=%d live=%d bonds=%d shields=%d wards=%d need=%d soft=%d talks=%d oob=%d aegised=%d vital=%ld\n",
                n, live, bonds, shields, wards, need, soft, vm->smx_talks, vm->smx_oob, ag_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX BIOSPHERE|GAIA|BIOME|ECO|LIFEWEB|HABITAT|MESH_BIOSPHERE|RAISE_BIOSPHERE a b c ...
   * Life-force biome after noosphere: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete biome mesh among live nodes, weaves a
   * cycle ring (i -> i+1) so free energy fills the biosphere, then gaia hub
   * gathers return so lattice raises a biosphere lock where life holds the hive.
   * Latches SMX_BIOSPHERED when mesh+cycles+gaiae are soft-OOB-free.
   * SMX_BIOMES = chain bonds; SMX_HABITAT hub = root gather pulses;
   * SMX_BIOSPHERE sum = bonds+cycles+gaiae; SMX_GAIA|SMX_ECO sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"BIOSPHERE")||kw(&L->cur,"GAIA")||kw(&L->cur,"BIOME")||
      kw(&L->cur,"ECO")||kw(&L->cur,"LIFEWEB")||kw(&L->cur,"HABITAT")||
      kw(&L->cur,"MESH_BIOSPHERE")||kw(&L->cur,"RAISE_BIOSPHERE")||
      kw(&L->cur,"BIOSPHERES")||kw(&L->cur,"BIOMES")||kw(&L->cur,"ECOS")||
      kw(&L->cur,"GAIAS")||kw(&L->cur,"HABITATS")||kw(&L->cur,"SEEDBIO")||
      kw(&L->cur,"LATTICE_BIOSPHERE")||kw(&L->cur,"WORLD_LIFE")||
      kw(&L->cur,"CYCLE_RING")||kw(&L->cur,"PULSE_BIOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int cycles = 0;
    int gaiae = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "BIOSPHERE needs >=2 cubes",
                  "SMX BIOSPHERE a b [c ...]  or  SMX LOGOS a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - biosphere needs clear channel */
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
    /* complete biome mesh among live */
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
    /* cycle ring - free energy holds every edge i -> i+1 both ways */
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
          else cycles++;
        }
      }
    }
    /* gaia hub - seed axis return from every live leaf */
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
          else gaiae++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int cycle_ok = (live >= 2 && cycles >= live && soft == 0) ? 1 : 0;
      if (!cycle_ok && live >= 2 && cycles * 2 >= live && soft == 0)
        cycle_ok = 1;
      int gaia_ok = (live >= 1 && gaiae >= live && soft == 0) ? 1 : 0;
      if (!gaia_ok && live >= 1 && gaiae * 2 >= live && soft == 0)
        gaia_ok = 1;
      int bs_ok = (mesh_ok && cycle_ok && gaia_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (bs_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (cycles > 0 ? 1 : 0) + (gaiae > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_BIOSPHERED", (long)bs_ok);
      var_set_num(vm, "SMX_BIOSPHERED_LATCH", (long)bs_ok);
      var_set_num(vm, "SMX_BIOSPHERE", (long)(bs_ok ? bonds + cycles + gaiae : 0));
      var_set_num(vm, "SMX_GAIA", (long)(bs_ok ? 1 : 0));
      var_set_num(vm, "SMX_ECO", (long)(bs_ok ? 1 : 0));
      var_set_num(vm, "SMX_BIOMES", (long)(bs_ok ? bonds : 0));
      var_set_num(vm, "SMX_BIOME", (long)(bs_ok ? bonds : 0));
      var_set_num(vm, "SMX_CYCLES", (long)(bs_ok ? cycles : 0));
      var_set_num(vm, "SMX_CYCLE", (long)(bs_ok ? cycles : 0));
      var_set_num(vm, "SMX_CYCLERING", (long)(bs_ok ? cycles : 0));
      var_set_num(vm, "SMX_GAIAE", (long)(bs_ok ? gaiae : 0));
      var_set_num(vm, "SMX_HABITAT", (long)(bs_ok ? gaiae : 0));
      var_set_num(vm, "SMX_HABITAT", (long)(bs_ok ? gaiae : 0));
      var_set_num(vm, "SMX_SEEDBIO", (long)(bs_ok ? gaiae : 0));
      var_set_num(vm, "SMX_MESH", (long)(bs_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + cycles + gaiae));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (bs_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX BIOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX BIOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX BIOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX BIOSPHERE nodes=%d live=%d bonds=%d cycles=%d gaiae=%d need=%d soft=%d talks=%d oob=%d biosphered=%d vital=%ld\n",
                n, live, bonds, cycles, gaiae, need, soft, vm->smx_talks, vm->smx_oob, bs_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX HYDROSPHERE|OCEAN|MARINE|WATERWEB|AQUASPHERE|HYDRO|MESH_HYDROSPHERE|RAISE_HYDROSPHERE a b c ...
   * Life-force hydrosphere after biosphere: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete hydros mesh among live nodes, weaves a
   * current ring (i -> i+1) so free energy fills the hydrosphere, then ocean hub
   * gathers return so lattice raises a hydrosphere lock where life holds the hive.
   * Latches SMX_HYDROSPHERED when mesh+eddies+tides are soft-OOB-free.
   * SMX_CURRENTS = chain bonds; SMX_BASIN hub = root gather pulses;
   * SMX_HYDROSPHERE sum = bonds+eddies+tides; SMX_OCEAN|SMX_MARINE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"HYDROSPHERE")||kw(&L->cur,"OCEAN")||kw(&L->cur,"MARINE")||
      kw(&L->cur,"WATERWEB")||kw(&L->cur,"AQUASPHERE")||kw(&L->cur,"HYDRO")||
      kw(&L->cur,"MESH_HYDROSPHERE")||kw(&L->cur,"RAISE_HYDROSPHERE")||
      kw(&L->cur,"HYDROSPHERES")||kw(&L->cur,"OCEANS")||kw(&L->cur,"MARINES")||
      kw(&L->cur,"CURRENTS")||kw(&L->cur,"BASINS")||kw(&L->cur,"SEEDHYDRO")||
      kw(&L->cur,"LATTICE_HYDROSPHERE")||kw(&L->cur,"WORLD_WATER")||
      kw(&L->cur,"CURRENT_RING")||kw(&L->cur,"PULSE_HYDROSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int eddies = 0;
    int tides = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "HYDROSPHERE needs >=2 cubes",
                  "SMX HYDROSPHERE a b [c ...]  or  SMX OCEAN a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - hydrosphere needs clear channel */
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
    /* complete hydros mesh among live */
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
    /* current ring - free energy holds every edge i -> i+1 both ways */
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
    /* ocean hub - seed axis return from every live leaf */
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
          else tides++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int cycle_ok = (live >= 2 && eddies >= live && soft == 0) ? 1 : 0;
      if (!cycle_ok && live >= 2 && eddies * 2 >= live && soft == 0)
        cycle_ok = 1;
      int gaia_ok = (live >= 1 && tides >= live && soft == 0) ? 1 : 0;
      if (!gaia_ok && live >= 1 && tides * 2 >= live && soft == 0)
        gaia_ok = 1;
      int hs_ok = (mesh_ok && cycle_ok && gaia_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (hs_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (eddies > 0 ? 1 : 0) + (tides > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_HYDROSPHERED", (long)hs_ok);
      var_set_num(vm, "SMX_HYDROSPHERED_LATCH", (long)hs_ok);
      var_set_num(vm, "SMX_HYDROSPHERE", (long)(hs_ok ? bonds + eddies + tides : 0));
      var_set_num(vm, "SMX_OCEAN", (long)(hs_ok ? 1 : 0));
      var_set_num(vm, "SMX_MARINE", (long)(hs_ok ? 1 : 0));
      var_set_num(vm, "SMX_CURRENTS", (long)(hs_ok ? bonds : 0));
      var_set_num(vm, "SMX_CURRENT", (long)(hs_ok ? bonds : 0));
      var_set_num(vm, "SMX_EDDIES", (long)(hs_ok ? eddies : 0));
      var_set_num(vm, "SMX_EDDY", (long)(hs_ok ? eddies : 0));
      var_set_num(vm, "SMX_EDDYRING", (long)(hs_ok ? eddies : 0));
      var_set_num(vm, "SMX_TIDES", (long)(hs_ok ? tides : 0));
      var_set_num(vm, "SMX_BASIN", (long)(hs_ok ? tides : 0));
      var_set_num(vm, "SMX_BASIN", (long)(hs_ok ? tides : 0));
      var_set_num(vm, "SMX_SEEDHYDRO", (long)(hs_ok ? tides : 0));
      var_set_num(vm, "SMX_MESH", (long)(hs_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + eddies + tides));
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
        var_set_str(vm, "LAST", "SMX HYDROSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HYDROSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX HYDROSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX HYDROSPHERE nodes=%d live=%d bonds=%d eddies=%d tides=%d need=%d soft=%d talks=%d oob=%d hydrosphered=%d vital=%ld\n",
                n, live, bonds, eddies, tides, need, soft, vm->smx_talks, vm->smx_oob, hs_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX ATMOSPHERE|SKY|AERO|AIRWEB|AEROSPHERE|WIND|MESH_ATMOSPHERE|RAISE_ATMOSPHERE a b c ...
   * Life-force atmosphere after hydrosphere: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete sky mesh among live nodes, weaves a
   * wind ring (i -> i+1) so free energy fills the atmosphere, then sky hub
   * gathers return so lattice raises an atmosphere lock where life holds the hive.
   * Latches SMX_ATMOSPHERED when mesh+winds+skies are soft-OOB-free.
   * SMX_WINDS = chain bonds; SMX_SKY hub = root gather pulses;
   * SMX_ATMOSPHERE sum = bonds+winds+skies; SMX_SKY|SMX_AERO sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"ATMOSPHERE")||kw(&L->cur,"SKY")||kw(&L->cur,"AERO")||
      kw(&L->cur,"AIRWEB")||kw(&L->cur,"AEROSPHERE")||kw(&L->cur,"WIND")||
      kw(&L->cur,"MESH_ATMOSPHERE")||kw(&L->cur,"RAISE_ATMOSPHERE")||
      kw(&L->cur,"ATMOSPHERES")||kw(&L->cur,"SKIES")||kw(&L->cur,"AEROS")||
      kw(&L->cur,"WINDS")||kw(&L->cur,"ZEPHYRS")||kw(&L->cur,"SEEDATMO")||
      kw(&L->cur,"LATTICE_ATMOSPHERE")||kw(&L->cur,"WORLD_AIR")||
      kw(&L->cur,"WIND_RING")||kw(&L->cur,"PULSE_ATMOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int winds = 0;
    int skies = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "ATMOSPHERE needs >=2 cubes",
                  "SMX ATMOSPHERE a b [c ...]  or  SMX SKY a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - atmosphere needs clear channel */
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
    /* complete sky mesh among live */
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
    /* wind ring - free energy holds every edge i -> i+1 both ways */
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
          else winds++;
        }
      }
    }
    /* sky hub - seed axis return from every live leaf */
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
          else skies++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int wind_ok = (live >= 2 && winds >= live && soft == 0) ? 1 : 0;
      if (!wind_ok && live >= 2 && winds * 2 >= live && soft == 0)
        wind_ok = 1;
      int sky_ok = (live >= 1 && skies >= live && soft == 0) ? 1 : 0;
      if (!sky_ok && live >= 1 && skies * 2 >= live && soft == 0)
        sky_ok = 1;
      int at_ok = (mesh_ok && wind_ok && sky_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (at_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (winds > 0 ? 1 : 0) + (skies > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_ATMOSPHERED", (long)at_ok);
      var_set_num(vm, "SMX_ATMOSPHERED_LATCH", (long)at_ok);
      var_set_num(vm, "SMX_ATMOSPHERE", (long)(at_ok ? bonds + winds + skies : 0));
      var_set_num(vm, "SMX_SKY", (long)(at_ok ? 1 : 0));
      var_set_num(vm, "SMX_AERO", (long)(at_ok ? 1 : 0));
      var_set_num(vm, "SMX_WINDS", (long)(at_ok ? bonds : 0));
      var_set_num(vm, "SMX_WIND", (long)(at_ok ? bonds : 0));
      var_set_num(vm, "SMX_ZEPHYRS", (long)(at_ok ? winds : 0));
      var_set_num(vm, "SMX_ZEPHYR", (long)(at_ok ? winds : 0));
      var_set_num(vm, "SMX_WINDRING", (long)(at_ok ? winds : 0));
      var_set_num(vm, "SMX_SKIES", (long)(at_ok ? skies : 0));
      var_set_num(vm, "SMX_AEROS", (long)(at_ok ? skies : 0));
      var_set_num(vm, "SMX_SEEDATMO", (long)(at_ok ? skies : 0));
      var_set_num(vm, "SMX_MESH", (long)(at_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + winds + skies));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (at_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ATMOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ATMOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX ATMOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX ATMOSPHERE nodes=%d live=%d bonds=%d winds=%d skies=%d need=%d soft=%d talks=%d oob=%d atmosphered=%d vital=%ld\n",
                n, live, bonds, winds, skies, need, soft, vm->smx_talks, vm->smx_oob, at_ok, vital);
    }
    bump(vm); return 1;
  }

  return 0;
}
