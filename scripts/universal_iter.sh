#!/usr/bin/env bash
# CubalC universal improve tick (mechanical half of the 6-min loop).
# Agent synthesizes language deltas; this script: pick algocube, evolve once,
# build, run proofs + science demos, write iteration plate for NexusCore.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${CUBALC_BIN:-$ROOT/out/cubalc}"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"
EV="$CUBALC_STATE/evolve"
mkdir -p "$EV" "$CUBALC_STATE"
ITER_F="$EV/UNIVERSAL_ITER.json"
LOG="$EV/universal_iter.log"

ts() { date -Iseconds; }
log() { echo "[$(ts)] $*" | tee -a "$LOG" >&2; }

[[ -x "$BIN" ]] || make -C "$ROOT" all

# Random algocube digit 0–9 from /dev/urandom + optional genome
DIGIT=$(( $(od -An -N1 -tu1 /dev/urandom | tr -d ' ') % 10 ))
if [[ -f "$EV/algo_genome.txt" ]]; then
  G0=$(head -c 1 "$EV/algo_genome.txt" | od -An -tu1 | tr -d ' ' || echo 0)
  DIGIT=$(( (DIGIT + ${G0:-0}) % 10 ))
fi

ITER=1
if [[ -f "$ITER_F" ]]; then
  ITER=$(python3 -c "import json;print(json.load(open('$ITER_F')).get('iter',0)+1)" 2>/dev/null || echo 1)
fi

log "ITER=$ITER algocube_digit=$DIGIT — build"
make -C "$ROOT" all 2>&1 | tail -3 | tee -a "$LOG"

log "evolve --once"
"$BIN" evolve --once 2>&1 | tee -a "$LOG" | tail -2 || true

log "proofs + science demos (timeout 25s each)"
PASS=0; FAIL=0
run_one() {
  local f="$1"
  if timeout 25 "$BIN" run "$f" >"$EV/last_run.out" 2>&1 && grep -q '"ok":true' "$EV/last_run.out"; then
    PASS=$((PASS+1)); echo "  PASS $f" | tee -a "$LOG"
  else
    FAIL=$((FAIL+1)); echo "  FAIL $f" | tee -a "$LOG"
    tail -4 "$EV/last_run.out" 2>/dev/null | tee -a "$LOG" || true
  fi
}
# curated fast proofs (language surface + new universal bitops)
for base in 01_arithmetic 02_cop_matrix 06_decide 09_algocube_harmony \
            11_cube_io_reverse 12_nest_compile 13_bitops_universal \
            14_io_codec_universal 15_rotate_pack_universal \
            16_control_flow_universal 17_cop_matrix_ops \
            18_data_cells_stack 19_math_modular_universal \
            20_string_ops_universal 21_fn_return_case \
            22_cell_fold_inc 23_rand_energy_rotbits 24_stack_ops_universal \
            25_cell_mem_universal 26_math_ilog_modinv 27_stack_depth_universal \
            28_cell_arith_sort 29_cell_argmin_rot 30_stack_cell_bridge \
            31_cell_logic 32_bitfield_divceil 33_math_binom_perm 34_stack_alu \
            35_stack_bitops 36_stack_compare 37_stack_select_clamp 38_stack_fold \
            39_stack_bitmetrics 40_stack_bitfield 41_stack_numthy 42_stack_double 43_stack_modarith 44_stack_combinators 45_stack_numpred 46_stack_combin 47_stack_pack 48_word_path 49_stack_science 50_seed_rng 51_sext_zext 52_stack_fold_mean 53_pack8_nibble 54_ilog10_parity 55_align_setbyte 56_pow10 57_mask_isdiv 58_select_clamp_range 59_clamp_bound_sat 60_overflow_bitops 61_shift_rotate_cmp 62_bitwise_logic_select 63_popcount_clz_ctz 64_muldiv_bswap 65_rotate_shift_extend 66_bool_logic_cmp 67_mem_set_inc_xchg 68_jump_jz_sloop 69_branch_beq_case 70_addc_divmod 71_callif_retif 72_iroot_nextprime 73_str_trim_prefix 74_for_downto_lpad 75_ndivs_sigma_phi 76_countbits_getbit 77_shlbits_findone \
            78_revbits_diff \
            79_word_bits_bridge \
            80_ternary_forever \
            81_range_bits \
            82_cell_div_scan \
            83_pack32_pext_zip \
            84_eqbits_subset \
            85_rand_shuffle \
            86_muxbits_match \
            87_copyrange_parity \
            88_rotrange_shift \
            89_eachcell_breakif \
            90_pext_zip_matrix \
            91_dot_maj_gray \
            92_forbit_pass \
            93_andrange_reduce \
            94_clz_jaccard \
            95_mobius_radical \
            96_trans_runs_mask \
            97_ndrop_sswapn \
            98_sgetcell_sreplace \
            99_sinsert_npush \
            100_coprime_ceilpow2 \
            101_maxrun_zruns \
            102_skeep \
            103_3dup_2tuck \
            104_3over_under \
            105_sfill_dropz \
            106_sinc_sdbl \
            107_saddn_smuln \
            108_ssubtoc_smultoc \
            109_sdivn_smodn \
            110_ssetbn_stestn \
            111_seqn_sminn \
            112_sroln_ssarn \
            113_sandi_sxori \
            114_snandi_sxnori \
            115_srandn_energysub \
            115_energy_rng_imm \
            116_dadd_dmul \
            117_energyxfer_srandrangen \
            118_sloadcells_cellxfer \
            119_3rot_3tuck \
            120_sbextn_smaskn \
            121_str_left_count \
            122_spick_sroll \
            123_3nip_4dup \
            124_4nip_4rot \
            125_smoddiv_jacobi \
            126_scopycell_srot \
            127_4tuck_5dup \
            128_sval_somega \
            129_sscan_sdiff \
            130_sandcell_sne \
            131_sorder_carmichael \
            132_sqpart_scrt \
            133_ispower_nthprime \
            134_ltcell_mux \
            135_energy_fleet \
            136_aliquot_perfect \
            137_dupz_swapif \
            138_anycell_eqrange \
            139_ddiv_dmin \
            140_findlast_nz \
            141_5nip_5rot \
            142_primecount_gap \
            143_dand_dneg \
            144_deq_dlt \
            145_dgcd_dlcm \
            146_dshl_dsar \
            147_dsqr_dcoprime \
            148_dclamp_dsel \
            149_dinc_dnot \
            150_drol_dwithin \
            151_dnand_dandn \
            152_dpopcnt_dclz \
            153_dorn_dparity \
            154_dffs_dbwidth \
            155_dclo_dispow2 \
            156_davg_dhamm \
            157_ddbl_dbswap \
            158_dlog2_dphi \
            159_dodd_dltz \
            160_drand_dsatadd \
            161_dclip_dsext \
            162_dlo8_dpack8 \
            163_dsatmul_drandrange \
            164_dlo16_dpack16 \
            165_dbls_dmask \
            166_dlo32_dpack32 \
            167_dsatdiv_drandbits \
            168_dbtest_dsetb \
            169_dbext_dbdep \
            170_dbhsi_dceilpow2 \
            171_dpext_dpdep \
            172_dzip_dunzip \
            173_ddivceil_ddivfloor \
            174_2rrot_6dup \
            175_daddmod_dsubmod \
            176_dwrap_dhypot \
            177_drelu_dcopysign \
            178_dmedian_dmaxabs \
            179_dpct_dlerp \
            180_dmadd_dmulhi \
            181_dmulmod_dpowmod \
            182_dscale_dclip100 \
            183_dudiv_dumod \
            184_dumin_dumax \
            185_dinv_dnorm100 \
            186_dumulhi_dbswap \
            187_dult_duge \
            188_dzext8_dzext16 \
            189_daddc_dsubb \
            190_dclip32_dext32 \
            191_dcmp_ducmp \
            192_dmodinv_dmoddiv \
            193_daddovf_dmulovf \
            194_drelu6_ddeadz \
            195_dthresh_dgate \
            196_duaddovf_dumulovf \
            197_daddc2_dsubb2 \
            198_dsel2_dmux2 \
            199_dshlc_dshrc \
            200_dlo4_dpack4 \
            201_dnegc2 \
            202_dnipif_dkeepif \
            203_dclip4_dext4 \
            204_drol8_dror16 \
            205_dshl8_dsar16 \
            206_dgeom_dharm_drms \
            207_dfib_dfact_dlog10 \
            208_dswapif_ddropif_ddupif \
            209_drol4_dnibswap \
            210_dunpack4_dshl4 \
            211_dclip01_dcomp100_dsumsq \
            212_dmobius_drad_dsqfree \
            213_ddiffsq_dstep \
            214_dleaky_dsoftsign \
            215_dunpack8_dunpack16 \
            216_doverif_dtuckif \
            217_drotif_drrotif \
            218_dshgate_dzeroif \
            219_dsetbn_dbtestn \
            220_droln_drorn \
            221_dandi_dxori \
            222_6swap_6nip \
            223_dshln_dsarn \
            224_daddn_dmuln \
            225_ddivn_dmodn \
            226_dnandi_dxnori \
            227_6rot_6over \
            228_dminn_dclampn \
            229_deqn_dgtn \
            230_6tuck \
            231_dnen_dgenn \
            232_dflipbn \
            233_dmaskn_dbextn \
            234_dbdepn_dormn \
            235_dland_dimp \
            236_dhmaskn_dclrln \
            237_dorhn_dclrh \
            238_dnandmn_dxnormn \
            239_dnandhn_dxnorhn \
            240_dpopmn_dallmn \
            241_daddmodn_dmulmodn \
            242_dpophn_dallhn \
            243_dbrevn_drolbn \
            244_dbrevhn_drolhn \
            245_sbrevn_srolbn \
            246_sbrevhn_srolhn \
            247_sandmn_sormn \
            248_sandhn_sorhn \
            249_spopmn_sallmn \
            250_dunpack32 \
            251_saddmodn_smulmodn \
            252_spophn_sallhn \
            253_sclrmn_shmaskn \
            254_snandmn_sxnormn \
            255_snandhn_sxnorhn \
            256_sclampn_swithinn \
            257_smodinvn_spowmodn \
            258_dmodinvn_dpowmodn \
            259_sbyten_ssetbyten \
            260_snibn_ssetnibn \
            261_dbyten_dsetbyten \
            262_dnibn_dsetnibn \
            263_swordn_sset16n \
            264_dwordn_dset16n \
            265_sget32n_sset32n \
            266_dget32n_dset32n \
            267_sand32n_sor32n \
            268_dand32n_dor32n \
            269_sand16n_sor16n \
            270_dand16n_dor16n \
            271_sand8n_sor8n \
            272_dand8n_dor8n \
            273_sand4n_sor4n \
            274_dand4n_dor4n \
            275_snand4n_snor4n \
            276_snand8n_snor8n \
            277_dnand8n_dnor8n \
            278_snand16n_snor16n \
            279_dnand16n_dnor16n \
            280_dnand4n_dnor4n \
            281_snand32n_snor32n \
            282_dnand32n_dnor32n \
            283_sandni_sorni \
            284_dandni_dorni \
            285_snandni_snorni \
            286_dnandni_dnorni \
            287_dbetweenn_dwithinn \
            288_ssubfromn_sdivfromn \
            289_dsubfromn_ddivfromn \
            290_saddovfn_ssubovfn \
            291_daddovfn_dsubovfn \
            292_ssataddn_ssatsubn \
            293_dsataddn_dsatsubn \
            294_ssatdivn_dsatdivn \
            295_saddcn_ssubbn \
            296_d0eq_d0ne \
            297_sshlc_sshrc \
            298_d0le_d0ge \
            299_7dup_7drop \
            300_sudivn_dumodn \
            301_sudivfromn_dumodfromn \
            302_swmodn_dwrapn \
            303_7nip_7rot \
            304_ssatdiv \
            305_ssatsubfromn_dsatdivfromn \
            306_7over_7tuck \
            307_sshlcn_dshrccn \
            308_sgcdn_dlcmn \
            309_sdivceiln_ddivfloorn \
            310_scoprimenn_dpown \
            311_sultn_dugen \
            312_suminn_dumaxn \
            313_snegc_dnegcn \
            314_8dup_8drop \
            315_smodtoc_smintoc \
            316_ssubfromtoc_smodfromtoc \
            317_ssatsubfromtoc_ssatdivfromtoc \
            318_ssataddtoc_ssatdivtoc \
            319_sandtoc_sxortoc \
            320_sabstoc_snottoc \
            321_8nip_8rot \
            322_ssatmultoc_sclamptoc \
            323_8over_8tuck \
            324_snandtoc_sxnortoc \
            325_sshltoc_ssartoc \
            326_sroltoc_srortoc \
            327_sdbltoc_shalftoc \
            328_9dup_9drop \
            329_ssqrtoc_sisqrtoc \
            330_sandntoc_sxorntoc \
            331_9nip_9rot \
            332_9over_9tuck \
            333_seqtoc_sgttoc \
            334_snetoc_sgetoc \
            335_sulttoc_sugtoc \
            336_scmptoc_sucmptoc \
            337_ssigntoc_snez \
            338_sltz_sgez_toc \
            339_soddtoc_seventoc \
            340_sinctoc_sdectoc \
            341_seqtocn_sgttocn \
            342_sletocn_sgetocn \
            343_sulttocn_sugtocn \
            344_scmptocn_sucmptocn \
            345_spopcnttoc_sclztoc \
            346_saddtocn_smultocn \
            347_sandtocn_sxortocn \
            348_sdivtocn_smaxtocn \
            349_ssubfromtocn_smodfromtocn \
            350_sshltocn_ssartocn \
            351_sroltocn_srortocn \
            352_snandtocn_sxnortocn \
            353_sudivfromtocn_sumodfromtocn \
            354_sandntocn_sxorntocn \
            355_sandnfromtocn_sxornfromtocn \
            356_sandnfromn_sxornfromn \
            357_dandnfromn_dxornfromn \
            358_snandnfromn_sxnornfromn \
            359_dnandnfromn_dxnornfromn \
            360_snandnfromtocn_sxnornfromtocn \
            361_ssubmodfromn_smoddivfromn \
            362_dsubmodfromn_dmoddivfromn \
            363_ssubmodfromtocn_smoddivfromtocn \
            364_saddmodtocn_smulmodtocn \
            365_spowmodtocn_smodinvtocn \
            366_sudivtocn_sumaxtocn \
            367_spowmodtoc_smodinvtoc \
            368_sudivtoc_sumaxtoc \
            369_ssubmodfromtoc_smoddivfromtoc \
            370_saddmodtoc_smulmodtoc \
            371_ssataddtocn_sclamptocn \
            372_ssatdivtocn_ssatdivfromtocn \
            373_sudivfromtoc_sumodfromtoc \
            374_saddctocn_ssubbtocn \
            375_ssetbtocn_sflpbtocn \
            376_sgcdtocn_slcmtocn \
            377_ssetbtoc_sflpbtoc \
            378_sbytetocn_sclrbytetocn \
            379_swordtocn_sclr16tocn \
            380_sdivceiltocn_sdivfloortocn \
            381_snibtocn_sclrnibtocn \
            382_sget32tocn_sclr32tocn \
            383_sandmntocn_sxormntocn \
            384_spopmntocn_sallmntocn \
            385_snandmntocn_sxnormntocn \
            386_sandhntocn_sxorhntocn \
            387_snandhntocn_sxnorhntocn \
            388_sclrmntocn_shmasktocn \
            389_sbrevtocn_srorbtocn \
            390_sbrevhntocn_srorhntocn \
            391_spophntocn_sallhntocn \
            392_spopmntoc_sallmntoc \
            393_spophntoc_sallhntoc \
            394_sandmntoc_sxormntoc \
            395_snandmntoc_sxnormntoc \
            396_sandhntoc_sxorhntoc \
            397_snandhntoc_sxnorhntoc \
            398_sclrmntoc_shmasktoc \
            399_sbrevtoc_srorbtoc \
            400_sbrevhntoc_srorhntoc \
            401_sbytetoc_sclrbytetoc \
            402_swordtoc_sclr16toc \
            403_sget32toc_sclr32toc \
            404_snibtoc_sclrnibtoc \
            405_ssexttoc_szexttoc \
            406_ssexttocn_szexttocn \
            407_ssext16toc_szext8toc \
            408_szext16toc_ssext4toc \
            409_ssext8tocn_szext4tocn \
            410_szext4toc_szext16tocn \
            411_szext32tocn_sclip8toc \
            412_sclip4toc_sclip32toc \
            413_sclip4tocn_sclip16tocn \
            414_sclip32tocn_sclips16toc \
            415_sclips4toc_sclips8tocn \
            416_sclips4tocn_sclips32tocn \
            417_sclips4_sclips16 \
            418_sclip4_sclips32 \
            419_dclips4_dclips16 \
            420_dclips32_drol32 \
            421_dshl32_dsar32 \
            422_ssar32_srotr32 \
            423_sparitytoc_sbitrev32toc \
            424_nandcell_xnorcell \
            425_sbswap16toc_sbitrev16toc \
            426_sshl32toc_ssar32toc \
            427_srotl32toc_srotr32tocn \
            428_sshl32tocn_ssar32tocn \
            429_shl32cell_sar32cell \
            430_rol32cell_bitrev32cell \
            431_paritycell_popcntcell \
            432_clzcell_bswap16cell \
            433_bswap64cell_rol16cell \
            434_ror16cell_shr16cell \
            435_sar16cell_abs16cell \
            436_popcnt16cell_ctz16cell \
            437_shl8cell_sar8cell \
            438_rol8cell_bitrev8cell \
            439_sext8cell_popcnt8cell \
            440_clz8cell_parity8cell \
            441_neg8cell_clip8cell \
            442_neg16cell_clip16cell \
            443_clips8cell_clips32cell; do
  f="$ROOT/programs/proof/${base}.cubalc"
  [[ -f "$f" ]] && run_one "$f"
done
# science demos sample (language direction probes)
if [[ -d "$ROOT/programs/science" ]]; then
  while IFS= read -r f; do run_one "$f"; done < <(
    find "$ROOT/programs/science" -name '*.cubalc' | sort | head -20
  )
fi
VER=$("$BIN" law 2>/dev/null | python3 -c "import sys,json,re; s=sys.stdin.read(); m=re.search(r'\"version\":\"([^\"]+)\"',s); print(m.group(1) if m else '?')" 2>/dev/null || echo "?")

python3 - <<PY
import json, time
from pathlib import Path
plate = {
  "schema": "cubalc.universal_iter.v1",
  "to": "NexusCore",
  "from": "cubalc_universal_loop",
  "ts": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
  "iter": $ITER,
  "algocube_digit": $DIGIT,
  "version": "$VER",
  "pass": $PASS,
  "fail": $FAIL,
  "ok": $FAIL == 0,
  "interval_sec": 360,
  "goal": "more_universal_each_iteration",
  "hold_flash": 1,
  "law": ["pure_science", "evolve", "flow_compile"],
}
Path("$ITER_F").write_text(json.dumps(plate, indent=2) + "\n")
print(json.dumps(plate))
PY

# Soft-copy plate for ProjectNexus if present
PN="${PROJECT_NEXUS_ROOT:-/data/Workdir/voldemar/ProjectNexus}"
if [[ -d "$PN" ]]; then
  mkdir -p "$PN/var/cubalc" 2>/dev/null || true
  cp -f "$ITER_F" "$PN/var/cubalc/UNIVERSAL_ITER.json" 2>/dev/null || true
fi

log "done iter=$ITER digit=$DIGIT pass=$PASS fail=$FAIL ver=$VER"
[[ "$FAIL" -eq 0 ]]
