# 敌人管理器系统说明

本文档说明 `AGridEnemyManager` 的当前实现、蓝图覆写方式、使用方式、职责边界和后续扩展点。该系统对应任务 7“敌人基础 AI 与行动回合”的基础设施，不替代战斗结算、网格占据或单个敌人 Pawn。

## 当前目标

`AGridEnemyManager` 负责在玩家完成一次合法行动后，统一调度所有存活敌人执行一轮基础行动。

当前流程：

1. 玩家移动或近战攻击成功进入动作结算。
2. 玩家视觉移动结束后，`AGridPawn` 调用 `ATurnManager::BeginEnemyTurn()`。
3. `AGridEnemyManager::ExecuteEnemyTurn()` 遍历存活敌人。
4. 每个敌人调用 `AGridEnemyPawn::ExecuteBasicTurn()`。该函数是 `BlueprintNativeEvent`，可由敌人蓝图覆写。
5. 敌人若与玩家相邻，触发 `ExecuteMeleeAttack()`。
6. 敌人若不相邻，尝试沿曼哈顿距离靠近玩家一格。
7. 敌方回合结束后，`ATurnManager::EndEnemyTurn()` 回到 `PlayerInput`。

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
| `ExecuteMeleeAttack()` | 蓝图可覆写事件，当前 C++ 默认只写日志 |

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
- 如果需要攻击玩家的真实数值效果，建议放在 `ExecuteMeleeAttack()` 或后续独立 Resolver 中，不建议写进 `AGridEnemyManager`。

## 关卡使用方式

1. 在关卡中放置一个 `AGridEnemyManager` 或其蓝图子类。
2. 确保关卡中存在 `AGridManager`、`ATurnManager` 和玩家 `AGridPawn`。
3. 放置 `AGridEnemyPawn` 或其蓝图子类，并设置 `StartGridCoord`。
4. 运行时玩家完成合法行动后，若 `AGridPawn` 能找到 `AGridEnemyManager`，会自动执行敌方回合。

如果关卡中没有 `AGridEnemyManager`，玩家行动后会直接回到 `PlayerInput`，敌人不会行动。

## 当前 AI 规则

未覆写 `ExecuteBasicTurn()` 时，C++ 默认基础敌人行动规则：

- 与玩家曼哈顿距离为 `1` 时，触发近战攻击事件。
- 不相邻时，优先沿绝对距离更大的轴靠近玩家。
- 如果优先方向被阻挡，尝试另一个轴。
- 如果两个方向都失败，敌人跳过行动。
- 移动不使用 NavMesh，只使用 `AGridManager::RequestMove()`。

当前近战攻击只提供 `ExecuteMeleeAttack()` 事件入口，尚未对玩家造成实际效果。

## 边界与后续扩展

当前未实现：

- 敌人攻击玩家的实际伤害或压制。
- 敌人行动动画队列。
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

1. 当前 C++ 默认攻击只输出日志。
2. 如需表现或数值效果，可在敌人蓝图中覆写 `ExecuteMeleeAttack()`。
3. 后续压制/伤害系统应通过独立 Resolver 或玩家组件处理，不建议直接写进 `AGridEnemyManager`。

蓝图 AI 没有生效时检查：

1. 是否覆写的是敌人蓝图子类中的 `Execute Basic Turn`，而不是只放了一个同名自定义事件。
2. 关卡中放置的敌人实例是否使用了该蓝图子类。
3. 覆写函数是否正确返回 `true` 或 `false`。
4. 如果调用 Parent，确认 Parent 默认移动没有因为目标格被占据或不可通行而返回 `false`。
