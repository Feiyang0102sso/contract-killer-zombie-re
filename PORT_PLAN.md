# CKZ 跨平台移植计划

> 分支：`pc-port`（`ckz_repo` 的 worktree）
> 起点：匹配仓库检查点 `2026-11-08-b176-family-a`（自家 71.01% 逐字节一致，可玩闭包 100%，已在 iPad 实机跑通）
> 状态：**草案，待审核**

## 1. 目标与非目标

**目标**

- 同一份源码编出 PC（Windows）、iOS（arm64）、Android（arm64-v8a）版本。
- 游戏行为与原版一致。正确性靠**对拍**证明，不靠肉眼。
- 尽量少改游戏代码，匹配仓库后续改进的函数仍能合并进来。

**非目标**

- 移植版不追求逐字节一致。
- 不恢复广告、统计、IAP、GameCenter、推送，继续用空实现。
- 不修原版玩法 bug，不重做美术，不改游戏逻辑。

## 2. 已知事实（计划的依据）

| 项 | 事实 | 对移植的意义 |
| --- | --- | --- |
| Swerve 引擎核心 | `swerve_core.s`：ARM 模式机器码，无符号，只导出 `_swvNew`（规模见 2.1） | 现代 iOS 只跑 arm64，Android 64 位成为主流，这块**必须替换** |
| Swerve 对外接口 | `tws.h` 为 COM 风格接口，游戏实际用到 **72 个方法**，分布在 58 个源文件；语义是 M3G 场景图 | 接口面窄，适配层工作量可控 |
| M3G 资源 | `zombsniper.big` 里有 **119 个 M3G 文件，全部是 1.0 版本**，只用 21 种标准对象类型（见 2.2） | 标准格式，**可以直接用开源 M3G 实现加载** |
| Swerve 移植层 | `malij297_*`（KTX/ETC/PNG/JPEG 加载、VBO 填充）已有源码 | 直接复用 |
| 平台层 | `platformglu` 是"抽象类 + `*_Cocoa` 实现"结构（CFile/CGraphics/CSocket/CSystemFont/CMoviePlayer…） | 已有现成接缝，新增 `*_SDL` 实现即可 |
| 渲染 | 游戏和 Swerve 都是 GLES1.1 固定管线 + OES FBO 扩展 | 需要 GLES1 支持（ANGLE 或系统原生） |
| 音频 | OpenAL | 换 OpenAL Soft，API 不变 |
| ObjC | 游戏 `src/` 只有 3 个 ObjC 文件，其余都在 platformglu/cocoa 与各 SDK 包 | ObjC 能完整隔离在 iOS 壳里，PC 和 Android 不需要 ObjC |
| 宽字符 | `wchar_t` 出现在 170 个文件（808 处），`L"..."` 在 125 个文件，`wcslen` 等 libc 宽字符函数约 190 处，`sizeof(wchar_t)` 53 处 | iOS/Android 上 `wchar_t` 是 4 字节，**Windows 上是 2 字节**，见风险 R7 |
| Lua 脚本 | `.big` 里是明文 Lua 源码，不是字节码 | 没有 32/64 位字节码兼容问题 |
| OpenSSL | 只有 `CSocket_Cocoa.cpp`（SSL 连接）和 MD5 在用 | 联网部分直接做空实现，MD5 用小型独立实现 |
| ARM 专用代码 | `platformglu/src/arm/ARM_math.*`，另外 `lvorbis/os.h` 有内联汇编 | 需要提供通用 C 版本 |
| 纹理 | 平台层有 PVRTC 处理分支，Swerve 侧有 ETC 软解码 | Android 不支持 PVRTC，要查清 `.big` 里实际用了哪些格式 |

### 2.1 Swerve 职责清单

**规模**

| 项 | 数字 |
| --- | --- |
| 代码 | 407,516 字节（ARM 模式） |
| 函数入口 | 3000 多个，其中大量是几条指令的 getter/setter；带栈帧的非叶函数约 1700 个 |
| 虚表中的函数地址 | 1053 个，约 30 个类 |
| 外部依赖 | 78 个 gl 函数；约 30 个 `malij297_*` 回调（内存、报错、zlib 解压、ETC 解码、上下文切换）；libc 只用 memcpy、三角函数、setjmp/longjmp |
| 字符串 | 52 条，基本是报错信息 |

**负责**

1. M3G 文件加载：分段解析、Adler32 校验、对象引用解析（解压调用游戏侧的 zlib）
2. 场景图：World、Group、Mesh、SkinnedMesh、Sprite3D、Camera、Light、Appearance、Texture2D 等约 30 个类，含引用计数和克隆
3. 动画：关键帧插值、AnimationController 混合、`animate(time)`
4. 骨骼蒙皮，矩阵与四元数运算
5. 渲染：GL 状态、光照、材质、多纹理混合、合成模式、背景、FBO、纹理上传与 mipmap、VBO/VAO
6. 射线拾取（游戏的命中判定）
7. 错误处理：setjmp/longjmp 模拟异常

**不负责**：文件读取、图片解码（`malij297_*`，已有源码）、2D UI（spriteGlu）、音频、输入、GL 上下文。

引擎本身带有 M3G 2.0（JSR-297）的部分功能，如 PointSpriteMode 和 VAO，但游戏代码和资源**只用到 M3G 1.0 的子集**。

### 2.2 M3G 资源统计

119 个文件，版本全部为 1.0，没有 MorphingMesh，没有外部引用。各类对象数量：

| 对象 | 数量 | 对象 | 数量 |
| --- | ---: | --- | ---: |
| AnimationTrack | 11,974 | PolygonMode | 537 |
| KeyframeSequence | 10,851 | AnimationController | 310 |
| VertexArray | 7,265 | CompositingMode | 190 |
| Group | 5,860 | Material | 154 |
| TriangleStripArray | 4,764 | Sprite3D | 135 |
| VertexBuffer | 2,121 | Background | 114 |
| Mesh | 2,114 | World | 114 |
| Appearance | 893 | Camera | 94 |
| Image2D | 892 | Light | 67 |
| Texture2D | 846 | SkinnedMesh | 45 |

动画数据占了大头，**关键帧插值和动画混合是对拍的重点**。

## 3. 总体架构

```text
┌──────────────────────────────────────────────────────┐
│ 游戏代码 src/（原样保留，只做 64 位修正与少量 #if）    │
├──────────────────────┬───────────────────────────────┤
│ swerve_compat        │ platformglu 抽象层（原样）     │
│  tws.h 适配层        │   *_SDL 新实现（替代 *_Cocoa）  │
│  开源 M3G 1.x 核心   │                               │
├──────────────────────┴───────────────────────────────┤
│ SDL3（窗口/输入/生命周期/文件）  OpenAL Soft（音频）  │
│ GLES1.1：ANGLE（Metal / Vulkan / D3D11）或系统原生    │
├──────────────────────────────────────────────────────┤
│ PC（Windows，VS sln） · iOS 壳 · Android 壳           │
└──────────────────────────────────────────────────────┘

对拍基准：Unicorn 运行原版 swerve_core（ARM）→ 记录 GL 调用流与场景状态
```

**主开发平台是 PC。** 移动端只做一层薄壳。所有逻辑问题都应该在 PC 上复现、在 PC 上修。

## 4. 工程约定

### 4.1 构建

| 平台 | 构建方式 |
| --- | --- |
| PC 32 位（主开发） | **Visual Studio 解决方案** `port/vs/ckz_pc.sln`，平台 `Win32` |
| PC 64 位（P5） | 同一个 sln 增加 `x64` 平台 |
| iOS | Xcode 工程（P6） |
| Android | Gradle + NDK（P6） |

- **PC 平台工具集默认用 `ClangCL`**（VS 自带的 clang-cl），MSVC 作为备选。理由：原版就是 clang 编的，前端行为（求值顺序、GNU 扩展、`__builtin_*`）一致，出错面最小；游戏代码里 GNU 扩展只有 3 个文件，`__builtin_memcpy/memset` 共 10 处，ClangCL 都能直接编。
- sln 按库拆 vcxproj：`game`（src/）、`platformglu`、`platform_sdl`、`swerve_compat`、`m3g_core`、`lua`、`spriteglu`、第三方开源库，主程序 `ckz_pc` 链接它们。
- 原版工具链（`toolchain/`）保持不动，与移植构建互不影响。

### 4.2 目录

新增内容放在新目录，不改变原有文件位置。

| 目录 | 内容 |
| --- | --- |
| `port/vs/` | VS 解决方案与各 vcxproj |
| `port/swerve_compat/` | `tws.h` 适配层 |
| `port/platform_sdl/` | platformglu 的 SDL 实现 |
| `port/stubs/` | 移植版专用的空实现 |
| `port/shells/{ios,android}/` | 移动端工程 |
| `third_party/` | 开源 M3G 核心、SDL3、OpenAL Soft、ANGLE 等，注明来源、版本和许可证 |
| `tools/swerve_oracle/` | 对拍台 |

### 4.3 改游戏代码的规则

只做三类改动，每处都加 `// PORT:` 注释，方便和 `main` 对比合并：

1. 64 位修正
2. 平台条件编译
3. 编译器兼容（新版 clang 的报错）

定期把 `main` 合并进 `pc-port`，冲突只可能出现在带 `// PORT:` 的位置。

## 5. 里程碑

规模标记：S（小）、M（中）、L（大）。每一步都有明确的完成标准，没达标不进入下一步。

### P0 构建骨架（S）

- 建 `port/vs/ckz_pc.sln`，`Win32` + `ClangCL`，先编译全部游戏 C++ 代码，暂不链接。
- ObjC 文件、Cocoa 实现、SDK 包、两个 `.s` 不进构建。
- 新编译器报的错按类别登记（语法、废弃 API、缺符号），逐类修复。
- 同时调查两件事：
  1. 宽字符的来源：资源里的文本按几字节存放，读进来后是否直接当 `wchar_t` 数组用（决定 R7 怎么处理）。
  2. 用 b143 解包脚本统计 `.big` 里的纹理格式（PVRTC 用量）。

**完成标准**：除平台层和 Swerve 以外，所有 C++ 编译单元在 Win32 下编译通过；链接缺口清单生成；R7、R3 有结论。

### P1 Swerve 对拍台（M）

- 用 Unicorn 加载原版 `zombsniper` 的 Swerve 段（或 `swerve_core` 按原地址），从 `_swvNew` 进入。
- 接管导入：gl* 记录参数并返回合理值，libc 和 `malij297_*` 回调用宿主实现。
- 调用约定按 iOS armv7（softfp，r9 易失），用宿主侧的小桩函数调用 `tws.h` 里的接口。
- 输入：119 个 M3G 资源 + 游戏的典型调用序列（加载、`animate(t)`、取变换、拾取、渲染）。
- 输出：结构化 JSON 记录，包括节点树、每个节点的世界矩阵、动画采样值、拾取结果、GL 调用流。

**完成标准**：119 个资源全部跑通加载和一帧渲染，生成基准数据集。

> 只有这一步需要碰 `.s`，而且只需要知道它的导入表和调用约定，不需要读懂内部逻辑。

### P2 swerve_compat（L）

**优先采用开源 M3G 实现**，自己写只做兜底。

**P2a 选型评估（S）**

| 候选 | 说明 | 待核实 |
| --- | --- | --- |
| m3gcore | Nokia M3G 1.1 的 C 语言核心实现，经 Symbian Foundation 开源 | 许可证、能否在 GLES1.1 上编译、仓库是否完整 |
| Desktop-M3G | C++ 的 M3G 1.1 实现 | 同上，以及完成度 |
| 其他（XMM3G、Symbian Java runtime 里的 M3G JNI 源码） | 备选 | 同上 |

评估标准，按顺序筛选：
1. 许可证允许本项目使用和分发；
2. 能编进 `Win32` 工程；
3. 能加载全部 119 个 M3G 文件；
4. 抽样对拍：节点树、矩阵、动画采样与 P1 基准一致。

**P2b 适配（M）**

- 选定的核心放进 `third_party/`，原则上不改；必须改时单独成补丁并记录。
- 在 `port/swerve_compat/` 写 `tws.h` 的 72 个方法，转发到 M3G 核心。
- 错误码、对象生命周期（引用计数）、userID 查找等按原版接口语义对齐。

**P2c 对拍收敛（M）**

- 按对象类别逐项对拍，记录进度表（对象类别 × 一致/不一致）。
- 浮点比较带容差。GL 调用流先比"语义"（状态、绘制次数、顶点数据），不要求调用顺序完全一致。
- 出现不一致时，**只读 `.s` 里对应的那个函数**，确认原版行为后改适配层或打补丁（局部反编译）。

**兜底**：如果没有候选通过 P2a，就按 JSR-184 规范自己实现 M3G 1.0 子集，对拍流程不变。

**完成标准**：119 个资源的节点树、矩阵、动画采样、拾取结果与基准一致（在容差内）；渲染截图与基准的 GL 回放截图一致。

### P3 SDL3 平台层（M，可与 P2 并行）

按 platformglu 现有抽象逐个补 `*_SDL` 实现：

| 子系统 | 原实现 | 新实现 |
| --- | --- | --- |
| 应用/主循环/生命周期 | AppDelegate / CApplet_mm | SDL3 主循环与事件 |
| 图形上下文 | EAGL | SDL3 + ANGLE（或原生 GLES1） |
| 文件 | CFile_Cocoa / CFileMgr_Cocoa | 标准 C 文件 + SDL 路径（资源目录、存档目录） |
| 触摸/陀螺仪 | UIKit / CoreMotion | SDL3 触摸与传感器；PC 用鼠标模拟 |
| 音频 | OpenAL | OpenAL Soft |
| 系统字体 | CSystemFont_Cocoa（CoreText） | stb_truetype 或 SDL_ttf，附带字体文件 |
| 视频（6 个 m4v） | CMoviePlayer_Cocoa | 第一版跳过；之后视情况接解码库 |
| 网络/SSL/推送/通知 | Cocoa + OpenSSL | 空实现 |
| ARM 数学 | ARM_math.cpp | 通用 C 版本 |

**完成标准**：PC 上能打开窗口、加载资源、进入主菜单，输入和声音正常（Swerve 未就绪时可先用空渲染兜底）。

### P4 PC 可玩（32 位）（M）

- 接上 swerve_compat，跑完整流程：主菜单 → 世界地图 → 关卡 → 结算 → 存档读档。
- 开 ASan（ClangCL 支持）跑一遍，修复内存错误。
- 和 iPad 原版对照录屏，确认关键场景一致（武器、僵尸动画、特效、UI）。

**完成标准**：主线关卡能通关，ASan 无报错。

### P5 64 位（M）

- sln 增加 `x64` 平台，打开 `-Wint-to-pointer-cast -Wpointer-to-int-cast -Wshorten-64-to-32`，逐条处理警告。
- 重点：从文件直接读进结构体的数据、指针和 int 互转、按 4 字节假设的偏移计算、哈希里用到的指针。
- **注意数据模型差异**：Windows x64 是 LLP64（`long` 仍是 4 字节），iOS/Android arm64 是 LP64（`long` 是 8 字节）。PC x64 跑通不代表移动端没问题，`long` 相关的问题要在 Android 模拟器（x86_64）上再验一遍。
- 存档格式保持与 32 位一致（固定宽度类型）。

**完成标准**：Windows x64 和 Android 模拟器上都能完整通关，存档可在 32/64 位之间互读。

### P6 iOS / Android 壳（M）

- **iOS**：Xcode 工程 + SDL3 + arm64；保留需要的 ObjC（生命周期、文件路径）。
- **Android**：Gradle + NDK + SDL3 的 Android 模板，arm64-v8a；资源用 SDL 读取 APK assets，或首次启动时解包。
- 处理纹理格式：Android 上不支持的格式（如 PVRTC）离线转成 ETC2/ASTC，或者在加载时解码成 RGBA。
- 屏幕适配：原版按 iPhone/iPad 分辨率布局，需要处理现代宽高比（先加黑边）。

**完成标准**：两个平台在真机上完整通关。

## 6. 风险与待核实

| # | 风险 / 疑点 | 处理 |
| --- | --- | --- |
| R1 | Swerve 的行为与 M3G 规范或开源实现有出入（插值细节、渲染排序、私有扩展） | P1 基准会暴露；只针对不一致的函数读 `.s` |
| R2 | ANGLE 的 GLES1 模拟在 Metal/Vulkan 上缺个别扩展（`*MALIJ297`、point sprite） | P3 先在 PC 上用 ANGLE 验证；不行再退回系统原生 GLES1 |
| R3 | `.big` 里的纹理格式（PVRTC 用量） | P0 统计 |
| R4 | 未匹配的 639 个 DIFF 函数里可能有行为偏差 | 对照 iPad 原版录屏；发现问题回到匹配仓库修，再合并过来 |
| R5 | 原版依赖 iOS 特定行为（文件路径大小写、浮点精度、`char` 有无符号） | iOS 上 `char` 是有符号的，Android arm64 默认无符号，因此统一加 `-fsigned-char`；文件名大小写统一 |
| R6 | 联网功能（gServe、Facebook）服务器早已下线 | 统一做空实现，不影响单机玩法 |
| R7 | **Windows 的 `wchar_t` 是 2 字节**，原版和移动端是 4 字节；`sizeof(wchar_t)` 有 53 处，若资源里的文本按 4 字节存放并直接当 `wchar_t` 读，PC 版会出错 | P0 先查清文本的存放格式；需要时在读取边界做转换，或游戏内部统一用固定 4 字节的字符类型 |
| R8 | 开源 M3G 实现的许可证不允许使用，或者缺少关键功能 | P2a 先核实；不通过就走兜底的自写方案 |

## 7. 决策记录

**已定**

| 决策 | 结论 |
| --- | --- |
| Swerve 替换方式 | 优先采用开源 M3G 实现 + `tws.h` 适配，以原版对拍；不整体反编译 `.s`，只在对拍不一致时局部阅读 |
| PC 32 位构建 | Visual Studio 解决方案（`Win32`），工具集默认 ClangCL |

**待定**

1. GLES1 后端：**ANGLE 全平台统一（推荐）**，还是各平台用系统原生 GLES1。
2. 视频：第一版跳过，还是接解码库。
3. 第三方库获取方式：git submodule，还是直接放进 `third_party/`。
4. 平台工具集：ClangCL（推荐）还是 MSVC。
