# 近战攻击与敌人阈值击杀系统说明

本文档说明任务 5、6、7 以及跨阵营友伤基础结算中与战斗结算相关功能的实现方式、可在蓝图中调用或修改的事件、值、参数，以及当前战斗系统与网格、地块属性系统之间的边界。该系统处理玩家主动向敌人格移动时触发的近战攻击、敌人免疫、单次阈值击杀、未击杀退回、敌人在敌方回合中对玩家造成的基础伤害、远程敌人瞄准与攻击线结算，以及敌人之间的跨阵营近战冲突和远程友伤结算入口。敌方回合调度和基础 AI 已拆分到 `AGridEnemyManager`，详见 [EnemyManager_SystemGuide.md](EnemyManager_SystemGuide.md)。本文不包含地块转换能量、PCG 或多房间逻辑。

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
10. 失败攻击视觉回到原格后，Pawn 会调用 `ResolvePickupAtCurrentTile()`，让原格上的拾取物有机会在进入敌方回合前结算。
11. 近战攻击无论是否击杀，都会调用 `ATurnManager::AddStep()` 消耗 1 步。

非法移动、撞墙、越界、障碍格、目标格不是敌人且非空，都不会触发攻击，也不会消耗步数。

敌人对玩家造成伤害的流程如下：

1. 玩家完成一次合法行动后，`AGridPawn::ResolvePostPlayerActionTurn()` 进入敌方回合。
2. `AGridEnemyManager::ExecuteEnemyTurn()` 遍历当前存活敌人。
3. 默认 `AGridEnemyPawn::ExecuteBasicTurn()` 判断敌人与玩家是否相邻。
4. 若相邻且未被压制，敌人先调用 `ApplyMeleeAttackDamage()` 完成数值结算，再触发 `ExecuteMeleeAttack()` 供蓝图播放表现。
5. `ApplyMeleeAttackDamage()` 通过玩家 Pawn 上的 `UCombatResolverComponent::ResolveEnemyMeleeAttack()` 修改 `UPlayerAttributeComponent`。
6. `ResolveEnemyMeleeAttack()` 在应用任何敌人伤害前会先调用 `PlayerPawn->ResolvePickupAtCurrentTile()`。因此玩家只剩 1 点 HP 且站在回血道具上同时受击时，会先尝试回血，再结算敌人伤害。
7. 默认近战造成 `AttackDamage` 点 HP 伤害，并可按敌人阵营额外扣减玩家对应属性值。
8. 如果本次攻击造成 HP 伤害，伤害应用后会再次调用 `ResolvePickupAtCurrentTile()`。这个二次尝试处理满血玩家站在回血道具上时，伤害前拾取无效果、伤害后应立即吃掉脚下道具的边界情况。
9. 如果玩家未被击败，敌人播放一次短距离前冲再回到原格的攻击表现，格子占据不变化。
10. 如果玩家 HP 降至 `0`，敌人清空玩家格占据，移动进入玩家所在格，`AGridEnemyManager` 将 `ATurnManager` 状态设为 `Defeat`，并停止后续敌人行动。

敌人跨阵营友伤的流程如下：

1. 敌人移动仍从 `AGridEnemyPawn::TryMoveToGridCoord()` 进入。
2. 如果目标格被另一名 `AGridEnemyPawn` 占据，敌人不会直接调用 `AGridManager::RequestMove()`，而是先调用 `UCombatResolverComponent::ResolveEnemyMeleeCollision()`。
3. 同阵营敌人不会造成友伤，移动失败且不改变占据。
4. 不同阵营敌人按目标格地块类型结算死亡结果：`Construct` 地块击杀酸性敌人，`Acid` 地块击杀构成敌人，`Minimal` 地块击杀 `KillThreshold` 较低者，阈值相同则双方死亡。
5. `AGridEnemyPawn` 根据结算结果调用 `Kill()` 并清空对应格子的占据；如果目标死亡且攻击者存活，攻击者会再通过 `RequestMove()` 进入目标格。
6. 敌方回合中的远程待结算攻击由 `AGridEnemyManager::ResolvePendingRangedAttacks()` 统一批量处理：先收集本窗口内所有攻击线和友伤命中，再统一应用死亡，避免行动顺序决定谁先死。
7. 蓝图或其他单发远程逻辑仍可直接调用 `ApplyRangedFriendlyFireDamage()`；该函数内部使用 `ResolveEnemyRangedFriendlyFire()`，异阵营目标直接死亡，同阵营不受影响。

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
- 未击杀回退到原格后会结算原格拾取物；这不是目标格进入效果，也不会改变目标格地块属性。
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
| `Ranged` | 远程敌人 | 使用两阶段瞄准/开火逻辑，并优先移动到与玩家同 X/Y 轴的位置 |

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
| `MaxRangedAttackDistance` | `int32` | `0` | 远程攻击最大延伸格数；`0` 表示直到地图边界或 Obstacle |
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
| `ActionState` | `EEnemyActionState` | 当前敌人行动状态；远程瞄准时为 `AimingRangedAttack` |
| `PendingRangedAttackTiles` | `TArray<FIntPoint>` | 上一回合锁定、下一次敌方回合开始时结算的远程攻击线 |
| `PendingRangedAttackDirection` | `FIntPoint` | 远程攻击线方向 |

### 蓝图可调用函数

| 函数 | 参数 | 返回值 | 说明 |
| --- | --- | --- | --- |
| `InitializeOnGrid` | `InGridManager`, `InStartCoord` | 无 | 将敌人初始化到指定格子，并占据该格 |
| `CanReceiveDamage` | 无 | `bool` | 返回敌人是否仍可受击，当前等价于 `!bDead` |
| `IsAlive` | 无 | `bool` | 返回敌人是否存活，当前等价于 `!bDead` |
| `CanAct` | 无 | `bool` | 返回敌人是否可行动，当前要求存活且拥有 `GridManager` |
| `TryMoveToGridCoord` | `TargetCoord` | `bool` | 尝试移动到目标格；目标为空时走 `AGridManager::RequestMove()`，目标为敌人时先结算跨阵营近战冲突 |
| `ExecuteBasicTurn` | `PlayerPawn` | `bool` | `BlueprintNativeEvent`。完整敌人行动入口，可在蓝图覆写 |
| `ExecuteMeleeAttack` | `PlayerPawn` | 无 | `BlueprintNativeEvent`。近战攻击表现入口，可在蓝图覆写 |
| `ApplyMeleeAttackDamage` | `PlayerPawn` | `FEnemyAttackResolveResult` | 对玩家应用敌人近战伤害。默认 AI 会在触发攻击表现前调用它 |
| `ApplyRangedAttackDamage` | `PlayerPawn` | `FEnemyAttackResolveResult` | 对玩家应用远程敌人伤害；不触发近战前冲或占据玩家格 |
| `ApplyRangedFriendlyFireDamage` | `TargetEnemy` | `bool` | 对目标敌人应用远程跨阵营友伤；异阵营目标直接死亡，同阵营返回 `false` |
| `HasPendingRangedAttack` | 无 | `bool` | 返回敌人是否处于远程瞄准状态且持有攻击线 |
| `ResolvePendingRangedAttack` | `PlayerPawn` | `bool` | 结算上一回合锁定的远程攻击线，并清除范围提示 |
| `ClearRangedAimMode` | 无 | 无 | 清除远程瞄准状态、攻击线缓存和 Telegraph 显示 |
| `OnMeleeAttackResolved` | `PlayerPawn`, `AttackResult` | 无 | `BlueprintImplementableEvent`。敌人近战伤害结算后触发表现或提示 |
| `OnFriendlyFireResolved` | `OtherEnemy`, `ResolveResult` | 无 | `BlueprintImplementableEvent`。敌人友伤结算后触发表现或提示 |
| `OnRangedAimStarted` | `PlayerPawn`, `AttackTiles` | 无 | `BlueprintImplementableEvent`。远程敌人进入瞄准模式后触发 |
| `OnRangedAttackResolved` | `PlayerPawn`, `AttackTiles`, `AttackResult`, `bHitPlayer` | 无 | `BlueprintImplementableEvent`。远程攻击线结算后触发 |
| `OnRangedAimCleared` | 无 | 无 | `BlueprintImplementableEvent`。远程瞄准状态被清除后触发 |
| `OnGridEnemyKilled` | `Enemy`, `DeathCoord`, `DeathWorldLocation` | 无 | 敌人死亡广播。`AGridEnemyManager` 会监听该事件，触发死亡地块相机聚焦，并给玩家授予 1 格通用转换能量 |
| `Kill` | 无 | 无 | 标记死亡、广播 `OnGridEnemyKilled`、隐藏 Actor、关闭碰撞 |

### 默认敌人 AI

如果蓝图没有覆写 `ExecuteBasicTurn()`，C++ 默认逻辑如下：

1. 如果敌人不可行动或玩家为空，返回 `false`。
2. 如果敌人与玩家曼哈顿距离为 `1`，先调用 `ApplyMeleeAttackDamage()` 结算伤害，再调用 `ExecuteMeleeAttack()` 并返回 `true`。
3. 如果敌人与玩家不相邻，调用 `AGridManager::FindPathAStar()` 计算到玩家格的四方向网格路径。
4. 如果路径至少包含起点和下一步，敌人尝试移动到路径上的下一格。
5. 如果移动成功，立即更新网格占据和 `CurrentGridCoord`，随后用 `MoveDuration` 播放 Actor 位置插值，并返回 `true`；如果 A* 找不到路径或移动被阻挡，则返回 `false`。

默认 `ExecuteMeleeAttack()` 会调用 `ApplyMeleeAttackDamage()`，并输出攻击日志。默认 AI 已经在进入该事件前完成一次伤害结算，因此内部有防重复保护，避免蓝图调用 Parent 时重复扣血。

如果蓝图只覆写 `ExecuteMeleeAttack`，且仍使用 C++ 默认 `ExecuteBasicTurn()`，伤害仍会在事件触发前结算。如果蓝图覆写了 `ExecuteBasicTurn()` 并完全绕过 Parent，那么需要显式调用 `ApplyMeleeAttackDamage()`，或调用 Parent `Execute Basic Turn`，否则只会播放自定义表现，不会造成数值效果。

敌人攻击玩家的表现规则：

- 未击败玩家时，敌人向玩家格方向前冲一小段距离，再回到原格。
- 击败玩家时，敌人通过 `AGridManager::ClearOccupant()` 清空玩家格，再通过 `RequestMove()` 占据玩家所在格，并播放进入该格的移动插值。
- 玩家失败后进入 `Defeat` 状态，敌人管理器不再继续执行后续敌人行动。

### 远程敌人瞄准与攻击线

`BehaviorType == Ranged` 的敌人使用默认两阶段远程 AI：

1. 如果敌人与玩家在同一 X 轴或 Y 轴，且敌人与玩家之间没有 `Obstacle`，敌人进入 `AimingRangedAttack`。
2. 进入瞄准后，敌人缓存 `PendingRangedAttackTiles`。这条线从敌人相邻格开始，沿瞄准方向延伸，直到遇到无效坐标或 `Obstacle`；`Obstacle` 格不会被包含。
3. `URangedAttackTelegraphComponent` 使用缓存的攻击线生成地格提示。该提示是纯表现层，不会调用 `SetTileType()`，也不会改变地块属性或占据状态。
4. 下一次敌方回合开始时，`AGridEnemyManager` 会先批量结算所有待结算远程攻击，再执行普通敌人行动。
5. 远程攻击按上一回合锁定的 `PendingRangedAttackTiles` 结算；玩家如果已经离开这条线，则不会被命中。
6. 命中玩家时调用 `ApplyRangedAttackDamage()`，复用敌人的 `AttackDamage`、`AttackAttributeDamage` 和阵营属性伤害设置，但不会触发近战前冲或占据玩家格。
7. 攻击线中如果包含其他敌人，敌人管理器会先用 `ResolveEnemyRangedFriendlyFire()` 收集所有跨阵营远程友伤结果，再统一清空占据并调用 `Kill()`。
8. 如果两个不同阵营远程敌人在同一结算窗口内互相命中，双方都会进入统一死亡集合，因此双方同时死亡，不受遍历顺序影响。
9. 如果远程敌人在开火回合被玩家属性压制，会取消待结算攻击线并清除提示。
10. 攻击结算后调用 `ClearRangedAimMode()`，清除攻击线缓存和 Telegraph 显示。
11. 如果远程敌人与玩家同轴但被 `Obstacle` 阻挡，或一步内找不到能获得无障碍攻击线的邻格，它会改用 `AGridManager::FindPathAStar()` 靠近玩家；后续回合一旦能攻击到玩家，会再次优先进入瞄准模式。

`URangedAttackTelegraphComponent` 默认复用 `GridSettings->TileMesh` 生成 `InstancedStaticMesh` 提示。可在敌人蓝图中配置 `TelegraphTileMesh`、`TelegraphMaterial`、`ZOffset` 和 `ScaleMultiplier`。如果未配置提示材质，组件仍会生成实例并使用网格默认材质；如果没有可用 Mesh，则只保留逻辑瞄准，不显示视觉提示。

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

### FEnemyFriendlyFireResolveResult

结构：`FEnemyFriendlyFireResolveResult`

路径：`Source/Chessboard_Roguelike/Public/Combat/CombatTypes.h`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `bResolved` | `bool` | 本次敌人友伤是否完成有效结算 |
| `bSameFaction` | `bool` | 两个敌人是否同阵营；同阵营不会造成伤害 |
| `bAttackerKilled` | `bool` | 攻击者是否在本次结算中死亡 |
| `bTargetKilled` | `bool` | 目标敌人是否在本次结算中死亡 |
| `CollisionTileType` | `ETileType` | 近战冲突发生的目标格地块类型；远程友伤保留默认值 |
| `Reason` | `EEnemyFriendlyFireResolveReason` | 结算原因，用于蓝图表现或调试日志 |

`EEnemyFriendlyFireResolveReason` 当前包含：

- `SameFaction`：同阵营，不造成伤害。
- `RangedDifferentFaction`：远程命中异阵营敌人，目标直接死亡。
- `ConstructTileKillsAcid`：构成地块上的近战冲突击杀酸性敌人。
- `AcidTileKillsConstruct`：酸性地块上的近战冲突击杀构成敌人。
- `MinimalTileLowerThreshold`：极简地块击杀 `KillThreshold` 较低者。
- `MinimalTileEqualThreshold`：极简地块双方 `KillThreshold` 相同，双方同时死亡。

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
| `ResolveEnemyMeleeCollision` | `Attacker`, `TargetEnemy`, `CollisionTileType` | `FEnemyFriendlyFireResolveResult` | 结算两个敌人在同一目标格发生的跨阵营近战冲突 |
| `ResolveEnemyRangedFriendlyFire` | `Attacker`, `TargetEnemy` | `FEnemyFriendlyFireResolveResult` | 结算远程敌人命中敌人时的跨阵营友伤 |

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

### 跨阵营友伤判定

敌人友伤不会复用玩家的双属性伤害，也不会进入敌人的免疫或单次阈值伤害流程。`UCombatResolverComponent` 只返回死亡判定，真正的 `Kill()` 和格子占据清理由 `AGridEnemyPawn` 执行。

```text
同阵营：不造成伤害
远程异阵营命中：目标直接死亡；同一结算窗口内互相命中时，同步进入死亡集合
近战异阵营冲突：
  Construct 地块：Acid 敌人死亡
  Acid 地块：Construct 敌人死亡
  Minimal 地块：KillThreshold 较低者死亡
  Minimal 地块且 KillThreshold 相同：双方死亡
```

近战冲突发生在敌人试图移动到敌人占据格时。目标死亡且攻击者存活时，攻击者会进入目标格；攻击者死亡时会清空自己的源格；双方死亡时两个格子的占据都会被清空。

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
| `OnFriendlyFireResolved` | `AGridEnemyPawn` | 敌人跨阵营友伤结算完成后 | 可根据 `FEnemyFriendlyFireResolveResult` 播放互伤、死亡或清除提示 |
| `OnRangedAimStarted` | `AGridEnemyPawn` | 远程敌人锁定攻击线后 | 可播放瞄准、警告音效或额外 UI |
| `OnRangedAttackResolved` | `AGridEnemyPawn` | 远程攻击线结算完成后 | 可根据 `AttackTiles`、`AttackResult` 和 `bHitPlayer` 播放开火、受击或落空表现 |
| `OnRangedAimCleared` | `AGridEnemyPawn` | 远程瞄准状态被清除后 | 可清理蓝图侧额外提示 |
| `OnGridEnemyKilled` | `AGridEnemyPawn` | `Kill()` 标记敌人死亡时 | `AGridEnemyManager` 监听后通知 `AGridPlayerController` 聚焦死亡地块，并通过玩家 `ConversionEnergyComponent` 授予转换能量 |

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

### 敌人撞到异阵营敌人没有触发友伤

检查顺序：

1. 移动是否通过 `TryMoveToGridCoord()`，而不是直接调用 `AGridManager::RequestMove()`。
2. 目标格 `OccupantType` 是否为 `Enemy`，且 `OccupantActor` 是否指向另一名 `AGridEnemyPawn`。
3. 两个敌人的 `Faction` 是否不同；同阵营不会造成友伤。
4. 目标格 `TileType` 是否为 `Construct`、`Acid` 或 `Minimal`。
5. 如果是 `Minimal` 地块，两个敌人的 `KillThreshold` 是否符合预期。

### 远程敌人命中异阵营敌人没有击杀

检查顺序：

1. 远程攻击逻辑是否在射线命中敌人后调用了 `ApplyRangedFriendlyFireDamage(TargetEnemy)`。
2. `TargetEnemy` 是否存活，且不是攻击者自己。
3. 两个敌人的 `Faction` 是否不同；同阵营返回 `false`。
4. 目标敌人的 `GridManager` 或攻击者的 `GridManager` 是否有效，以便清空目标格占据。

### 远程敌人没有显示攻击范围

检查顺序：

1. 敌人 `BehaviorType` 是否为 `Ranged`。
2. 敌人与玩家是否处于同一 X 轴或 Y 轴。
3. 敌人与玩家之间是否存在 `TileType == Obstacle` 或 `OccupantType == Obstacle` 的地块；障碍会阻断瞄准。
4. 敌人是否已经持有有效 `GridManager`，且棋盘已经生成。
5. `RangedAttackTelegraphComponent` 是否存在；默认 `AGridEnemyPawn` 构造时会创建该组件。
6. `TelegraphTileMesh` 是否已配置，或 `GridSettings->TileMesh` 是否有效。
7. 如需明显区别攻击范围，是否给 `TelegraphMaterial` 配置了警告材质。

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
- 敌人攻击未击败玩家时前冲后退回原格。
- 敌人击败玩家时占据玩家所在格。
- 敌人可按阵营扣减玩家构成值或酸性值。
- 玩家 HP 归零后进入 `Defeat` 回合状态。
- 敌人近战伤害结果可通过 `OnMeleeAttackResolved` 供蓝图表现层使用。
- 敌人跨阵营近战冲突会通过 `ResolveEnemyMeleeCollision()` 统一结算，并由 `TryMoveToGridCoord()` 应用死亡和占格变化。
- 敌方回合中的远程友伤由 `AGridEnemyManager::ResolvePendingRangedAttacks()` 批量同步结算，异阵营目标直接死亡；互相命中的远程敌人会同时死亡。
- 蓝图或其他单发远程逻辑可通过 `ApplyRangedFriendlyFireDamage()` 复用 `ResolveEnemyRangedFriendlyFire()`。
- 远程敌人可在同 X/Y 且无遮挡时进入 `AimingRangedAttack`，通过 `URangedAttackTelegraphComponent` 显示锁定攻击线，并在下一次敌方回合开始时结算。
- 远程攻击命中玩家时使用 `ApplyRangedAttackDamage()`，不会触发近战前冲或占据玩家所在格。
- 敌人友伤结果可通过 `OnFriendlyFireResolved` 供蓝图表现层使用。
- `ExecuteBasicTurn` 和 `ExecuteMeleeAttack` 可在蓝图中覆写。

未实现：

- 跨阵营清除链的专用队列、连锁表现和 UI 提示。
- 击杀掉落地块转换能量。
- 战斗专用音效和特效事件。
- 敌人 DataAsset 配置表。
