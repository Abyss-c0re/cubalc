# CubalC — AI language runtime (agents first; humans optional)
CC ?= gcc
PREFIX ?= $(HOME)/.local
USE_OPENCL ?= 0
CFLAGS ?= -O2 -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude -march=native
LDFLAGS ?= -lm -lpthread
SRC = src/cubalc_main.c src/cubalc_core.c src/cubalc_smx.c src/cubalc_cubechain.c \
      src/cubalc_hw.c src/cubalc_lang.c src/cubalc_translate.c src/cubalc_hostops.c \
      src/cubalc_async.c
HDR = include/cubalc.h include/cubalc_law.h include/cubalc_smx.h include/cubalc_cubechain.h \
      include/cubalc_hw.h include/cubalc_lang.h include/cubalc_translate.h \
      include/cubalc_hostops.h include/cubalc_async.h

ifeq ($(USE_OPENCL),1)
  CFLAGS += -DCUBALC_HAVE_OPENCL
  LDFLAGS += -lOpenCL
endif

.PHONY: all clean test law install human demo peers oversee

all: out/cubalc

out/cubalc: $(SRC) $(HDR)
	@mkdir -p out
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

clean:
	rm -f out/cubalc
	rm -rf state

# Core tests: no HTTP required
test: all
	@bash tests/lang_suite.sh
	./out/cubalc smx-selftest
	./out/cubalc smx-exchange
	./out/cubalc smx-bus prove
	./out/cubalc smx-bus prove-tcp 17733
	./out/cubalc run programs/proof/08_peer_fold.cubalc
	./out/cubalc run programs/proof/09_smx_manifest.cubalc
	./out/cubalc run programs/proof/10_p2p_cubalc.cubalc
	./out/cubalc run programs/p2p/mesh_local.cubalc
	./out/cubalc run programs/manifest_smx.cubalc
	./out/cubalc run programs/harmonize_hive.cubalc
	CUBALC_PEER0_DIGIT=5 CUBALC_PEER1_DIGIT=3 ./out/cubalc peers
	./out/cubalc law

law: all
	./out/cubalc law

# Host adapter: optional peer env → peer_fold.cubalc (not language hardcode)
peers oversee: all
	@bash scripts/peer_fold.sh

# Human-friendly real cubes (optional)
human: all
	@CUBALC_HUMAN=1 CUBALC_ASCII=1 ./out/cubalc run programs/hello_cube.cubalc

demo: human
	@echo "viz JSON: $$PWD/state/cubalc_viz_frame.json"
	@echo "open tools/cube_view.html in a browser for optional 3D-ish cubes"

install: all
	install -d $(PREFIX)/bin
	install -m755 out/cubalc $(PREFIX)/bin/cubalc
	@echo "installed $(PREFIX)/bin/cubalc"
