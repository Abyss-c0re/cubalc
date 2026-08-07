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
LDFLAGS ?= $(LDFLAGS_SYS)

CORE_SRC = \
	src/cubalc_main.c src/cubalc_core.c src/cubalc_algocube.c src/cubalc_evolve.c \
	src/cubalc_smx.c src/cubalc_cubechain.c src/cubalc_hw.c \
	src/cubalc_translate.c src/cubalc_hostops.c src/cubalc_async.c \
	src/cubalc_isa.c src/cubalc_jit.c

LANG_SRC = \
	src/lang/lang_core.c \
	src/lang/lang_ops_core.c \
	src/lang/lang_ops_smx.c \
	src/lang/lang_ops_toc.c \
	src/lang/lang_ops_stack.c \
	src/lang/lang_ops_dual.c \
	src/lang/lang_ops_math.c \
	src/lang/lang_ops_bit.c \
	src/lang/lang_ops_cell.c \
	src/lang/lang_ops_flow.c \
	src/lang/lang_parse.c \
	src/lang/lang_run.c

SRC = $(CORE_SRC) $(LANG_SRC)

HDR = \
	include/cubalc.h include/cubalc_law.h include/cubalc_platform.h \
	include/cubalc_algocube.h include/cubalc_evolve.h include/cubalc_smx.h \
	include/cubalc_cubechain.h include/cubalc_hw.h include/cubalc_lang.h \
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

.PHONY: all clean test law install human demo peers oversee jit-test \
	evolve evolve-loop showcase science universal-iter modular-check

all: $(BIN)

$(BIN): $(SRC) $(HDR)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)
	@echo "built $@ ($(CUBALC_TARGET))"

clean:
	rm -f $(BIN)

modular-check:
	@test -f src/lang/lang_parse.c && test -f include/lang/cubalc_lang_internal.h
	@echo modular-ok

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
