# CKZ 跨平台移植计划

> 分支：`pc-port`（`ckz_repo` 的 worktree）
> 起点：匹配仓库检查点 `2026-11-08-b176-family-a`（自家 71.01% 逐字节一致，可玩闭包 100%，已在 iPad 实机跑通）
> 状态：**P3、P4 已达成（PC 可玩）；P2 原生 M3G 引擎已成为默认后端（M0、M1、M3、M4、M5 完成，M2 对拍工具未单独做）**

## 0. 当前进度（2026-11-09）

- PC 32 位已可玩：游戏和平台层是原生 x86，只有 Swerve 核心（`swerve-arm.bin`）在 Unicorn 里跑，见 `CKZ_source/pc_port/swerve_compat/ArmMachine.cpp`。
- Unicorn 方案的性能上限已经摸清，**决定换成原生 M3G 引擎**（P2）。依据：
  - 已做的优化：unicorn `notdirty_write` 补丁、除法改为在 guest 内执行。升级界面的加载卡顿从约 1.5 秒降到 0.4 秒。
  - 剩下的瓶颈是结构性的：僵尸多时每帧约 1900 次 API 调用和约 3100 次导入陷阱，再加上每帧约 1 万个顶点的 CPU 蒙皮。实测僵尸多时约 30 帧，清空后 60–70 帧。
  - 试过把 setjmp 放进 guest、用 unicorn 多出口代替陷阱 hook，都没有显著收益，已撤回。
  - 目标是 165 帧（每帧约 6ms），只能去掉模拟器。
- 调试工具（只在测试版 `zombsniper_tests.exe` 里，见 `tests/ArmProfiler.h`）：`zombsniper.cfg` 设 `Engine=swerve`，环境变量 `CKZ_ARM_PROFILE=1` 启动，按 F8 开始和停止录制，会写出 `arm_profile.txt`（guest 基本块热点）和 `arm_callers.txt`（按游戏侧调用栈统计的 Swerve 耗时）。
- **2026-11-09 夜更新**：原生 m3gcore 后端已设为默认（`CKZ_SWERVE=arm` 切回模拟版；2026-11-10 起改为 `zombsniper.cfg` 的 `Engine=swerve`）。修好了命中特效白色巨型三角形和爆头无伤害两个 bug；生存模式 15 只僵尸时稳定 165 帧（垂直同步上限），主线程 CPU 约 1.2ms/帧。详见 P2 各里程碑。
- 自动化测试设施（**2026-11-10 起移到 `tests/`，只编进测试版**：`build_pc.bat tests` 生成 `zombsniper_tests.exe`，与游戏同目录、共用素材；各项仍靠环境变量开启，见 `tests/TestTools.h`）：`CKZ_AUTOPLAY=survival` 自动开始生存模式；`CKZ_DEBUG_KEYS=1` 后 F10 开火、方向键转视角、F11 朝最近的头开一枪、F7 瞄准（鼠标被捕获时 SDL 读 raw input，外部注入的鼠标消息无效，键盘消息有效）；`CKZ_PICK_TEST=1` 每 1.5 秒对屏幕内每个僵尸的头部做拾取自检；`CKZ_HIDDEN=1` 隐藏窗口；`CKZ_M3G_LOADTEST=<目录>` 跑 m3g 加载测试。游戏版的调试项改由 `zombsniper.cfg` 控制：`DebugMode=1` 打开黑框和 F9（导出下一帧），再加 `FpsLog=1` 每秒记录帧率、工作耗时、主线程 CPU 时间和存活僵尸数；`CKZ_SWERVE`、`CKZ_NO_VSYNC`、`CKZ_FPS_LOG` 已由 `Engine`、`VSync`、`FpsLog` 取代。所有输出写入 exe 同目录的同名 `.log`。`CKZ_INPUT_TRACE`、`CKZ_GL_DEBUG`、`CKZ_NATIVE_TRACE` 已删除。

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

### P1 Swerve 对拍台（已有雏形，并入 P2 的 M2）

Unicorn 跑原版 Swerve 的部分已经在游戏里用着（`ArmMachine.cpp`：按原地址加载 `swerve-arm.bin`，从 `_swvNew` 进入，导入走陷阱交给宿主实现）。剩下的工作是录制基准和对比，放到 P2 的 M2 里做。

### P2 原生 M3G 引擎替换（L，进行中）

**目标**：去掉 Unicorn，Swerve 由原生的开源 M3G 核心加适配层取代，行为以原版 Swerve 为准对拍。

**选型结论（P2a，已完成）**

| 候选 | 许可 | 情况 | 结论 |
| --- | --- | --- | --- |
| [J2ME-Loader](https://github.com/nikita36078/J2ME-Loader) 里的 m3gcore 移植（`app/src/main/cpp/m3g/`） | 文件保留 Nokia 的 EPL-1.0 头 | Nokia 官方 M3G 1.1 的 C 核心，约 3.8 万行，含加载、动画混合、SkinnedMesh、拾取；已去掉 Symbian 依赖，在 Android 上跑过大量真实游戏，2026 年仍在修 bug | **采用** |
| [SymbianSource 官方镜像](https://github.com/SymbianSource/oss.FCL.sf.os.graphics) `m3g/m3gcore11/` | EPL-1.0 | 同一份代码，没有移植层 | 作为上游原始实现，用来比对 |
| [Desktop-M3G](https://github.com/BaalNetbek/desktop-m3g) | MIT | C++，缺 bindTarget 和立即模式，真实游戏验证少 | 备选，或用来交叉校验 |
| FreeJ2ME-Plus、KEmulator | GPL / 不明 | Java 实现 | 只作行为参考，不搬代码 |

EPL-1.0 是文件级弱 copyleft：我们改过的 m3gcore 文件要以 EPL 提供源码并保留版权头；适配层和游戏代码不受影响。

**结构**

```text
游戏代码（不改）
   │  tws.h / SwerveApi.h（PC 移植里的副本，唯一的接口边界）
   ├── arm 后端：Unicorn + 原版 Swerve（保留，用来对拍和兜底）
   └── native 后端：适配层 → m3gcore（C）→ GLES1 shim → 桌面 GL
```

后端运行时可切换：默认是 native；`zombsniper.cfg` 设 `Engine=swerve` 切回原版模拟（最初是环境变量 `CKZ_SWERVE=arm`）。同一个游戏、同一份存档可以直接做 A/B。

#### M0 落地核心（S）—— 已完成（2026-11-09）

> 结果：m3gcore 取自 J2ME-Loader `afffdde`，原样放进 `3rd_party/m3gcore/`，没有改动。平台层是自己写的，在 `pc_port/m3g_platform/`（假 EGL、zlib inflate、日志），另在 GLES shim 里补了 4 个定点函数。`vs/m3gcore.vcxproj` 编成静态库，链进 `ckz_pc`。
> 用 `CKZ_M3G_LOADTEST` 测试原生加载 `.big` 里的 **119 个 M3G：全部成功**。根对象为 114 个 World、5 个 Group，总计 269MB，耗时 3.1 秒。细节见 `3rd_party/README.md`。

- 从 J2ME-Loader 取 m3g 源码，钉在一个固定提交，放进 `CKZ_source/pc_port/3rd_party/m3gcore/`，在 `3rd_party/README.md` 记录来源、提交和许可。
- 和 SymbianSource 官方镜像逐文件比对，列出 J2ME-Loader 改过的地方及原因。
- 清点平台层依赖：EGL、zlib、日志和断言、定点 GL 函数（`glTexParameterx`、`glOrthox` 等）。
  - EGL 换成「使用外部已激活的 GL 上下文」。
  - zlib 改用游戏自带的那份。
  - shim 里补上缺的定点函数。
- 新建 `vs/m3gcore.vcxproj`，编成 Win32 静态库。
- 写一个最小加载器测试：用原生 m3gcore 加载 `.big` 里全部 119 个 M3G。

**完成标准**：静态库编译通过；119 个文件全部加载成功；平台层改动清单写进 `3rd_party/README.md`。

#### M1 后端抽象（S～M）

- 把 `tws.h` / `SwerveApi.h` 里的 `ckz::arm::Method(...)` 调用改成走一层可切换的实现，arm 后端的行为保持不变。
- 统计游戏实际调用的方法（原估计 72 个），列成适配清单，后续 M3 按这张表推进。

**完成标准**：默认走 arm 后端时，游戏表现和帧率与现在一致；适配清单完成。

> 结果（已完成，做法与计划不同）：没有改 `tws.h` 的调用方式。原生对象和 ARM 对象一样，第一个字是函数表指针，游戏照旧按槽位调用；`ckz::arm::Method` 发现表项不是 guest 代码时直接调用宿主函数（self 作为第一个参数）。后端只在 `swvNew` 处选择一次（`ArmMachine.cpp`），**默认 native，`CKZ_SWERVE=arm` 走原版模拟**。适配清单就是 `swerve_native/Native*.cpp` 里各 `Fill*Slots` 的槽位表；未实现的槽位会打印 `Swerve native: <类> slot N is not implemented` 和调用栈。

#### M2 对拍工具（M）

- 独立的小程序 `ckz_m3g_compare`：对每个 M3G 分别用两个后端加载，然后导出并比较：
  - 节点树、对象属性、userID；
  - 若干时间点 `animate(t)` 后各节点的世界矩阵；
  - 固定射线的拾取结果；
  - 一帧离屏渲染的截图（逐像素差，带容差）。
- 输出进度表：对象类别 × 一致/不一致。

**完成标准**：能对 119 个文件输出差异报告。

> 现状（未单独做）：改用「游戏内 A/B」：同一个 exe、同一份存档，分别用两个后端跑同一段注入脚本，对比 F9 导出的帧和日志；语义存疑的槽位直接在两个后端上用同样的参数探测（例如用非单位矩阵调用每个 Transform 槽位）。这种方式已经找出下面 M3/M4 列的全部语义差异。独立的逐文件对拍工具仍值得做，用来系统覆盖动画插值和拾取，留作后续。

#### M3 native 后端（L）

- 按适配清单实现：加载器、Graphics3D 的 bind/render/release、World/Camera/Light/Background、变换、动画、拾取、Image2D 与动态图像、离屏渲染。
- 错误码、引用计数、对象生命周期、userID 查找按原接口语义对齐。原版用 setjmp/longjmp 模拟异常，适配层改为返回错误码。
- m3gcore 原则上不改；必须改时单独成补丁，注释标 `CKZ:`，并在 README 登记。

**完成标准**：M2 报告里各对象类别全部「一致」（容差内）。

> 结果（游戏用到的槽位已全部实现）：代码在 `pc_port/swerve_native/`，平台层在 `pc_port/m3g_platform/`。m3gcore 源文件未修改，唯一需要内部结构的扩展放在 `m3g_platform/M3gCkzCore.c`（见 `3rd_party/README.md`）。和标准 M3G / m3gcore 不同、已在 ARM 后端核实的 Swerve 语义：
>
> | 位置 | Swerve 的实际语义 | 不处理的后果 |
> | --- | --- | --- |
> | Transformable 槽位 41（tws.h 名为 `getCompositeTransform`） | 只返回通用矩阵（M3G `getTransform`） | 武器相机转向错误 |
> | Transformable 槽位 43 `setTransform` | 标准语义，但接受底行有 1e-9 级误差的矩阵 | m3gcore 拒绝，节点不动；适配层把接近 (0,0,0,1) 的底行归位 |
> | Transform 槽位 6（tws.h 名为 `multiply`） | **`Transform.set(Transform)`，即拷贝**，不是乘法 | 粒子基矩阵被自乘，缩放平方，命中处出现铺满屏幕的白色三角形（bug A） |
> | Transform 槽位 10 / 11 | `postRotate`（角度 + 轴）/ `postRotateQuat`（x, y, z, w） | —（与 m3gcore 一致，用非单位矩阵核实） |
> | Transformable 槽位 33 | 朝向的四元数 | — |
> | Group 槽位 83/84 `pick` | **拾取会进入 SkinnedMesh 的骨架**；m3gcore 只测蒙皮三角形 | 僵尸头部网格和头部命中球都挂在头骨上，爆头完全没有命中（bug B）。适配层在 m3gcore 拾取后补拾取所有可达骨架，取最近的命中 |
> | Group 槽位 82 `removeChild` | 允许移除被 m3gcore 标为骨骼（`hasBones`）的节点；m3gcore 复制节点时会把该标志带给克隆体 | 斩首飞出的头颅（20% 概率）无法移除，永久留在场景里 |
> | Swerve 扩展格式文件（文件头 `\xbbSWERVE\xab`，50 个） | 私有对象类型 25 是 PVRTC 贴图 | `SwerveExtendedM3g.cpp` 转成标准 Image2D |
> | Graphics3D `BindTarget` / `ReleaseTarget` | 与游戏共用 GL 上下文 | 保存、恢复游戏的 GL 状态；`glClearDepthf(1)` |

#### M4 游戏内收敛（M）

- 用 native 后端跑完整流程：主菜单 → 地图 → 任务 → 结算 → 升级动画 → 存档读档。
- 出现不一致时，**只读 `.s` 里对应的那个函数**，确认原版行为后修改适配层或给 m3gcore 打补丁。

**完成标准**：主要场景（武器、僵尸动画与蒙皮、特效、天空盒、UI 里的 3D）与 arm 后端画面一致。

> 结果（已完成，2026-11-09）：以下场景两个后端的 F9 帧逐张对比一致：主菜单与地图、地图上的升级动画（3D 僵尸头）、生存模式的武器/手部/僵尸/场景、机瞄与狙击镜、地面和墙面的命中特效（烟雾、碎屑、闪光）、爆头特效与 "Headshot!" 提示。头部拾取自检（`CKZ_PICK_TEST`）：原生修复前 0/50 命中头部，修复后与原版一样 100%。原生运行日志里已没有 m3gcore 报错和未实现槽位。
> 未逐项核对：剧情任务全流程、结算后的存档读档（存档与后端无关）、Evelyn 等任务专属场景（前一轮已核对过武器、手部、僵尸、Evelyn、主相机）。

#### M5 性能（S）

- 实测僵尸多和清空两种场景的帧率。
- 按热点优化，候选：蒙皮、渲染状态切换，必要时把蒙皮挪到 GPU。

**完成标准**：僵尸多时稳定在 165 帧（每帧约 6ms）。

> 结果（已达成，2026-11-09）：开垂直同步（165Hz 显示器），生存模式自动开局，`CKZ_FPS_LOG=1` 每秒记录：
>
> | 场景 | ARM 模拟（原版） | 原生 m3gcore |
> | --- | --- | --- |
> | 11–15 只僵尸 | 平均 34 帧，最低 31 帧，每帧约 30ms | 165 帧，主线程 CPU 约 1.2ms/帧 |
> | 6–10 只僵尸 | 平均 112 帧，最低 39 帧 | 165 帧，主线程 CPU 约 1.2ms/帧（最多 2.3ms） |
>
> 原生后端被显示器刷新率封顶，CPU 余量约 5 倍，所以没有做额外优化。游戏中最长一帧 10–16ms（开局一次刷出 15 只僵尸时）；更长的停顿只出现在玩家死亡切到结算界面时（约 85ms）。

**M5 之后**：native 后端设为默认，Unicorn 只留在对拍工具里，`swerve-arm.bin` 不再随游戏发布。

> 进度：native 后端已设为默认（`swvNew`）。Unicorn 和 `swerve-arm.bin` 暂时仍随 exe 构建和发布，作为 `Engine=swerve` 的对照；等用户实测确认后再挪进独立的对拍工具。

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
| R8 | 开源 M3G 实现的许可证不允许使用，或者缺少关键功能 | P2a 已核实：m3gcore 为 EPL-1.0，可用；功能缺口在 M2 对拍中暴露 |
| R9 | 游戏还通过 Swerve 处理非 M3G 内容：PNG/JPEG 图片、动态图像（`malij297_DynamicImage2D*`）、离屏渲染（`OffScreenImage*`），m3gcore 不一定覆盖 | M1 清点实际用量；缺的部分用游戏现有的解码代码和 FBO 补 |
| R10 | m3gcore 的 EGL 层改成「使用外部上下文」的工作量是推测 | M0 验证 |

## 7. 决策记录

**已定**

| 决策 | 结论 |
| --- | --- |
| Swerve 替换方式 | 优先采用开源 M3G 实现 + `tws.h` 适配，以原版对拍；不整体反编译 `.s`，只在对拍不一致时局部阅读 |
| PC 32 位构建 | Visual Studio 解决方案（`Win32`），工具集默认 ClangCL |
| 过渡方案（2026-11） | 先用 Unicorn 跑原版 Swerve 让 PC 版可玩；性能上限不够，不再继续优化模拟器 |
| M3G 核心（2026-11-09） | 采用 J2ME-Loader 移植的 Nokia m3gcore（EPL-1.0），以 SymbianSource 官方镜像为上游对照；Desktop-M3G 作备选 |
| 后端切换 | native 与 arm 两个后端并存，运行时切换，直到 M5 完成 |

**待定**

1. GLES1 后端：**ANGLE 全平台统一（推荐）**，还是各平台用系统原生 GLES1。
2. 视频：第一版跳过，还是接解码库。
3. 第三方库获取方式：git submodule，还是直接放进 `third_party/`。（现状：都直接放进 `CKZ_source/pc_port/3rd_party/`）
4. 平台工具集：ClangCL（推荐）还是 MSVC。
