<p align="center">
  <img src="packaging/sce_sys/icon0.png" alt="Kylin Explorer" height="128" width="128"/>
</p>

<p align="center">
  <b>Kylin Explorer</b><br/>
  面向已越狱 PS5 的自制文件管理器
</p>

<p align="center">
  <b>简体中文</b>
  ·
  <a href="README.md">English</a>
</p>

<p align="center">
  <b><a href="#特点">特点</a></b>
  ·
  <b><a href="#安装">安装</a></b>
  ·
  <b><a href="#操控">操控</a></b>
  ·
  <b><a href="#开发">开发</a></b>
  ·
  <b><a href="#致谢">致谢</a></b>
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
  基于 <a href="https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain">OpenOrbis PS4 Toolchain</a> 构建的 PS4 形态 PKG，<br/>
  面向<b>手柄优先</b>的浏览体验，运行在具备提权文件系统访问能力的越狱 PS5 上。
</p>

<br>

# 特点

为自制环境设计的实用文件管理器，而不是简单复刻桌面资源管理器。

- **浏览与导航** — 目录列表、历史、排序、L1/R1 快捷位置
- **多选** — Square 标记；批量复制 / 剪切 / 删除
- **受保护的写入** — 新建目录、重命名、复制、移动、删除均带确认
- **长任务不卡死** — 进度条、复制速度；在安全范围内支持取消与回滚
- **UTF-8 与多语言** — 界面英 / 简中；CJK 字体回退
- **更多能力** — 设置、温度显示、已安装 PKG 列表、原生 PKG 安装客户端
- **启动提权等待** — 通过 etaHEN 兼容的 sandbox trigger 完成权限握手后再访问完整文件系统

<br>

# 安装

### 运行环境

1. **已越狱的 PS5**，并具备提权文件系统访问（etaHEN 兼容的 sandbox trigger）。
2. DualShock / DualSense 类手柄。

### 安装包

1. 从 [Releases](https://github.com/aydencharles/ps5-kylin-explorer/releases) 下载最新 `.pkg`  
   （或自行编译，见 [开发](#开发)）。
2. 使用常用的自制 PKG 安装方式安装。
3. 在主页启动 **Kylin Explorer**。

> [!NOTE]
> 若未获得提权，应用会停留在等待权限的界面。  
> 粘贴到已存在目录的「合并」策略尚未实现，当前会明确拒绝，避免不可恢复的覆盖。

<br>

# 操控

| 按键 | 作用 |
| --- | --- |
| 方向键 上 / 下 | 移动选中项 |
| L2 + 方向键 上 / 下 | 一次跳转 10 项 |
| 方向键 左 / 右 | 切换排序方式 |
| Cross（叉） | 打开目录 |
| Square（方） | 标记 / 取消标记 |
| Circle（圆） | 沿浏览历史返回 |
| Triangle（三角） | 上下文菜单 |
| Options | 刷新当前目录 |
| L1 / R1 | 切换快捷位置 |

上下文菜单包含：刷新、排序、语言、属性、新建文件夹、重命名、复制 / 剪切 / 粘贴、发送到、删除、最近操作等。

<br>

# 开发

```shell
git clone https://github.com/aydencharles/ps5-kylin-explorer.git
cd ps5-kylin-explorer
export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis-PS4-Toolchain
```

### 依赖

| 依赖 | 用途 |
| --- | --- |
| [OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain) | 交叉编译、FSELF、GP4、PKG |
| Clang + LLD | 目标编译 / 链接（macOS：`brew install llvm`） |
| `rsvg-convert` | SVG → PNG（内嵌 UI 图标） |
| ImageMagick (`magick`) | SVG → RGBA；PNG → DXT5 DDS（`sce_sys`） |
| Rosetta 2（Apple Silicon） | OpenOrbis 的 macOS 工具为 x86_64 |

Host 单测（`make test`）**不需要** PS4 工具链。

### 环境变量

```shell
export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis-PS4-Toolchain
```

详见 [`.env.example`](.env.example)。

| 变量 | 必需 | 说明 |
| --- | --- | --- |
| `OO_PS4_TOOLCHAIN` | 是（eboot/pkg） | OpenOrbis 根目录 |
| `DEBUG=1` | 否 | 调试符号、`-O0` |
| `MINIMAL_BOOT_TEST=1` | 否 | 无 UI 的最小启动二进制 |
| `HOST_CXX` | 否 | Host 测试编译器（默认 `clang++`） |

### 打包构建

```shell
# 推荐
./scripts/build.sh --clean release

# 或使用 make
make test
make eboot          # -> dist/eboot.bin
make pkg            # -> dist/*.pkg
make pkg DEBUG=1
make clean
```

### 产物

| 路径 | 说明 |
| --- | --- |
| `dist/eboot.bin` | FSELF 可执行文件 |
| `dist/IV0000-KYLN00002_00-KYLINEXPLORER000.pkg` | 可安装包 |
| `dist/pkg.gp4` | GP4 工程副本 |
| `build/obj/` | 目标文件、ELF、OELF |
| `build/stage/` | 临时打包根目录 |
| `assets/misc/*.rgba` | 生成的 UI 图标（不入库） |

`packaging/sce_sys/icon0.png`、`pic0.png` 原样进包；  
`pic0.dds` 在打包阶段由 PNG 生成（DXT5），不提交到仓库。

FSELF 使用 `Makefile` 中固定的 `PAID` / `AUTH_INFO`。请保持不变——OpenOrbis 示例 PAID 在目标 PS5 上可能导致启动失败。

### 仓库结构

```text
.
├── src/                 应用源码
├── assets/              字体与 SVG 图标源（链接期嵌入）
├── packaging/           静态 PKG 载荷（sce_sys、sce_module）
├── tests/               Host 单测
├── scripts/build.sh     Debug / Release 封装脚本
├── Makefile
├── build/               中间产物（gitignore）
└── dist/                发布产物（gitignore）
```

<br>

# 反馈问题前

1. 确认主机已越狱，且文件系统提权可用。
2. 若长时间停在权限等待界面，检查 sandbox trigger 是否被当前自制环境消费。
3. 先搜索是否已有类似报告：[Issues](https://github.com/aydencharles/ps5-kylin-explorer/issues)
4. 尽量写清：系统 / 越狱环境版本、复现步骤，并附截图或视频。

<br>

# 贡献

- 较大功能请先开 issue 讨论，避免重复劳动。
- 改动尽量聚焦，风格与现有代码保持一致。
- 提交前可运行：`make test`
- 欢迎向当前开发分支发起 Pull Request。

<br>

# 致谢

Kylin Explorer 的开发离不开 OpenOrbis 与自制社区的基础工作。

- [OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain)
- [cJSON](https://github.com/DaveGamble/cJSON)（MIT）
- [Noto fonts](https://fonts.google.com/noto) / Gontserrat（SIL OFL）

<br>

# 许可证

本项目采用 [GNU General Public License v3.0](LICENSE)。

第三方组件遵循各自许可证（见上文）。

> 实验性自制软件，仅面向**已经越狱**的主机。风险自担，不提供任何担保。
