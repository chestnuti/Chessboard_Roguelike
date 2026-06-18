# Combat Preview 系统说明

本文档说明战斗预览系统的实现方式和蓝图接入流程。Combat Preview 负责在玩家输入前预览两类信息：

- 玩家下一步或变身目标是否可以击杀敌人。
- 敌人下一回合是否可以攻击玩家当前格或变身悬停目标格。

Combat Preview 只负责预测和表现通知，不会修改棋盘占格、地块类型、敌人生命状态、玩家属性或回合状态。

## 相关文件

| 文件 | 说明 |
| --- | --- |
| `Source/Chessboard_Roguelike/Public/Combat/CombatPreviewTypes.h` | 定义 `FCombatPreviewState`。 |
| `Source/Chessboard_Roguelike/Public/Combat/CombatPreviewReceiver.h` | 定义蓝图可实现接口 `ICombatPreviewReceiver`。 |
| `Source/Chessboard_Roguelike/Public/Player/GridPawn.h` | 暴露 Combat Preview 刷新和清理 API。 |
| `Source/Chessboard_Roguelike/Private/Player/GridPawn.cpp` | 计算玩家可攻击坐标、敌人威胁状态，并通过接口通知敌人。 |
| `Source/Chessboard_Roguelike/Public/Enemy/GridEnemyPawn.h` | 暴露 `CanAttackCoordNextTurn()` 预测接口。 |
| `Source/Chessboard_Roguelike/Private/Enemy/GridEnemyPawn.cpp` | 实现近战和远程敌人的威胁预测。 |
| `Source/Chessboard_Roguelike/Private/Player/GridPlayerController.cpp` | 在变身选格和鼠标悬停时刷新预览。 |

## 运行时职责

| 类 / 接口 | 职责 |
| --- | --- |
| `AGridPawn` | 作为预览发起者，收集玩家可攻击坐标，遍历存活敌人，计算并广播预览状态。 |
| `AGridEnemyPawn` | 提供 `CanAttackCoordNextTurn()`，判断自己下一回合是否能攻击指定坐标。 |
| `UCombatResolverComponent` | 复用真实近战结算规则，判断玩家攻击目标是否会击杀敌人。 |
| `ICombatPreviewReceiver` | 敌人蓝图实现该接口后接收 `FCombatPreviewState`，并更新头顶 Widget 或材质实例。 |
| `AGridPlayerController` | 在变身目标选择、鼠标悬停和取消流程中调用 Pawn 的预览 API。 |

## 预览数据

结构体：`FCombatPreviewState`

路径：`Source/Chessboard_Roguelike/Public/Combat/CombatPreviewTypes.h`

| 字段 | 类型 | 含义 |
| --- | --- | --- |
| `bCanBeKilledByPlayer` | `bool` | 玩家当前普通攻击或变身捕获目标可以击杀该敌人。 |
| `bCanAttackPreviewPlayerCoord` | `bool` | 该敌人下一回合可以攻击当前预览的玩家坐标。 |
| `bIsRelevant` | `bool` | 本轮预览中该敌人需要显示至少一种提示。 |

当预览被清理时，系统会发送默认构造的 `FCombatPreviewState`，三个字段均为 `false`。

## 接口事件

接口：`ICombatPreviewReceiver`

路径：`Source/Chessboard_Roguelike/Public/Combat/CombatPreviewReceiver.h`

蓝图事件：

```text
UpdateCombatPreview(PreviewState)
```

敌人蓝图需要实现该接口。推荐在事件中把状态写入动态材质实例：

```text
CanBeKilled = PreviewState.bCanBeKilledByPlayer ? 1.0 : 0.0
CanAttackPlayer = PreviewState.bCanAttackPreviewPlayerCoord ? 1.0 : 0.0
HasCombatPreview = PreviewState.bIsRelevant ? 1.0 : 0.0
```

材质参数可以用于头顶 Widget、敌人 Mesh、描边材质或 Niagara 参数。C++ 不限制具体表现方式。

## 玩家侧 API

类：`AGridPawn`

路径：

- `Source/Chessboard_Roguelike/Public/Player/GridPawn.h`
- `Source/Chessboard_Roguelike/Private/Player/GridPawn.cpp`

| 函数 | 说明 |
| --- | --- |
| `RefreshCombatPreview(PreviewPlayerCoord)` | 使用普通四方向攻击坐标刷新预览，并把 `PreviewPlayerCoord` 作为敌人威胁判断目标。 |
| `RefreshDefaultCombatPreview()` | 使用玩家当前坐标刷新默认状态预览。 |
| `RefreshTransformCombatPreview(TransformTargets, PreviewPlayerCoord)` | 使用变身目标中的 `Capture` 格刷新可击杀预览，并用 `PreviewPlayerCoord` 判断敌人威胁。 |
| `ClearCombatPreview()` | 对上一轮通知过的敌人发送空状态，并清空缓存。 |

`AGridPawn` 内部使用 `PreviewedCombatEnemies` 缓存上一轮已经通知过的敌人，避免敌人离开预览范围后材质状态残留。

## 敌人侧威胁预测

类：`AGridEnemyPawn`

函数：

```cpp
bool CanAttackCoordNextTurn(FIntPoint TargetCoord, const AGridPawn* PlayerPawn) const;
```

判断规则：

| 敌人类型 | 预测规则 |
| --- | --- |
| 近战敌人 | 与 `TargetCoord` 的曼哈顿距离等于 `1` 时返回 `true`。 |
| 远程敌人 | 与 `TargetCoord` 同 X 或同 Y，且攻击线未被 `Obstacle` 阻挡，并且在 `MaxRangedAttackDistance` 内时返回 `true`。 |
| 被压制敌人 | `IsSuppressedByPlayer(PlayerPawn)` 为 `true` 时返回 `false`。 |
| 死亡或未初始化敌人 | 敌人死亡、没有 `GridManager`、没有有效玩家引用时返回 `false`。 |

该函数不进入 `AimingRangedAttack`，不写入 `PendingRangedAttackTiles`，不调用 `RangedAttackTelegraphComponent`，也不造成伤害。

## 默认移动预览流程

玩家回到 `PlayerInput` 后，`AGridPawn::RefreshPlayerNextMoveTiles()` 会刷新普通下一步可移动格。该函数结尾会调用：

```cpp
RefreshDefaultCombatPreview();
```

默认预览流程：

1. 清理上一轮 Combat Preview。
2. 收集玩家上下左右四个坐标作为普通攻击坐标。
3. 遍历 `AGridEnemyManager::GetAliveEnemies()` 返回的存活敌人；如果没有 EnemyManager，则回退到 `TActorIterator<AGridEnemyPawn>`。
4. 如果敌人在玩家攻击坐标中，调用 `CombatResolverComponent->ResolvePlayerMeleeAttack(this, Enemy).bKilled` 判断是否可被击杀。
5. 调用 `Enemy->CanAttackCoordNextTurn(CurrentGridCoord, this)` 判断敌人是否威胁玩家当前格。
6. 如果任意预览字段为 `true`，通过 `ICombatPreviewReceiver::UpdateCombatPreview()` 通知敌人蓝图。

## 变身目标预览流程

进入 `TransformTargeting` 后，`AGridPlayerController::ShowTransformTargetHighlights()` 会在显示合法目标格后调用：

```cpp
GridPawn->RefreshTransformCombatPreview(PendingTransformTargets, GridPawn->CurrentGridCoord);
```

变身预览使用 `PendingTransformTargets` 中 `TargetType == ETransformTargetType::Capture` 的坐标作为玩家可攻击坐标。这样 Knight、Bishop、Rook、Queen 等不同形态都会复用同一套可击杀判定。

鼠标悬停合法目标格时，Controller 会调用：

```cpp
GridPawn->RefreshTransformCombatPreview(PendingTransformTargets, HoverCoord);
```

此时敌人威胁判断使用 `HoverCoord`。这可以预览“如果玩家变身移动到该格，下一回合是否会被攻击”。

当鼠标离开合法目标、右键拖动相机或无法取得鼠标格时，预览会回退到玩家当前格：

```cpp
GridPawn->RefreshTransformCombatPreview(PendingTransformTargets, GridPawn->CurrentGridCoord);
```

## 清理时机

系统会在以下位置清理 Combat Preview：

| 时机 | 调用点 |
| --- | --- |
| 玩家普通移动开始 | `AGridPawn::TryResolveMoveOrAttackToCoord()` 成功进入玩家行动后。 |
| 玩家攻击敌人开始 | `AGridPawn::ResolveEnemyMeleeAttack()` 成功进入玩家行动后。 |
| 取消或完成变身目标选择 | `AGridPlayerController::ClearTransformTargetHighlights()`。 |
| 敌人死亡 | `AGridEnemyPawn::Kill()` 向自身发送空 `FCombatPreviewState`。 |

清理只影响表现状态，不影响真实战斗结算。

## 蓝图接线清单

1. 打开敌人蓝图，例如 `BP_ConstructEnemy_Melee` 或 `BP_AcidEnemy_Ranged`。
2. 在 Class Settings 中添加接口 `CombatPreviewReceiver`。
3. 在事件图中实现 `UpdateCombatPreview`。
4. 获取头顶 `WidgetComponent` 或敌人 Mesh 使用的 Dynamic Material Instance。
5. 设置材质 Scalar 参数：

```text
CanBeKilled = PreviewState.bCanBeKilledByPlayer ? 1.0 : 0.0
CanAttackPlayer = PreviewState.bCanAttackPreviewPlayerCoord ? 1.0 : 0.0
HasCombatPreview = PreviewState.bIsRelevant ? 1.0 : 0.0
```

6. 在材质中根据参数切换颜色、描边、发光、图标透明度或 Widget 动画。

## 调试建议

- 敌人没有显示可击杀效果：检查敌人蓝图是否实现 `CombatPreviewReceiver` 接口。
- 效果一直残留：检查 `UpdateCombatPreview` 收到空状态时是否把材质参数重置为 `0`。
- 显示可击杀但实际打不死：检查蓝图是否没有使用 `PreviewState.bCanBeKilledByPlayer`，或敌人材质状态来自旧逻辑。C++ 预览已经复用 `UCombatResolverComponent::ResolvePlayerMeleeAttack()`。
- 远程威胁预览不出现：检查敌人 `BehaviorType` 是否为 `Ranged`，`MaxRangedAttackDistance` 是否允许命中，攻击线中是否有 `Obstacle`。
- 被压制敌人仍显示威胁：检查玩家属性值是否满足 `IsSuppressedByPlayer()` 使用的压制条件。
- 变身悬停风险不刷新：检查 Controller 是否处于 `TransformTargeting`，且鼠标所在格是否在 `PendingTransformTargets` 中。
