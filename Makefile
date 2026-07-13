# =============================================================================
# Kylin Explorer (ps5-kylin-explorer)
# OpenOrbis PS4-style homebrew package for jailbroken PS5 + Kylin Core
# =============================================================================

# --- Package identity --------------------------------------------------------
TITLE       := Kylin Explorer
VERSION     := 0.62
TITLE_ID    := KYLN00002
CONTENT_ID  := IV0000-KYLN00002_00-KYLINEXPLORER000

# --- Build knobs (override on CLI or via environment) ------------------------
#   OO_PS4_TOOLCHAIN  OpenOrbis toolchain root (required for eboot/pkg)
#   DEBUG=1           Debug symbols, -O0, KYLIN_DEBUG
#   MINIMAL_BOOT_TEST=1  Minimal eboot without UI libs
#   HOST_CXX          Host compiler for unit tests (default: clang++)
MINIMAL_BOOT_TEST ?= 0
DEBUG              ?= 0

# FSELF PAID / AUTH_INFO — keep fixed for PS5 launch compatibility
FSELF_PAID      := 0x3100000000000002
FSELF_AUTH_INFO := 000000000000000000000000001C004000FF000000000080000000000000000000000000000000000000008000400040000000000000008000000000000000080040FFFF000000F000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000

# --- Layout ------------------------------------------------------------------
#   src/            application sources
#   assets/         fonts, SVG icon sources (embedded at link time)
#   packaging/      static PKG payload (sce_sys artwork, about, sce_module)
#   tests/          host-side unit tests
#   build/          intermediates (objects, stage tree) — not committed
#   dist/           publishable artifacts (eboot.bin, .pkg, .gp4) — not committed
SRCDIR      := src
ASSETSDIR   := assets
PKGDIR      := packaging
TESTDIR     := tests
INTDIR      := build/obj
STAGEDIR    := build/stage
DISTDIR     := dist
HOST_TEST_DIR := build/host-tests
ICON_GEN    := $(ASSETSDIR)/misc

TOOLCHAIN   ?= $(OO_PS4_TOOLCHAIN)
COMMONDIR   := $(TOOLCHAIN)/samples/_common

EMBEDDED_ICON_SIZE := 256
SVG_TO_RGBA := rsvg-convert -a -w $(EMBEDDED_ICON_SIZE) -h $(EMBEDDED_ICON_SIZE)

# --- Toolchain libs / flags --------------------------------------------------
ifeq ($(MINIMAL_BOOT_TEST),1)
  LIBS       := -lc -lkernel -lc++
  EXTRAFLAGS := -DKYLIN_MINIMAL_BOOT_TEST
else
  LIBS       := -lc -lkernel -lc++ -lSceVideoOut -lSceSysmodule -lSceFreeType -lScePad -lSceUserService -lSceImeDialog -lSceSystemService -lSceSysUtil
  EXTRAFLAGS := -DGRAPHICS_USES_FONT
endif

ifeq ($(DEBUG),1)
  EXTRAFLAGS += -DKYLIN_DEBUG
  OPTFLAGS   := -O0 -g
else
  OPTFLAGS   := -O3
  EXTRAFLAGS += -DNDEBUG
endif

# --- Generated / packaged asset lists ----------------------------------------
GAMEPAD_RGBA := $(ICON_GEN)/icon-button-cross.rgba \
                $(ICON_GEN)/icon-button-circle.rgba \
                $(ICON_GEN)/icon-button-square.rgba \
                $(ICON_GEN)/icon-button-triangle.rgba \
                $(ICON_GEN)/icon-button-options.rgba \
                $(ICON_GEN)/icon-button-l1.rgba \
                $(ICON_GEN)/icon-button-l2.rgba \
                $(ICON_GEN)/icon-button-r1.rgba \
                $(ICON_GEN)/icon-button-r2.rgba \
                $(ICON_GEN)/icon-button-touchpad.rgba \
                $(ICON_GEN)/icon-button-up-down.rgba \
                $(ICON_GEN)/icon-button-left-right.rgba \
                $(ICON_GEN)/icon-button-l3.rgba \
                $(ICON_GEN)/icon-button-r3.rgba \
                $(ICON_GEN)/icon-button-rs.rgba
CONTEXT_RGBA := $(ICON_GEN)/icon-menu-refresh.rgba \
                $(ICON_GEN)/icon-menu-sort.rgba \
                $(ICON_GEN)/icon-menu-properties.rgba \
                $(ICON_GEN)/icon-menu-new-folder.rgba \
                $(ICON_GEN)/icon-menu-rename.rgba \
                $(ICON_GEN)/icon-menu-copy.rgba \
                $(ICON_GEN)/icon-menu-cut.rgba \
                $(ICON_GEN)/icon-menu-forward.rgba \
                $(ICON_GEN)/icon-menu-delete.rgba
EXPLORER_RGBA := $(ICON_GEN)/icon-explorer-folder.rgba \
                 $(ICON_GEN)/icon-explorer-file.rgba
SETTINGS_RGBA := $(ICON_GEN)/icon-settings-language.rgba \
                 $(ICON_GEN)/icon-settings-temperature.rgba \
                 $(ICON_GEN)/icon-settings-installed-apps.rgba \
                 $(ICON_GEN)/icon-settings-operation-log.rgba
COMMON_RGBA := $(ICON_GEN)/icon-common-in-operation.rgba \
               $(ICON_GEN)/icon-common-success.rgba \
               $(ICON_GEN)/icon-common-error.rgba \
               $(ICON_GEN)/icon-common-package-installing.rgba \
               $(ICON_GEN)/icon-common-usb-0.rgba \
               $(ICON_GEN)/icon-common-usb-1.rgba \
               $(ICON_GEN)/icon-common-usb-2.rgba \
               $(ICON_GEN)/icon-common-usb-3.rgba \
               $(ICON_GEN)/icon-common-usb-4.rgba \
               $(ICON_GEN)/icon-common-usb-5.rgba \
               $(ICON_GEN)/icon-common-usb-6.rgba \
               $(ICON_GEN)/icon-common-ext-1.rgba

# Extra files copied into the PKG (fonts are embedded; rgba are build-time only)
STAGE_ASSETS := $(shell find $(ASSETSDIR) -type f \
                  ! -path '$(ASSETSDIR)/fonts/*' \
                  ! -name '.DS_Store' \
                  ! -name '*.stamp' \
                  ! -name 'icon-source.svg' \
                  ! -path '$(ASSETSDIR)/images/*' \
                  ! -path '$(ASSETSDIR)/icons/source/*' \
                  ! -path '$(ASSETSDIR)/icons/generated/*' \
                  ! -name 'icon-*.rgba' 2>/dev/null)

# Final PKG system images live under packaging/sce_sys/ (icon0.png, pic0.png, ...).
# DDS backgrounds are generated at package time from the matching PNG (not committed).
# param.sfo is generated at build time into the stage tree.
LIBMODULES    := $(wildcard $(PKGDIR)/sce_module/*)
PKG_SCESYS    := $(shell find $(PKGDIR)/sce_sys -type f \
                    ! -name '.DS_Store' ! -name '*.dds' 2>/dev/null)

# --- Sources -----------------------------------------------------------------
ifeq ($(MINIMAL_BOOT_TEST),1)
  CPPFILES := $(SRCDIR)/main.cpp $(SRCDIR)/boot_log.cpp
  OBJS     := $(patsubst $(SRCDIR)/%.cpp,$(INTDIR)/%.o,$(CPPFILES))
else
  CPPFILES := $(filter-out $(SRCDIR)/boot_log_host.cpp,$(wildcard $(SRCDIR)/*.cpp))
  ASMFILES := $(wildcard $(SRCDIR)/*.S)
  OBJS     := $(patsubst $(SRCDIR)/%.cpp,$(INTDIR)/%.o,$(CPPFILES)) \
              $(patsubst $(SRCDIR)/%.S,$(INTDIR)/%.o,$(ASMFILES)) \
              $(INTDIR)/third_party/cJSON.o
endif

CXXFLAGS := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c -std=gnu++17 -D__ORBIS__ \
            $(OPTFLAGS) $(EXTRAFLAGS) -isysroot $(TOOLCHAIN) \
            -isystem $(TOOLCHAIN)/include -isystem $(TOOLCHAIN)/include/c++/v1 \
            -I$(COMMONDIR) -I$(SRCDIR)/third_party
ASFLAGS  := --target=x86_64-pc-freebsd12-elf -fPIC -c
LDFLAGS  := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr \
            -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  LLVM_DIR := $(firstword $(wildcard /opt/homebrew/opt/llvm /usr/local/opt/llvm))
  CXX      := $(LLVM_DIR)/bin/clang++
  LD_LLD   := $(firstword $(wildcard $(LLVM_DIR)/bin/ld.lld \
                   /opt/homebrew/opt/llvm@18/bin/ld.lld) \
                   $(shell command -v ld.lld 2>/dev/null))
  BINDIR   := $(TOOLCHAIN)/bin/macos
  FSELF    := $(BINDIR)/create-fself-macos
else
  CXX      := clang++
  LD_LLD   := ld.lld
  BINDIR   := $(TOOLCHAIN)/bin/linux
  FSELF    := $(BINDIR)/create-fself
endif

HOST_CXX      ?= clang++
HOST_CXXFLAGS ?= -std=gnu++17 -I.

HOST_TESTS := $(HOST_TEST_DIR)/file_system_host_test \
              $(HOST_TEST_DIR)/file_browser_host_test \
              $(HOST_TEST_DIR)/file_operations_host_test \
              $(HOST_TEST_DIR)/file_operations_forced_copy_move_host_test \
              $(HOST_TEST_DIR)/operation_log_host_test \
              $(HOST_TEST_DIR)/utf8_host_test \
              $(HOST_TEST_DIR)/utils_host_test \
              $(HOST_TEST_DIR)/context_menu_host_test \
              $(HOST_TEST_DIR)/localization_host_test \
              $(HOST_TEST_DIR)/sfo_parser_host_test

# --- Artifacts ---------------------------------------------------------------
EBOOT     := $(DISTDIR)/eboot.bin
ELF       := $(INTDIR)/kylin-explorer.elf
OELF      := $(INTDIR)/kylin-explorer.oelf
PKG       := $(DISTDIR)/$(CONTENT_ID).pkg
GP4       := $(STAGEDIR)/pkg.gp4
GP4_DIST  := $(DISTDIR)/pkg.gp4
STAGE_SFO := $(STAGEDIR)/sce_sys/param.sfo

.PHONY: all eboot pkg test host-tests clean dirs help stage
all: eboot

help:
	@echo "Targets:"
	@echo "  make eboot          -> $(EBOOT)"
	@echo "  make pkg            -> $(PKG)"
	@echo "  make test           -> host unit tests"
	@echo "  make clean          -> remove build/ and dist/"
	@echo ""
	@echo "Variables:"
	@echo "  OO_PS4_TOOLCHAIN    OpenOrbis root (required for eboot/pkg)"
	@echo "  DEBUG=1             debug build"
	@echo "  MINIMAL_BOOT_TEST=1 minimal boot binary"

dirs:
	mkdir -p $(INTDIR) $(DISTDIR) $(STAGEDIR)/sce_sys/about $(STAGEDIR)/sce_module $(HOST_TEST_DIR) $(ICON_GEN)

# =============================================================================
# Host tests (no PS4 toolchain required)
# =============================================================================
test: host-tests

host-tests: $(HOST_TESTS)
	$(HOST_TEST_DIR)/file_system_host_test
	$(HOST_TEST_DIR)/file_browser_host_test
	$(HOST_TEST_DIR)/file_operations_host_test
	$(HOST_TEST_DIR)/file_operations_forced_copy_move_host_test
	$(HOST_TEST_DIR)/operation_log_host_test
	$(HOST_TEST_DIR)/utf8_host_test
	$(HOST_TEST_DIR)/utils_host_test
	$(HOST_TEST_DIR)/context_menu_host_test
	$(HOST_TEST_DIR)/localization_host_test
	$(HOST_TEST_DIR)/sfo_parser_host_test

$(HOST_TEST_DIR):
	mkdir -p $@

$(HOST_TEST_DIR)/file_system_host_test: $(TESTDIR)/file_system_host_test.cpp $(SRCDIR)/file_system.cpp $(SRCDIR)/utils.cpp $(SRCDIR)/logger.cpp $(SRCDIR)/boot_log_host.cpp | $(HOST_TEST_DIR)
	$(HOST_CXX) $(HOST_CXXFLAGS) $^ -o $@

$(HOST_TEST_DIR)/file_browser_host_test: $(TESTDIR)/file_browser_host_test.cpp $(SRCDIR)/file_browser.cpp $(SRCDIR)/file_system.cpp $(SRCDIR)/utils.cpp $(SRCDIR)/logger.cpp $(SRCDIR)/boot_log_host.cpp | $(HOST_TEST_DIR)
	$(HOST_CXX) $(HOST_CXXFLAGS) $^ -o $@

$(HOST_TEST_DIR)/file_operations_host_test: $(TESTDIR)/file_operations_host_test.cpp $(SRCDIR)/file_operations.cpp $(SRCDIR)/file_operations_worker.cpp $(SRCDIR)/file_browser.cpp $(SRCDIR)/file_system.cpp $(SRCDIR)/utils.cpp $(SRCDIR)/logger.cpp $(SRCDIR)/boot_log_host.cpp | $(HOST_TEST_DIR)
	$(HOST_CXX) $(HOST_CXXFLAGS) $^ -o $@

$(HOST_TEST_DIR)/file_operations_forced_copy_move_host_test: $(TESTDIR)/file_operations_forced_copy_move_host_test.cpp $(SRCDIR)/file_operations.cpp $(SRCDIR)/file_operations_worker.cpp $(SRCDIR)/file_browser.cpp $(SRCDIR)/file_system.cpp $(SRCDIR)/utils.cpp $(SRCDIR)/logger.cpp $(SRCDIR)/boot_log_host.cpp | $(HOST_TEST_DIR)
	$(HOST_CXX) $(HOST_CXXFLAGS) -DKYLIN_TEST_FORCE_COPY_MOVE $^ -o $@

$(HOST_TEST_DIR)/operation_log_host_test: $(TESTDIR)/operation_log_host_test.cpp $(SRCDIR)/operation_log.cpp $(SRCDIR)/logger.cpp | $(HOST_TEST_DIR)
	$(HOST_CXX) $(HOST_CXXFLAGS) $^ -o $@

$(HOST_TEST_DIR)/utf8_host_test: $(TESTDIR)/utf8_host_test.cpp $(SRCDIR)/utf8.cpp | $(HOST_TEST_DIR)
	$(HOST_CXX) $(HOST_CXXFLAGS) $^ -o $@

$(HOST_TEST_DIR)/utils_host_test: $(TESTDIR)/utils_host_test.cpp $(SRCDIR)/utils.cpp | $(HOST_TEST_DIR)
	$(HOST_CXX) $(HOST_CXXFLAGS) $^ -o $@

$(HOST_TEST_DIR)/context_menu_host_test: $(TESTDIR)/context_menu_host_test.cpp $(SRCDIR)/context_menu.cpp | $(HOST_TEST_DIR)
	$(HOST_CXX) $(HOST_CXXFLAGS) $^ -o $@

$(HOST_TEST_DIR)/localization_host_test: $(TESTDIR)/localization_host_test.cpp $(SRCDIR)/localization.cpp | $(HOST_TEST_DIR)
	$(HOST_CXX) $(HOST_CXXFLAGS) $^ -o $@

$(HOST_TEST_DIR)/sfo_parser_host_test: $(TESTDIR)/sfo_parser_host_test.cpp $(SRCDIR)/sfo_parser.cpp | $(HOST_TEST_DIR)
	$(HOST_CXX) $(HOST_CXXFLAGS) $^ -o $@

# =============================================================================
# Target compile / link / fself
# =============================================================================
check-toolchain:
	@if [ -z "$(TOOLCHAIN)" ] || [ ! -d "$(TOOLCHAIN)" ]; then \
	  echo "error: OO_PS4_TOOLCHAIN is not set or does not exist."; \
	  echo "  export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis-PS4-Toolchain"; \
	  exit 1; \
	fi

$(INTDIR):
	mkdir -p $@

$(DISTDIR):
	mkdir -p $@

$(INTDIR)/%.o: $(SRCDIR)/%.cpp | $(INTDIR) check-toolchain
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(INTDIR)/%.o: $(SRCDIR)/%.S | $(INTDIR) check-toolchain
	@mkdir -p $(dir $@)
	$(CXX) $(ASFLAGS) -o $@ $<

$(INTDIR)/third_party/cJSON.o: $(SRCDIR)/third_party/cJSON.cpp | $(INTDIR) check-toolchain
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(INTDIR)/embedded_font.o: $(ASSETSDIR)/fonts/NotoSansSC-Regular.ttf

$(INTDIR)/embedded_icons.o: $(GAMEPAD_RGBA) $(CONTEXT_RGBA) $(EXPLORER_RGBA) $(SETTINGS_RGBA) $(COMMON_RGBA)

$(ELF): $(OBJS) | $(INTDIR) check-toolchain
	$(LD_LLD) $(OBJS) -o $@ $(LDFLAGS)

eboot: $(EBOOT)

$(EBOOT): $(ELF) | $(DISTDIR) check-toolchain
	$(FSELF) -in=$< -out=$(OELF) --eboot $@ --paid $(FSELF_PAID) --authinfo $(FSELF_AUTH_INFO)

# =============================================================================
# Icon generation (SVG -> raw RGBA, embedded at link time)
# =============================================================================
$(ICON_GEN)/gamepad.stamp:
	mkdir -p $(ICON_GEN)
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/gamepad/plain-cross.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-button-cross.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/gamepad/plain-circle.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-button-circle.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/gamepad/plain-square.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-button-square.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/gamepad/plain-triangle.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-button-triangle.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/gamepad/plain-big-option.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-button-options.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/gamepad/plain-L1.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-button-l1.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/gamepad/plain-L2.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-button-l2.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/gamepad/plain-R1.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-button-r1.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/gamepad/plain-R2.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-button-r2.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/gamepad/outline-medium-pad.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-button-touchpad.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/gamepad/plain-top.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-button-up-down.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/gamepad/plain-left.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-button-left-right.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/gamepad/press-L3-1.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-button-l3.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/gamepad/press-R3-1.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-button-r3.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/gamepad/plain-R.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-button-rs.rgba
	touch $@

$(GAMEPAD_RGBA): $(ICON_GEN)/gamepad.stamp
	@test -f $@ || { rm -f $(ICON_GEN)/gamepad.stamp; $(MAKE) $(ICON_GEN)/gamepad.stamp; }

$(ICON_GEN)/contextmenu.stamp:
	mkdir -p $(ICON_GEN)
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/contextmenu/refresh-ccw.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-menu-refresh.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/contextmenu/arrow-down-a-z.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-menu-sort.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/contextmenu/table-properties.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-menu-properties.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/contextmenu/folder-plus.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-menu-new-folder.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/contextmenu/folder-pen.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-menu-rename.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/contextmenu/copy.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-menu-copy.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/contextmenu/scissors.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-menu-cut.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/contextmenu/forward.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-menu-forward.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/contextmenu/trash.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-menu-delete.rgba
	touch $@

$(CONTEXT_RGBA): $(ICON_GEN)/contextmenu.stamp
	@test -f $@ || { rm -f $(ICON_GEN)/contextmenu.stamp; $(MAKE) $(ICON_GEN)/contextmenu.stamp; }

$(ICON_GEN)/explorer.stamp:
	mkdir -p $(ICON_GEN)
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/explorer/folder.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-explorer-folder.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/explorer/file.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-explorer-file.rgba
	touch $@

$(EXPLORER_RGBA): $(ICON_GEN)/explorer.stamp
	@test -f $@ || { rm -f $(ICON_GEN)/explorer.stamp; $(MAKE) $(ICON_GEN)/explorer.stamp; }

$(ICON_GEN)/settings.stamp:
	mkdir -p $(ICON_GEN)
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/settings/languages.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-settings-language.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/settings/thermometer.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-settings-temperature.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/settings/package-search.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-settings-installed-apps.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/settings/logs.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-settings-operation-log.rgba
	touch $@

$(SETTINGS_RGBA): $(ICON_GEN)/settings.stamp
	@test -f $@ || { rm -f $(ICON_GEN)/settings.stamp; $(MAKE) $(ICON_GEN)/settings.stamp; }

$(ICON_GEN)/common.stamp:
	mkdir -p $(ICON_GEN)
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/common/in_operation.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-common-in-operation.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/common/success.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-common-success.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/common/error.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-common-error.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/common/package_installing.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-common-package-installing.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/common/usb_0.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-common-usb-0.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/common/usb_1.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-common-usb-1.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/common/usb_2.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-common-usb-2.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/common/usb_3.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-common-usb-3.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/common/usb_4.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-common-usb-4.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/common/usb_5.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-common-usb-5.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/common/usb_6.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-common-usb-6.rgba
	$(SVG_TO_RGBA) "$(ASSETSDIR)/icons/source/common/ext_1.svg" | magick PNG:- -background none -gravity center -extent $(EMBEDDED_ICON_SIZE)x$(EMBEDDED_ICON_SIZE) -depth 8 RGBA:$(ICON_GEN)/icon-common-ext-1.rgba
	touch $@

$(COMMON_RGBA): $(ICON_GEN)/common.stamp
	@test -f $@ || { rm -f $(ICON_GEN)/common.stamp; $(MAKE) $(ICON_GEN)/common.stamp; }

# =============================================================================
# PKG staging + packaging
# =============================================================================
pkg: $(PKG)

# Write param.sfo into the stage tree (generated; not taken from packaging/).
define WRITE_PARAM_SFO
	$(BINDIR)/PkgTool.Core sfo_new $(STAGE_SFO)
	$(BINDIR)/PkgTool.Core sfo_setentry $(STAGE_SFO) APP_TYPE --type Integer --maxsize 4 --value 1
	$(BINDIR)/PkgTool.Core sfo_setentry $(STAGE_SFO) APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(BINDIR)/PkgTool.Core sfo_setentry $(STAGE_SFO) ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(BINDIR)/PkgTool.Core sfo_setentry $(STAGE_SFO) CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(BINDIR)/PkgTool.Core sfo_setentry $(STAGE_SFO) CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(BINDIR)/PkgTool.Core sfo_setentry $(STAGE_SFO) DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(BINDIR)/PkgTool.Core sfo_setentry $(STAGE_SFO) SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(BINDIR)/PkgTool.Core sfo_setentry $(STAGE_SFO) TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(BINDIR)/PkgTool.Core sfo_setentry $(STAGE_SFO) TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(BINDIR)/PkgTool.Core sfo_setentry $(STAGE_SFO) VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'
endef

# Assemble a package-root tree under build/stage so create-gp4 paths match PKG layout.
# PNG system images are copied from packaging/sce_sys/; DDS is generated from PNG.
stage: $(EBOOT) $(PKG_SCESYS) $(LIBMODULES) | dirs check-toolchain
	@test -n "$(PKG_SCESYS)" || { echo "error: no files under $(PKGDIR)/sce_sys/"; exit 1; }
	@test -f "$(PKGDIR)/sce_sys/icon0.png" || { echo "error: missing $(PKGDIR)/sce_sys/icon0.png"; exit 1; }
	@test -f "$(PKGDIR)/sce_sys/pic0.png" || { echo "error: missing $(PKGDIR)/sce_sys/pic0.png"; exit 1; }
	@mkdir -p $(STAGEDIR)/sce_module
	cp -f $(EBOOT) $(STAGEDIR)/eboot.bin
	@# Replace staged sce_sys entirely so removed packaging files do not linger
	rm -rf $(STAGEDIR)/sce_sys
	mkdir -p $(STAGEDIR)/sce_sys
	@# Copy static payload; never ship prebuilt .dds from packaging/
	rsync -a --exclude '*.dds' --exclude '.DS_Store' $(PKGDIR)/sce_sys/ $(STAGEDIR)/sce_sys/
	@# Generate DXT5 DDS backgrounds for the host menu (dimensions should be multiples of 4)
	magick $(STAGEDIR)/sce_sys/pic0.png -define dds:compression=dxt5 $(STAGEDIR)/sce_sys/pic0.dds
	@if [ -f "$(STAGEDIR)/sce_sys/pic1.png" ]; then \
	  magick $(STAGEDIR)/sce_sys/pic1.png -define dds:compression=dxt5 $(STAGEDIR)/sce_sys/pic1.dds; \
	fi
	$(WRITE_PARAM_SFO)
	cp -f $(LIBMODULES) $(STAGEDIR)/sce_module/
	@# Optional loose assets (most icons/fonts are embedded)
	@if [ -n "$(STAGE_ASSETS)" ]; then \
	  for f in $(STAGE_ASSETS); do \
	    rel=$${f#./}; \
	    mkdir -p $(STAGEDIR)/$$(dirname $$rel); \
	    cp -f $$f $(STAGEDIR)/$$rel; \
	  done; \
	fi

$(GP4): stage | check-toolchain
	@# PkgTool resolves orig_path relative to the directory that contains the .gp4
	@# file, so the GP4 must live next to the staged eboot/sce_sys/sce_module tree.
	cd $(STAGEDIR) && $(BINDIR)/create-gp4 -out pkg.gp4 --content-id=$(CONTENT_ID) \
	  --files "$$(find . -type f ! -name '.DS_Store' ! -name 'pkg.gp4' ! -name '*.pkg' | sed 's|^\./||' | tr '\n' ' ')"

$(PKG): $(GP4) | $(DISTDIR) check-toolchain
	cd $(STAGEDIR) && $(BINDIR)/PkgTool.Core pkg_build pkg.gp4 .
	@if [ -f "$(STAGEDIR)/$(CONTENT_ID).pkg" ]; then \
	  mv -f "$(STAGEDIR)/$(CONTENT_ID).pkg" "$(PKG)"; \
	elif [ -f "$(CONTENT_ID).pkg" ]; then \
	  mv -f "$(CONTENT_ID).pkg" "$(PKG)"; \
	else \
	  echo "error: package not produced by PkgTool.Core"; \
	  find build dist -name '*.pkg' 2>/dev/null; \
	  exit 1; \
	fi
	cp -f "$(GP4)" "$(GP4_DIST)"
	@echo "Built $(PKG)"

# =============================================================================
# Clean
# =============================================================================
clean:
	rm -rf build dist
	rm -f $(ICON_GEN)/icon-*.rgba $(ICON_GEN)/*.stamp
	rm -rf $(ASSETSDIR)/icons/generated
	@# legacy root-level outputs from older layout
	rm -f eboot.bin pkg.gp4 $(CONTENT_ID).pkg IV0000-KYLN00001_00-KYLINEXPLORER000.pkg
