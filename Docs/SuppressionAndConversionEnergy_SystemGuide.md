# 压制状态与地块转换能量系统说明

本文说明任务 8 和任务 9 相关的当前实现方式，包括玩家属性满值压制、敌人阵营判定、击杀成功后的地块转换能量事件，以及后续任务 10 的扩展入口。

## 当前目标

任务 8：实现压制状态与阵营相克。

- 构成值满时，构成主义敌人停止对玩家发起攻击，进入被压制状态。
- 酸性值满时，酸性敌人停止对玩家发起攻击，进入被压制状态。
- 压制只影响敌人的主动行动，不影响玩家攻击敌人。

任务 9：实现地块转换能量掉落与单槽持有。

- 玩家击杀敌人后，C++ 广播击杀成功事件。
- 蓝图负责保存单槽能量、替换旧能量和刷新 UI。
- C++ 不直接持有玩家的转换能量状态，便于原型阶段在 `BP_GridPawn` 中快速调整。

## 相关文件

| 文件 | 职责 |
| --- | --- |
| `Source/Chessboard_Roguelike/Public/Player/PlayerAttributeComponent.h` | 暴露玩家属性和压制查询接口 |
| `Source/Chessboard_Roguelike/Private/Player/PlayerAttributeComponent.cpp` | 实现满值判断和阵营压制规则 |
| `Source/Chessboard_Roguelike/Public/Enemy/GridEnemyPawn.h` | 暴露敌人压制查询和蓝图表现事件 |
| `Source/Chessboard_Roguelike/Private/Enemy/GridEnemyPawn.cpp` | 在敌人行动入口执行压制判定 |
| `Source/Chessboard_Roguelike/Public/Player/GridPawn.h` | 暴露玩家击杀敌人事件给蓝图 |
| `Source/Chessboard_Roguelike/Private/Player/GridPawn.cpp` | 在击杀成功分支计算掉落能量类型并触发事件 |

## 压制规则

压制状态来自玩家当前属性值，不是敌人自身的持久状态。

| 玩家状态 | 被压制敌人 | 结果 |
| --- | --- | --- |
| 构成值达到 `MaxConstructValue` | `EEnemyFaction::Construct` | 敌人本回合不移动、不主动攻击 |
| 酸性值达到 `MaxAcidValue` | `EEnemyFaction::Acid` | 敌人本回合不移动、不主动攻击 |
| 两项都满 | 两类敌人 | 两类敌人都被压制 |
| 未满值 | 无 | 敌人按原 AI 行动 |

当前压制规则由 `UPlayerAttributeComponent::CanSuppressFaction()` 统一判断：

```cpp
case EEnemyFaction::Construct:
	return IsConstructSuppressionActive();
case EEnemyFaction::Acid:
	return IsAcidSuppressionActive();
```

## 玩家属性组件接口

类：`UPlayerAttributeComponent`

新增蓝图可读接口：

| 函数 | 返回 | 用途 |
| --- | --- | --- |
| `IsConstructSuppressionActive()` | `bool` | 构成值是否已满，是否压制构成主义敌人 |
| `IsAcidSuppressionActive()` | `bool` | 酸性值是否已满，是否压制酸性敌人 |
| `CanSuppressFaction(EEnemyFaction EnemyFaction)` | `bool` | 按敌人阵营返回是否会被当前玩家状态压制 |

HUD 或表现蓝图可以读取这些函数来显示压制状态，但不应直接修改属性值。

## 敌人行动流程

类：`AGridEnemyPawn`

新增接口：

| 函数或事件 | 类型 | 用途 |
| --- | --- | --- |
| `IsSuppressedByPlayer(const AGridPawn* PlayerPawn)` | `BlueprintPure` | 判断当前敌人是否被玩家属性压制 |
| `OnSuppressedByPlayer(AGridPawn* PlayerPawn)` | `BlueprintImplementableEvent` | 被压制时的蓝图表现入口 |

默认敌人回合入口 `ExecuteBasicTurn_Implementation()` 的顺序如下：

1. 检查敌人是否可行动，玩家是否有效。
2. 调用 `IsSuppressedByPlayer(PlayerPawn)`。
3. 如果被压制，触发 `OnSuppressedByPlayer(PlayerPawn)`，然后返回 `false`。
4. 如果未被压制，继续原有逻辑：相邻则攻击，不相邻则尝试靠近玩家。

注意：压制分支返回 `false`，表示本敌人本回合没有完成移动或攻击。`AGridEnemyManager` 仍会继续遍历其他敌人。

## 击杀能量事件

类：`AGridPawn`

新增蓝图事件：

```cpp
UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
void OnPlayerKilledEnemy(AGridEnemyPawn* KilledEnemy, ETileType DroppedEnergyType);
```

触发位置：

- `AGridPawn::ResolveEnemyMeleeAttack()`
- 仅在 `CombatResult.bKilled == true` 的分支触发。
- 敌人被 `Kill()`，目标格被清空，玩家成功移动到目标格，并结算地块进入效果后触发。

掉落能量类型映射：

| 被击杀敌人阵营 | `DroppedEnergyType` |
| --- | --- |
| `EEnemyFaction::Construct` | `ETileType::Construct` |
| `EEnemyFaction::Acid` | `ETileType::Acid` |

当前 C++ 只负责通知击杀成功和掉落类型，不保存能量。

## BP_GridPawn 推荐实现

在 `BP_GridPawn` 中添加单槽能量变量：

| 变量 | 类型 | 用途 |
| --- | --- | --- |
| `bHasConversionEnergy` | `bool` | 当前是否持有转换能量 |
| `HeldConversionEnergyType` | `ETileType` | 当前持有的转换能量类型 |
| `LastReplacedEnergyType` | `ETileType`，可选 | UI 或调试表现用 |

推荐封装蓝图函数：

| 函数 | 用途 |
| --- | --- |
| `GrantConversionEnergy(NewEnergyType)` | 获得或替换当前单槽能量 |
| `HasConversionEnergy()` | 返回是否持有能量 |
| `GetHeldConversionEnergyType()` | 返回当前能量类型 |
| `ConsumeConversionEnergy()` | 第 10 项使用能量后清空单槽 |

`OnPlayerKilledEnemy` 蓝图事件推荐流程：

1. 接收 `KilledEnemy` 和 `DroppedEnergyType`。
2. 判断 `DroppedEnergyType` 是否为 `Construct` 或 `Acid`。
3. 调用 `GrantConversionEnergy(DroppedEnergyType)`。
4. 刷新 HUD 或播放获得能量反馈。

## 第 10 项扩展方式

任务 10 需要使用当前单槽能量执行 3x3 地块转换。建议只依赖 `BP_GridPawn` 的能量函数，不直接读取内部变量。

推荐流程：

1. 新增输入，例如 `IA_UseConversionEnergy`。
2. 输入触发后调用 `HasConversionEnergy()`。
3. 读取 `GetHeldConversionEnergyType()`。
4. 选择 3x3 中心格，原型阶段可以先用玩家当前格 `CurrentGridCoord`。
5. 遍历中心格周围 9 个坐标。
6. 对有效且可转换的格子调用 `AGridManager::SetTileType(Coord, HeldConversionEnergyType)`。
7. 至少一个格子转换成功后调用 `ConsumeConversionEnergy()`。
8. 触发 UI 和特效反馈。

这种接法让第 10 项只关心“是否有能量、能量是什么、使用后是否消费”，不需要知道能量来自哪个敌人或如何替换。

## 调试检查点

### 敌人没有被压制

检查顺序：

1. 玩家 Pawn 上是否存在 `PlayerAttributeComponent`。
2. `ConstructValue` 或 `AcidValue` 是否真的达到对应最大值。
3. 敌人的 `Faction` 是否设置正确。
4. 敌人是否使用了 `AGridEnemyPawn` 或其子类，并执行默认 `ExecuteBasicTurn`，或蓝图覆写中是否调用了父级逻辑。
5. 如果蓝图完全覆写 `ExecuteBasicTurn` 且不调用父级，需要在蓝图中主动调用 `IsSuppressedByPlayer`。

### 被压制表现没有播放

检查顺序：

1. 敌人蓝图是否实现了 `OnSuppressedByPlayer`。
2. 敌人是否确实进入了 `ExecuteBasicTurn`。
3. 当前玩家属性是否满足压制条件。

### 击杀后没有获得能量

检查顺序：

1. 玩家是否真的击杀敌人，而不是未击杀退回。
2. `BP_GridPawn` 是否实现了 `OnPlayerKilledEnemy`。
3. `DroppedEnergyType` 是否为 `Construct` 或 `Acid`。
4. `GrantConversionEnergy` 是否正确设置了 `bHasConversionEnergy` 和 `HeldConversionEnergyType`。

## 当前边界

已经实现：

- 玩家属性满值压制同阵营敌人。
- 被压制敌人跳过默认主动行动。
- 被压制时提供蓝图表现事件。
- 玩家击杀敌人后向蓝图发送掉落能量类型。
- 单槽能量推荐由 `BP_GridPawn` 持有，便于原型迭代。

尚未实现：

- 3x3 地块转换执行逻辑。
- 能量 UI 的最终样式。
- 压制状态专用特效和音效。
- 多人复制或存档持久化。
