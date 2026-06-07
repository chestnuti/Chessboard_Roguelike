# 压制状态与地块转换能量系统说明

本文说明任务 8、任务 9 和任务 10 相关的当前实现方式，包括玩家属性满值压制、敌人阵营判定、击杀成功后的地块转换能量事件，以及玩家使用能量执行 3x3 地块转换的流程。

## 当前目标

任务 8：实现压制状态与阵营相克。

- 构成值满时，构成主义敌人停止对玩家发起攻击，进入被压制状态。
- 酸性值满时，酸性敌人停止对玩家发起攻击，进入被压制状态。
- 压制只影响敌人的主动行动，不影响玩家攻击敌人。

任务 9：实现地块转换能量掉落与单槽持有。

- 玩家击杀敌人后，C++ 广播击杀成功事件。
- 蓝图负责保存单槽能量、替换旧能量和刷新 UI。
- C++ 不直接持有玩家的转换能量状态，便于原型阶段在 `BP_GridPawn` 中快速调整。

任务 10：实现能量使用与 3x3 地块转换。

- 蓝图负责切换玩家手动指定的目标地块类型。
- 蓝图负责校验当前是否持有转换能量，并在转换成功后消费能量。
- C++ 负责执行以玩家当前格为中心的 3x3 地块转换。

## 相关文件

| 文件 | 职责 |
| --- | --- |
| `Source/Chessboard_Roguelike/Public/Player/PlayerAttributeComponent.h` | 暴露玩家属性和压制查询接口 |
| `Source/Chessboard_Roguelike/Private/Player/PlayerAttributeComponent.cpp` | 实现满值判断和阵营压制规则 |
| `Source/Chessboard_Roguelike/Public/Enemy/GridEnemyPawn.h` | 暴露敌人压制查询和蓝图表现事件 |
| `Source/Chessboard_Roguelike/Private/Enemy/GridEnemyPawn.cpp` | 在敌人行动入口执行压制判定 |
| `Source/Chessboard_Roguelike/Public/Player/GridPawn.h` | 暴露玩家击杀敌人事件给蓝图 |
| `Source/Chessboard_Roguelike/Private/Player/GridPawn.cpp` | 在击杀成功分支计算掉落能量类型并触发事件；执行 3x3 地块转换 |

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

## 3x3 地块转换

任务 10 当前采用蓝图和 C++ 混合实现。

蓝图负责：

- `SwitchConversionTileType`：切换玩家手动指定的转换目标地块类型。
- `TryUseConversionEnergy`：检查是否持有能量，调用 C++ 转换函数，并在转换成功后消费能量。
- 输入控制：项目已配置能量使用和地块类型切换相关输入。

C++ 负责：

```cpp
UFUNCTION(BlueprintCallable, Category = "Grid")
bool ConvertAreaAroundPlayer(AGridManager* InGridManager, ETileType EnergyType);
```

函数位置：

- `Source/Chessboard_Roguelike/Public/Player/GridPawn.h`
- `Source/Chessboard_Roguelike/Private/Player/GridPawn.cpp`

运行规则：

1. 传入当前使用的 `AGridManager` 和目标 `EnergyType`。
2. 如果 `InGridManager` 为空，直接返回 `false`。
3. 以玩家 `CurrentGridCoord` 为中心，生成 9 个坐标：中心格、上下左右、四个斜向相邻格。
4. 对每个坐标调用 `AGridManager::IsTileConvertible(Coord)`。
5. 可转换时调用 `AGridManager::SetTileType(Coord, EnergyType)`。
6. 至少找到一个可转换格子时返回 `true`。
7. 没有任何格子可转换时返回 `false`。

该函数只负责地块类型转换，不负责：

- 检查玩家是否持有能量。
- 切换或选择目标地块类型。
- 消费能量。
- 播放 UI、音效或特效反馈。
- 推进回合状态。

这些流程仍由 `BP_GridPawn` 的 `TryUseConversionEnergy` 控制。

注意：`SetTileType()` 只修改地块类型，不修改占据状态。因此玩家或敌人当前占据的格子也可以被转换，只要该格 `bConvertible == true` 且不是 `Obstacle`。

## 推荐使用流程

当前推荐执行链如下：

```text
输入切换目标类型
-> SwitchConversionTileType
-> 更新当前目标 ETileType

输入使用能量
-> TryUseConversionEnergy
-> 检查 bHasConversionEnergy
-> BeginConversionEnergyCameraZoom（长按开始时，仅当持有能量）
-> 读取当前目标 ETileType
-> ConvertAreaAroundPlayer(GridManager, TargetTileType)
-> 返回 true 时 ConsumeConversionEnergy
-> EndConversionEnergyCameraZoom（使用成功、输入完成或取消时）
-> 刷新 HUD / 播放反馈
```

蓝图应只在 `ConvertAreaAroundPlayer` 返回 `true` 时消费能量。这样玩家在周围没有任何可转换格子时，不会白白损失能量。

### 长按相机反馈

玩家持有转换能量时，长按空格可以触发地块转换能量相机缩放：

1. 在增强输入 `Started` 或现有长按开始节点中，先检查 `bHasConversionEnergy`。
2. 为真时调用玩家控制器的 `BeginConversionEnergyCameraZoom()`。
3. 当长按触发实际使用、输入完成或输入取消时，调用 `EndConversionEnergyCameraZoom()`。

如果使用 C++ 的 `AGridPlayerController::UseEnergyAction` 绑定入口，建议在 `BP_GridPlayerController` 中覆写 `CanStartConversionEnergyCameraZoom()`，返回当前是否持有转换能量。相机缩放本身由 `UCombatCameraDirectorComponent` 处理，参数详见 [CombatCamera_SystemGuide.md](../03_CombatAndEnemies/CombatCamera_SystemGuide.md)。

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

### 使用能量后没有转换地块

检查顺序：

1. 输入事件是否正确触发 `TryUseConversionEnergy`。
2. `bHasConversionEnergy` 是否为 `true`。
3. 当前目标地块类型是否为有效的 `ETileType`，通常应为 `Construct` 或 `Acid`。
4. 传入 `ConvertAreaAroundPlayer` 的 `GridManager` 是否有效。
5. 玩家周围 3x3 范围内是否存在 `bConvertible == true` 且非 `Obstacle` 的格子。
6. `ConvertAreaAroundPlayer` 的返回值是否为 `true`。
7. 是否只在返回 `true` 后调用了 `ConsumeConversionEnergy`。

## 当前边界

已经实现：

- 玩家属性满值压制同阵营敌人。
- 被压制敌人跳过默认主动行动。
- 被压制时提供蓝图表现事件。
- 玩家击杀敌人后向蓝图发送掉落能量类型。
- 单槽能量推荐由 `BP_GridPawn` 持有，便于原型迭代。
- 蓝图已实现能量目标类型切换和能量使用入口。
- C++ 已实现以玩家为中心的 3x3 地块转换函数。

尚未实现：

- 能量 UI 的最终样式。
- 压制状态专用特效和音效。
- 多人复制或存档持久化。
