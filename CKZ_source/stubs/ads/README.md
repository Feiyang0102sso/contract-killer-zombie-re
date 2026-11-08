# 广告/统计 SDK 空实现（不是原版代码）

b107 为“首次链接/试玩”补的空实现，**不属于原版工程**，原版构建路径下没有 `stubs/` 目录。

| 文件 | 替代的原版组件 | 原版形态 |
|---|---|---|
| `AdMarvelStub.m` | AdMarvel SDK（`AdMarvelView`） | 预编译库 `packages/admanager/code/cocoa/libs/libAdMarvelSDK_armv7reduced.a` |
| `FlurryStub.m` | Flurry Analytics（`FlurryAnalytics`） | 预编译库 `packages/flurry/code/cocoa/libs/` |
| `TapjoyStub.m` | Tapjoy SDK（`TapjoyConnect`、`TJCCallsWrapper`） | A 区源码 `packages/tapjoy/src/ios/` |
| `PlayHavenStub.m` | PlayHaven SDK（`PHAPIRequest` 及子类、`PHNotificationView`） | A 区源码 `packages/playhaven/src/cocoa/` |

规则（用户决定，2026-11-08）：
- 只实现 Glu 接入层（`CAdManager_AdMarvel_Cocoa_Delegate.mm`、`CFlurry_Cocoa_mm.mm`、`TapjoyManager.mm`、`CPHManager.mm`）实际发送的消息；
- 一律返回 nil / 0 / NO，不发通知、不回调 delegate、不联网；
- 不改任何 Glu 代码；声明沿用各包里的 PROVISIONAL 头文件；
- 最终版改为保留原二进制块，届时删除本目录。
