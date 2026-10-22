/* CubalC lang — lang_ops_cell_matbrain.c
 * CELL cubebrain/fold: SETDIGIT·FOLDBITS·EEGOPEN/CH/SCALE/WIN/DEMO (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_matbrain.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int cubalc_lang_ops_cell_try_matbrain(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* SETDIGIT cube expr — inject CubeBrain/peer algocube digit 0–9 into matrix */
  if (kw(&L->cur,"SETDIGIT")||kw(&L->cur,"INJECT_DIGIT")||kw(&L->cur,"PEER_DIGIT")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SETDIGIT cube n"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long d = parse_expr(vm, L);
    do_setdigit(vm, id, d);
    bump(vm); return 1;
  }
  /* FOLDBITS cube "01…"|LAST — fold IO bitstring into cube State Matrix */
  if (kw(&L->cur,"FOLDBITS")||kw(&L->cur,"LOADBITS")||kw(&L->cur,"FOLD_BITS")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"FOLDBITS cube bits"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    char bits[CUBALC_HOST_STR_MAX]; bits[0]=0;
    if (resolve_str_arg(vm, L, bits, sizeof bits)!=0){
      /* allow bare bitstring without quotes if all 0/1 — rare; fail soft */
      fail(vm,"FOLDBITS cube \"01…\"|LAST"); return -1;
    }
    do_foldbits(vm, id, bits);
    bump(vm); return 1;
  }
  /* EEGOPEN path — set EEG_PATH for EEGREAD stream */
  if (kw(&L->cur,"EEGOPEN")||kw(&L->cur,"EEG_SOURCE")||kw(&L->cur,"BRAINOPEN")){
    lex_next(L);
    char path[CUBALC_HOST_STR_MAX]; path[0]=0;
    if (resolve_str_arg(vm, L, path, sizeof path)!=0){
      fail(vm,"EEGOPEN path"); return -1;
    }
    var_set_str(vm,"EEG_PATH",path);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* EEGCH n — channel count (1..16, default 8) */
  if (kw(&L->cur,"EEGCH")||kw(&L->cur,"EEG_CH")||kw(&L->cur,"BRAINCH")){
    lex_next(L);
    long n = parse_expr(vm, L);
    if (n < 1) n = 1;
    if (n > CUBALC_EEG_MAX_CH) n = CUBALC_EEG_MAX_CH;
    var_set_num(vm,"EEG_CH",n);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* EEGSCALE µV — magnitude scale for band thresholds */
  if (kw(&L->cur,"EEGSCALE")||kw(&L->cur,"EEG_SCALE")||kw(&L->cur,"BRAINSCALE")){
    lex_next(L);
    long n = parse_expr(vm, L);
    if (n < 1) n = (long)CUBALC_EEG_DEF_SCALE;
    var_set_num(vm,"EEG_SCALE",n);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* EEGWIN n — samples per window before pack */
  if (kw(&L->cur,"EEGWIN")||kw(&L->cur,"EEG_WIN")||kw(&L->cur,"BRAINWIN")){
    lex_next(L);
    long n = parse_expr(vm, L);
    if (n < 1) n = 16;
    if (n > CUBALC_EEG_MAX_WIN) n = CUBALC_EEG_MAX_WIN;
    var_set_num(vm,"EEG_WIN",n);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* EEGDEMO [cube] — synthetic multi-channel window → State Matrix */
  if (kw(&L->cur,"EEGDEMO")||kw(&L->cur,"BRAINDEMO")||kw(&L->cur,"EEGSYNTH")){
    lex_next(L);
    char id[48]="eeg";
    if (L->cur.kind==TK_IDENT){
      snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    }
    do_eegdemo(vm, id);
    bump(vm); return 1;
  }
  /* EEGPACK cube csv|LAST — one multi-channel sample line → matrix fold */
  if (kw(&L->cur,"EEGPACK")||kw(&L->cur,"BRAINPACK")||kw(&L->cur,"EEGSAMPLE")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"EEGPACK cube csv"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    char csv[CUBALC_HOST_STR_MAX]; csv[0]=0;
    if (resolve_str_arg(vm, L, csv, sizeof csv)!=0){
      fail(vm,"EEGPACK cube \"ch0,ch1,…\"|LAST"); return -1;
    }
    do_eegpack_csv(vm, id, csv);
    bump(vm); return 1;
  }
  /* EEGREAD [cube] — read window from EEG_PATH / CUBALC_EEG_PATH → fold */
  if (kw(&L->cur,"EEGREAD")||kw(&L->cur,"BRAINREAD")||kw(&L->cur,"EEGFRAME")){
    lex_next(L);
    char id[48]="eeg";
    if (L->cur.kind==TK_IDENT){
      snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    }
    do_eegread(vm, id);
    bump(vm); return 1;
  }
  /* EEGFOLD cube — re-fold last EEG_BITS into cube (after pack) */
  if (kw(&L->cur,"EEGFOLD")||kw(&L->cur,"BRAINFOLD")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"EEGFOLD cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    char bits[CUBALC_ATOM_BITS + 1]; bits[0]=0;
    for (int i = 0; i < vm->n_vars; i++) {
      if (vm->vars[i].is_str && strcmp(vm->vars[i].name,"EEG_BITS")==0) {
        snprintf(bits,sizeof bits,"%s",vm->vars[i].sval);
        break;
      }
    }
    if (!bits[0]){ fail(vm,"EEGFOLD need EEG_BITS (run EEGDEMO/PACK/READ first)"); return -1; }
    do_foldbits(vm, id, bits);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* CLEARBITS·NOTBITS·COPYBITS·ANDBITS·… → cell_matlogic peel (G_cell_margin) */
  if (kw(&L->cur,"DECONSTRUCT")||kw(&L->cur,"DESTROY")){
    lex_next(L);
    char id[48]="hive";
    if (L->cur.kind==TK_IDENT){ snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L); }
    do_deconstruct(vm,id); bump(vm); return 1;
  }
  if (kw(&L->cur,"RECONSTRUCT")||kw(&L->cur,"CONSTRUCT")||kw(&L->cur,"CREATE")){
    lex_next(L);
    char id[48]="hive";
    if (L->cur.kind==TK_IDENT){ snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L); }
    do_reconstruct(vm,id); bump(vm); return 1;
  }
  /* POSE / TRACK — Truth Matrix pose energy (Cube Law digit 2 = SoT)
   * POSE raw   → rawTracking cube (unfiltered device energy)
   * POSE sot   → tracking cube (Source of Truth)
   * POSE all   → both + plug raw→sot→wall→view
   * Folds law bits; never invents a second parallel gun/hand truth.
   */
  if (kw(&L->cur,"POSE")||kw(&L->cur,"TRACK")||kw(&L->cur,"POSEFLOW")){
    lex_next(L);
    char mode[16]="all";
    if (L->cur.kind==TK_IDENT){
      snprintf(mode,sizeof mode,"%s",L->cur.text);
      lex_next(L);
    } else if (L->cur.kind==TK_STR){
      snprintf(mode,sizeof mode,"%s",L->cur.text);
      lex_next(L);
    }
    ensure_world(vm);
    int want_raw = (strcmp(mode,"raw")==0 || strcmp(mode,"all")==0 || strcmp(mode,"energy")==0);
    int want_sot = (strcmp(mode,"sot")==0 || strcmp(mode,"tracking")==0
                    || strcmp(mode,"all")==0 || strcmp(mode,"truth")==0);
    if (!want_raw && !want_sot){ want_raw=1; want_sot=1; }
    if (want_raw){
      place_cube(vm,"raw","raw_energy",1);
      do_setdigit(vm,"raw",3); /* digit 3 nanobot raw */
      do_foldbits(vm,"raw","1010101010101010");
    }
    if (want_sot){
      place_cube(vm,"sot","kernel_sot",1);
      do_setdigit(vm,"sot",2); /* digit 2 cube SoT */
      do_foldbits(vm,"sot","1100110011001100");
    }
    place_cube(vm,"wall","modifier",1);
    place_cube(vm,"view","consumer",1);
    place_cube(vm,"map3d","spatial_manifest",1); /* Dynamic 3D MAP face */
    if (want_raw && want_sot){
      /* raw → sot → wall → view → map3d (single truth flow) */
      int ir=find_cube(vm,"raw"), is=find_cube(vm,"sot");
      int iw=find_cube(vm,"wall"), iv=find_cube(vm,"view"), im=find_cube(vm,"map3d");
      if (ir>=0 && is>=0) cubalc_cube_plug(&vm->ch, ir, is);
      if (is>=0 && iw>=0) cubalc_cube_plug(&vm->ch, is, iw);
      if (iw>=0 && iv>=0) cubalc_cube_plug(&vm->ch, iw, iv);
      if (iv>=0 && im>=0) cubalc_cube_plug(&vm->ch, iv, im);
    }
    do_flow(vm, 4);
    if (vm->trace) fprintf(vm->trace,"# POSE mode=%s n=%d\n", mode, vm->ch.n_cubes);
    if (vm->res) snprintf(vm->res->last_print,sizeof vm->res->last_print,
                          "pose %s n=%d", mode, vm->ch.n_cubes);
    bump(vm); return 1;
  }
  /* MANIFEST — deconstruct stuck energy, reconstruct under Cube Law, flow, publish */
  if (kw(&L->cur,"MANIFEST")||kw(&L->cur,"PROPHECY")||kw(&L->cur,"SUMMON")){
    lex_next(L);
    char target[48]="hive";
    if (L->cur.kind==TK_IDENT){ snprintf(target,sizeof target,"%s",L->cur.text); lex_next(L); }
    else if (L->cur.kind==TK_STR){ snprintf(target,sizeof target,"%s",L->cur.text); lex_next(L); }
    ensure_world(vm);
    place_cube(vm,"nexus","nanobot_hive",1);
    place_cube(vm,"create","construct",1);
    place_cube(vm,"destroy","deconstruct",0);
    place_cube(vm,"gvrmod","device_free",1);
    place_cube(vm,"map3d","spatial_manifest",1);
    place_cube(vm,"lizard","headset",1);
    /* DECONSTRUCT stuck way → RECONSTRUCT → pose flow */
    do_deconstruct(vm,"destroy");
    do_deconstruct(vm,target);
    do_reconstruct(vm,"create");
    do_reconstruct(vm,"nexus");
    /* plug ring of free devices under SoT */
    int inx=find_cube(vm,"nexus"), ig=find_cube(vm,"gvrmod");
    int im=find_cube(vm,"map3d"), il=find_cube(vm,"lizard");
    int ic=find_cube(vm,"create");
    if (inx>=0 && ig>=0) cubalc_cube_plug(&vm->ch, inx, ig);
    if (ig>=0 && im>=0) cubalc_cube_plug(&vm->ch, ig, im);
    if (im>=0 && il>=0) cubalc_cube_plug(&vm->ch, im, il);
    if (ic>=0 && inx>=0) cubalc_cube_plug(&vm->ch, ic, inx);
    do_setdigit(vm,"nexus",4); /* All Hail NexusCore */
    do_setdigit(vm,"gvrmod",0); /* free device */
    do_setdigit(vm,"map3d",9); /* hivemind unity spatial */
    do_flow(vm, 8);
    do_harmony(vm,"hive");
    long d=do_decide(vm,"nexus");
    if (vm->trace) fprintf(vm->trace,"# MANIFEST %s decide=%ld harmony=%ld\n",
                           target, d, (long)lround(vm->ch.unity*100));
    if (vm->res) snprintf(vm->res->last_print,sizeof vm->res->last_print,
                          "manifest %s decide %ld unity %.2f", target, d, vm->ch.unity);
    bump(vm); return 1;
  }
  return 0;
}
