# CubalC — pure C COP/flow runtime (multiplatform)
# Law: cube is SoT · flow before compile · devices free · HTTP never required

CC       ?= gcc
PREFIX   ?= $(HOME)/.local
USE_OPENCL ?= 0
BUILD    ?= out

UNAME_S := $(shell uname -s 2>/dev/null || echo unknown)
ifeq ($(OS),Windows_NT)
  CUBALC_TARGET := windows
  EXE := .exe
  LDFLAGS_SYS := -lm
else ifeq ($(UNAME_S),Darwin)
  CUBALC_TARGET := darwin
  EXE :=
  LDFLAGS_SYS := -lm -lpthread
else
  CUBALC_TARGET := posix
  EXE :=
  LDFLAGS_SYS := -lm -lpthread
endif

CFLAGS_COMMON := -O2 -Wall -Wextra -std=c11 -Iinclude
ifeq ($(CUBALC_CROSS),)
  ifneq ($(CUBALC_TARGET),windows)
    CFLAGS_COMMON += -march=native
  endif
endif
CFLAGS  ?= $(CFLAGS_COMMON)
CUBEVIZ ?= ../cubebrain/modules/viz

LDFLAGS ?= $(LDFLAGS_SYS)

CORE_SRC = \
	src/cubalc_main.c src/cubalc_cmd_doctor.c src/cubalc_cmd_selftest.c src/cubalc_cmd_run.c src/cubalc_cmd_stdlib.c src/cubalc_cmd_vars.c src/cubalc_cmd_formrel.c src/cubalc_cmd_contracts.c src/cubalc_cmd_whyguide.c src/cubalc_cmd_cliinfo.c src/cubalc_cmd_argmap.c src/cubalc_cmd_endarg.c src/cubalc_cmd_findarg.c src/cubalc_cmd_libmatch.c src/cubalc_cmd_recipe.c src/cubalc_cmd_checkdeps.c src/cubalc_cmd_freshlibs.c src/cubalc_cmd_libage.c src/cubalc_cmd_examples.c src/cubalc_cmd_guide.c src/cubalc_cmd_topic.c src/cubalc_cmd_runsnip.c src/cubalc_cmd_formsfor.c src/cubalc_cmd_snip.c src/cubalc_cmd_open.c src/cubalc_cmd_matchtopics.c src/cubalc_cmd_tips.c src/cubalc_cmd_topichint.c src/cubalc_cmd_formhints.c src/cubalc_cmd_errrun.c src/cubalc_cmd_cat.c src/cubalc_cmd_usability.c src/cubalc_cmd_forms.c src/cubalc_cmd_init.c src/cubalc_cmd_which.c src/cubalc_cmd_search.c src/cubalc_cmd_plate.c src/cubalc_cmd_plate_flat.c src/cubalc_cmd_plate_nest.c src/cubalc_cmd_plate_mut.c src/cubalc_cmd_plate_sync.c src/cubalc_cmd_plate_view.c src/cubalc_cmd_plate_alias.c src/cubalc_cmd_smx_exchange.c src/cubalc_cmd_smx_bus.c src/cubalc_cmd_smx_selftest.c src/cubalc_cmd_protect.c src/cubalc_cmd_showcase.c src/cubalc_cmd_genesis.c src/cubalc_cmd_sync.c src/cubalc_run_util.c src/cubalc_core.c src/cubalc_algocube.c src/cubalc_evolve.c \
	src/cubalc_smx.c src/cubalc_cubechain.c src/cubalc_hw.c src/cubalc_eeg.c src/cubalc_sot.c \
	src/cubalc_viz_matrix.c \
	src/cubalc_translate.c src/cubalc_hostops.c src/cubalc_hostops_tree_mut.c src/cubalc_hostops_tree_query.c src/cubalc_hostops_tree_edit.c src/cubalc_hostops_tree_lines.c src/cubalc_hostops_tree_find.c src/cubalc_hostops_tree_linefind.c src/cubalc_hostops_chat.c src/cubalc_hostops_json_delta.c \
	src/cubalc_hostops_json_leaf.c src/cubalc_hostops_json_leafdiff.c src/cubalc_hostops_json_path.c src/cubalc_hostops_json_flat.c src/cubalc_hostops_json_core.c src/cubalc_hostops_stat_time.c src/cubalc_hostops_env.c src/cubalc_hostops_inode.c src/cubalc_hostops_link.c src/cubalc_hostops_ownermode.c src/cubalc_hostops_json_agg.c \
	src/cubalc_hostops_json_mut.c src/cubalc_hostops_json_query.c src/cubalc_async.c \
	src/cubalc_isa.c src/cubalc_jit.c \
	src/cubalc_quantum.c src/cubalc_dim5.c src/cubalc_relativity.c src/cubalc_chaos.c src/cubalc_science_loop.c

LANG_SRC = \
	src/lang/lang_core.c \
	src/lang/lang_ops_bit.c \
	src/lang/lang_ops_cell.c \
	src/lang/lang_ops_cell_nibble4.c \
	src/lang/lang_ops_cell_width8.c \
	src/lang/lang_ops_cell_bitwise_w.c \
	src/lang/lang_ops_cell_w16rot.c \
	src/lang/lang_ops_cell_shift32.c \
	src/lang/lang_ops_cell_rot32.c \
	src/lang/lang_ops_cell_metrics.c \
	src/lang/lang_ops_cell_rng.c \
	src/lang/lang_ops_cell_energy.c \
	src/lang/lang_ops_cell_matbits.c \
	src/lang/lang_ops_cell_matscan.c \
	src/lang/lang_ops_cell_matmetric.c \
	src/lang/lang_ops_cell_matruns.c \
	src/lang/lang_ops_cell_matfind.c \
	src/lang/lang_ops_cell_matrange.c \
	src/lang/lang_ops_cell_matword.c \
	src/lang/lang_ops_cell_matpath.c \
	src/lang/lang_ops_cell_matfill.c \
	src/lang/lang_ops_cell_matalgebra.c \
	src/lang/lang_ops_cell_matrel.c \
	src/lang/lang_ops_cell_matmux.c \
	src/lang/lang_ops_cell_matbrain.c \
	src/lang/lang_ops_cell_energyflow.c \
	src/lang/lang_ops_cell_runtime.c \
	src/lang/lang_ops_cell_include_query.c \
	src/lang/lang_ops_cell_include.c src/lang/lang_ops_cell_libcat.c src/lang/lang_ops_cell_libpick.c src/lang/lang_ops_cell_libread.c src/lang/lang_ops_cell_libmeta.c src/lang/lang_ops_cell_libage.c \
	src/lang/lang_ops_cell_preload.c \
	src/lang/lang_ops_cell_matlogic.c \
	src/lang/lang_ops_core.c \
	src/lang/lang_ops_core_agent.c \
	src/lang/lang_ops_core_assert.c \
	src/lang/lang_ops_core_async.c \
	src/lang/lang_ops_core_clearerr.c \
	src/lang/lang_ops_core_endarg.c \
	src/lang/lang_ops_core_flag.c \
	src/lang/lang_ops_core_flagpath.c \
	src/lang/lang_ops_core_cop.c \
	src/lang/lang_ops_core_ctrl.c \
	src/lang/lang_ops_core_discover.c \
	src/lang/lang_ops_core_flat.c \
	src/lang/lang_ops_core_flat_bag.c \
	src/lang/lang_ops_core_bag_push.c \
	src/lang/lang_ops_core_flat_get.c \
	src/lang/lang_ops_core_flat_has.c \
	src/lang/lang_ops_core_flat_mut.c \
	src/lang/lang_ops_core_flat_nest_agg.c \
	src/lang/lang_ops_core_flat_nest_kv.c \
	src/lang/lang_ops_core_flat_nest_merge.c \
	src/lang/lang_ops_core_flat_nest_pick.c \
	src/lang/lang_ops_core_flat_nest_sort.c \
	src/lang/lang_ops_core_flat_nest_sortobj.c \
	src/lang/lang_ops_core_flat_nest_sum.c \
	src/lang/lang_ops_core_flat_nest_thr.c \
	src/lang/lang_ops_core_flat_nest_top.c \
	src/lang/lang_ops_core_flat_nestq.c \
	src/lang/lang_ops_core_flat_path.c \
	src/lang/lang_ops_core_flat_query.c \
	src/lang/lang_ops_core_flat_stats.c \
	src/lang/lang_ops_core_forms.c \
	src/lang/lang_ops_core_help.c \
	src/lang/lang_ops_core_let.c \
	src/lang/lang_ops_core_plate.c \
	src/lang/lang_ops_core_plate_ext.c \
	src/lang/lang_ops_core_plate_nest.c \
	src/lang/lang_ops_core_print.c \
	src/lang/lang_ops_core_sys.c \
	src/lang/lang_ops_dual.c \
	src/lang/lang_ops_dual_pack.c \
	src/lang/lang_ops_dual_clip.c \
	src/lang/lang_ops_dual_field32.c \
	src/lang/lang_ops_dual_unary32.c \
	src/lang/lang_ops_dual_unary16.c \
	src/lang/lang_ops_dual_unary8.c \
	src/lang/lang_ops_dual_unary4.c src/lang/lang_ops_dual_pair.c src/lang/lang_ops_dual_fieldrot.c src/lang/lang_ops_dual_immshift.c src/lang/lang_ops_dual_imm_alu.c src/lang/lang_ops_dual_imm_sat.c src/lang/lang_ops_dual_imm_div.c src/lang/lang_ops_dual_imm_bit.c src/lang/lang_ops_dual_imm_range.c src/lang/lang_ops_dual_imm_cmp.c src/lang/lang_ops_dual_bitpath.c src/lang/lang_ops_dual_bitmet.c src/lang/lang_ops_dual_datapath.c src/lang/lang_ops_dual_imm_field.c src/lang/lang_ops_dual_imm_wbit.c src/lang/lang_ops_dual_imm_hbit.c src/lang/lang_ops_dual_imm_harith.c src/lang/lang_ops_dual_imm_hdiv.c src/lang/lang_ops_dual_imm_hcmp.c src/lang/lang_ops_dual_imm_hord.c src/lang/lang_ops_dual_imm_hsord.c src/lang/lang_ops_dual_imm_hsmm.c src/lang/lang_ops_dual_imm_hsclamp.c src/lang/lang_ops_dual_imm_hpred.c src/lang/lang_ops_dual_imm_h0ord.c src/lang/lang_ops_dual_imm_barith.c src/lang/lang_ops_dual_imm_bdiv.c src/lang/lang_ops_dual_imm_bcmp.c src/lang/lang_ops_dual_imm_bord.c src/lang/lang_ops_dual_imm_bsord.c src/lang/lang_ops_dual_imm_bsmm.c src/lang/lang_ops_dual_imm_bsclamp.c src/lang/lang_ops_dual_imm_bpred.c src/lang/lang_ops_dual_imm_b0ord.c \
	src/lang/lang_ops_flow.c \
	src/lang/lang_ops_flow_case.c \
	src/lang/lang_ops_flow_class.c \
	src/lang/lang_ops_flow_ctrl.c \
	src/lang/lang_ops_flow_fget.c \
	src/lang/lang_ops_flow_field.c \
	src/lang/lang_ops_flow_field_b.c \
	src/lang/lang_ops_flow_fn.c \
	src/lang/lang_ops_flow_for.c \
	src/lang/lang_ops_flow_fstr.c \
	src/lang/lang_ops_flow_fstr_a.c \
	src/lang/lang_ops_flow_fstr_bagf.c \
	src/lang/lang_ops_flow_fstr_catfall.c \
	src/lang/lang_ops_flow_fstr_core.c \
	src/lang/lang_ops_flow_fstr_cutf.c \
	src/lang/lang_ops_flow_fstr_oopchain.c \
	src/lang/lang_ops_flow_fstr_oopmeta.c \
	src/lang/lang_ops_flow_fstr_slice.c \
	src/lang/lang_ops_flow_fstr_fieldbag.c \
	src/lang/lang_ops_flow_fstr_b.c \
	src/lang/lang_ops_flow_fstr_kinship.c \
	src/lang/lang_ops_flow_fstr_cousin.c \
	src/lang/lang_ops_flow_fstr_dumpobj.c \
	src/lang/lang_ops_flow_fstr_listobjs.c \
	src/lang/lang_ops_flow_fstr_context.c \
	src/lang/lang_ops_flow_fstr_info.c \
	src/lang/lang_ops_flow_fstr_lifecycle.c \
	src/lang/lang_ops_flow_fstr_hasexist.c \
	src/lang/lang_ops_flow_fstr_hasmeth.c \
	src/lang/lang_ops_flow_fstr_hasfield.c \
	src/lang/lang_ops_flow_ifcmp.c \
	src/lang/lang_ops_flow_ifbag.c \
	src/lang/lang_ops_flow_ifjson.c \
	src/lang/lang_ops_flow_ifflag.c \
	src/lang/lang_ops_flow_ifsize.c \
	src/lang/lang_ops_flow_ifmtime.c \
	src/lang/lang_ops_flow_iffresh.c \
	src/lang/lang_ops_flow_ifeqfile.c \
	src/lang/lang_ops_flow_iflen.c \
	src/lang/lang_ops_flow_iflines.c \
	src/lang/lang_ops_flow_ifnum.c \
	src/lang/lang_ops_flow_ifzero.c \
	src/lang/lang_ops_flow_ifcanread.c \
	src/lang/lang_ops_flow_ifjsoneq.c \
	src/lang/lang_ops_flow_ifobj.c \
	src/lang/lang_ops_flow_iffield.c \
	src/lang/lang_ops_flow_ifenv.c \
	src/lang/lang_ops_flow_iffile.c \
	src/lang/lang_ops_flow_ifbetween.c \
	src/lang/lang_ops_flow_iffn.c \
	src/lang/lang_ops_flow_ifvar.c \
	src/lang/lang_ops_flow_oop.c \
	src/lang/lang_ops_flow_scene.c \
	src/lang/lang_ops_flow_send.c \
	src/lang/lang_ops_math.c \
	src/lang/lang_ops_math_nbit.c \
	src/lang/lang_ops_math_numthy.c \
	src/lang/lang_ops_math_mod.c \
	src/lang/lang_ops_math_bitpath.c \
	src/lang/lang_ops_math_unary.c \
	src/lang/lang_ops_math_stack_sci.c \
	src/lang/lang_ops_math_field32n.c \
	src/lang/lang_ops_math_field16n.c \
	src/lang/lang_ops_math_field8n.c \
	src/lang/lang_ops_math_field4n.c \
	src/lang/lang_ops_math_fieldacc.c \
	src/lang/lang_ops_sched.c \
	src/lang/lang_ops_smx.c \
	src/lang/lang_ops_smx_cascade.c \
	src/lang/lang_ops_smx_core.c \
	src/lang/lang_ops_smx_cosmos.c \
	src/lang/lang_ops_smx_galaxy.c \
	src/lang/lang_ops_smx_land.c \
	src/lang/lang_ops_smx_pneuma.c \
	src/lang/lang_ops_smx_pneuma_a.c \
	src/lang/lang_ops_smx_pneuma_sphere0.c \
	src/lang/lang_ops_smx_pneuma_sphere1.c \
	src/lang/lang_ops_smx_pneuma_sphere2.c \
	src/lang/lang_ops_smx_pneuma_neural.c \
	src/lang/lang_ops_smx_pneuma_ward.c \
	src/lang/lang_ops_smx_pneuma_glia.c \
	src/lang/lang_ops_smx_pneuma_gliax.c \
	src/lang/lang_ops_smx_pneuma_vaso.c \
	src/lang/lang_ops_smx_pneuma_hepatic.c \
	src/lang/lang_ops_smx_pneuma_b.c \
	src/lang/lang_ops_smx_water.c \
	src/lang/lang_ops_smx_zodiac.c \
	src/lang/lang_ops_stack.c \
	src/lang/lang_ops_sys_bulk.c \
	src/lang/lang_ops_sys_bulk_mut.c \
	src/lang/lang_ops_sys_cli.c \
	src/lang/lang_ops_sys_codec.c \
	src/lang/lang_ops_sys_env.c \
	src/lang/lang_ops_sys_files.c \
	src/lang/lang_ops_sys_files_bulk.c \
	src/lang/lang_ops_sys_files_line.c \
	src/lang/lang_ops_sys_files_match.c \
	src/lang/lang_ops_sys_files_cwd.c \
	src/lang/lang_ops_sys_files_net.c \
	src/lang/lang_ops_sys_grep.c \
	src/lang/lang_ops_sys_hostmeta.c \
	src/lang/lang_ops_sys_identity.c \
	src/lang/lang_ops_sys_json.c \
	src/lang/lang_ops_sys_kv.c \
	src/lang/lang_ops_sys_kvfilter.c \
	src/lang/lang_ops_sys_kvmath.c \
	src/lang/lang_ops_sys_net.c \
	src/lang/lang_ops_sys_path_fs.c \
	src/lang/lang_ops_sys_path_writeout.c \
	src/lang/lang_ops_sys_path_tree.c \
	src/lang/lang_ops_sys_path_sync.c \
	src/lang/lang_ops_sys_path_link.c \
	src/lang/lang_ops_sys_path_util.c \
	src/lang/lang_ops_sys_plate.c \
	src/lang/lang_ops_sys_plate_expand.c \
	src/lang/lang_ops_sys_stat.c \
	src/lang/lang_ops_sys_streq.c \
	src/lang/lang_ops_sys_string.c \
	src/lang/lang_ops_sys_time_walk.c \
	src/lang/lang_ops_sys_time_date.c \
	src/lang/lang_ops_sys_bagmut.c \
	src/lang/lang_ops_sys_human.c \
	src/lang/lang_ops_sys_rand.c \
	src/lang/lang_ops_sys_numagg.c \
	src/lang/lang_ops_sys_bagshape.c \
	src/lang/lang_ops_sys_bagmap.c \
	src/lang/lang_ops_sys_bagwin.c \
	src/lang/lang_ops_sys_baglen.c \
	src/lang/lang_ops_sys_textio.c \
	src/lang/lang_ops_sys_runtime.c \
	src/lang/lang_ops_sys_argjoin.c \
	src/lang/lang_ops_sys_misc.c \
	src/lang/lang_ops_sys_wait.c \
	src/lang/lang_ops_toc.c \
	src/lang/lang_ops_toc_cell.c \
	src/lang/lang_ops_toc_imm.c \
	src/lang/lang_ops_toc_field.c \
	src/lang/lang_ops_toc_extclip.c \
	src/lang/lang_ops_toc_mask.c \
	src/lang/lang_ops_toc_bitops.c \
	src/lang/lang_ops_toc_alu.c \
	src/lang/lang_ops_toc_mod.c \
	src/lang/lang_ops_toc_revsat.c \
	src/lang/lang_ops_toc_bwshift.c \
	src/lang/lang_ops_toc_unary.c \
	src/lang/lang_ops_toc_cmp.c \
	src/lang/lang_ops_toc_metrics.c \
	src/lang/lang_parse.c \
	src/lang/lang_run.c



SRC = $(CORE_SRC) $(LANG_SRC)
ifneq ($(wildcard $(CUBEVIZ)/cube_viz_path.c),)
  SRC += $(CUBEVIZ)/cube_viz_path.c
  CFLAGS += -I$(CUBEVIZ) -DCUBALC_HAS_CUBEVIZ
endif

HDR = \
	include/cubalc.h \
	include/cubalc_cmd_plate.h include/cubalc_cmd_plate_flat.h include/cubalc_cmd_plate_nest.h include/cubalc_cmd_plate_mut.h include/cubalc_cmd_plate_sync.h include/cubalc_cmd_plate_view.h include/cubalc_cmd_plate_alias.h \
	include/cubalc_cmd_search.h \
	include/cubalc_cmd_which.h \
	include/cubalc_cmd_init.h \
	include/cubalc_cmd_forms.h \
	include/cubalc_cmd_doctor.h \
	include/cubalc_cmd_selftest.h include/cubalc_cmd_usability.h \
	include/cubalc_cmd_smx_exchange.h \
	include/cubalc_cmd_smx_bus.h \
	include/cubalc_cmd_smx_selftest.h \
	include/cubalc_cmd_protect.h \
	include/cubalc_cmd_showcase.h \
	include/cubalc_cmd_genesis.h \
	include/cubalc_cmd_sync.h \
	include/cubalc_run_util.h \
	include/cubalc_cmd_law.h \
	include/cubalc_algocube.h include/cubalc_evolve.h include/cubalc_smx.h \
	include/cubalc_cubechain.h include/cubalc_hw.h include/cubalc_eeg.h include/cubalc_sot.h \
	include/cubalc_viz_matrix.h include/cubalc_lang.h \
	include/lang/cubalc_lang_internal.h \
	include/cubalc_translate.h include/cubalc_hostops.h include/cubalc_async.h \
	include/cubalc_isa.h include/cubalc_jit.h

ifeq ($(USE_OPENCL),1)
  CFLAGS += -DCUBALC_HAVE_OPENCL
  ifeq ($(CUBALC_TARGET),darwin)
    LDFLAGS += -framework OpenCL
  else
    LDFLAGS += -lOpenCL
  endif
endif

BIN := $(BUILD)/cubalc$(EXE)
SOLVER_BIN := $(BUILD)/matrix_harmonic_solver$(EXE)
EEG_BIN := $(BUILD)/eeg_matrix_stream$(EXE)

# Light sources for standalone harmonic solver (no monorepo main/lang)
SOLVER_SRC = \
	tools/matrix_harmonic_solver.c \
	src/cubalc_core.c src/cubalc_algocube.c src/cubalc_hw.c

EEG_SRC = \
	tools/eeg_matrix_stream.c \
	src/cubalc_core.c src/cubalc_algocube.c src/cubalc_hw.c src/cubalc_eeg.c src/cubalc_sot.c \
	src/cubalc_viz_matrix.c
ifneq ($(wildcard $(CUBEVIZ)/cube_viz_path.c),)
  EEG_SRC += $(CUBEVIZ)/cube_viz_path.c
endif

.PHONY: all clean test law install human demo peers oversee jit-test \
	evolve evolve-loop showcase science universal-iter modular-check \
	matrix-solver eeg-stream

all: $(BIN)

$(BIN): $(SRC) $(HDR)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)
	@echo "built $@ ($(CUBALC_TARGET))"

# Standalone matrix harmonic solver — CPU multi-thread + optional OpenCL
matrix-solver: $(SOLVER_BIN)

$(SOLVER_BIN): $(SOLVER_SRC) $(HDR)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(SOLVER_SRC) $(LDFLAGS)
	@echo "built $@ backend-ready ($(CUBALC_TARGET) USE_OPENCL=$(USE_OPENCL))"

# Real-time EEG → State Matrix streamer (standalone)
eeg-stream: $(EEG_BIN)

$(EEG_BIN): $(EEG_SRC) $(HDR) include/cubalc_eeg.h include/cubalc_sot.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(EEG_SRC) $(LDFLAGS)
	@echo "built $@ EEG→matrix ($(CUBALC_TARGET))"

clean:
	rm -f $(BIN) $(SOLVER_BIN) $(EEG_BIN)

modular-check:
	@test -f src/lang/lang_parse.c && test -f include/lang/cubalc_lang_internal.h
	@test -f src/lang/lang_ops_core.c && test -f src/lang/lang_ops_core_sys.c
	@test -f src/lang/lang_ops_smx_core.c && test -f src/lang/lang_ops_smx_cascade.c
	@test -f src/lang/DO_NOT_AMALGAMATE_CORE
	@test -f src/cubalc_dim5.c && test -f include/cubalc_dim5.h
	@test -f src/cubalc_quantum.c && test -f include/cubalc_quantum.h
	@test -f src/cubalc_relativity.c && test -f include/cubalc_relativity.h
	@test -f src/cubalc_chaos.c && test -f include/cubalc_chaos.h
	@test -f src/cubalc_science_loop.c && test -f include/cubalc_science_loop.h
	@cl=$$(wc -l < src/lang/lang_ops_core.c); test $$cl -le 200 || (echo "HERESY: lang_ops_core.c=$$cl lines (thin dispatcher max 200)" >&2; exit 1)
	@sl=$$(wc -l < src/lang/lang_ops_smx.c); test $$sl -le 50 || (echo "HERESY: lang_ops_smx.c=$$sl lines (thin dispatcher max 50)" >&2; exit 1)
	@grep -q 'lang_ops_core_sys.c' Makefile
	@grep -q 'lang_ops_smx_core.c' Makefile
	@grep -q 'lang_ops_flow_ifvar.c' Makefile || true
	@if [ -x out/cubalc ]; then ./out/cubalc science prove >/dev/null; fi
	@if [ -x out/cubalc ]; then ./out/cubalc run programs/proof/2501_sys_planes_modular_smoke.cubalc >/dev/null; fi
	@if [ -x out/cubalc ] && [ -f programs/proof/2502_quantum_qasm_depth.sh ]; then bash programs/proof/2502_quantum_qasm_depth.sh >/dev/null; fi
	@if [ -x out/cubalc ] && [ -f programs/proof/2503_dim5_seed_mesh.sh ]; then bash programs/proof/2503_dim5_seed_mesh.sh >/dev/null; fi
	@if [ -x out/cubalc ] && [ -f programs/proof/2504_relativity_viz.sh ]; then bash programs/proof/2504_relativity_viz.sh >/dev/null; fi
	@if [ -x out/cubalc ] && [ -f programs/proof/2505_chaos_attractor.sh ]; then bash programs/proof/2505_chaos_attractor.sh >/dev/null; fi
	@if [ -x out/cubalc ] && [ -f programs/proof/2506_science_pack_quartet.sh ]; then bash programs/proof/2506_science_pack_quartet.sh >/dev/null; fi
	@$(MAKE) --no-print-directory science-guard
	@echo modular-ok thin-core thin-smx science-planes sys-smoke quantum-qasm dim5-mesh rel-viz chaos-attr pack-quartet science-guard

thin-guard:
	@cl=$$(wc -l < src/lang/lang_ops_core.c); sl=$$(wc -l < src/lang/lang_ops_smx.c); \
	  echo "thin-guard core=$$cl smx=$$sl"; \
	  test $$cl -le 200 && test $$sl -le 50 && test -f src/lang/DO_NOT_AMALGAMATE_CORE

# hold_ceilings: science modular planes stay lean (anti bloat after B/E/C/D streak)
science-guard:
	@q=$$(wc -l < src/cubalc_quantum.c); d=$$(wc -l < src/cubalc_dim5.c); \
	 r=$$(wc -l < src/cubalc_relativity.c); c=$$(wc -l < src/cubalc_chaos.c); \
	 l=$$(wc -l < src/cubalc_science_loop.c); \
	 echo "science-guard q=$$q dim5=$$d rel=$$r chaos=$$c loop=$$l"; \
	 test $$q -le 500 || (echo "HERESY: cubalc_quantum.c=$$q (max 500)" >&2; exit 1); \
	 test $$d -le 300 || (echo "HERESY: cubalc_dim5.c=$$d (max 300)" >&2; exit 1); \
	 test $$r -le 300 || (echo "HERESY: cubalc_relativity.c=$$r (max 300)" >&2; exit 1); \
	 test $$c -le 300 || (echo "HERESY: cubalc_chaos.c=$$c (max 300)" >&2; exit 1); \
	 test $$l -le 600 || (echo "HERESY: cubalc_science_loop.c=$$l (max 600)" >&2; exit 1)

	@cl=$$(wc -l < src/lang/lang_ops_core.c); sl=$$(wc -l < src/lang/lang_ops_smx.c); \
	 f=$$(wc -l < src/lang/lang_ops_flow.c); m=$$(wc -l < src/cubalc_main.c); \
	 du=$$(wc -l < src/lang/lang_ops_dual.c); ce=$$(wc -l < src/lang/lang_ops_cell.c); \
	 ho=$$(wc -l < src/cubalc_hostops.c); \
	 echo "G-ceilings flow=$$f main=$$m dual=$$du cell=$$ce hostops=$$ho core=$$cl smx=$$sl"; \
	 test $$f -le 7000 || (echo "WARN: flow=$$f >7000 prefer A3" >&2; exit 1); \
	 test $$m -le 10000 || (echo "WARN: main=$$m >10000 prefer F_main" >&2; exit 1); \
	 test $$du -le 10000 && test $$ce -le 10000 && test $$ho -le 10000 || \
	   (echo "WARN: dual/cell/hostops >10k prefer G peel" >&2; exit 1)

.PHONY: test science-guard
test: all
	@bash tests/lang_suite.sh
	./$(BIN) smx-selftest
	@./$(BIN) protect board || true
	@if [ -f programs/proof/08_peer_fold.cubalc ]; then ./$(BIN) run programs/proof/08_peer_fold.cubalc; fi
	@if [ -x tests/jit_suite.sh ]; then bash tests/jit_suite.sh; fi
	@CUBALC_PEER0_DIGIT=5 CUBALC_PEER1_DIGIT=3 ./$(BIN) peers || true
	@./$(BIN) evolve --once

law: all
	./$(BIN) law

evolve: all
	CUBALC_STATE=$${CUBALC_STATE:-$(CURDIR)/state} ./$(BIN) evolve --once

evolve-loop: all
	CUBALC_STATE=$${CUBALC_STATE:-$(CURDIR)/state} ./$(BIN) evolve-loop --hz $${EVOLVE_HZ:-5}

peers oversee: all
	@bash scripts/peer_fold.sh

human: all
	@CUBALC_HUMAN=1 CUBALC_ASCII=1 ./$(BIN) run programs/hello_cube.cubalc

demo: human
	@echo "viz JSON: $$PWD/state/cubalc_viz_frame.json"

jit-test: all
	@bash tests/jit_suite.sh

install: all
	install -d $(PREFIX)/bin
	install -m755 $(BIN) $(PREFIX)/bin/cubalc$(EXE)
	@echo "installed $(PREFIX)/bin/cubalc$(EXE)"

.PHONY: sync
sync:
	@ROOT=$$(cd $$(CURDIR)/../.. && pwd); bash "$$ROOT/scripts/sync_cubalc_product.sh"

showcase: all
	CUBALC_STATE=$${CUBALC_STATE:-$(CURDIR)/state} ./$(BIN) showcase

science: all
	@bash programs/science/run_demos.sh

universal-iter: all
	@bash scripts/universal_iter.sh

# Language proof plate (live curated suite)
prove: all
	./out/cubalc selftest > state/LANGUAGE_PROOF.json
	@echo "wrote state/LANGUAGE_PROOF.json"
