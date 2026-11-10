# PC 移植的第三方依赖

| 目录 | 使用内容 | 上游 |
| --- | --- | --- |
| `unicorn-2.1.4/` | Win32 x86 宿主执行 ARM 指令所需的 C 源码、头文件和许可 | [Unicorn 2.1.4](https://github.com/unicorn-engine/unicorn/releases/tag/2.1.4) |
| `SDL3-3.2.28/` | 官方 Win32 x86 SDK 的头文件、导入库和 DLL | [SDL 3.2.28](https://github.com/libsdl-org/SDL/releases/tag/release-3.2.28) |
| `openal-soft-1.24.3-bin/` | 官方 Win32 x86 发布包的头文件、导出表和 DLL | [OpenAL Soft 1.24.3](https://github.com/kcat/openal-soft/releases/tag/1.24.3) |
| `powervr/` | PVRTC 解码器源码和许可 | [PowerVR Native SDK](https://github.com/powervr-graphics/Native_SDK/tree/63f0098acdc11ec9a072c30000d2fa670a955bee) |
| `gl-include/` | GLES 1.1 和 Khronos 平台头文件 | [OpenGL Registry](https://github.com/KhronosGroup/OpenGL-Registry/tree/6af574a14089ccfee87efe230ebcdd8742859813)、[EGL Registry](https://github.com/KhronosGroup/EGL-Registry/tree/57b4876de0f33677ece92dd9de0ef105ce69139d) |
| `libvorbis-1.2.0/` | 恢复版 Vorbis 库缺少的 `lpc.c` 和许可 | [Xiph.org 发布包](https://downloads.xiph.org/releases/vorbis/libvorbis-1.2.0.tar.gz) |

Unicorn 源码包含两处 Win32 修补。 `qemu/accel/tcg/translate-all.c` 将最后一次 JIT 内存提交限制在已保留的地址范围内。`qemu/include/qemu/compiler.h` 为 ClangCL 保留 ARM 向量访问所需的对齐属性。

另有一处性能修补（注释标 `CKZ:`）。`qemu/accel/tcg/cputlb.c` 的 `notdirty_write` 改为按 Unicorn 映射权限判断页面是否可执行，并且不再因 `*_UNMAPPED` hook 拒绝清除 dirty（新增的 `uc_mem_access_hook_installed` 在 `qemu/include/exec/exec-all.h`）。上游按 TLB 的 `addr_code` 判断，而 ARM 关闭 MMU 时所有页都可执行，结果每次写内存都要走慢路径。修补后，Swerve 升级界面的加载卡顿由约 1.5 秒降到约 0.7 秒，动画期间的模拟耗时约降为原来的四分之一。
