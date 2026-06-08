# 可拾取道具系统说明

本文档说明运行时拾取物的职责边界、地图生成接入、玩家拾取流程，以及后续扩展新道具类型的方式。当前首个实现是回血道具：玩家进入道具所在格后，调用 `UPlayerAttributeComponent::Heal(1)` 回复 1 点 HP。

## 功能流程

运行时流程如下：

1. `ULSystemDungeonGenerator` 在 `BuildSpawnCandidates` 阶段生成 `RewardSpawnCandidates`。
2. `ADungeonRunManager::GenerateAndInitializeRun()` 应用地图并初始化玩家，可选生成敌人。
3. 如果 `bSpawnPickups = true`，`ADungeonRunManager::SpawnPickupsFromLayout()` 从 `RewardSpawnCandidates` 中选择可通行、未被占据、未已有道具的坐标。
4. `DungeonGenerationSettings.PickupSpawnPool` 根据候选点 `Depth` 过滤道具类型，并按 `Weight` 随机选择具体 `AGridPickupActor` 子类。
5. 生成的拾取物调用 `InitializeOnGrid(GridManager, CandidateCoord)`，并注册到 `AGridPickupManager`。
6. 玩家通过 `AGridPawn::TryMove()` 或击杀敌人后进入新格。
7. 移动成功后，Pawn 先结算地块进入效果，再调用 `ResolvePickupAtCurrentTile()`。
8. `AGridPickupManager::TryCollectPickupAt(CurrentGridCoord, PlayerPawn)` 查找该格道具并调用 `TryCollect()`。
9. `AGridHealthPickupActor` 调用玩家的 `UPlayerAttributeComponent::Heal(HealAmount)`。
10. 如果治疗成功，道具广播收集事件、从 Manager 注销并销毁；如果玩家满血且 `bConsumeWhenEffectFails = false`，回血道具会保留。

拾取物不写入 `AGridManager::Tiles` 的 `OccupantType`。它们由 `AGridPickupManager` 单独按坐标维护，因此不会阻挡 `RequestMove()`。

## C++ 类型

### AGridPickupActor

路径：

- `Source/Chessboard_Roguelike/Public/Pickup/GridPickupActor.h`
- `Source/Chessboard_Roguelike/Private/Pickup/GridPickupActor.cpp`

职责：

- 表示一个位于逻辑格坐标上的可拾取 Actor。
- 持有 `GridCoord`、`GridManager`、`PickupId` 和拾取消耗规则。
- 提供可被蓝图或 C++ 子类覆写的 `ApplyPickupEffect()`。

主要字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `GridCoord` | `FIntPoint` | 当前逻辑格坐标 |
| `GridManager` | `AGridManager*` | 坐标到世界位置转换使用的棋盘管理器 |
| `PickupId` | `FName` | 道具标识，便于调试和蓝图分支 |
| `bConsumeOnSuccessfulEffect` | `bool` | 效果成功时是否消耗道具 |
| `bConsumeWhenEffectFails` | `bool` | 效果失败时是否也消耗道具 |
| `VisualZOffset` | `float` | 显示时相对格子中心的 Z 偏移 |
| `PickupMesh` | `UStaticMeshComponent*` | 默认表现组件，不参与阻挡碰撞 |

主要函数：

| 函数 | 说明 |
| --- | --- |
| `InitializeOnGrid(InGridManager, InGridCoord)` | 绑定棋盘并把 Actor 放到对应格中心 |
| `TryCollect(PlayerPawn)` | 校验、应用效果，并按规则销毁道具 |
| `CanCollect(PlayerPawn)` | `BlueprintNativeEvent`，默认要求道具未被收集且玩家有效 |
| `ApplyPickupEffect(PlayerPawn)` | `BlueprintNativeEvent`，默认返回 `false`，子类实现具体效果 |
| `OnCollected(PlayerPawn, bEffectApplied)` | 蓝图表现事件，可播放音效、粒子或 UI 提示 |

### AGridHealthPickupActor

路径：

- `Source/Chessboard_Roguelike/Public/Pickup/GridHealthPickupActor.h`
- `Source/Chessboard_Roguelike/Private/Pickup/GridHealthPickupActor.cpp`

职责：

- 作为第一个具体拾取物子类。
- 默认 `PickupId = Health`。
- 默认 `HealAmount = 1`。
- `ApplyPickupEffect()` 调用玩家 `UPlayerAttributeComponent::Heal(HealAmount)`。

如果玩家 HP 已满，`Heal()` 返回 `false`。默认情况下回血道具不会被消耗，玩家之后再次进入该格仍可拾取。

### AGridPickupManager

路径：

- `Source/Chessboard_Roguelike/Public/Pickup/GridPickupManager.h`
- `Source/Chessboard_Roguelike/Private/Pickup/GridPickupManager.cpp`

职责：

- 使用 `TMap<FIntPoint, AGridPickupActor*>` 维护道具坐标索引。
- 提供按坐标注册、注销、查询和拾取接口。
- `ClearAllPickups()` 用于重新生成地图前销毁旧道具。

主要函数：

| 函数 | 说明 |
| --- | --- |
| `RegisterPickup(Pickup)` | 按 `Pickup->GridCoord` 注册道具 |
| `UnregisterPickupAt(Coord)` | 移除某坐标上的道具记录 |
| `GetPickupAt(Coord)` | 查询某坐标上的道具 |
| `TryCollectPickupAt(Coord, PlayerPawn)` | 让玩家尝试拾取该坐标道具 |
| `ClearAllPickups()` | 销毁并清空当前所有道具 |

## PCG 配置

### DungeonRunManager

新增字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `bSpawnPickups` | `bool` | 是否在运行初始化时从奖励候选点生成道具 |
| `PickupManager` | `AGridPickupManager*` | 可手动指定；未指定且需要生成道具时会自动生成原生 Manager |

`ADungeonRunManager::SpawnPickupsFromLayout()` 会在敌人生成之后执行。这样如果某个奖励候选点已经被敌人占据，`GridManager->IsOccupied()` 会让道具生成跳过该格。

### DungeonGenerationSettings

新增字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `PickupSpawnCount` | `int32` | 本次运行最多生成多少个拾取物 |
| `PickupSpawnPool` | `TArray<FDungeonPickupSpawnEntry>` | 可生成道具类型池 |

`FDungeonPickupSpawnEntry` 字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `PickupClass` | `TSubclassOf<AGridPickupActor>` | 被生成的道具类 |
| `Weight` | `int32` | 同一候选深度下的相对抽取权重 |
| `MinDepth` | `int32` | 允许出现的最小候选深度 |
| `MaxDepth` | `int32` | 允许出现的最大候选深度 |

## 蓝图资产

已创建的蓝图：

- `Content/Blueprints/Pickups/BP_HealthPickup.uasset`
- `Content/Blueprints/Pickups/BP_GridPickupManager.uasset`

`BP_HealthPickup` 继承 `AGridHealthPickupActor`，默认使用一个基础球体 Mesh 作为可见表现。你可以在蓝图里替换 `PickupMesh`、调整 `VisualZOffset`，或实现 `OnCollected` 播放反馈。

## 扩展新道具

新增一种道具时优先创建 `AGridPickupActor` 的 C++ 或蓝图子类，并覆写 `ApplyPickupEffect()`：

```text
AGridPickupActor
-> AGridHealthPickupActor        // 回复 HP
-> AGridEnergyPickupActor        // 增加构成/酸性能量
-> AGridKeyPickupActor           // 增加钥匙或门禁资源
-> AGridTemporaryBuffPickupActor // 添加临时效果
```

扩展步骤：

1. 创建新的 `AGridPickupActor` 子类。
2. 在子类中实现 `ApplyPickupEffect(PlayerPawn)`。
3. 创建对应蓝图派生类并配置表现。
4. 在 `DungeonGenerationSettings.PickupSpawnPool` 中添加一项。
5. 设置 `PickupClass`、`Weight`、`MinDepth` 和 `MaxDepth`。

不需要修改 `AGridManager::RequestMove()`，也不需要新增 `EGridOccupantType`。道具始终由 `AGridPickupManager` 维护。

## 调试点

### 地图上没有生成道具

检查顺序：

1. `ADungeonRunManager.bSpawnPickups` 是否为 `true`。
2. `DungeonGenerationSettings.PickupSpawnCount` 是否大于 `0`。
3. `DungeonGenerationSettings.PickupSpawnPool` 是否至少有一个有效 `PickupClass`。
4. `RewardCandidateCount` 是否大于 `0`，且 `LastGeneratedLayout.RewardSpawnCandidates` 是否非空。
5. 候选格是否被敌人或玩家占据；被占据的格会跳过生成。
6. 候选点 `Depth` 是否落在池条目的 `MinDepth` / `MaxDepth` 范围内。

### 走到道具格没有回血

检查顺序：

1. 玩家是否成功进入该格；非法移动不会触发拾取。
2. 关卡中是否存在 `AGridPickupManager`，或 `DungeonRunManager` 是否成功自动生成了 Manager。
3. 道具是否已注册到 `PickupsByCoord`。
4. 玩家 Pawn 是否存在 `UPlayerAttributeComponent`。
5. 玩家是否已经满血；默认回血道具满血时不会消耗也不会广播 HP 变化。

### 道具挡住玩家移动

拾取物不应调用 `GridManager->TryOccupyTile()`，也不应写入 `FTileData.OccupantType`。如果玩家被阻挡，优先检查该格是否被敌人、障碍或其他占据 Actor 占用。
