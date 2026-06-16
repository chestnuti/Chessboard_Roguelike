# 敌人管理器系统说明

本文档说明 `AGridEnemyManager` 的当前实现、蓝图覆写方式、使用方式、职责边界和后续扩展点。该系统对应任务 7“敌人基础 AI 与行动回合”的基础设施，不替代战斗结算、网格占据或单个敌人 Pawn。

## 当前目标

`AGridEnemyManager` 负责在玩家完成一次合法行动后，统一调度所有存活敌人执行一轮基础行动。

当前流程：

1. 玩家移动或近战攻击成功进入动作结算。
2. 玩家视觉移动结束后，`AGridPawn` 调用 `ATurnManager::BeginEnemyTurn()`。
3. `AGridEnemyManager::ExecuteEnemyTurn()` 遍历存活敌人。
4. 每个敌人调用 `AGridEnemyPawn::ExecuteBasicTurn()`。该函数是 `BlueprintNativeEvent`，可由敌人蓝图覆写。
5. 敌人若与玩家相邻，默认先结算近战伤害，再触发 `ExecuteMeleeAttack()` 供蓝图播放表现；未击败玩家时前冲后退回原格，击败玩家时占据玩家所在格。
6. 敌人若不相邻，通过 `AGridManager::FindPathAStar()` 计算到玩家格的四方向网格路径，并尝试移动到路径上的下一格；目标为空时立即更新逻辑占格并播放短距离视觉插值，目标为敌人时先进入跨阵营近战冲突结算。
7. 若存在敌人正在播放移动插值，`AGridEnemyManager` 会保持 `EnemyTurnResolve`，等所有敌人移动完成后再结束敌方回合。
8. 若玩家 HP 在敌方回合中降至 `0`，`ATurnManager` 进入 `Defeat`，敌人管理器停止后续敌人行动。
9. 如果玩家未失败，敌方回合结束后，`ATurnManager::EndEnemyTurn()` 回到 `PlayerInput`。
10. 如果最后一个敌人死亡，`AGridEnemyManager` 广播 `OnAllEnemiesCleared`。常规 PCG 关卡中，`ADungeonRunManager` 监听该事件并将回合状态切换为 `Victory`。

## 类职责

| 类 | 职责 |
| --- | --- |
| `AGridManager` | 维护格子数据、占据状态、移动合法性和坐标转换 |
| `ATurnManager` | 维护回合状态和玩家步数 |
| `AGridEnemyPawn` | 表示单个敌人，持有阵营、行为类型、阈值、坐标、死亡状态和基础行动 |
| `AGridEnemyManager` | 维护敌人列表，统一执行敌方回合，处理敌人死亡后的相机聚焦和玩家转换能量授予，并在敌人全灭时通知关卡推进系统 |

## 新增文件

```text
Source/Chessboard_Roguelike/Public/Enemy/GridEnemyManager.h
Source/Chessboard_Roguelike/Private/Enemy/GridEnemyManager.cpp
Source/Chessboard_Roguelike/Public/Enemy/RangedAttackTelegraphComponent.h
Source/Chessboard_Roguelike/Private/Enemy/RangedAttackTelegraphComponent.cpp
```

## EnemyManager API

| 函数 | 说明 |
| --- | --- |
| `InitializeEnemyManager()` | 注入 `GridManager`、`TurnManager` 和玩家 Pawn 引用 |
| `RegisterEnemy()` | 注册单个敌人 |
| `UnregisterEnemy()` | 移除单个敌人 |
| `RebuildEnemyList()` | 通过 `TActorIterator` 重新扫描场景中的敌人 |
| `ClearAllEnemies()` | 解绑死亡事件、清空棋盘占位、销毁已注册敌人，并重置敌人列表。进入下一关前由 `ADungeonRunManager` 调用 |
| `ExecuteEnemyTurn()` | 执行一轮敌方行动 |
| `GetAliveEnemies()` | 返回当前存活敌人列表 |
| `OnAllEnemiesCleared` | 敌人全灭事件。最后一个敌人死亡后广播，`ADungeonRunManager` 会监听它并完成当前关卡 |

## EnemyPawn 新增 API

| 函数 | 说明 |
| --- | --- |
| `IsAlive()` | 当前等价于 `!bDead` |
| `CanAct()` | 当前要求敌人存活且拥有 `GridManager` |
| `TryMoveToGridCoord()` | 尝试移动到指定格子；目标为空时调用 `AGridManager::RequestMove()`，目标为敌人时调用友伤结算 |
| `ExecuteBasicTurn()` | 蓝图可覆写事件。C++ 默认实现为：相邻则攻击，否则使用 A* 寻路靠近玩家一格 |
| `ExecuteMeleeAttack()` | 蓝图可覆写事件。默认实现会结算敌人近战伤害并写日志 |
| `ApplyMeleeAttackDamage()` | 蓝图可调用函数。对玩家应用敌人近战伤害，返回 `FEnemyAttackResolveResult` |
| `ApplyRangedAttackDamage()` | 蓝图可调用函数。对玩家应用远程敌人伤害，不触发近战前冲或占格 |
| `ApplyRangedFriendlyFireDamage()` | 蓝图可调用函数。远程敌人命中另一名敌人时可调用；异阵营目标直接死亡 |
| `HasPendingRangedAttack()` | 返回该敌人是否处于远程瞄准状态并持有待结算攻击线 |
| `ResolvePendingRangedAttack()` | 结算上一回合锁定的远程攻击线，并清除攻击范围提示 |
| `ClearRangedAimMode()` | 清除远程瞄准状态、攻击线缓存和 Telegraph 显示 |
| `OnMeleeAttackResolved()` | 蓝图可实现事件。敌人近战伤害结算后触发，用于表现和提示 |
| `OnFriendlyFireResolved()` | 蓝图可实现事件。敌人友伤结算后触发，用于互伤、死亡或清除提示 |
| `OnRangedAimStarted()` | 蓝图可实现事件。远程敌人进入瞄准模式并生成攻击线后触发 |
| `OnRangedAttackResolved()` | 蓝图可实现事件。远程攻击线结算完成后触发 |
| `OnRangedAimCleared()` | 蓝图可实现事件。远程瞄准状态和地格提示被清除后触发 |
| `OnGridEnemyKilled` | 敌人死亡广播。敌人管理器注册敌人时会订阅它，用于触发死亡地块相机聚焦，并给玩家授予 1 格通用转换能量 |
| `ShouldDelayEnemyTurnEnd()` | 返回是否需要等待敌人移动视觉插值完成后再结束敌方回合 |

## GridManager 寻路 API

| 函数 | 说明 |
| --- | --- |
| `FindPathAStar()` | 使用四方向 A* 在当前逻辑网格上查找路径，返回包含起点和终点的坐标数组 |

`FindPathAStar(StartCoord, GoalCoord, OutPath, bAllowOccupiedGoal)` 会读取 `AGridManager::Tiles` 中的可通行和占据状态，不依赖 NavMesh。默认情况下，被占据的终点不可达；当 `bAllowOccupiedGoal` 为 `true` 时，终点允许被占据，但中途节点仍必须为空。默认敌人 AI 使用该参数寻路到玩家当前格，只取 `OutPath[1]` 作为本回合实际移动目标，因此不会直接走入玩家格。真正移动仍由 `TryMoveToGridCoord()` 负责最终合法性校验；如果下一格被敌人占据，`TryMoveToGridCoord()` 会按目标格地块类型结算跨阵营近战友伤，而不是让 `RequestMove()` 直接进入已占据格。

## 蓝图覆写敌人 AI

`ExecuteBasicTurn()` 是完整敌人行动入口。你可以在任意 `AGridEnemyPawn` 蓝图子类中覆写它，实现自定义 AI。

推荐步骤：

1. 打开敌人蓝图，例如 `BP_ConstructEnemy`。
2. 在 `My Blueprint` 面板中展开 `Overrides`。
3. 选择 `Execute Basic Turn`。
4. 使用传入的 `PlayerPawn` 读取玩家位置或调用其他玩家组件。
5. 根据你的规则调用 `TryMoveToGridCoord()`、`ExecuteMeleeAttack()`，或直接跳过行动。
6. 返回 `true` 表示本敌人本回合执行过动作；返回 `false` 表示没有行动。

蓝图覆写时的返回值只用于 `AGridEnemyManager` 的行动统计，不会自动改变回合状态。敌方回合仍由 `AGridEnemyManager::ExecuteEnemyTurn()` 统一结束。

常见覆写模式：

| 模式 | 做法 |
| --- | --- |
| 保留默认 AI 并加表现 | 在蓝图中调用 Parent `Execute Basic Turn`，再播放额外表现 |
| 自定义近战敌人 | 判断与玩家距离，相邻时调用 `ExecuteMeleeAttack()`，否则调用 `TryMoveToGridCoord()` |
| 待机敌人 | 直接返回 `false` |
| 特殊敌人 | 根据 `BehaviorType`、阵营或自定义变量决定移动、攻击或跳过 |

注意事项：

- 不要在蓝图中直接修改 `AGridManager::Tiles`。
- 敌人移动应优先通过 `TryMoveToGridCoord()`，让 `AGridManager::RequestMove()` 负责合法性和占据状态。
- 如果蓝图自行修改 `CurrentGridCoord` 或 Actor 位置，可能导致视觉位置和格子占据不同步。
- 如果只覆写 `ExecuteMeleeAttack()`，并继续使用 C++ 默认 `ExecuteBasicTurn()`，伤害仍会在事件触发前结算。
- 如果覆写 `ExecuteBasicTurn()` 并完全不调用 Parent，相邻攻击时需要显式调用 `ApplyMeleeAttackDamage()` 或调用 Parent `Execute Basic Turn`，否则不会修改玩家 HP。
- 如果蓝图远程攻击命中敌人，需要显式调用 `ApplyRangedFriendlyFireDamage()`，否则只会播放表现，不会清空目标敌人。
- 不建议在 `AGridEnemyManager` 中写具体伤害规则；管理器只负责调度和失败中断，伤害仍由 `AGridEnemyPawn` 与 `UCombatResolverComponent` 处理。

## 远程敌人调度

当 `AGridEnemyPawn::BehaviorType == EEnemyBehaviorType::Ranged` 时，默认 C++ AI 会使用两阶段远程攻击：

1. 如果存在敌人已经处于 `AimingRangedAttack`，`AGridEnemyManager::ExecuteEnemyTurn()` 会在普通敌人行动前先调用 `ResolvePendingRangedAttacks()` 批量处理本窗口内所有待结算远程攻击。
2. 远程攻击结算后，该敌人会被记录为本回合已行动，不会在同一轮敌方回合里再次移动或重新瞄准。
3. 如果敌人尚未瞄准，`ExecuteBasicTurn()` 会先检查敌人与玩家是否同 X 或同 Y，且从敌人到玩家之间没有 `Obstacle`。
4. 满足条件时，敌人进入 `AimingRangedAttack`，缓存 `PendingRangedAttackTiles` 和 `PendingRangedAttackDirection`，并通过 `URangedAttackTelegraphComponent` 显示攻击线。
5. 攻击线从敌人相邻格开始，沿瞄准方向延伸，遇到无效坐标或 `Obstacle` 停止；`Obstacle` 格本身不会被包含在攻击范围里。
6. 下一次敌方回合开始时，攻击按上一回合锁定的 `PendingRangedAttackTiles` 结算。玩家如果已经离开该线，则不会被命中。
7. 如果攻击线中包含其他敌人，敌人管理器会先收集本窗口内所有跨阵营远程友伤命中，再统一清空占据并调用 `Kill()`。
8. 如果两个不同阵营远程敌人在同一结算窗口内互相命中，双方都会在统一应用阶段死亡，不受遍历顺序影响。
9. 如果远程敌人在开火回合被玩家属性压制，会取消待结算攻击线并清除提示。
10. 如果远程敌人不能瞄准玩家，它会优先选择能在一步后与玩家同 X 或同 Y 且无障碍视线的邻格；没有这样的邻格时，再选择更接近对齐的邻格，最后才回退到 A* 靠近玩家。

`URangedAttackTelegraphComponent` 是纯表现组件，不修改 `AGridManager::Tiles`、`ETileType` 或占据状态。默认情况下它会复用 `GridSettings->TileMesh` 来生成 `InstancedStaticMesh` 提示；可在蓝图子类中设置 `TelegraphTileMesh`、`TelegraphMaterial`、`ZOffset` 和 `ScaleMultiplier` 调整表现。

## 关卡使用方式

1. 在关卡中放置一个 `AGridEnemyManager` 或其蓝图子类。
2. 确保关卡中存在 `AGridManager`、`ATurnManager` 和玩家 `AGridPawn`。
3. 放置 `AGridEnemyPawn` 或其蓝图子类，并设置 `StartGridCoord`。
4. 运行时玩家完成合法行动后，若 `AGridPawn` 能找到 `AGridEnemyManager`，会自动执行敌方回合。

如果关卡中没有 `AGridEnemyManager`，玩家行动后会直接回到 `PlayerInput`，敌人不会行动。

## 当前 AI 规则

未覆写 `ExecuteBasicTurn()` 时，C++ 默认基础敌人行动规则：

- 与玩家曼哈顿距离为 `1` 时，触发近战攻击事件。
- 相邻攻击会对玩家造成 `AttackDamage` 点 HP 伤害；默认值为 `1`。
- 若敌人启用 `bApplyFactionAttributeDamage`，`Construct` 敌人会扣玩家构成值，`Acid` 敌人会扣玩家酸性值。
- 未击败玩家时，敌人播放前冲并退回原格的攻击表现。
- 击败玩家时，敌人清空玩家格占据并移动进入玩家所在格。
- 玩家 HP 降至 `0` 时，本轮敌方回合立即中断并进入 `Defeat`。
- 不相邻时，调用 `AGridManager::FindPathAStar(CurrentGridCoord, PlayerCoord, PathToPlayer, true)` 计算路径。
- 若路径至少包含起点和下一步，敌人尝试移动到 `PathToPlayer[1]`。
- 如果下一步被另一名敌人占据，`TryMoveToGridCoord()` 会调用 `UCombatResolverComponent::ResolveEnemyMeleeCollision()`。同阵营不造成伤害；异阵营按目标格地块类型决定死亡结果。
- 友伤导致目标死亡且攻击者存活时，攻击者进入目标格；攻击者死亡时清空自身源格；双方死亡时两个格子的占据都会被清空。
- 若 A* 找不到路径，或下一步移动被最终合法性校验阻挡，敌人跳过行动。
- 移动不使用 NavMesh，只使用 `AGridManager::RequestMove()`。
- 成功移动后，敌人 Actor 会从旧格中心插值到新格中心，默认时长为 `MoveDuration = 0.15`。
- 移动插值期间，敌人管理器延迟调用 `ATurnManager::EndEnemyTurn()`，防止玩家在敌人视觉移动未完成时输入。
- `BehaviorType == Ranged` 的敌人会优先尝试进入远程瞄准模式；无法瞄准时优先移动到能与玩家同 X/Y 轴并取得无障碍视线的位置，而不是直接靠近玩家。
- 远程敌人进入瞄准模式后会显示锁定攻击线，并在下一次敌方回合开始时由敌人管理器优先批量结算所有待结算攻击线。

## 边界与后续扩展

当前已实现：

- 敌方回合统一遍历存活敌人。
- 默认基础 AI 相邻攻击、非相邻使用 A* 靠近玩家。
- 敌人移动视觉插值，以及移动完成前的敌方回合锁定。
- 敌人近战对玩家造成 HP 伤害。
- 敌人攻击未击败玩家时退回原格，击败玩家时占据玩家格。
- 敌人近战可按阵营扣减玩家构成值或酸性值。
- 敌人移动撞上异阵营敌人时，会按目标格地块类型结算近战友伤。
- 敌方回合中的远程友伤会先收集所有命中再统一应用死亡，支持不同阵营远程敌人在同一结算窗口内互击同步死亡。
- 蓝图或其他单发远程逻辑可通过 `ApplyRangedFriendlyFireDamage()` 复用跨阵营远程友伤结算。
- 远程敌人会在同 X/Y 且无遮挡时进入瞄准模式，并通过 `URangedAttackTelegraphComponent` 显示从敌人出发到 Obstacle 前停止的直线攻击范围。
- 敌方回合开始时会先结算所有待结算远程攻击；已开火的远程敌人本回合不会再执行普通行动。
- 友伤结果可通过 `OnFriendlyFireResolved()` 给蓝图表现层使用。
- 敌人死亡会通过 `OnGridEnemyKilled` 通知敌人管理器，并由玩家控制器上的 `CombatCameraDirectorComponent` 短暂聚焦死亡地块。
- 最后一名敌人死亡后，敌人管理器广播 `OnAllEnemiesCleared`，供 `ADungeonRunManager` 或蓝图 UI 触发胜利结算。
- `ClearAllEnemies()` 可在关卡重新生成前销毁旧敌人 Actor，并清理它们占据的棋盘格。
- 玩家失败时进入 `Defeat` 并停止后续敌人行动。

当前未实现：

- 敌人逐个行动动画队列；当前移动插值是并行播放。
- 房间激活和跨房间追逐的专用激活列表。
- 跨阵营清除链的专用队列、连锁表现和 UI 提示。

推荐后续扩展：

| 后续任务 | 建议接入点 |
| --- | --- |
| 压制状态 | 在 `AGridEnemyPawn::CanAct()` 中判断是否被压制 |
| 房间激活 | 在 `AGridEnemyManager` 中维护激活敌人列表 |
| 远程敌人配置表 | 将射程、提示材质、是否穿透敌人等参数迁移到 DataAsset |
| 友伤链 | 在 `TryMoveToGridCoord()` / `ApplyRangedFriendlyFireDamage()` 产生死亡后，扩展为队列式连锁清理和表现 |
| 行动动画 | 将 `ExecuteEnemyTurn()` 从同步遍历改为队列式逐个执行 |

## 调试点

敌人不行动时检查：

1. 关卡中是否放置了 `AGridEnemyManager`。
2. `AGridPawn` 是否能找到 `AGridManager` 和 `ATurnManager`。
3. 敌人是否成功执行 `InitializeOnGrid()`。
4. 敌人 `GridManager` 是否为空。
5. 目标移动格是否被玩家、其他敌人、障碍或不可通行地块占据。

敌人攻击没有效果时检查：

1. 玩家 Pawn 是否存在 `PlayerAttributeComponent` 和 `CombatResolverComponent`。
2. 敌人是否被压制；被压制时默认 AI 不会攻击。
3. 敌人 `AttackDamage` 是否大于 `0`。
4. 如果覆写了 `ExecuteBasicTurn()`，是否调用了 Parent 或显式调用 `ApplyMeleeAttackDamage()`。
5. 如果只覆写 `ExecuteMeleeAttack()`，确认关卡里的敌人仍然走默认 `ExecuteBasicTurn()`，或者覆写逻辑自行处理伤害。

蓝图 AI 没有生效时检查：

1. 是否覆写的是敌人蓝图子类中的 `Execute Basic Turn`，而不是只放了一个同名自定义事件。
2. 关卡中放置的敌人实例是否使用了该蓝图子类。
3. 覆写函数是否正确返回 `true` 或 `false`。
4. 如果调用 Parent，确认 Parent 默认移动没有因为目标格被占据或不可通行而返回 `false`。
