# 近战攻击与敌人阈值击杀系统说明

本文档说明任务 5、6、7 中与战斗结算相关功能的实现方式、可在蓝图中调用或修改的事件、值、参数，以及当前战斗系统与网格、地块属性系统之间的边界。该系统处理玩家主动向敌人格移动时触发的近战攻击、敌人免疫、单次阈值击杀、未击杀退回，以及敌人在敌方回合中对玩家造成的基础近战伤害。敌方回合调度和基础 AI 已拆分到 `AGridEnemyManager`，详见 [EnemyManager_SystemGuide.md](EnemyManager_SystemGuide.md)。本文不包含友伤、地块转换能量、远程攻击、PCG 或多房间逻辑。

## 功能流程

运行时流程如下：

1. 玩家通过 `AGridPawn::TryMove()` 请求向相邻格移动。
2. Pawn 检查目标格是否存在、可通行。
3. 如果目标格为空，继续走原有移动流程：`AGridManager::RequestMove()`、更新 `CurrentGridCoord`、触发地块进入效果、增加步数、播放移动插值。
4. 如果目标格占据类型为 `Enemy`，Pawn 读取 `FTileData.OccupantActor` 并转换为 `AGridEnemyPawn`。
5. Pawn 调用 `UCombatResolverComponent::ResolvePlayerMeleeAttack()` 结算玩家本次近战。
6. CombatResolver 从玩家的 `UPlayerAttributeComponent` 读取当前构成值和酸性值，生成双属性伤害。
7. CombatResolver 根据敌人 `Faction` 应用免疫规则，并判断有效单次伤害是否达到敌人 `KillThreshold`。
8. 若击杀成功，Pawn 调用敌人 `Kill()`，通过 `AGridManager::ClearOccupant()` 清空敌人格，再调用 `RequestMove()` 让玩家进入该格。
9. 若未击杀，敌人和玩家的格子占据状态都不改变，Pawn 播放一次短距离前冲再退回的失败攻击表现。
10. 近战攻击无论是否击杀，都会调用 `ATurnManager::AddStep()` 消耗 1 步。

非法移动、撞墙、越界、障碍格、目标格不是敌人且非空，都不会触发攻击，也不会消耗步数。

敌人对玩家造成伤害的流程如下：

1. 玩家完成一次合法行动后，`AGridPawn::ResolvePostPlayerActionTurn()` 进入敌方回合。
2. `AGridEnemyManager::ExecuteEnemyTurn()` 遍历当前存活敌人。
3. 默认 `AGridEnemyPawn::ExecuteBasicTurn()` 判断敌人与玩家是否相邻。
4. 若相邻且未被压制，敌人先调用 `ApplyMeleeAttackDamage()` 完成数值结算，再触发 `ExecuteMeleeAttack()` 供蓝图播放表现。
5. `ApplyMeleeAttackDamage()` 通过玩家 Pawn 上的 `UCombatResolverComponent::ResolveEnemyMeleeAttack()` 修改 `UPlayerAttributeComponent`。
6. 默认近战造成 `AttackDamage` 点 HP 伤害，并可按敌人阵营额外扣减玩家对应属性值。
7. 如果玩家 HP 降至 `0`，`AGridEnemyManager` 将 `ATurnManager` 状态设为 `Defeat`，并停止后续敌人行动。

## 攻击与退回规则

| 目标格状态 | 结果 | 是否消耗步数 | 是否触发地块效果 |
| --- | --- | --- | --- |
| 空格 | 玩家正常移动到目标格 | 是 | 是 |
| 敌人，击杀成功 | 敌人死亡，玩家移动到敌人格 | 是 | 是 |
| 敌人，未击杀 | 玩家退回原格，敌人留在目标格 | 是 | 否 |
| 越界或不可通行 | 无动作 | 否 | 否 |
| 障碍或非敌人占据 | 无动作 | 否 | 否 |

说明：

- 玩家攻击敌人时不会先占据目标格，只有击杀成功后才进入敌人格。
- 未击杀时不会触发目标格的 `Construct` / `Acid` 地块效果，因为玩家最终没有进入该格。
- 击杀成功后会触发目标格地块效果，因为玩家最终占据该格。

## 敌人基础类型

### 敌人阵营

枚举：`EEnemyFaction`

路径：`Source/Chessboard_Roguelike/Public/Enemy/EnemyTypes.h`

| 值 | 含义 | 免疫规则 |
| --- | --- | --- |
| `Construct` | 构成主义敌人 | 免疫构成伤害 |
| `Acid` | 酸性敌人 | 免疫酸性伤害 |

### 敌人行为类型

枚举：`EEnemyBehaviorType`

路径：`Source/Chessboard_Roguelike/Public/Enemy/EnemyTypes.h`

| 值 | 含义 | 当前阶段用途 |
| --- | --- | --- |
| `Melee` | 近战敌人 | 当前默认基础 AI 可使用 |
| `Ranged` | 远程敌人 | 为后续远程敌人逻辑预留 |

当前玩家近战判定只使用敌人阵营和击杀阈值，`BehaviorType` 不参与玩家攻击结算。敌方回合 AI 可在 `ExecuteBasicTurn()` 的蓝图覆写中读取 `BehaviorType` 并自行分发逻辑。

## GridEnemyPawn

类：`AGridEnemyPawn`

路径：

- `Source/Chessboard_Roguelike/Public/Enemy/GridEnemyPawn.h`
- `Source/Chessboard_Roguelike/Private/Enemy/GridEnemyPawn.cpp`

默认用途：

- 表示棋盘上的敌方棋子。
- BeginPlay 时可自动查找 `AGridManager` 并占据 `StartGridCoord`。
- 被玩家击杀后隐藏 Actor，并关闭碰撞。
- 在敌方回合中可执行基础 AI，或由蓝图覆写完整行动逻辑。

### 可修改参数

| 字段 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `Faction` | `EEnemyFaction` | `Construct` | 敌人阵营，决定免疫哪一种玩家伤害 |
| `BehaviorType` | `EEnemyBehaviorType` | `Melee` | 敌人行为类型，当前仅预留 |
| `KillThreshold` | `int32` | `5` | 单次击杀阈值，不是累计 HP |
| `AttackDamage` | `int32` | `1` | 敌人近战命中玩家时造成的 HP 伤害 |
| `AttackAttributeDamage` | `int32` | `1` | 启用阵营属性伤害时，对玩家对应属性值造成的扣减量 |
| `bApplyFactionAttributeDamage` | `bool` | `true` | 是否让 `Construct` 敌人扣构成值、`Acid` 敌人扣酸性值 |
| `MoveDuration` | `float` | `0.15` | 敌人移动视觉插值时长；逻辑占格仍在移动成功时立即更新 |
| `StartGridCoord` | `FIntPoint` | `(1, 0)` | 自动初始化时占据的格子 |
| `bAutoInitializeOnBeginPlay` | `bool` | `true` | 是否在 BeginPlay 自动查找 GridManager 并占格 |

`KillThreshold` 在 Details 面板中最小值为 `1`。战斗结算时也会把运行时阈值保护到至少 `1`，避免误填 `0` 后出现 0 伤害击杀。

### 运行时状态

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `CurrentGridCoord` | `FIntPoint` | 当前逻辑坐标 |
| `bDead` | `bool` | 是否已死亡 |
| `bIsMoving` | `bool` | 是否正在播放移动视觉插值 |
| `GridManager` | `AGridManager*` | 当前使用的棋盘管理器 |

### 蓝图可调用函数

| 函数 | 参数 | 返回值 | 说明 |
| --- | --- | --- | --- |
| `InitializeOnGrid` | `InGridManager`, `InStartCoord` | 无 | 将敌人初始化到指定格子，并占据该格 |
| `CanReceiveDamage` | 无 | `bool` | 返回敌人是否仍可受击，当前等价于 `!bDead` |
| `IsAlive` | 无 | `bool` | 返回敌人是否存活，当前等价于 `!bDead` |
| `CanAct` | 无 | `bool` | 返回敌人是否可行动，当前要求存活且拥有 `GridManager` |
| `TryMoveToGridCoord` | `TargetCoord` | `bool` | 通过 `AGridManager::RequestMove()` 尝试移动到目标格；成功后更新逻辑坐标并播放视觉插值 |
| `ExecuteBasicTurn` | `PlayerPawn` | `bool` | `BlueprintNativeEvent`。完整敌人行动入口，可在蓝图覆写 |
| `ExecuteMeleeAttack` | `PlayerPawn` | 无 | `BlueprintNativeEvent`。近战攻击表现入口，可在蓝图覆写 |
| `ApplyMeleeAttackDamage` | `PlayerPawn` | `FEnemyAttackResolveResult` | 对玩家应用敌人近战伤害。默认 AI 会在触发攻击表现前调用它 |
| `OnMeleeAttackResolved` | `PlayerPawn`, `AttackResult` | 无 | `BlueprintImplementableEvent`。敌人近战伤害结算后触发表现或提示 |
| `Kill` | 无 | 无 | 标记死亡、隐藏 Actor、关闭碰撞 |

### 默认敌人 AI

如果蓝图没有覆写 `ExecuteBasicTurn()`，C++ 默认逻辑如下：

1. 如果敌人不可行动或玩家为空，返回 `false`。
2. 如果敌人与玩家曼哈顿距离为 `1`，先调用 `ApplyMeleeAttackDamage()` 结算伤害，再调用 `ExecuteMeleeAttack()` 并返回 `true`。
3. 如果敌人与玩家不相邻，优先沿距离更大的轴靠近玩家一格。
4. 如果优先方向被阻挡，尝试另一个轴。
5. 如果移动成功，立即更新网格占据和 `CurrentGridCoord`，随后用 `MoveDuration` 播放 Actor 位置插值，并返回 `true`；否则返回 `false`。

默认 `ExecuteMeleeAttack()` 会调用 `ApplyMeleeAttackDamage()`，并输出攻击日志。默认 AI 已经在进入该事件前完成一次伤害结算，因此内部有防重复保护，避免蓝图调用 Parent 时重复扣血。

如果蓝图只覆写 `ExecuteMeleeAttack`，且仍使用 C++ 默认 `ExecuteBasicTurn()`，伤害仍会在事件触发前结算。如果蓝图覆写了 `ExecuteBasicTurn()` 并完全绕过 Parent，那么需要显式调用 `ApplyMeleeAttackDamage()`，或调用 Parent `Execute Basic Turn`，否则只会播放自定义表现，不会造成数值效果。

### 蓝图覆写 AI

在敌人蓝图中可以覆写：

- `Execute Basic Turn`：重写完整行动决策。
- `Execute Melee Attack`：只重写相邻攻击效果。

覆写 `Execute Basic Turn` 时，建议优先调用 `TryMoveToGridCoord()` 来移动敌人，不要直接修改 `CurrentGridCoord`、Actor Location 或 `AGridManager::Tiles`。返回值用于敌人管理器统计本回合有多少敌人行动过。

### 推荐蓝图使用方式

1. 基于 `AGridEnemyPawn` 创建 `BP_GridEnemyPawn`。
2. 在 `Enemy` 分类下设置 `Faction`、`BehaviorType`、`KillThreshold`。
3. 在 `Grid` 分类下设置 `StartGridCoord`。
4. 将 `BP_GridEnemyPawn` 放入测试关卡。
5. 确保该坐标存在、可通行、且没有被玩家或其他敌人占据。
6. 如需自定义 AI，在蓝图 Overrides 中覆写 `Execute Basic Turn`。

示例：

| 敌人 | `Faction` | `KillThreshold` | 说明 |
| --- | --- | ---: | --- |
| 构成敌人测试版 | `Construct` | `3` | 免疫构成伤害，需要酸性有效伤害达到 3 |
| 酸性敌人测试版 | `Acid` | `3` | 免疫酸性伤害，需要构成有效伤害达到 3 |

## 战斗伤害结构

### FCombatDamage

结构：`FCombatDamage`

路径：`Source/Chessboard_Roguelike/Public/Combat/CombatTypes.h`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `ConstructDamage` | `int32` | 本次构成伤害 |
| `AcidDamage` | `int32` | 本次酸性伤害 |

玩家近战时，该结构由 `UCombatResolverComponent::BuildPlayerDamage()` 自动生成：

- `ConstructDamage = PlayerAttributeComponent->GetConstructValue()`
- `AcidDamage = PlayerAttributeComponent->GetAcidValue()`

### FCombatResolveResult

结构：`FCombatResolveResult`

路径：`Source/Chessboard_Roguelike/Public/Combat/CombatTypes.h`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `bKilled` | `bool` | 本次攻击是否击杀目标 |
| `bImmuneConstruct` | `bool` | 目标是否免疫构成伤害 |
| `bImmuneAcid` | `bool` | 目标是否免疫酸性伤害 |
| `EffectiveConstructDamage` | `int32` | 应用免疫后的构成有效伤害 |
| `EffectiveAcidDamage` | `int32` | 应用免疫后的酸性有效伤害 |
| `KillThreshold` | `int32` | 本次结算使用的敌人击杀阈值 |

该结果只描述战斗判定，不直接修改敌人、玩家或格子占据。

### FEnemyAttackDamage

结构：`FEnemyAttackDamage`

路径：`Source/Chessboard_Roguelike/Public/Combat/CombatTypes.h`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `HealthDamage` | `int32` | 本次敌人攻击造成的 HP 伤害 |
| `ConstructDelta` | `int32` | 本次敌人攻击对玩家构成值造成的变化，通常为负数 |
| `AcidDelta` | `int32` | 本次敌人攻击对玩家酸性值造成的变化，通常为负数 |

默认近战伤害由 `UCombatResolverComponent::BuildEnemyMeleeDamage()` 从敌人参数生成：

- `HealthDamage = EnemyActor->AttackDamage`
- `Construct` 敌人在 `bApplyFactionAttributeDamage = true` 时生成 `ConstructDelta = -AttackAttributeDamage`
- `Acid` 敌人在 `bApplyFactionAttributeDamage = true` 时生成 `AcidDelta = -AttackAttributeDamage`

### FEnemyAttackResolveResult

结构：`FEnemyAttackResolveResult`

路径：`Source/Chessboard_Roguelike/Public/Combat/CombatTypes.h`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `bDamageApplied` | `bool` | 本次攻击是否实际修改了玩家 HP 或属性值 |
| `bPlayerDefeated` | `bool` | 本次攻击结算后玩家是否已失败 |
| `AppliedHealthDamage` | `int32` | 实际应用的 HP 伤害 |
| `AppliedConstructDelta` | `int32` | 实际应用的构成值变化 |
| `AppliedAcidDelta` | `int32` | 实际应用的酸性值变化 |
| `RemainingHealth` | `int32` | 结算后的玩家剩余 HP |

## CombatResolverComponent

类：`UCombatResolverComponent`

路径：

- `Source/Chessboard_Roguelike/Public/Combat/CombatResolverComponent.h`
- `Source/Chessboard_Roguelike/Private/Combat/CombatResolverComponent.cpp`

默认挂载位置：`AGridPawn`

### 蓝图可调用函数

| 函数 | 参数 | 返回值 | 说明 |
| --- | --- | --- | --- |
| `BuildPlayerDamage` | `PlayerActor` | `FCombatDamage` | 从玩家属性组件读取当前双属性伤害 |
| `ResolvePlayerMeleeAttack` | `PlayerActor`, `EnemyActor` | `FCombatResolveResult` | 结算玩家对目标敌人的近战攻击 |
| `BuildEnemyMeleeDamage` | `EnemyActor` | `FEnemyAttackDamage` | 从敌人参数生成近战伤害包 |
| `ResolveEnemyMeleeAttack` | `EnemyActor`, `PlayerPawn` | `FEnemyAttackResolveResult` | 应用敌人对玩家的近战伤害、属性扣减和失败判定 |

### 免疫与击杀判定

玩家攻击始终是双属性输出：

```text
构成伤害 = 当前构成值
酸性伤害 = 当前酸性值
```

敌人免疫规则：

```text
Construct 敌人：免疫构成伤害
Acid 敌人：免疫酸性伤害
```

击杀规则：

```text
有效构成伤害或有效酸性伤害 >= KillThreshold 时，敌人死亡
```

当前实现取两个有效伤害中的较大值作为单次有效伤害：

```text
EffectiveSingleHitDamage = Max(EffectiveConstructDamage, EffectiveAcidDamage)
```

敌人不会累计受伤。未击杀时，不会降低 `KillThreshold`，也不会记录剩余 HP。

### 判定示例

| 玩家当前属性 | 敌人阵营 | 阈值 | 有效伤害 | 结果 |
| --- | --- | ---: | --- | --- |
| 构成 5 / 酸性 2 | `Construct` | `3` | 酸性 2 | 不死亡 |
| 构成 5 / 酸性 2 | `Acid` | `3` | 构成 5 | 死亡 |
| 构成 1 / 酸性 4 | `Construct` | `4` | 酸性 4 | 死亡 |
| 构成 1 / 酸性 4 | `Acid` | `4` | 构成 1 | 不死亡 |

## GridPawn 战斗入口

类：`AGridPawn`

路径：

- `Source/Chessboard_Roguelike/Public/Player/GridPawn.h`
- `Source/Chessboard_Roguelike/Private/Player/GridPawn.cpp`

新增组件：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `CombatResolverComponent` | `UCombatResolverComponent*` | 负责玩家近战伤害与免疫结算 |

相关函数：

| 函数 | 可调用性 | 说明 |
| --- | --- | --- |
| `TryMove` | `BlueprintCallable` | 玩家四向行动入口；目标为敌人时触发近战 |
| `ResolveEnemyMeleeAttack` | C++ 私有 | 处理玩家攻击敌人格后的击杀、退回、步数和表现 |
| `StartFailedAttackVisualMove` | C++ 私有 | 未击杀时播放短距离前冲再退回的视觉反馈 |

`TryMove()` 现在不再把所有占据格都视为非法移动。目标格为 `Enemy` 时，它会进入战斗流程；目标格为其他占据类型时仍阻挡且不消耗步数。

## 当前事件与蓝图覆写点

当前阶段已经新增敌人近战伤害结果事件，玩家近战结果仍保持轻量返回值模式。

已有相关事件仍然有效：

| 事件 | 所属类 | 触发时机 | 与战斗的关系 |
| --- | --- | --- | --- |
| `OnPlayerAttributeChanged` | `UPlayerAttributeComponent` | 玩家构成值或酸性值变化 | 击杀后玩家进入目标地块并触发地块效果时可能广播 |
| `OnPlayerHealthChanged` | `UPlayerAttributeComponent` | 玩家 HP 变化 | 敌人近战造成 HP 伤害或后续治疗时广播 |
| `OnPlayerDefeated` | `UPlayerAttributeComponent` | 玩家 HP 从正数降至 0 | 可用于 UI 或 GameMode 监听失败 |
| `OnTileTypeChanged` | `AGridManager` | 地块类型变化 | 击杀后进入 Construct/Acid 地块并消耗地块时可能广播 |
| `ExecuteBasicTurn` | `AGridEnemyPawn` | 敌方回合遍历到该敌人时 | 可覆写完整敌人 AI |
| `ExecuteMeleeAttack` | `AGridEnemyPawn` | 默认 AI 判断与玩家相邻时 | 可覆写敌人近战表现 |
| `OnMeleeAttackResolved` | `AGridEnemyPawn` | 敌人近战伤害结算完成后 | 可根据 `FEnemyAttackResolveResult` 播放受击、死亡或提示表现 |

如果后续需要 UI 或特效直接响应玩家攻击敌人的结果，建议新增 `OnPlayerMeleeAttackResolved` 一类的事件，并广播 `FCombatResolveResult`、目标敌人和目标坐标。敌人攻击玩家的结果当前已通过 `OnMeleeAttackResolved` 暴露给蓝图表现层。

## 常见调试点

### 玩家撞到敌人没有攻击

检查顺序：

1. 目标格 `FTileData.OccupantType` 是否为 `Enemy`。
2. 目标格 `OccupantActor` 是否指向 `AGridEnemyPawn` 或其蓝图子类。
3. 敌人是否成功执行 `InitializeOnGrid()`。
4. 敌人的 `StartGridCoord` 是否与地图有效坐标匹配。
5. 该格是否被障碍或其他 Actor 提前占据。

### 敌人一放进关卡就没有占格

检查顺序：

1. 关卡中是否存在 `BP_GridManager`。
2. `BP_GridManager` 是否有有效 `GridSettings`。
3. 敌人的 `bAutoInitializeOnBeginPlay` 是否为 `true`。
4. `StartGridCoord` 是否存在且可通行。
5. `StartGridCoord` 是否已经被玩家、障碍或另一名敌人占据。

### 敌人不会在玩家行动后行动

检查顺序：

1. 关卡中是否放置了 `AGridEnemyManager` 或其蓝图子类。
2. 玩家 Pawn 是否能找到 `AGridEnemyManager`。
3. 敌人是否存活，且 `CanAct()` 返回 `true`。
4. 敌人是否成功占据格子，并持有有效 `GridManager`。
5. 如果覆写了 `Execute Basic Turn`，确认覆写函数返回了正确的 `bool`。

### 蓝图覆写 AI 后移动异常

检查顺序：

1. 是否通过 `TryMoveToGridCoord()` 移动，而不是直接写 `CurrentGridCoord`。
2. 目标格是否有效、可通行、未被占据。
3. 是否误把玩家所在格作为移动目标。敌人攻击玩家应走 `ExecuteMeleeAttack()`，不是移动进玩家格。
4. 是否在蓝图中直接改了 `AGridManager::Tiles`，导致占据状态和 Actor 位置不同步。

### 攻击后敌人不死

检查顺序：

1. 玩家当前构成值和酸性值是否符合预期。
2. 敌人的 `Faction` 是否导致对应伤害被免疫。
3. 未被免疫的有效伤害是否达到 `KillThreshold`。
4. 是否误把 `KillThreshold` 当作累计血量使用；当前系统不累计伤害。

### 未击杀却触发了地块效果

按当前代码不应发生。检查顺序：

1. 玩家是否实际上击杀了敌人并进入目标格。
2. 是否有蓝图额外调用了 `ResolveTileEnterEffect()`。
3. 目标格的 `OccupantType` 是否被其他逻辑错误改成了 `Empty`，导致走了普通移动流程。

### 击杀后玩家没有进入敌人格

检查顺序：

1. Output Log 是否出现 `ResolveEnemyMeleeAttack killed enemy but failed to move player`。
2. 敌人格是否在 `ClearOccupant()` 后被其他逻辑重新占据。
3. 玩家当前源格 `OccupantActor` 是否仍指向玩家 Pawn。
4. `AGridManager::RequestMove()` 的源格和目标格是否都有效。

### 敌人相邻但没有扣玩家 HP

检查顺序：

1. 敌人是否走了 C++ 默认 `ExecuteBasicTurn()`，或蓝图覆写中是否调用了 Parent / `ApplyMeleeAttackDamage()`。
2. 玩家 Pawn 上是否存在 `CombatResolverComponent` 和 `PlayerAttributeComponent`。
3. 敌人是否被玩家当前属性压制；被压制时默认 AI 会跳过攻击。
4. 敌人 `AttackDamage` 是否大于 `0`。
5. 玩家是否已经处于 `IsDefeated()` 状态；失败后不会继续重复扣血。

## 当前边界

已实现：

- 玩家向敌人格移动触发近战攻击。
- 未击杀时玩家退回原格并消耗步数。
- 击杀时敌人死亡，玩家占据敌人格。
- 玩家双属性伤害。
- 敌人阵营免疫。
- 单次阈值击杀，不累计伤害。
- 击杀后进入目标格时触发原有地块效果。
- 敌人管理器统一调度敌方回合。
- 敌人默认基础 AI：相邻攻击，否则尝试靠近玩家。
- 敌人移动视觉插值，移动期间敌方回合保持锁定。
- 敌人默认近战会对玩家造成 HP 伤害。
- 敌人可按阵营扣减玩家构成值或酸性值。
- 玩家 HP 归零后进入 `Defeat` 回合状态。
- 敌人近战伤害结果可通过 `OnMeleeAttackResolved` 供蓝图表现层使用。
- `ExecuteBasicTurn` 和 `ExecuteMeleeAttack` 可在蓝图中覆写。

未实现：

- 敌人远程攻击。
- 敌人友伤。
- 击杀掉落地块转换能量。
- 战斗专用音效和特效事件。
- 敌人 DataAsset 配置表。
