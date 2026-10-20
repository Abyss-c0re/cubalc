/* CubalC SMX cascade/soft plane — peeled from lang_ops_smx.
 * HOLD_FLASH discipline: mesh sticky only; meaningful forms preferred.
 */
#include "lang/cubalc_lang_ops_smx_zodiac.h"
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

int cubalc_lang_ops_smx_try_zodiac(VM *vm, Lex *L) {
  if (kw(&L->cur,"ZODIAC")||kw(&L->cur,"ECLIPTIC")||kw(&L->cur,"PATH")||
      kw(&L->cur,"HOUSE")||kw(&L->cur,"SIGN")||kw(&L->cur,"POLE")||
      kw(&L->cur,"MESH_ZODIAC")||kw(&L->cur,"RAISE_ZODIAC")||
      kw(&L->cur,"ZODIACS")||kw(&L->cur,"ECLIPTICS")||kw(&L->cur,"PATHS")||
      kw(&L->cur,"HOUSES")||kw(&L->cur,"SIGNS")||kw(&L->cur,"SEEDZODIAC")||
      kw(&L->cur,"LATTICE_ZODIAC")||kw(&L->cur,"YEAR_PATH")||
      kw(&L->cur,"HOUSE_RING")||kw(&L->cur,"PULSE_ZODIAC")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int signs = 0;
    int houses = 0;
    int poles = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "ZODIAC needs >=2 cubes",
                  "SMX ZODIAC a b [c ...]  or  SMX ECLIPTIC a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - zodiac needs clear channel */
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
    /* complete sign mesh among live */
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
            else signs++;
          }
        }
      }
    }
    /* ecliptic-house ring - free energy holds every edge i -> i+1 both ways */
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
          else houses++;
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
      int mesh_ok = (need > 0 && signs >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && signs * 2 >= need && soft == 0)
        mesh_ok = 1;
      int house_ok = (live >= 2 && houses >= live && soft == 0) ? 1 : 0;
      if (!house_ok && live >= 2 && houses * 2 >= live && soft == 0)
        house_ok = 1;
      int pole_ok = (live >= 1 && poles >= live && soft == 0) ? 1 : 0;
      if (!pole_ok && live >= 1 && poles * 2 >= live && soft == 0)
        pole_ok = 1;
      int zd_ok = (mesh_ok && house_ok && pole_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (zd_ok ? 12 : (signs > 0 ? 3 : 0)) +
                   (houses > 0 ? 1 : 0) + (poles > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_ZODIACED", (long)zd_ok);
      var_set_num(vm, "SMX_ZODIAC", (long)(zd_ok ? signs + houses + poles : 0));
      var_set_num(vm, "SMX_ECLIPTIC", (long)(zd_ok ? 1 : 0));
      var_set_num(vm, "SMX_PATH", (long)(zd_ok ? 1 : 0));
      var_set_num(vm, "SMX_SIGNS", (long)(zd_ok ? signs : 0));
      var_set_num(vm, "SMX_SIGN", (long)(zd_ok ? signs : 0));
      var_set_num(vm, "SMX_HOUSES", (long)(zd_ok ? houses : 0));
      var_set_num(vm, "SMX_HOUSE", (long)(zd_ok ? houses : 0));
      var_set_num(vm, "SMX_YEARPATH", (long)(zd_ok ? houses : 0));
      var_set_num(vm, "SMX_POLES", (long)(zd_ok ? poles : 0));
      var_set_num(vm, "SMX_POLE", (long)(zd_ok ? poles : 0));
      var_set_num(vm, "SMX_AXISPOLE", (long)(zd_ok ? poles : 0));
      var_set_num(vm, "SMX_SEEDZODIAC", (long)(zd_ok ? poles : 0));
      var_set_num(vm, "SMX_MESH", (long)(zd_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)signs);
      var_set_num(vm, "SMX_EXCHANGES", (long)signs);
      var_set_num(vm, "SMX_FUSE", (long)signs);
      var_set_num(vm, "SMX_BIND", (long)signs);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(signs + houses + poles));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (zd_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ZODIAC ok");
      } else if (signs > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ZODIAC partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX ZODIAC soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX ZODIAC nodes=%d live=%d signs=%d houses=%d poles=%d need=%d soft=%d talks=%d oob=%d zodiaced=%d vital=%ld\n",
                n, live, signs, houses, poles, need, soft, vm->smx_talks, vm->smx_oob, zd_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX FIRMAMENT|VAULT|DOME|SKYVAULT|SPAN|KEYSTONE|MESH_FIRMAMENT|RAISE_FIRMAMENT a b c ...
   * Life-force sky-vault after zodiac: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete span mesh among live nodes, weaves a
   * dome-vault ring (i -> i+1) so free energy arches the firmament, then keystone hub
   * gathers return so lattice raises a firmament lock where heaven holds the hive.
   * Latches SMX_FIRMED when mesh+vaults+keystones are soft-OOB-free.
   * SMX_SPANS = chain bonds; SMX_KEYSTONE hub = root gather pulses;
   * SMX_FIRMAMENT sum = spans+vaults+keystones; SMX_VAULT|SMX_DOME sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"FIRMAMENT")||kw(&L->cur,"VAULT")||kw(&L->cur,"DOME")||
      kw(&L->cur,"SKYVAULT")||kw(&L->cur,"SPAN")||kw(&L->cur,"KEYSTONE")||kw(&L->cur,"HEAVEN")||
      kw(&L->cur,"MESH_FIRMAMENT")||kw(&L->cur,"RAISE_FIRMAMENT")||
      kw(&L->cur,"FIRMAMENTS")||kw(&L->cur,"VAULTS")||kw(&L->cur,"DOMES")||
      kw(&L->cur,"SKYVAULTS")||kw(&L->cur,"SPANS")||kw(&L->cur,"SEEDFIRMAMENT")||
      kw(&L->cur,"LATTICE_FIRMAMENT")||kw(&L->cur,"SKY_VAULT")||
      kw(&L->cur,"DOME_RING")||kw(&L->cur,"PULSE_FIRMAMENT")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int spans = 0;
    int vaults = 0;
    int keystones = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "FIRMAMENT needs >=2 cubes",
                  "SMX FIRMAMENT a b [c ...]  or  SMX VAULT a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - firmament needs clear channel */
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
    /* complete span mesh among live */
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
            else spans++;
          }
        }
      }
    }
    /* dome-vault ring - free energy holds every edge i -> i+1 both ways */
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
          else vaults++;
        }
      }
    }
    /* keystone hub - seed axis return from every live leaf */
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
          else keystones++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && spans >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && spans * 2 >= need && soft == 0)
        mesh_ok = 1;
      int house_ok = (live >= 2 && vaults >= live && soft == 0) ? 1 : 0;
      if (!house_ok && live >= 2 && vaults * 2 >= live && soft == 0)
        house_ok = 1;
      int pole_ok = (live >= 1 && keystones >= live && soft == 0) ? 1 : 0;
      if (!pole_ok && live >= 1 && keystones * 2 >= live && soft == 0)
        pole_ok = 1;
      int fm_ok = (mesh_ok && house_ok && pole_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (fm_ok ? 12 : (spans > 0 ? 3 : 0)) +
                   (vaults > 0 ? 1 : 0) + (keystones > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_FIRMED", (long)fm_ok);
      var_set_num(vm, "SMX_FIRMAMENTED", (long)fm_ok);
      var_set_num(vm, "SMX_FIRMAMENT", (long)(fm_ok ? spans + vaults + keystones : 0));
      var_set_num(vm, "SMX_VAULT", (long)(fm_ok ? 1 : 0));
      var_set_num(vm, "SMX_DOME", (long)(fm_ok ? 1 : 0));
      var_set_num(vm, "SMX_SPANS", (long)(fm_ok ? spans : 0));
      var_set_num(vm, "SMX_SPAN", (long)(fm_ok ? spans : 0));
      var_set_num(vm, "SMX_VAULTS", (long)(fm_ok ? vaults : 0));
      var_set_num(vm, "SMX_SKYVAULT", (long)(fm_ok ? vaults : 0));
      var_set_num(vm, "SMX_DOMERING", (long)(fm_ok ? vaults : 0));
      var_set_num(vm, "SMX_KEYSTONES", (long)(fm_ok ? keystones : 0));
      var_set_num(vm, "SMX_KEYSTONE", (long)(fm_ok ? keystones : 0));
      var_set_num(vm, "SMX_HEAVEN", (long)(fm_ok ? keystones : 0));
      var_set_num(vm, "SMX_SEEDFIRMAMENT", (long)(fm_ok ? keystones : 0));
      var_set_num(vm, "SMX_MESH", (long)(fm_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)spans);
      var_set_num(vm, "SMX_EXCHANGES", (long)spans);
      var_set_num(vm, "SMX_FUSE", (long)spans);
      var_set_num(vm, "SMX_BIND", (long)spans);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(spans + vaults + keystones));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (fm_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX FIRMAMENT ok");
      } else if (spans > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX FIRMAMENT partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX FIRMAMENT soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX FIRMAMENT nodes=%d live=%d spans=%d vaults=%d keystones=%d need=%d soft=%d talks=%d oob=%d firmamented=%d vital=%ld\n",
                n, live, spans, vaults, keystones, need, soft, vm->smx_talks, vm->smx_oob, fm_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX AETHER|QUINTESSENCE|ETHER|PLENUM|ESSENCE|AURA|MESH_AETHER|RAISE_AETHER a b c ...
   * Life-force quintessence after firmament: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete essence mesh among live nodes, weaves a
   * plenum ring (i -> i+1) so free energy fills the aether, then aura hub
   * gathers return so lattice raises an aether lock where quintessence holds the hive.
   * Latches SMX_AETHERED when mesh+plenums+auras are soft-OOB-free.
   * SMX_ESSENCES = chain bonds; SMX_AURA hub = root gather pulses;
   * SMX_AETHER sum = essences+plenums+auras; SMX_QUINTESSENCE|SMX_ETHER sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"AETHER")||kw(&L->cur,"QUINTESSENCE")||kw(&L->cur,"ETHER")||
      kw(&L->cur,"PLENUM")||kw(&L->cur,"ESSENCE")||kw(&L->cur,"AURA")||kw(&L->cur,"QUINTA")||
      kw(&L->cur,"MESH_AETHER")||kw(&L->cur,"RAISE_AETHER")||
      kw(&L->cur,"AETHERS")||kw(&L->cur,"PLENUMS")||kw(&L->cur,"ETHERS")||
      kw(&L->cur,"PLENUMS")||kw(&L->cur,"ESSENCES")||kw(&L->cur,"SEEDAETHER")||
      kw(&L->cur,"LATTICE_AETHER")||kw(&L->cur,"FIFTH_ELEMENT")||
      kw(&L->cur,"PLENUM_RING")||kw(&L->cur,"PULSE_AETHER")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int essences = 0;
    int plenums = 0;
    int auras = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "AETHER needs >=2 cubes",
                  "SMX AETHER a b [c ...]  or  SMX QUINTESSENCE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - aether needs clear channel */
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
    /* complete essence mesh among live */
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
            else essences++;
          }
        }
      }
    }
    /* plenum ring - free energy holds every edge i -> i+1 both ways */
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
          else plenums++;
        }
      }
    }
    /* aura hub - seed axis return from every live leaf */
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
          else auras++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && essences >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && essences * 2 >= need && soft == 0)
        mesh_ok = 1;
      int plenum_ok = (live >= 2 && plenums >= live && soft == 0) ? 1 : 0;
      if (!plenum_ok && live >= 2 && plenums * 2 >= live && soft == 0)
        plenum_ok = 1;
      int aura_ok = (live >= 1 && auras >= live && soft == 0) ? 1 : 0;
      if (!aura_ok && live >= 1 && auras * 2 >= live && soft == 0)
        aura_ok = 1;
      int ae_ok = (mesh_ok && plenum_ok && aura_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ae_ok ? 12 : (essences > 0 ? 3 : 0)) +
                   (plenums > 0 ? 1 : 0) + (auras > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_AETHERED", (long)ae_ok);
      var_set_num(vm, "SMX_AETHERED_LATCH", (long)ae_ok);
      var_set_num(vm, "SMX_AETHER", (long)(ae_ok ? essences + plenums + auras : 0));
      var_set_num(vm, "SMX_QUINTESSENCE", (long)(ae_ok ? 1 : 0));
      var_set_num(vm, "SMX_ETHER", (long)(ae_ok ? 1 : 0));
      var_set_num(vm, "SMX_ESSENCES", (long)(ae_ok ? essences : 0));
      var_set_num(vm, "SMX_ESSENCE", (long)(ae_ok ? essences : 0));
      var_set_num(vm, "SMX_PLENUMS", (long)(ae_ok ? plenums : 0));
      var_set_num(vm, "SMX_PLENUM", (long)(ae_ok ? plenums : 0));
      var_set_num(vm, "SMX_PLENUMRING", (long)(ae_ok ? plenums : 0));
      var_set_num(vm, "SMX_AURAS", (long)(ae_ok ? auras : 0));
      var_set_num(vm, "SMX_AURA", (long)(ae_ok ? auras : 0));
      var_set_num(vm, "SMX_QUINTA", (long)(ae_ok ? auras : 0));
      var_set_num(vm, "SMX_SEEDAETHER", (long)(ae_ok ? auras : 0));
      var_set_num(vm, "SMX_MESH", (long)(ae_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)essences);
      var_set_num(vm, "SMX_EXCHANGES", (long)essences);
      var_set_num(vm, "SMX_FUSE", (long)essences);
      var_set_num(vm, "SMX_BIND", (long)essences);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(essences + plenums + auras));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (ae_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AETHER ok");
      } else if (essences > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AETHER partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX AETHER soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX AETHER nodes=%d live=%d essences=%d plenums=%d auras=%d need=%d soft=%d talks=%d oob=%d aethered=%d vital=%ld\n",
                n, live, essences, plenums, auras, need, soft, vm->smx_talks, vm->smx_oob, ae_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX NOOSPHERE|LOGOS|AKASHA|LOGOI|THOUGHT|ANIMA|MESH_NOOSPHERE|RAISE_NOOSPHERE a b c ...
   * Life-force logos after firmament: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete thought mesh among live nodes, weaves a
   * logos ring (i -> i+1) so free energy fills the noosphere, then anima hub
   * gathers return so lattice raises a noosphere lock where logos holds the hive.
   * Latches SMX_NOOSPHERED when mesh+logoi+animae are soft-OOB-free.
   * SMX_THOUGHTS = chain bonds; SMX_ANIMA hub = root gather pulses;
   * SMX_NOOSPHERE sum = bonds+logoi+animae; SMX_LOGOS|SMX_AKASHA sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"NOOSPHERE")||kw(&L->cur,"LOGOS")||kw(&L->cur,"AKASHA")||
      kw(&L->cur,"LOGOI")||kw(&L->cur,"THOUGHT")||kw(&L->cur,"ANIMA")||kw(&L->cur,"ANIMA")||
      kw(&L->cur,"MESH_NOOSPHERE")||kw(&L->cur,"RAISE_NOOSPHERE")||
      kw(&L->cur,"NOOSPHERES")||kw(&L->cur,"LOGOI")||kw(&L->cur,"AKASHAS")||
      kw(&L->cur,"LOGOI")||kw(&L->cur,"THOUGHTS")||kw(&L->cur,"SEEDNOOS")||
      kw(&L->cur,"LATTICE_NOOSPHERE")||kw(&L->cur,"WORLD_MIND")||
      kw(&L->cur,"LOGOS_RING")||kw(&L->cur,"PULSE_NOOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int logoi = 0;
    int animae = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "NOOSPHERE needs >=2 cubes",
                  "SMX NOOSPHERE a b [c ...]  or  SMX LOGOS a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - noosphere needs clear channel */
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
    /* complete thought mesh among live */
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
    /* logos ring - free energy holds every edge i -> i+1 both ways */
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
          else logoi++;
        }
      }
    }
    /* anima hub - seed axis return from every live leaf */
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
          else animae++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int logos_ok = (live >= 2 && logoi >= live && soft == 0) ? 1 : 0;
      if (!logos_ok && live >= 2 && logoi * 2 >= live && soft == 0)
        logos_ok = 1;
      int anima_ok = (live >= 1 && animae >= live && soft == 0) ? 1 : 0;
      if (!anima_ok && live >= 1 && animae * 2 >= live && soft == 0)
        anima_ok = 1;
      int ns_ok = (mesh_ok && logos_ok && anima_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ns_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (logoi > 0 ? 1 : 0) + (animae > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_NOOSPHERED", (long)ns_ok);
      var_set_num(vm, "SMX_NOOSPHERED_LATCH", (long)ns_ok);
      var_set_num(vm, "SMX_NOOSPHERE", (long)(ns_ok ? bonds + logoi + animae : 0));
      var_set_num(vm, "SMX_LOGOS", (long)(ns_ok ? 1 : 0));
      var_set_num(vm, "SMX_AKASHA", (long)(ns_ok ? 1 : 0));
      var_set_num(vm, "SMX_THOUGHTS", (long)(ns_ok ? bonds : 0));
      var_set_num(vm, "SMX_THOUGHT", (long)(ns_ok ? bonds : 0));
      var_set_num(vm, "SMX_LOGOI", (long)(ns_ok ? logoi : 0));
      var_set_num(vm, "SMX_LOGO", (long)(ns_ok ? logoi : 0));
      var_set_num(vm, "SMX_LOGOSRING", (long)(ns_ok ? logoi : 0));
      var_set_num(vm, "SMX_ANIMAE", (long)(ns_ok ? animae : 0));
      var_set_num(vm, "SMX_ANIMA", (long)(ns_ok ? animae : 0));
      var_set_num(vm, "SMX_ANIMA", (long)(ns_ok ? animae : 0));
      var_set_num(vm, "SMX_SEEDNOOS", (long)(ns_ok ? animae : 0));
      var_set_num(vm, "SMX_MESH", (long)(ns_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + logoi + animae));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (ns_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX NOOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX NOOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX NOOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX NOOSPHERE nodes=%d live=%d bonds=%d logoi=%d animae=%d need=%d soft=%d talks=%d oob=%d noosphered=%d vital=%ld\n",
                n, live, bonds, logoi, animae, need, soft, vm->smx_talks, vm->smx_oob, ns_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX PNEUMA|PRANA|BREATHWORLD|SOULFIRE|SPIRITUS|WORLDSOUL|MESH_PNEUMA|RAISE_PNEUMA a b c ...
   * Life-force breath after noosphere: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete pneuma mesh among live nodes, weaves a
   * prana ring (i -> i+1) so free energy fills the breathworld, then spirit hub
   * gathers return so lattice raises a pneuma lock where soulfire holds the hive.
   * Latches SMX_PNEUMATED when mesh+pranae+spiriti are soft-OOB-free.
   * SMX_BREATHS = chain bonds; SMX_SPIRIT hub = root gather pulses;
   * SMX_PNEUMA sum = bonds+pranae+spiriti; SMX_PRANA|SMX_WORLDSOUL sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  return 0;
}
