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
            113_sandi_sxori; do
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
