<p align="center">
  <img src="packaging/sce_sys/icon0.png" alt="Kylin Explorer" height="128" width="128"/>
</p>

<p align="center">
  <b>Kylin Explorer</b><br/>
  A homebrew file explorer for jailbroken PS5
</p>

<p align="center">
  <a href="README_ZH-CN.md">简体中文</a>
  ·
  <b>English</b>
</p>

<p align="center">
  <b><a href="#features">Features</a></b>
  ·
  <b><a href="#install">Install</a></b>
  ·
  <b><a href="#controls">Controls</a></b>
  ·
  <b><a href="#build">Build</a></b>
  ·
  <b><a href="#acknowledgement">Acknowledgement</a></b>
</p>

---

<p align="center">
  <a href="https://github.com/aydencharles/ps5-kylin-explorer/releases"><img src="https://img.shields.io/github/v/release/aydencharles/ps5-kylin-explorer" alt="release"/></a>
  <a href="https://github.com/aydencharles/ps5-kylin-explorer/stargazers"><img src="https://img.shields.io/github/stars/aydencharles/ps5-kylin-explorer?style=flat" alt="stars"/></a>
  <a href="https://github.com/aydencharles/ps5-kylin-explorer/network/members"><img src="https://img.shields.io/github/forks/aydencharles/ps5-kylin-explorer" alt="forks"/></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPLv3-blue.svg" alt="license"/></a>
  <img src="https://img.shields.io/badge/-PS5%20Homebrew-003791?style=flat&logo=PlayStation" alt="PS5"/>
  <img src="https://img.shields.io/badge/-OpenOrbis-lightgrey?style=flat" alt="OpenOrbis"/>
</p>

<p align="center">
  Built as a PS4-style package with the <a href="https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain">OpenOrbis PS4 Toolchain</a>.<br/>
  Designed for <b>controller-first</b> browsing on a jailbroken PS5 with elevated filesystem access.
</p>

<br>

# Features

A practical file manager for homebrew PS5 setups — not a desktop clone.

- **Browse & navigate** — directory listing, history, sort modes, L1/R1 shortcuts
- **Multi-select** — mark items with Square; batch copy / cut / delete
- **Guarded writes** — create folder, rename, copy, move, delete with confirmation
- **Long jobs stay responsive** — progress overlay, copy speed, cancel & rollback where safe
- **UTF-8 & i18n** — English / Simplified Chinese UI; CJK font fallback
- **Extras** — settings, temperature display, installed PKG list, native PKG install client
- **Startup privileges** — waits on an etaHEN-compatible sandbox trigger before full FS access

<br>

# Install

### Requirements

1. A **jailbroken PS5** with elevated filesystem privileges (etaHEN-compatible sandbox trigger).
2. A DualShock / DualSense-style controller.

### Package

1. Download the latest `.pkg` from [Releases](https://github.com/aydencharles/ps5-kylin-explorer/releases)  
   (or build one yourself — see [Build](#build)).
2. Install the package with your preferred homebrew package installer.
3. Launch **Kylin Explorer** from the home menu.

> [!NOTE]
> Without elevated privileges the app will idle at the jailbreak wait screen.  
> Directory merge-on-paste is intentionally rejected until a recoverable strategy exists.

<br>

# Controls

| Control | Action |
| --- | --- |
| D-pad up / down | Move selection |
| L2 + D-pad up / down | Jump selection by 10 |
| D-pad left / right | Cycle sort mode |
| Cross | Open directory |
| Square | Mark / unmark item |
| Circle | Back through history |
| Triangle | Context menu |
| Options | Refresh |
| L1 / R1 | Cycle shortcut locations |

Context menu covers refresh, sort, language, properties, new folder, rename, copy / cut / paste, send-to, delete, and recent operations.

<br>

# Build

```shell
git clone https://github.com/aydencharles/ps5-kylin-explorer.git
cd ps5-kylin-explorer
export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis-PS4-Toolchain
```

### Dependencies

| Dependency | Purpose |
| --- | --- |
| [OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain) | Cross compile, FSELF, GP4, PKG |
| Clang + LLD | Target compiler / linker (`brew install llvm` on macOS) |
| `rsvg-convert` | SVG → PNG for embedded UI icons |
| ImageMagick (`magick`) | SVG → RGBA; PNG → DXT5 DDS for `sce_sys` |
| Rosetta 2 (Apple Silicon) | OpenOrbis macOS tools are x86_64 |

Host unit tests (`make test`) do **not** require the PS4 toolchain.

### Environment

```shell
export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis-PS4-Toolchain
```

See [`.env.example`](.env.example).

| Variable | Required | Description |
| --- | --- | --- |
| `OO_PS4_TOOLCHAIN` | yes (eboot/pkg) | OpenOrbis toolchain root |
| `DEBUG=1` | no | Debug symbols, `-O0` |
| `MINIMAL_BOOT_TEST=1` | no | Minimal eboot without UI libraries |
| `HOST_CXX` | no | Host compiler for tests (default `clang++`) |

### Package build

```shell
# recommended
./scripts/build.sh --clean release

# or via make
make test
make eboot          # -> dist/eboot.bin
make pkg            # -> dist/*.pkg
make pkg DEBUG=1
make clean
```

### Outputs

| Path | Description |
| --- | --- |
| `dist/eboot.bin` | FSELF binary |
| `dist/IV0000-KYLN00002_00-KYLINEXPLORER000.pkg` | Installable package |
| `dist/pkg.gp4` | GP4 project copy |
| `build/obj/` | Objects, ELF, OELF |
| `build/stage/` | Temporary package root |
| `assets/misc/*.rgba` | Generated UI icons (not committed) |

`packaging/sce_sys/icon0.png` and `pic0.png` are packaged as-is.  
`pic0.dds` is generated at package time (DXT5) and is not committed.

FSELF uses fixed `PAID` / `AUTH_INFO` values in the `Makefile`. Keep them unchanged — the default OpenOrbis sample PAID can hard-fail on the target PS5 environment.

### Repository layout

```text
.
├── src/                 Application sources
├── assets/              Fonts & SVG icon sources (embedded)
├── packaging/           Static PKG payload (sce_sys, sce_module)
├── tests/               Host-side unit tests
├── scripts/build.sh     Debug / release wrapper
├── Makefile
├── build/               Intermediates (gitignored)
└── dist/                Publishable artifacts (gitignored)
```

<br>

# Troubleshooting

1. Confirm the console is jailbroken and filesystem privileges are available.
2. If the app sticks on the privilege wait screen, check that the sandbox trigger is consumed by your homebrew stack.
3. Search existing reports: [Issues](https://github.com/aydencharles/ps5-kylin-explorer/issues)
4. Describe the problem with console firmware / jailbreak stack version, repro steps, and screenshots when possible.

<br>

# Contributing

- Open an issue before large features to avoid duplicated work.
- Keep changes focused; match existing code style.
- Host tests: `make test`
- Pull requests are welcome on the active development branch.

<br>

# Acknowledgement

Kylin Explorer is built on the shoulders of the homebrew and OpenOrbis communities.

- [OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain)
- [cJSON](https://github.com/DaveGamble/cJSON) (MIT)
- [Noto fonts](https://fonts.google.com/noto) / Gontserrat (SIL OFL)

<br>

# License

This project is licensed under the [GNU General Public License v3.0](LICENSE).

Third-party components keep their own licenses (see above).

> Experimental homebrew for **already jailbroken** hardware. Use at your own risk. No warranty.
