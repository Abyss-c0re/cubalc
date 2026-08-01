# CubalC — AI language runtime (agents first; humans optional)
CC ?= gcc
PREFIX ?= $(HOME)/.local
USE_OPENCL ?= 0
CFLAGS ?= -O2 -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude -march=native
LDFLAGS ?= -lm -lpthread
SRC = src/cubalc_main.c src/cubalc_core.c src/cubalc_algocube.c src/cubalc_evolve.c \
      src/cubalc_smx.c src/cubalc_cubechain.c src/cubalc_hw.c src/cubalc_lang.c \
      src/cubalc_translate.c src/cubalc_hostops.c src/cubalc_async.c src/cubalc_isa.c \
      src/cubalc_jit.c
HDR = include/cubalc.h include/cubalc_law.h include/cubalc_algocube.h include/cubalc_evolve.h \
      include/cubalc_smx.h include/cubalc_cubechain.h include/cubalc_hw.h include/cubalc_lang.h \
      include/cubalc_translate.h include/cubalc_hostops.h include/cubalc_async.h \
      include/cubalc_isa.h include/cubalc_jit.h

ifeq ($(USE_OPENCL),1)
  CFLAGS += -DCUBALC_HAVE_OPENCL
  LDFLAGS += -lOpenCL
endif

.PHONY: all clean test law install human demo peers oversee jit-test evolve evolve-loop

all: out/cubalc

out/cubalc: $(SRC) $(HDR)
	@mkdir -p out
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

clean:
	rm -f out/cubalc

test: all
	@bash tests/lang_suite.sh
	./out/cubalc smx-selftest
	@if [[ -f programs/proof/08_peer_fold.cubalc ]]; then ./out/cubalc run programs/proof/08_peer_fold.cubalc; fi
	@if [[ -x tests/jit_suite.sh ]]; then bash tests/jit_suite.sh; fi
	@CUBALC_PEER0_DIGIT=5 CUBALC_PEER1_DIGIT=3 ./out/cubalc peers || true
	@./out/cubalc evolve --once

law: all
	./out/cubalc law

# Braincube solves · algocube optimizes — pure C continuous loop
evolve: all
	CUBALC_STATE=$${CUBALC_STATE:-$(CURDIR)/state} ./out/cubalc evolve --once

evolve-loop: all
	CUBALC_STATE=$${CUBALC_STATE:-$(CURDIR)/state} ./out/cubalc evolve-loop --hz $${EVOLVE_HZ:-5}

peers oversee: all
	@bash scripts/peer_fold.sh

human: all
	@CUBALC_HUMAN=1 CUBALC_ASCII=1 ./out/cubalc run programs/hello_cube.cubalc

demo: human
	@echo "viz JSON: $$PWD/state/cubalc_viz_frame.json"

jit-test: all
	@bash tests/jit_suite.sh

install: all
	install -d $(PREFIX)/bin
	install -m755 out/cubalc $(PREFIX)/bin/cubalc
	@echo "installed $(PREFIX)/bin/cubalc"

.PHONY: sync
sync:
	@ROOT=$$(cd $$(CURDIR)/../.. && pwd); bash "$$ROOT/scripts/sync_cubalc_product.sh"
