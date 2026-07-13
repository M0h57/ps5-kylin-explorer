#!/usr/bin/env bash
# Build Kylin Explorer (eboot + PKG) into dist/
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

MODE="debug"
CLEAN_BUILD=false
DO_TEST=false

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

usage() {
    cat <<EOF
Usage: $(basename "$0") [options] [debug|release]

Options:
  -c, --clean       Clean build/ and dist/ before building
  -m, --mode MODE   debug (default) or release
  -t, --test        Run host unit tests before packaging
  -h, --help        Show this help

Environment:
  OO_PS4_TOOLCHAIN  Path to OpenOrbis PS4 Toolchain (required)

Examples:
  export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis-PS4-Toolchain
  ./scripts/build.sh                  # debug PKG -> dist/
  ./scripts/build.sh --clean release  # clean release PKG
  ./scripts/build.sh --test release
EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help) usage ;;
        -c|--clean) CLEAN_BUILD=true; shift ;;
        -t|--test) DO_TEST=true; shift ;;
        -m|--mode)
            MODE="$2"
            shift 2
            ;;
        debug|release)
            MODE="$1"
            shift
            ;;
        *)
            echo -e "${RED}Error: unknown argument '$1'${NC}" >&2
            usage
            ;;
    esac
done

if [[ "$MODE" != "debug" && "$MODE" != "release" ]]; then
    echo -e "${RED}Error: mode must be debug or release${NC}" >&2
    exit 1
fi

if [[ -z "${OO_PS4_TOOLCHAIN:-}" || ! -d "$OO_PS4_TOOLCHAIN" ]]; then
    echo -e "${RED}Error: set OO_PS4_TOOLCHAIN to your OpenOrbis toolchain root${NC}" >&2
    echo "  export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis-PS4-Toolchain"
    exit 1
fi

CONTENT_ID="IV0000-KYLN00002_00-KYLINEXPLORER000"
PKG_PATH="dist/${CONTENT_ID}.pkg"
EBOOT_PATH="dist/eboot.bin"

echo -e "${BLUE}=== Kylin Explorer build ===${NC}"
echo -e "Mode            : ${YELLOW}${MODE}${NC}"
echo -e "Clean           : ${YELLOW}${CLEAN_BUILD}${NC}"
echo -e "Host tests      : ${YELLOW}${DO_TEST}${NC}"
echo -e "OO_PS4_TOOLCHAIN: ${YELLOW}${OO_PS4_TOOLCHAIN}${NC}"
echo

if [[ "$CLEAN_BUILD" == true ]]; then
    echo -e "${BLUE}[1/4] Cleaning...${NC}"
    make clean
else
    echo -e "${BLUE}[1/4] Incremental build (skip clean)${NC}"
fi

if [[ "$DO_TEST" == true ]]; then
    echo -e "${BLUE}[2/4] Host tests...${NC}"
    make test
else
    echo -e "${BLUE}[2/4] Skipping host tests${NC}"
fi

echo -e "${BLUE}[3/4] Building package (${MODE})...${NC}"
if [[ "$MODE" == "debug" ]]; then
    make pkg DEBUG=1
else
    make pkg
fi

echo -e "${BLUE}[4/4] Verifying outputs...${NC}"
ok=true
for f in "$EBOOT_PATH" "$PKG_PATH"; do
    if [[ -f "$f" ]]; then
        echo -e "${GREEN}  ok  ${f} ($(du -h "$f" | cut -f1))${NC}"
    else
        echo -e "${RED}  missing ${f}${NC}"
        ok=false
    fi
done

if [[ "$ok" != true ]]; then
    exit 1
fi

echo -e "${GREEN}Done. Install ${PKG_PATH} on a jailbroken PS5 (requires Kylin Core).${NC}"
