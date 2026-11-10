# 测试

`tests/` 是 PC 移植的测试程序 `zombsniper_tests.exe`（工程 [`vs/ckz_tests.vcxproj`](../vs/ckz_tests.vcxproj)），用 [GoogleTest 1.15.2](../CKZ_source/pc_port/3rd_party/googletest-1.15.2/) 写断言。它和游戏放在同一个目录，共用素材、存档和 `zombsniper.cfg`，日志写在同目录的 `zombsniper_tests.log`。

```bat
build_pc.bat tests
```

## 运行

| 命令 | 作用 |
| --- | --- |
| `zombsniper_tests.exe` | 跑全部自动化测试 |
| `zombsniper_tests.exe --gtest_filter=Mission.*` | 只跑游戏内断言（约 2 分钟） |
| `zombsniper_tests.exe --gtest_filter=Weapons.*` | 游戏里每一把武器各开一局生存模式测伤害（约 15 秒一把） |
| `zombsniper_tests.exe --gtest_filter=Zombsniper/M3gBackends.*` | 只跑两个 Swerve 后端的逐文件对拍（约 10 分钟） |
| `zombsniper_tests.exe --gtest_list_tests` | 列出所有测试 |
| `zombsniper_tests.exe --play` | 正常玩游戏，附带 [`tests_tools/TestTools.h`](tests_tools/TestTools.h) 的调试键、自动开局等工具 |

自动化测试**在后台运行**：窗口启动即最小化，任务栏上有它的按钮，右键关闭就会立刻结束整轮测试；声音走 OpenAL Soft 的空设备，全程静音。帧率由测试程序限制在 60 帧（窗口最小化后垂直同步不再起作用，不限帧曾导致显卡驱动蓝屏）。

`--play` 默认是前台、有声音。设环境变量 `CKZ_HIDDEN=1` 可以让它也进入后台模式（最小化 + 静音），`CKZ_MUTE=1` 只静音。

| 环境变量 | 作用 |
| --- | --- |
| `CKZ_ENGINE=swerve` | 覆盖 `zombsniper.cfg` 的 `Engine`，让同一套游戏断言在原版 Swerve（Unicorn）上跑，用来和原生后端对照 |
| `CKZ_DIFF_ALL=1` | 对拍时输出每一条差异（默认每个快照只列前 12 条），排查单个文件时用 |

想同时跑两个实例（例如一边对拍一边跑游戏断言），把 exe 复制成另一个名字再运行：日志按 exe 名分开，重新编译时也不会因为 exe 被占用而链接失败。

测试结果在日志里：每个测试一行 `[PASS]`/`[FAIL]`/`[SKIP]`，失败时上面有 gtest 的详细输出；访问违例会记下模块偏移和栈顶，可以用 `llvm-symbolizer --obj=zombsniper_tests.exe 0x<偏移+0x10000000>` 还原。

## 测试内容

### `M3gBackendTest.cpp`：原生 m3gcore 与原版 Swerve 对拍

对 `zombsniper.big` 里的每个 M3G（172 个，含 50 个 Swerve 扩展格式），分别用原生后端和 Unicorn 里的原版 Swerve 通过游戏自己的 `CSwerveLoader` 加载，再用游戏调用的同一批槽位逐项比较。以原版为准，任何差异都算原生后端的错误：

- 节点类型、userID、子节点、动画轨道（控制器权重/速度/位置、序列时长/重复模式）
- 平移、缩放、朝向四元数、合成矩阵、到根节点的矩阵、可渲染/可拾取开关、相机投影
- 网格数据：顶点数、位置缩放偏移和数值、每个子网格的三角形索引（游戏的碰撞网格 `CCollision::BuildMesh` 就从这里来）
- 在 5 个世界时间 `animate()` 之后，再比较上面全部内容
- World 根节点：经活动相机在 9×9 屏幕网格上的拾取（slot 84，射击和爆头判定用），以及从相机出发的射线拾取（slot 83，投射物命中僵尸的精确判定用）；比较是否命中、距离、命中节点、法线和射线

另有「单控制器」阶段：重新加载场景，每次只把一个动画控制器权重设为 1、从小数序列位置 0.6 开始播放，在 95/455/1305 ms 后比较（游戏 `CDH_Animation` 就是这样播动画的）。

比较规则：数值按相对误差 2e-3；四元数 q 与 -q 视为相同；动画轨道按集合比较（顺序两边不同，游戏不依赖顺序）。拾取结果里命中与否、距离、射线严格比较，法线允许约 2°（原版蒙皮数值略有不同，僵尸身上的面法线会差一度左右）；两边命中**同一距离的不同节点**（墙上的贴花等共面面片）记为平局并在日志里计数，不算失败。

需要 `zombsniper.cfg` 的 `Engine=m3g` 和 exe 旁边的 `swerve-arm.bin`。

2026-11-10 的结果：172 个文件中 168 个一致。剩下 4 个都是拾取的数值边界：一个远景网格的插值法线差约 6°、一个蒙皮网格的法线、两个第一人称手臂模型（蒙皮）动画后的擦边射线。游戏的射击不对手臂模型做拾取。

### `GameplayTest.cpp`：生存模式里的游戏断言

测试程序从片头一路点进地图，把装备固定为 18 号步枪（0 号槽）和 37 号加特林（1 号槽），开始生存模式，然后逐项断言。中途遇到波次对话框会自动点掉；玩家被僵尸打死、任务结束时会回地图重开一局，长测试不会因此中断。

| 测试 | 断言 |
| --- | --- |
| `HeadPickFindsHeadHitZone` | 屏幕上每个僵尸的头部命中球中心，拾取结果都是 `HIT_HEAD` |
| `BodyPickFindsBody` | 头部下方的躯干点至少一半拾取为身体，且不会是头 |
| `HeadshotKillsAndBeheads` | 爆头击杀时 `m_beheaded` 置位（原版规则：头部命中使血量归零即斩首） |
| `BodyShotsDamage` | 打躯干会扣血，命中类型不是头 |
| `HandGrenadeHitsGroundAndExplodes` | 手雷先碰到关卡几何再爆炸；在关卡外"无声消失"判为失败 |
| `LauncherShellHitsSomething` | 榴弹在最大寿命之前撞到东西；寿命到期消失（不爆炸）判为失败 |
| `LauncherShellExplodesOnZombie` | 等僵尸走到 25 米内，转动视角把准星对准它的躯干再发射榴弹；6 发里至少三分之一要在僵尸身上（2.5 米内、离地 0.3 米以上）爆炸。这一项专门盯住修掉的射线拾取 bug：旧代码下榴弹全部穿过僵尸落在身后地面。只在原生后端上断言：原版在模拟器里帧率低、榴弹每帧跨度大，飞行过程无法一一对照（实测原生 8/10 打在身上，原版 2/6，修复前原生 0/2；原版偶尔也会穿过） |
| `ExplosivesFromManyAngles` | 16 个方向各扔一颗手雷、打一发榴弹：每颗手雷都要爆炸，每发榴弹都不能寿命到期。没碰到关卡就爆炸的手雷只统计、不判失败（见下） |
| `EveryEquippedWeaponDamages` | 存档里装备的每把武器都能造成伤害 |
| `Weapons.EveryWeaponHitsZombies` | 游戏里每一把武器依次放进 0 号槽、各开一局生存模式，精确射击僵尸头部：必须造成伤害；治疗枪（`projectile_type 2`）反过来，不能伤到僵尸。测完恢复存档原来的武器 |

手雷「凭空消失」：关卡的碰撞网格只覆盖玩家前方的区域，手雷落到玩家身后或侧后方时会穿过地面，1 秒后被出界检查（`HandGrenadeBallistics::PostSimulate`）在地面下引爆，看不见也没有伤害。16 个方向里原生 7～8 颗、原版 7～8 颗如此，是原版关卡本身的特性。

射击走 `CSwerveGame::Shot`。爆头和打躯干两项在每发前把散布（`m_zoom`）清零，只测命中逻辑；武器伤害保留游戏自己的散布，按多发统计。加特林要先转起来，测试会像按住扳机一样每帧调用 `Shot`，直到真的打出子弹。

游戏断言可以用 `CKZ_ENGINE=swerve` 在原版后端上原样再跑一遍，比较两个后端的游戏行为。2026-11-10 两个后端上 9 项 Mission 断言的结果一致（榴弹打僵尸一项见上）。

## 目录

| 文件 | 内容 |
| --- | --- |
| `TestProgram.*` | 入口：选择模式、后台运行、静音、gtest |
| `GameHarness.*` | 在测试里一帧一帧驱动真实游戏：进地图、进任务、点击、等待条件 |
| `BigArchive.*` | 只读 `.big` 归档（游戏自己的 `CBigFile_v2` 会改动运行中的归档路径，所以不用它） |
| `M3gBackendTest.cpp`、`GameplayTest.cpp` | 上面两组测试 |
| `tests_tools/` | 手动和脚本驱动用的工具：调试键、自动开局、头部拾取自检、ARM 性能分析、M3G 加载测试 |
