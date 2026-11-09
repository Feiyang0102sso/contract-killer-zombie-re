# Contract Killer: Zombies Reverse

## 概述

本项目基于 Glu Mobile《Contract Killer: Zombies》iOS 3.1.1 版本的主程序 `zombsniper`（armv7 Mach-O）。原版程序没有剥离符号，还保留了完整的 STABS 调试映射：832 个编译单元的源文件路径、目标文件名、每个函数的归属和原版链接顺序都有据可查。源码目录结构、文件划分和函数顺序都按这份映射还原。

原版源码还原阶段的目标是 **1:1 还原源码**：用原版工具链编译恢复出的源码，得到与原版逐字节一致的 `zombsniper`。修复游戏原本的 bug 不在本项目范围内，原版行为（包括原版自身的 bug）一律按原样保留。

只有源码具备极高的可信度，后续的修复和移植才有实际意义。

## 编译器、SDK 与第三方库

原版二进制里没有编译器版本记录。下表的版本依据来自逐函数的机器码比对、调试映射里的构建路径，以及二进制内的版本字符串。

> ✅ 已对齐　⚠️ 版本一致，但有一项无法完全核实　❓ 原版版本未确定

### 工具链

| 组件 | 原版 | 本项目 | 对齐 | 备注 |
| --- | --- | --- | :---: | --- |
| 编译器 | Apple clang 3.0 | clang-211.12（Xcode 4.2.1 4D502 自带） | ⚠️ | 按代码生成特征判定：公开源码能编出的 211.10.1 会多出 `vorr`、拆开 `vpop`，换用 211.12 后这些差异全部消失 |
| 汇编器 | cctools as | cctools-809（同一 Xcode 的外部 `as`，clang 加 `-no-integrated-as`） | ⚠️ | 按编码判定：IT 块里低寄存器 `add` 原版全是 T1 编码，clang 内置汇编器只出 T2；换用外部 `as` 后自家 13 个、第三方库 12 个函数变为一致，无退化 |
| 链接器 | ld64 | ld64-127.2（同一 Xcode） | ⚠️ | 文件头标志、32 个 dylib 的顺序和弱加载与原版一致；整文件尚未逐字节一致 |
| iOS SDK | — | iPhoneOS 5.0 SDK（同一 Xcode） | ⚠️ | `-miphoneos-version-min=4.0` |
| 编译参数 | — | `-Os -g -fno-strict-aliasing -fvisibility=hidden`<br>C++：`-fno-rtti -fno-exceptions`<br>Objective-C++：`-fno-rtti` | ✅ | 由逐字节一致的函数反推；原版 Glu 符号全是 private extern，可证明开了 `-fvisibility=hidden` |
| 构建路径 | `/Applications/buildAgent/work/c426877c688cbe05/` | 同左（bind mount） | ✅ | `__FILE__` 字符串和调试信息里的编译目录与原版相同 |

Xcode 4.2.1 的 clang、as 和 ld 是 x86_64 Mach-O 程序。原版还原工作使用带 3 处补丁的 [maloader](https://github.com/shinh/maloader) 在 Linux 上运行它们。

### 第三方库

下表的“函数比对”是与原版逐函数比对的结果（一致 / 不一致）。

| 库 | 版本 | 来源 | 函数比对 | 备注 |
| --- | --- | --- | --- | --- |
| Lua | 5.1.4 | 官方源码 + Glu 定制 | 449 / 26 | 另有 1 个等价；按 C++ 编译，`lua_Number = float`；按原版只开 5 个标准库 |
| LuaBridge | 0.2 | 官方源码 + Glu 改动 | 184 / 1 | |
| TinyXML | 2.5 | 官方源码 | 120 / 14 | |
| libjpeg | 6b | 官方源码 + Glu 定制 | 150 / 2 | |
| libpng | 1.2.41 | 官方源码 + Glu 定制 | 96 / 9 | |
| libogg | 1.1.3 | 官方源码 | 28 / 1 | |
| libvorbis | 1.2.0 | 官方源码 + Glu 私改 | 128 / 5 | |
| zlib（inflate） | 1.2.3 | 官方源码 | 12 / 0 | 原版只编进了 inflate 部分 |
| Facebook iOS SDK | 提交 `91f25642` | 官方源码 | 155 / 0 | |
| ASIHTTPRequest | 提交 `331e21f` | 官方源码 | 140 / 1 | |
| Reachability | 2.2 | Apple 示例代码 + Glu 私改 | 12 / 0 | |
| Swerve 移植层（`malij297_*`） | — | 按机器码恢复 | 53 / 7 | 无公开源码 |
| Swerve 引擎核心 | — | **从原版二进制搬出的汇编** | 逐字节一致 | 原版是无符号的预编译库（420,049 字节），见下文说明 |
| OpenSSL | 1.0.0a–d | **从原版二进制搬出的汇编** | 逐字节一致 | 原版是 llvm-gcc-4.2 编的预编译库（577,912 字节） |
| AdMarvel、Flurry、PlayHaven、Tapjoy | — | 空实现 | — | 广告/统计 SDK，**不是原版代码**，只为能链接、能玩；在 `CKZ_source/stubs/ads/` |

Swerve 引擎核心和 OpenSSL 在原版里是预编译静态库。它们以汇编源码的形式放在 `CKZ_source/packages/{swerve,openssl}/code/cocoa/libs/*_core.s`，按原地址重定位后与原版逐字节相同。这两个 `.s` 是由原版二进制生成的，不是手写源码，因此不计入 GitHub 的语言统计。

## 结果

统计范围是 Glu 自家代码：8479 个函数，共 1,134,088 字节。第三方库、预编译库和广告 SDK 不在其中。

| 状态 | 函数 | 字节 | 字节占比 |
| --- | ---: | ---: | ---: |
| 一致（MATCH） | 7823 | 805,360 | 71.01% |
| 等价（只差寄存器或指令顺序） | 17 | 1,944 | 0.17% |
| 不一致 | 639 | 326,784 | 28.81% |

- 不一致和等价共 656 个函数（328,728 字节）。状态页按原因列出 39 个（11.7 KB） 已证明等价或人工确认写法差，115 个（31.8 KB） 疑似噪声，153 个（122.1 KB） 有常量、条件、有无符号或调用目标差异的线索，349 个（155.4 KB） 未判定。
- **全部函数都有实现**，整个游戏可以链接成 `zombsniper`，并已在越狱 iPad 上进入关卡试玩。
- 整个程序（3,953,724 字节，含第三方库和预编译库）中有 46.96% 已逐字节一致。
- 整个文件尚未逐字节一致：除上面的不一致函数外，弱符号的排列顺序也与原版不同。

“一致”的判定：屏蔽重定位后字节相同，并且每个引用（调用目标、字符串内容、GOT 导入、selector 等）都指向同一个东西。函数尾部的对齐填充不计；但未链接目标文件里被重定位覆盖的尾部字节（文字池的值）不当填充裁掉。

## Windows x86 构建与运行

安装 Visual Studio 2022 或 2026 的 Windows C++ x86 工具和 Windows SDK。根目录的 `build_pc.bat` 通过 `vswhere` 查找已安装的 Visual Studio。也可打开 `vs/ckz_pc.sln`，选择 `Release | Win32` 构建。

VS 工程直接使用 [`CKZ_source/pc_port/3rd_party/`](CKZ_source/pc_port/3rd_party/) 下的第三方源码和 Win32 SDK。构建需要 Python 3.11 以上，可用 `CkzPython` 指定解释器。

构建会自动把仓库中独立的 Swerve ARM 镜像放到 EXE 同目录。

构建完成后，手动将原包里的 `zombsniper.big`、`vga.big`、`en.big`、`res_map.dat` 和 `ckz_intro_scene_1024.m4v` 放到 `bin/Win32/Release/assets/`。`swerve-arm.bin` 由构建放在 EXE 同目录，不要挪进 `assets/`。

所需文件及其位置统一定义在 [`AssetPaths.h`](CKZ_source/pc_port/platform_sdl/AssetPaths.h)。启动时逐个检查，缺哪个就报哪个的完整路径。

运行 `bin/Win32/Release/ckz_pc.exe`。存档默认写入同目录的 `saves/`。运行需要支持桌面 OpenGL 兼容上下文和帧缓冲对象的驱动，以及 x86 版 Visual C++ 运行库。

| 输入 | 操作 |
| --- | --- |
| 移动鼠标 | 转动视角（战斗中鼠标被锁定并隐藏） |
| 鼠标左键 | 开火，按住连发；菜单和地图中为点击 |
| 鼠标右键 | 开镜 / 关镜 |
| `R` | 装弹 |
| `Q` / `E` | 切换武器 |
| 滚轮 | 开镜后缩放（仅带镜狙击枪） |
| `1` / `2` | 手雷 / 增益道具 |
| `Esc` | 暂停 |

键位统一定义在 [`KeyBindings.h`](CKZ_source/pc_port/platform_sdl/KeyBindings.h)，改键和调鼠标灵敏度只需改这一个文件。暂停、弹出对话框或回到菜单时鼠标自动释放。PC 版固定使用原版的“开火按钮”模式，设置菜单里的开火按钮开关不生效。

游戏加载完成后，直接在键盘上敲出作弊码即可生效，定义在 [`Cheat.h`](CKZ_source/pc_port/platform_sdl/Cheat.h)：

| 作弊码 | 效果 |
| --- | --- |
| `chxp` | 等级 +1 |
| `chxxp` | 等级 +10 |
| `chm` | 现金和点券各 +9,999,999 |

PC 版原生编译游戏代码，使用 SDL3 接入窗口和输入、桌面 OpenGL 适配 GLES 1.1，并以 OpenAL Soft 播放声音。Unicorn 在进程内执行原版 ARMv7 Swerve 核心。片头视频由 Windows 自带的 Media Foundation 解码，鼠标左键点一下即可跳过。广告、联网服务和真实货币支付按离线模式处理。

## 内存检查

安装 Visual Studio 的 x86 AddressSanitizer 组件后，可用 MSBuild 属性 `CkzAddressSanitizer=true` 构建带原生内存检查的版本，产物目录带 `-ASan` 后缀。

## 目录

| 路径 | 内容 |
| --- | --- |
| `vs/` | Visual Studio 解决方案、三个项目及共用属性表 |
| `build_pc.bat` | Release Win32 一键构建入口 |
| `CKZ_source/` | 恢复出的游戏源码，`packages/` 和 `src/` 沿用原目录布局 |
| `CKZ_source/pc_port/` | PC 平台层、Swerve 兼容层和第三方依赖 |
| `scripts/` | LLVM 准备和 Swerve 镜像部署脚本 |
| `out/`、`bin/` | 构建中间产物和可运行文件 |

比对工具（逐函数、数据、链接三层校验）和恢复过程的记录不在本仓库中。

## 免责声明

- 本项目与 Glu Mobile 及其关联方没有任何关系，也未获得其授权或认可。
- Contract Killer 系列的名称、商标、美术、音频、关卡等游戏内容，以及原版程序，版权均归其权利人所有。
- 本仓库包含恢复出的源码、构建脚本、独立的 Swerve ARM 兼容镜像，以及**从原版二进制生成的 Swerve 引擎核心和 OpenSSL 汇编块**。运行需要自备合法取得的游戏素材。
- 本项目仅用于学习、研究和保存目的，不得用于商业用途。
- 代码按“现状”提供，不提供任何形式的担保；使用风险自负。
