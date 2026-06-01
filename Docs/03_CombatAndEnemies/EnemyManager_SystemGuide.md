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
6. 敌人若不相邻，尝试沿曼哈顿距离靠近玩家一格；移动成功时立即更新逻辑占格，并播放短距离视觉插值。
7. 若存在敌人正在播放移动插值，`AGridEnemyManager` 会保持 `EnemyTurnResolve`，等所有敌人移动完成后再结束敌方回合。
8. 若玩家 HP 在敌方回合中降至 `0`，`ATurnManager` 进入 `Defeat`，敌人管理器停止后续敌人行动。
9. 如果玩家未失败，敌方回合结束后，`ATurnManager::EndEnemyTurn()` 回到 `PlayerInput`。

## 类职责

| 类 | 职责 |
| --- | --- |
| `AGridManager` | 维护格子数据、占据状态、移动合法性和坐标转换 |
| `ATurnManager` | 维护回合状态和玩家步数 |
| `AGridEnemyPawn` | 表示单个敌人，持有阵营、行为类型、阈值、坐标、死亡状态和基础行动 |
| `AGridEnemyManager` | 维护敌人列表，统一执行敌方回合 |

## 新增文件

```text
Source/Chessboard_Roguelike/Public/Enemy/GridEnemyManager.h
Source/Chessboard_Roguelike/Private/Enemy/GridEnemyManager.cpp
```

## EnemyManager API

| 函数 | 说明 |
| --- | --- |
| `InitializeEnemyManager()` | 注入 `GridManager`、`TurnManager` 和玩家 Pawn 引用 |
| `RegisterEnemy()` | 注册单个敌人 |
| `UnregisterEnemy()` | 移除单个敌人 |
| `RebuildEnemyList()` | 通过 `TActorIterator` 重新扫描场景中的敌人 |
| `ExecuteEnemyTurn()` | 执行一轮敌方行动 |
| `GetAliveEnemies()` | 返回当前存活敌人列表 |

## EnemyPawn 新增 API

| 函数 | 说明 |
| --- | --- |
| `IsAlive()` | 当前等价于 `!bDead` |
| `CanAct()` | 当前要求敌人存活且拥有 `GridManager` |
| `TryMoveToGridCoord()` | 通过 `AGridManager::RequestMove()` 尝试移动到指定格子 |
| `ExecuteBasicTurn()` | 蓝图可覆写事件。C++ 默认实现为：相邻则攻击，否则靠近玩家一格 |
| `ExecuteMeleeAttack()` | 蓝图可覆写事件。默认实现会结算敌人近战伤害并写日志 |
| `ApplyMeleeAttackDamage()` | 蓝图可调用函数。对玩家应用敌人近战伤害，返回 `FEnemyAttackResolveResult` |
| `OnMeleeAttackResolved()` | 蓝图可实现事件。敌人近战伤害结算后触发，用于表现和提示 |
| `ShouldDelayEnemyTurnEnd()` | 返回是否需要等待敌人移动视觉插值完成后再结束敌方回合 |

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
- 不建议在 `AGridEnemyManager` 中写具体伤害规则；管理器只负责调度和失败中断，伤害仍由 `AGridEnemyPawn` 与 `UCombatResolverComponent` 处理。

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
- 不相邻时，优先沿绝对距离更大的轴靠近玩家。
- 如果优先方向被阻挡，尝试另一个轴。
- 如果两个方向都失败，敌人跳过行动。
- 移动不使用 NavMesh，只使用 `AGridManager::RequestMove()`。
- 成功移动后，敌人 Actor 会从旧格中心插值到新格中心，默认时长为 `MoveDuration = 0.15`。
- 移动插值期间，敌人管理器延迟调用 `ATurnManager::EndEnemyTurn()`，防止玩家在敌人视觉移动未完成时输入。

## 边界与后续扩展

当前已实现：

- 敌方回合统一遍历存活敌人。
- 默认基础 AI 相邻攻击、非相邻靠近玩家。
- 敌人移动视觉插值，以及移动完成前的敌方回合锁定。
- 敌人近战对玩家造成 HP 伤害。
- 敌人攻击未击败玩家时退回原格，击败玩家时占据玩家格。
- 敌人近战可按阵营扣减玩家构成值或酸性值。
- 玩家失败时进入 `Defeat` 并停止后续敌人行动。

当前未实现：

- 敌人逐个行动动画队列；当前移动插值是并行播放。
- 远程敌人瞄准和攻击线。
- 房间激活和跨房间追逐。
- 友伤和跨阵营清除链。

推荐后续扩展：

| 后续任务 | 建议接入点 |
| --- | --- |
| 压制状态 | 在 `AGridEnemyPawn::CanAct()` 中判断是否被压制 |
| 房间激活 | 在 `AGridEnemyManager` 中维护激活敌人列表 |
| 远程敌人 | 在 `ExecuteBasicTurn()` 中根据 `BehaviorType` 分发 |
| 友伤链 | 在 `ExecuteEnemyTurn()` 后统一清理死亡敌人 |
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
