# Contract Killer: Zombies Reverse

**English** | [中文](README_CN.md)

## Overview

This project rebuilds the game for PC on top of the iOS source code.

The game source is a 1:1 reconstruction of `zombsniper` from the iOS 3.1.1 release, see the [original iOS reconstruction (`ori_IOS` branch)](https://github.com/Feiyang0102sso/contract-killer-zombie-re/tree/ori_IOS). On PC it is compiled natively for x86. The parts of the original that depend on iOS are replaced by the bridge layer below.

## Bridge layer

The game code (`CKZ_source/src/`, `CKZ_source/packages/`) is compiled as is. The bridge layer lives in [`CKZ_source/pc_port/`](CKZ_source/pc_port/) and replaces everything that is specific to the iOS platform:

| Original iOS | PC version | Location |
| --- | --- | --- |
| App lifecycle, touch | SDL3 window and events; mouse and keyboard are turned into the original touch controls | `platform_sdl/Applet.cpp`, `Controls.cpp` |
| OpenGL ES 1.1 | Desktop OpenGL compatibility context. The game draws into a 1024×768 framebuffer, which is scaled to the window keeping its aspect ratio | `platform_sdl/GlesDesktop.cpp`, `Graphics.cpp` |
| OpenAL | OpenAL Soft | `platform_sdl/Audio.cpp` |
| Intro movie player | Decoded by Windows' built-in Media Foundation; one left click skips it | `platform_sdl/Movie.cpp` |
| System font (dynamic text) | Windows system font; the original bitmap fonts are unchanged | `platform_sdl/SystemFont.cpp` |
| Files and saves | Win32 wide-character paths, so the game can live in a non-ASCII directory; saves go to `saves/` next to the EXE | `platform_sdl/Files.cpp` |
| Ads, analytics, online services, in-app purchases | Empty implementations, the game runs offline | `stubs/` |
| Swerve 3D engine | Two switchable backends, see below | `swerve_compat/`, `swerve_native/` |

### m3g engine

The original 3D engine, Swerve, is a precompiled ARM library without source. The game only calls it through function tables ([`SwerveApi.h`](CKZ_source/pc_port/swerve_compat/SwerveApi.h)): the first word of every object points to a table of functions, and the game calls them by slot. Putting our own functions in the same slots therefore replaces the whole engine.

`[graph] Engine` in `zombsniper.cfg` selects the backend:

| Value | Backend | Notes |
| --- | --- | --- |
| `m3g` (default) | Native m3gcore | Swerve's object interface reimplemented (`swerve_native/`) on [m3gcore](CKZ_source/pc_port/3rd_party/m3gcore/), the C implementation of Nokia M3G 1.1. Runs natively on x86 at a high frame rate |
| `swerve` | Original Swerve | Unicorn emulates, inside the process, the ARMv7 engine core `swerve-arm.bin` cut from the original binary (`swerve_compat/`). Behaves exactly like the original but is slower; mainly used for comparison |

## Windows x86 build

### Toolchain

| Component | Version | How to get it |
| --- | --- | --- |
| Visual Studio | 2022 or 2026 | Install it yourself with the Windows C++ x86 tools and the Windows SDK; provides MSBuild, headers, libraries and `rc.exe` |
| Python | 3.11 or later | Install it yourself; a `.venv` in the repository or its parent directory is preferred, or set the MSBuild property `CkzPython` |
| LLVM (`clang-cl`, `lld-link`) | 21.1.8 | Downloaded, verified and installed into `.cache/deps/` by [`prepare_toolchain.py`](scripts/prepare_toolchain.py) on the first build; needs internet access |
| SDL3, OpenAL Soft, Unicorn, m3gcore | See [`3rd_party/README.md`](CKZ_source/pc_port/3rd_party/README.md) | Included in the repository |

Double-click `build_pc.bat` in the root directory to build; it finds the installed Visual Studio through `vswhere`. Alternatively, open `vs/ckz_pc.sln` and build `Release | Win32`.

The output is `bin/Win32/Release/zombsniper.exe`. The build puts `SDL3.dll`, `OpenAL32.dll`, `zombsniper.cfg` and `swerve-arm.bin` next to the EXE.

### Game assets

The assets are not part of the repository. Take them from the original ipa (`Payload/zombsniper.app/`) and copy them to `bin/Win32/Release/assets/` by hand:

| File | Contents |
| --- | --- |
| `zombsniper.big` | Game data archive |
| `vga.big` | 1024×768 graphics archive |
| `en.big` | English text archive |
| `res_map.dat` | Resource name map |
| `ckz_intro_scene_1024.m4v` | Intro movie |

The required files are listed in [`AssetPaths.h`](CKZ_source/pc_port/platform_sdl/AssetPaths.h). They are checked one by one at startup, and any missing file is reported with its full path.

Running the game needs a graphics driver with a desktop OpenGL compatibility context and framebuffer objects, plus the x86 Visual C++ runtime.

## CFG settings

Settings live in `zombsniper.cfg` next to the EXE.

| Key | Default | Purpose |
| --- | --- | --- |
| `[common] Title` | `Contract Killer: Zombies` | Window title |
| `[graph] Engine` | `m3g` | 3D engine: `m3g` is the native m3gcore; `swerve` is the original Swerve emulated by Unicorn, slower, for comparison |
| `[graph] WindowWidth` / `WindowHeight` | `2048` / `1536` | Window size, scaled down proportionally when larger than the desktop |
| `[graph] FullScreen` | `0` | Borderless full screen; the picture stays 4:3 with black bars on the sides |
| `[graph] VSync` | `1` | Vertical sync |
| `[game] DrawFPS` | `1` | Show the frame rate in the top-left corner |
| `[debug] DebugMode` | `0` | Open a console window and enable the debug hotkey F9 (dump the next frame) |
| `[debug] FpsLog` | `0` | Log frame rate, frame time, CPU time and live zombie count every second; needs `DebugMode=1` |

## Controls

| Input | Action |
| --- | --- |
| Move mouse | Turn the view (in combat the mouse is locked and hidden) |
| Left mouse button | Fire, hold for automatic fire; click in menus and on the map |
| Right mouse button | Scope in / out |
| `R` | Reload |
| `Q` / `E` | Switch weapon |
| Mouse wheel | Zoom while scoped (scoped sniper rifles only) |
| `1` / `2` | Grenade / power-up |
| `Esc` | Pause |

## Cheat codes

| Code | Effect |
| --- | --- |
| `chxp` | Level +1 |
| `chxxp` | Level +10 |
| `chm` | +9,999,999 cash and gold |

Note that the level cheats affect how the random pool of side missions is generated. Some main missions may end up with no side missions to play alongside them. Don't worry: even if the pool goes wrong, side missions refresh normally once all main missions are finished.

## Disclaimer

- This project is not affiliated with, authorized or endorsed by Glu Mobile or its affiliates.
- The Contract Killer name, trademarks, art, audio, levels and other game content, as well as the original program, are the property of their respective owners.
- This repository contains recovered source code, build scripts, a standalone Swerve ARM compatibility image, and **Swerve engine core and OpenSSL assembly blocks generated from the original binary**. You need legally obtained game assets to run it.
- This project is for learning, research and preservation only, and must not be used commercially.
- The code is provided "as is", without warranty of any kind; use at your own risk.
