# PC 移植的第三方依赖

| 目录 | 使用内容 | 上游 |
| --- | --- | --- |
| `unicorn-2.1.4/` | Win32 x86 宿主执行 ARM 指令所需的 C 源码、头文件和许可 | [Unicorn 2.1.4](https://github.com/unicorn-engine/unicorn/releases/tag/2.1.4) |
| `SDL3-3.2.28/` | 官方 Win32 x86 SDK 的头文件、导入库和 DLL | [SDL 3.2.28](https://github.com/libsdl-org/SDL/releases/tag/release-3.2.28) |
| `openal-soft-1.24.3-bin/` | 官方 Win32 x86 发布包的头文件、导出表和 DLL | [OpenAL Soft 1.24.3](https://github.com/kcat/openal-soft/releases/tag/1.24.3) |
| `powervr/` | PVRTC 解码器源码和许可 | [PowerVR Native SDK](https://github.com/powervr-graphics/Native_SDK/tree/63f0098acdc11ec9a072c30000d2fa670a955bee) |
| `gl-include/` | GLES 1.1 和 Khronos 平台头文件 | [OpenGL Registry](https://github.com/KhronosGroup/OpenGL-Registry/tree/6af574a14089ccfee87efe230ebcdd8742859813)、[EGL Registry](https://github.com/KhronosGroup/EGL-Registry/tree/57b4876de0f33677ece92dd9de0ef105ce69139d) |
| `libvorbis-1.2.0/` | 恢复版 Vorbis 库缺少的 `lpc.c` 和许可 | [Xiph.org 发布包](https://downloads.xiph.org/releases/vorbis/libvorbis-1.2.0.tar.gz) |
| `m3gcore/` | Nokia M3G 1.1 核心（C），原生 M3G 引擎，用来替换 Swerve；`inc/m3g_*.h` 与 `src/*.c、*.inl` | [J2ME-Loader `afffdde`](https://github.com/nikita36078/J2ME-Loader/tree/afffdde4e677b3d5ef516e713732abc488f19fc0/app/src/main/cpp/m3g)；上游原始实现见 [SymbianSource `ff133bc`](https://github.com/SymbianSource/oss.FCL.sf.os.graphics/tree/ff133bc50e6158bfb08cc093b0f0055321dcde99/m3g/m3gcore11) |
| `googletest-1.15.2/` | GoogleTest 的 `include/`、`src/` 和许可，只编进测试程序 `zombsniper_tests.exe`（`vs/ckz_tests.vcxproj`，`gtest-all.cc` 单文件编译） | [GoogleTest v1.15.2](https://github.com/google/googletest/releases/tag/v1.15.2) |

Unicorn 源码包含两处 Win32 修补。 `qemu/accel/tcg/translate-all.c` 将最后一次 JIT 内存提交限制在已保留的地址范围内。`qemu/include/qemu/compiler.h` 为 ClangCL 保留 ARM 向量访问所需的对齐属性。

另有一处性能修补（注释标 `CKZ:`）。`qemu/accel/tcg/cputlb.c` 的 `notdirty_write` 改为按 Unicorn 映射权限判断页面是否可执行，并且不再因 `*_UNMAPPED` hook 拒绝清除 dirty（新增的 `uc_mem_access_hook_installed` 在 `qemu/include/exec/exec-all.h`）。上游按 TLB 的 `addr_code` 判断，而 ARM 关闭 MMU 时所有页都可执行，结果每次写内存都要走慢路径。修补后，Swerve 升级界面的加载卡顿由约 1.5 秒降到约 0.7 秒，动画期间的模拟耗时约降为原来的四分之一。

## m3gcore

**许可**：每个文件都带 Nokia 的 Eclipse Public License v1.0 头（[EPL-1.0 全文](https://www.eclipse.org/legal/epl-v10.html)）。EPL 是文件级弱 copyleft：随游戏分发的、改过的 m3gcore 文件必须以 EPL 提供源码并保留版权头；平台层、适配层和游戏代码是独立模块，不受影响。

**来源**：取自 J2ME-Loader 的移植版，除下文「CKZ 补丁」列出的几处外原样复制。J2ME-Loader 相对 Symbian 原版的改动主要有两类：一是 64 位修正，指针改存为 `M3Gulong`（mesh、loader、morphingmesh 等）；二是在 Android 上跑真实游戏时修的渲染问题（rendercontext、image、appearance、sprite 等）。没有拿过来的文件：`m3g_android*.cpp`（平台层）、`jni/`、`javax_*.h`、`CSynchronization.*`（Java 绑定）。

**平台层**：在 `pc_port/m3g_platform/`，是我们自己写的代码，不属于 EPL。

| 文件 | 作用 |
| --- | --- |
| `include/EGL/egl.h` + `CkzEgl.cpp` | 假 EGL。桌面上只有 SDL 创建的那一个 GL 上下文，并且一直是激活状态，所以 m3gcore 的配置、上下文、surface 查询都按「只有这一个上下文」来回答；surface 只记录尺寸，供 `m3gBindEGLSurfaceTarget` 使用。 |
| `M3gPlatform.cpp` | M3G 段解压：游戏的 zlib 只有 inflate，没有 `uncompress`，所以这里手动调 inflate。另有日志、断言，以及原生位图/窗口查询（不支持，返回失败）。 |
| `M3gCkzCore.c` + `include/M3gCkzExtensions.h` | m3gcore 的编译入口：`#include` 原样的 `m3g_core.c`（unity build），再追加需要内部结构的扩展函数。`ckzM3gRemoveChild`：移除节点时跳过 m3gcore 的 `hasBones` 检查（m3gcore 复制节点时会把这个标志带给克隆体，斩首飞出的头颅因此无法移除；原版 Swerve 允许移除，已在 ARM 后端探测确认）。`ckzM3gGetSwerveTriangleIndices`：按 Swerve 的顶点顺序输出三角形索引（见下表）。 |

M0 验证用的 m3g 加载测试在仓库根目录的 `tests/tests_tools/M3gLoadTest.cpp`，只编进测试程序：设置环境变量 `CKZ_M3G_LOADTEST=<目录>` 后，用 m3gcore 加载目录里所有 `.m3g` 并逐个打印结果。

需要改变 m3gcore 行为时，优先在适配层处理，必须碰内部结构时加到 `M3gCkzCore.c`；只有改的是 m3gcore 内部的静态函数、无法从外面覆盖时，才直接改源文件，注释标 `CKZ:`。

**CKZ 补丁**（2026-11-10，都由 `tests/M3gBackendTest.cpp` 在游戏全部 172 个 M3G 上对拍原版 Swerve 找到并验证）：

| 文件 | 改动 | 原版 Swerve 的行为 | 不改的后果 |
| --- | --- | --- | --- |
| `src/m3g_keyframesequence.c`、`inc/m3g_keyframesequence.h`、`src/m3g_animationtrack.c` | `m3gGetSample` 改收浮点时间，不再先四舍五入到整毫秒 | 在精确的序列时间插值 | 游戏用小数位置和速度驱动控制器，所有动画都被量化到 1 ms，姿势有细微偏差 |
| `src/m3g_keyframesequence.c` | CONSTANT 序列的时间钳到 `[0, duration - 1]` | 播完后停在结束前 1 ms 的姿势，而不是最后一个关键帧 | 一次性动画的停止姿势不同；可见性轨道（值 0.496/1.496）在结尾被判为隐藏 |
| `src/m3g_mesh.c` | 拾取命中没有顶点法线的网格、或命中 SkinnedMesh 时，法线取三角形面法线 | 同左 | m3gcore 对无法线网格返回 (0,0,1)，对僵尸返回插值顶点法线：命中特效、投射物反弹方向不同 |
| `src/m3g_sprite.c` | 精灵拾取不再做纹理 alpha 测试 | 整个精灵矩形都可拾取 | 透明处的特效精灵在原版会挡子弹，原生直接穿过 |

m3gcore 用到的定点 GL 函数（`glClearColorx`、`glClearDepthx`、`glOrthox`、`glTexParameterx`）补在 `platform_sdl/GlesDesktop.cpp`。编译宏为 `M3G_TARGET_WIN32`，`M3G_GL_ES_1_1` 由 `m3g_defs.h` 自动打开。
