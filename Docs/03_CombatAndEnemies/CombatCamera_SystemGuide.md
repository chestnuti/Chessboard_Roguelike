# 战斗相机聚焦系统说明

本文档说明敌人死亡时的短暂相机聚焦实现。该系统用于让镜头在敌人死亡瞬间轻微偏向死亡地块，即使死亡来源是远离玩家的敌人友伤或后续远程友伤，也能统一触发同一套表现。

## 当前目标

敌人死亡聚焦不依赖玩家 Pawn 的移动 Tick。玩家 Pawn 当前只在视觉移动期间启用 Tick，移动结束后会关闭 Tick；因此战斗相机效果由玩家控制器上的独立组件驱动。

运行时流程如下：

1. 所有敌人死亡仍统一调用 `AGridEnemyPawn::Kill()`。
2. `Kill()` 在标记死亡时广播 `OnGridEnemyKilled`，携带死亡敌人、死亡格坐标和死亡格世界位置。
3. `AGridEnemyManager::RegisterEnemy()` 会订阅每个已注册敌人的 `OnGridEnemyKilled`。
4. `AGridEnemyManager::HandleEnemyKilled()` 收到死亡事件后，通过玩家 Pawn 找到 `AGridPlayerController`。
5. `AGridPlayerController::FocusCombatCameraOnGridTile()` 转发死亡格世界位置。
6. `UCombatCameraDirectorComponent::FocusGridTileBriefly()` 在玩家当前视角目标或 Pawn 上查找 `USpringArmComponent`，短暂修改 `TargetOffset` 和 `TargetArmLength`。
7. 聚焦结束后组件恢复 SpringArm 的原始 `TargetOffset` 和 `TargetArmLength`，并关闭自己的 Tick。

## 新增文件

```text
Source/Chessboard_Roguelike/Public/Camera/CombatCameraDirectorComponent.h
Source/Chessboard_Roguelike/Private/Camera/CombatCameraDirectorComponent.cpp
```

## 新增接口

| 接口 | 所属类 | 说明 |
| --- | --- | --- |
| `OnGridEnemyKilled` | `AGridEnemyPawn` | 敌人死亡广播，参数为死亡敌人、死亡格坐标和死亡格世界位置 |
| `HandleEnemyKilled()` | `AGridEnemyManager` | 监听敌人死亡事件，并通知本地玩家控制器触发相机聚焦 |
| `CombatCameraDirectorComponent` | `AGridPlayerController` | 玩家控制器默认创建的战斗相机导演组件 |
| `FocusCombatCameraOnGridTile()` | `AGridPlayerController` | 蓝图可调用入口，将目标世界位置转发给战斗相机组件 |
| `FocusGridTileBriefly()` | `UCombatCameraDirectorComponent` | 蓝图可调用入口，让镜头短暂偏向指定世界位置 |
| `StopFocus()` | `UCombatCameraDirectorComponent` | 立即恢复相机原始状态并停止聚焦 |

## 相机组件参数

| 参数 | 默认值 | 说明 |
| --- | ---: | --- |
| `FocusInDuration` | `0.12` | 镜头偏向死亡地块的真实时间 |
| `FocusHoldDuration` | `0.18` | 保持聚焦的真实时间 |
| `FocusOutDuration` | `0.18` | 镜头恢复到原位的真实时间 |
| `MaxFocusOffset` | `650` | 从玩家到死亡地块的最大平面偏移采样距离，避免远处死亡把镜头拉得过猛 |
| `FocusOffsetScale` | `0.65` | 死亡地块平面偏移转换到 SpringArm `TargetOffset` 的比例 |
| `ZoomInDistance` | `60` | 聚焦期间缩短的 SpringArm 臂长 |

组件使用 `GetWorld()->GetRealTimeSeconds()` 推进时间，因此全局 Time Dilation 或 HitStop 不会阻止镜头回退。

## 使用方式

默认情况下，`AGridPlayerController` 构造时会创建 `CombatCameraDirectorComponent`。如果关卡使用 `BP_GridPlayerController`，蓝图实例会继承该组件。

该组件默认查找顺序为：

1. 当前 `PlayerController` 的 `ViewTarget` 上的 `USpringArmComponent`。
2. 当前受控 Pawn 上的 `USpringArmComponent`。
3. 组件 Owner 自身上的 `USpringArmComponent`。

如果当前相机没有 SpringArm，组件会安全跳过并输出 Verbose 日志。当前实现最适合 `BP_GridPawn` 持有 SpringArm 和 Camera 的俯视相机结构。

## 接入覆盖

因为触发点统一在 `AGridEnemyPawn::Kill()`，以下死亡来源都会触发聚焦：

- 玩家近战击杀敌人。
- 敌人近战跨阵营友伤击杀目标。
- 敌人近战跨阵营友伤导致攻击者死亡。
- 远程敌人调用 `ApplyRangedFriendlyFireDamage()` 击杀目标。
- 后续新增陷阱、地块效果或清除链，只要最终调用 `Kill()`，也会自动触发。

## 调试点

镜头没有聚焦时检查：

1. 敌人是否通过 `AGridEnemyManager::RegisterEnemy()` 注册，或 `RebuildEnemyList()` 是否已在 BeginPlay 执行。
2. 敌人死亡是否走到了 `AGridEnemyPawn::Kill()`，而不是只在蓝图里隐藏 Actor。
3. 玩家 Pawn 是否能通过 `GetController()` 找到 `AGridPlayerController`。
4. 当前 ViewTarget 或玩家 Pawn 是否存在 `USpringArmComponent`。
5. 如果聚焦幅度太小，调高 `FocusOffsetScale` 或 `MaxFocusOffset`；如果太晃，降低两者或缩短 `ZoomInDistance`。
