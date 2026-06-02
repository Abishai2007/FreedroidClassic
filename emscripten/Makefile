# FreedroidClassic — Emscripten / WebAssembly build
#
# ── Quick start (Homebrew on macOS) ─────────────────────────────────────────
#
#   brew install emscripten          # provides emcc / emcmake / emmake
#   make sdl3-deps                   # clone & cross-compile SDL3 + SDL3_image
#   make                             # build freedroid.html / .js / .wasm
#   make serve                       # serve on http://localhost:8080
#
# ── Variables you can override ───────────────────────────────────────────────
#
#   SDL3_BRANCH   SDL3 git tag/branch to check out  (default: release-3.2.x)
#   WASM_LIBS     install prefix for WASM libs       (default: ./wasm-libs)
#   OUT           output filename                    (default: freedroid.html)
#
# ── Notes ────────────────────────────────────────────────────────────────────
#
#   • SDL3_mixer is NOT linked; nosound.c supplies silent no-op stubs.
#     The game runs without audio; a future step can add SDL3_mixer for WASM.
#
#   • -sASYNCIFY lets blocking C calls (wait_for_key_pressed, menus,
#     the takeover mini-game) yield to the browser without freezing the tab.
#     It increases binary size by ~1.5×; this is the accepted trade-off for
#     a first working web build.
#
# ─────────────────────────────────────────────────────────────────────────────

EMCC    := emcc
EMCMAKE := emcmake cmake
EMMAKE  := emmake make

SDL3_BRANCH     ?= release-3.2.x
SDL3_IMG_BRANCH ?= release-3.2.x
WASM_LIBS       ?= $(CURDIR)/wasm-libs
SDL3_SRC        := $(CURDIR)/sdl3-src
SDL3_IMG_SRC    := $(CURDIR)/sdl3-image-src
BUILD           := build
OUT             := freedroid.html

# ── Source files ─────────────────────────────────────────────────────────────
SRC := ../src

SRCS_COMMON := \
	BFont.c bullet.c enemy.c fd_memory.c graphics.c highscore.c \
	influ.c init.c input.c level_editor.c main.c map.c menu.c \
	misc.c sdl_compat.c ship.c takeover.c text.c view.c

# nosound.c lives here; sound.c (SDL3_mixer) is excluded from the WASM build
SRCS_WASM := $(SRCS_COMMON) nosound.c

OBJS := $(addprefix $(BUILD)/,$(patsubst %.c,%.o,$(SRCS_WASM)))

# ── Compiler flags ────────────────────────────────────────────────────────────
INCLUDES := \
	-I$(SRC) \
	-I$(CURDIR) \
	-I$(WASM_LIBS)/include

# -DNO_SDL3_MIXER  prevents system.h from including SDL3_mixer headers
# -DFD_DATADIR     must match the --preload-file destination paths below
CFLAGS := \
	-std=gnu17 -O2 \
	-DNO_SDL3_MIXER \
	-DFD_DATADIR=\"/freedroid\" \
	-DLOCAL_DATADIR=\"/freedroid\" \
	$(INCLUDES)

# ── Linker / Emscripten flags ─────────────────────────────────────────────────
# --preload-file packages the data directories into freedroid.data, which the
# browser downloads alongside the .wasm.  The @/freedroid/... suffix sets the
# virtual filesystem path the game sees at runtime.
LDFLAGS := \
	-L$(WASM_LIBS)/lib \
	-lSDL3 \
	-lSDL3_image \
	-sUSE_LIBPNG=1 \
	-sUSE_LIBJPEG=1 \
	-sWASM=1 \
	-sALLOW_MEMORY_GROWTH=1 \
	-sASYNCIFY \
	-sSTACK_SIZE=262144 \
	--preload-file $(abspath ../graphics)@/freedroid/graphics \
	--preload-file $(abspath ../map)@/freedroid/map \
	--shell-file shell.html

# ── Build rules ───────────────────────────────────────────────────────────────
.PHONY: all sdl3-deps serve clean distclean help

all: $(OUT)

$(OUT): $(OBJS) | _check-emcc
	$(EMCC) $^ $(LDFLAGS) -o $@
	@echo ""
	@echo "Build complete: $(OUT)"
	@echo "Run 'make serve' then open http://localhost:8080/$(OUT)"

# nosound.c lives in this directory, not in $(SRC)
$(BUILD)/nosound.o: nosound.c | $(BUILD)
	$(EMCC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(SRC)/%.c | $(BUILD) _check-emcc
	$(EMCC) $(CFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $@

# ── Dependency build ──────────────────────────────────────────────────────────
# Downloads and cross-compiles SDL3 + SDL3_image for WASM.
# Run once; subsequent `make` calls use the cached wasm-libs/.

sdl3-deps: $(WASM_LIBS)/lib/libSDL3.a $(WASM_LIBS)/lib/libSDL3_image.a
	@echo "SDL3 WASM dependencies are up to date."

$(WASM_LIBS)/lib/libSDL3.a: | _check-emcc
	@if [ ! -d $(SDL3_SRC) ]; then \
		echo "Cloning SDL3 ($(SDL3_BRANCH))..."; \
		git clone --depth 1 --branch $(SDL3_BRANCH) \
			https://github.com/libsdl-org/SDL.git $(SDL3_SRC); \
	fi
	mkdir -p $(SDL3_SRC)/build-wasm
	cd $(SDL3_SRC)/build-wasm && \
		$(EMCMAKE) cmake .. \
			-DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_INSTALL_PREFIX=$(WASM_LIBS) \
			-DSDL_SHARED=OFF \
			-DSDL_STATIC=ON \
			-DSDL_AUDIO=ON \
			-DSDL_VIDEO=ON \
			-DSDL_RENDER=OFF && \
		$(EMMAKE) make -j$$(sysctl -n hw.ncpu 2>/dev/null || nproc) install
	@echo "SDL3 WASM library installed to $(WASM_LIBS)"

$(WASM_LIBS)/lib/libSDL3_image.a: $(WASM_LIBS)/lib/libSDL3.a
	@if [ ! -d $(SDL3_IMG_SRC) ]; then \
		echo "Cloning SDL3_image ($(SDL3_IMG_BRANCH))..."; \
		git clone --depth 1 --branch $(SDL3_IMG_BRANCH) \
			https://github.com/libsdl-org/SDL_image.git $(SDL3_IMG_SRC); \
	fi
	mkdir -p $(SDL3_IMG_SRC)/build-wasm
	cd $(SDL3_IMG_SRC)/build-wasm && \
		$(EMCMAKE) cmake .. \
			-DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_INSTALL_PREFIX=$(WASM_LIBS) \
			-DCMAKE_PREFIX_PATH=$(WASM_LIBS) \
			-DSDLIMAGE_AVIF=OFF \
			-DSDLIMAGE_WEBP=OFF \
			-DSDLIMAGE_JXL=OFF \
			-DSDLIMAGE_TIFF=OFF \
			-DSDLIMAGE_PNG=ON \
			-DSDLIMAGE_JPG=ON && \
		$(EMMAKE) make -j$$(sysctl -n hw.ncpu 2>/dev/null || nproc) install
	@echo "SDL3_image WASM library installed to $(WASM_LIBS)"

# ── Utility targets ───────────────────────────────────────────────────────────
serve:
	@echo "Serving on http://localhost:8080  (Ctrl-C to stop)"
	@echo "Note: WASM requires a proper HTTP server — opening file:// won't work."
	python3 -m http.server 8080

clean:
	rm -rf $(BUILD) $(OUT) \
		freedroid.js freedroid.wasm freedroid.data

distclean: clean
	rm -rf wasm-libs sdl3-src sdl3-image-src

help:
	@echo "FreedroidClassic Emscripten build"
	@echo ""
	@echo "Targets:"
	@echo "  sdl3-deps   Clone and cross-compile SDL3 + SDL3_image for WASM"
	@echo "  all         Build $(OUT), .js, .wasm, .data  (default)"
	@echo "  serve       Serve the directory on http://localhost:8080"
	@echo "  clean       Remove build artifacts"
	@echo "  distclean   Also remove downloaded SDL3 sources and wasm-libs/"
	@echo ""
	@echo "Variables:"
	@echo "  SDL3_BRANCH=$(SDL3_BRANCH)"
	@echo "  WASM_LIBS=$(WASM_LIBS)"
	@echo "  OUT=$(OUT)"
	@echo ""
	@echo "Prerequisites:"
	@echo "  brew install emscripten   # provides emcc, emcmake, emmake"

# Internal: fail fast if emcc is not in PATH
.PHONY: _check-emcc
_check-emcc:
	@command -v $(EMCC) >/dev/null 2>&1 || \
		{ echo ""; \
		  echo "ERROR: '$(EMCC)' not found."; \
		  echo "Install Emscripten with:  brew install emscripten"; \
		  echo ""; exit 1; }
