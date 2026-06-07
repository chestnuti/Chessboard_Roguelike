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
6. `UCombatCameraDirectorComponent::FocusGridTileBriefly()` 在玩家当前视角目标或 Pawn 上查找 `USpringArmComponent`，缓存 SpringArm 的 Camera Lag 设置，并在聚焦期间临时关闭 Camera Lag。
7. 组件根据死亡地块距离计算本次运行时聚焦时长，短暂修改 `TargetOffset` 和 `TargetArmLength`。
8. 聚焦结束后组件恢复 SpringArm 的原始 `TargetOffset`、`TargetArmLength` 和 Camera Lag 设置，并关闭自己的 Tick。

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
| `BeginConversionEnergyCameraZoom()` | `AGridPlayerController` | 蓝图可调用入口，开始地块转换能量长按缩放 |
| `EndConversionEnergyCameraZoom()` | `AGridPlayerController` | 蓝图可调用入口，结束地块转换能量长按缩放并快速回弹 |
| `CanStartConversionEnergyCameraZoom()` | `AGridPlayerController` | 蓝图可覆写判断，建议返回当前是否持有转换能量 |
| `BeginConversionEnergyZoom()` | `UCombatCameraDirectorComponent` | 相机组件内部入口，缓慢拉近 SpringArm |
| `EndConversionEnergyZoom()` | `UCombatCameraDirectorComponent` | 相机组件内部入口，快速恢复 SpringArm 臂长 |

## 相机组件参数

| 参数 | 默认值 | 说明 |
| --- | ---: | --- |
| `FocusInDuration` | `0.12` | 镜头偏向死亡地块的真实时间 |
| `FocusHoldDuration` | `0.18` | 保持聚焦的真实时间 |
| `FocusOutDuration` | `0.18` | 镜头恢复到原位的真实时间 |
| `ExtraFocusInDurationAtMaxDistance` | `0.16` | 当死亡地块距离达到 `MaxFocusOffset` 时，额外增加的进入聚焦时间 |
| `ExtraFocusHoldDurationAtMaxDistance` | `0.22` | 当死亡地块距离达到 `MaxFocusOffset` 时，额外增加的保持聚焦时间 |
| `MaxFocusOffset` | `650` | 从玩家到死亡地块的最大平面偏移采样距离，避免远处死亡把镜头拉得过猛 |
| `FocusOffsetScale` | `0.65` | 死亡地块平面偏移转换到 SpringArm `TargetOffset` 的比例 |
| `ZoomInDistance` | `300` | 聚焦期间缩短的 SpringArm 臂长 |
| `bDisableSpringArmLagDuringFocus` | `true` | 聚焦期间是否临时关闭 SpringArm 的 Camera Lag 和 Camera Rotation Lag |
| `ConversionEnergyZoomInDuration` | `0.45` | 持有转换能量并长按输入时，镜头缓慢拉近所用时间 |
| `ConversionEnergyZoomOutDuration` | `0.12` | 转换能量使用完成或取消时，镜头快速回弹所用时间 |
| `ConversionEnergyZoomInDistance` | `240` | 长按转换能量时缩短的 SpringArm 臂长 |
| `bDisableSpringArmLagDuringConversionEnergyZoom` | `true` | 转换能量缩放期间是否临时关闭 SpringArm 的 Camera Lag 和 Camera Rotation Lag |

组件使用 `GetWorld()->GetRealTimeSeconds()` 推进时间，因此全局 Time Dilation 或 HitStop 不会阻止镜头回退。

运行时聚焦时长会按死亡地块与玩家之间的平面距离自适应：

```text
DistanceAlpha = Clamp(Distance2D / MaxFocusOffset, 0, 1)
RuntimeFocusInDuration = FocusInDuration + ExtraFocusInDurationAtMaxDistance * DistanceAlpha
RuntimeFocusHoldDuration = FocusHoldDuration + ExtraFocusHoldDurationAtMaxDistance * DistanceAlpha
RuntimeFocusOutDuration = FocusOutDuration
```

这样近距离击杀仍然轻快，远距离友伤击杀会有更长的镜头进入和停留时间。

## 使用方式

默认情况下，`AGridPlayerController` 构造时会创建 `CombatCameraDirectorComponent`。如果关卡使用 `BP_GridPlayerController`，蓝图实例会继承该组件。

该组件默认查找顺序为：

1. 当前 `PlayerController` 的 `ViewTarget` 上的 `USpringArmComponent`。
2. 当前受控 Pawn 上的 `USpringArmComponent`。
3. 组件 Owner 自身上的 `USpringArmComponent`。

如果当前相机没有 SpringArm，组件会安全跳过并输出 Verbose 日志。当前实现最适合 `BP_GridPawn` 持有 SpringArm 和 Camera 的俯视相机结构。

如果 SpringArm 启用了 Camera Lag，默认会在战斗聚焦期间临时关闭，避免真实相机还在追赶目标时组件已经开始复位。聚焦完成后会恢复原来的 `bEnableCameraLag`、`bEnableCameraRotationLag`、`CameraLagSpeed` 和 `CameraRotationLagSpeed`。

## 地块转换能量长按缩放

地块转换能量的持有状态仍由蓝图维护。相机系统只提供表现接口：

1. 长按空格或对应增强输入 `Started` 时，如果玩家持有能量，调用 `AGridPlayerController::BeginConversionEnergyCameraZoom()`。
2. 组件用 `ConversionEnergyZoomInDuration` 将 SpringArm `TargetArmLength` 缓慢拉近到 `原始臂长 - ConversionEnergyZoomInDistance`。
3. 能量实际使用成功、输入 `Triggered`、`Completed` 或 `Canceled` 时，调用 `AGridPlayerController::EndConversionEnergyCameraZoom()`。
4. 组件用 `ConversionEnergyZoomOutDuration` 快速恢复到长按前的 SpringArm 臂长。

如果在 C++ 中为 `UseEnergyAction` 赋值，`AGridPlayerController` 会自动绑定 `Started`、`Triggered`、`Completed` 和 `Canceled`。其中 `Started` 会先调用 `CanStartConversionEnergyCameraZoom()`；建议在 `BP_GridPlayerController` 中覆写该函数，并返回玩家当前的 `bHasConversionEnergy`。如果能量逻辑仍完全在 `BP_GridPawn` 的输入图中，也可以不赋值 `UseEnergyAction`，直接在已有蓝图输入链路中调用 Begin/End 两个函数。

转换能量缩放与死亡地块聚焦共享同一个 SpringArm。死亡聚焦优先级更高，会打断正在进行的转换能量缩放；转换能量缩放开始时也会停止未完成的死亡聚焦，避免两个效果同时改写 `TargetArmLength`。

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
5. 如果远距离击杀仍然看不清，确认 `bDisableSpringArmLagDuringFocus` 是否为 `true`。
6. 如果聚焦停留太短，调高 `ExtraFocusHoldDurationAtMaxDistance` 或 `FocusHoldDuration`。
7. 如果聚焦幅度太小，调高 `FocusOffsetScale` 或 `MaxFocusOffset`；如果太晃，降低两者或缩短 `ZoomInDistance`。
8. 如果长按空格没有拉近镜头，检查 `UseEnergyAction` 是否在 `BP_GridPlayerController` 中赋值，或现有蓝图输入链路是否调用了 `BeginConversionEnergyCameraZoom()`。
9. 如果没有能量时也触发了缩放，覆写 `CanStartConversionEnergyCameraZoom()`，让它返回当前 `bHasConversionEnergy`。
