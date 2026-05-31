# 棋盘格移动原型使用手册

本文档说明当前棋盘格地图、WASD 单格移动、占据系统、基础回合锁、地块属性效果和属性 HUD 的使用方式。核心逻辑在 C++ 中，蓝图和 DataAsset 主要用于配置、派生和测试。

## 快速入口

测试关卡：

- `Content/Maps/L_GridMovement_Test.umap`

主要蓝图：

- `Content/Blueprints/Grid/BP_GridManager.uasset`
- `Content/Blueprints/Player/BP_GridPawn.uasset`
- `Content/Blueprints/Player/BP_GridPlayerController.uasset`
- `Content/Blueprints/Core/BP_TurnManager.uasset`
- `Content/Blueprints/Core/BP_GameMode_GridPrototype.uasset`

主要 C++ PCG 类型：

- `Source/Chessboard_Roguelike/Public/PCG/DungeonGenerationSettings.h`
- `Source/Chessboard_Roguelike/Public/PCG/LSystemDungeonGenerator.h`
- `Source/Chessboard_Roguelike/Public/PCG/DungeonRunManager.h`

主要 DataAsset：

- `Content/Data/Grid/DA_GridSettings_Prototype.uasset`
- `Content/Data/DA_TileAttributeEffectConfig.uasset`
- `Content/Data/Input/IMC_GridMovement.uasset`
- `Content/Data/Input/IA_MoveUp.uasset`
- `Content/Data/Input/IA_MoveDown.uasset`
- `Content/Data/Input/IA_MoveLeft.uasset`
- `Content/Data/Input/IA_MoveRight.uasset`

专题说明：

- [TileAttributeEffect_SystemGuide.md](../02_TileAttributesAndEnergy/TileAttributeEffect_SystemGuide.md): 地块属性变化、双属性值、HUD、地块材质刷新和蓝图接口说明。
- [CombatAndEnemy_SystemGuide.md](../03_CombatAndEnemies/CombatAndEnemy_SystemGuide.md): 近战攻击、敌人属性、免疫规则、单次阈值击杀和退回规则说明。

## 运行流程

1. 关卡中放置 `BP_GridManager` 和 `BP_TurnManager`。
2. `BP_GridManager` 指定 `DA_GridSettings_Prototype`。
3. `BP_GameMode_GridPrototype` 使用 `BP_GridPawn` 作为 Default Pawn，使用 `BP_GridPlayerController` 作为 Player Controller。
4. `AGridPawn::BeginPlay()` 自动查找 `AGridManager` 和 `ATurnManager`。
5. Pawn 调用 `InitializeOnGrid()` 占据起点格，并把 `TurnState` 设置为 `PlayerInput`。
6. Controller 通过 Enhanced Input 接收 WASD，转成 `FIntPoint` 方向后调用 Pawn 的 `TryMove()`。
7. Pawn 请求 `GridManager::RequestMove()`，成功后更新逻辑坐标、步数、地块效果和视觉插值。

## PCG 运行流程

PCG 地牢关卡建议使用 `ADungeonRunManager` 作为统一入口，而不是让 Pawn 和敌人各自在 `BeginPlay()` 中抢先初始化。

推荐流程：

1. 关卡中放置 `BP_GridManager`、`BP_TurnManager`、玩家 Pawn，以及一个 `ADungeonRunManager` 派生蓝图或原生 Actor。
2. 在 `DungeonRunManager` 上指定 `DungeonGenerationSettings`。
3. 保持 `bAutoFindReferences = true`，或手动指定 `GridManager`、`TurnManager`、`PlayerPawn`、`EnemyManager`。
4. 如果希望自动生成敌人，设置 `bSpawnEnemies = true`，并在 `DungeonGenerationSettings.EnemySpawnPool` 中配置敌人类型池。
5. BeginPlay 时 `DungeonRunManager` 调用 `GenerateAndInitializeRun()`。
6. `ULSystemDungeonGenerator` 生成 `FGeneratedDungeonLayout`。
7. `GridManager->ApplyTileLayout(Layout.Tiles, Layout.Width, Layout.Height)` 应用生成结果。
8. 玩家被初始化到 `Layout.StartCoord`。
9. 可选敌人根据 `Layout.EnemySpawnCandidates` 生成并注册到 `EnemyManager`。
10. `TurnManager` 切换到 `PlayerInput`。

注意：

- PCG 生成器只产出数据，不直接 Spawn Actor，也不直接修改 `GridManager.Tiles`。
- `ADungeonRunManager` 会在运行开始前关闭玩家的自动网格初始化，避免玩家先占用手工起点。
- 敌人使用 deferred spawn，并在生成前关闭 `bAutoInitializeOnBeginPlay`，避免敌人先占用默认 `StartGridCoord`。
- 敌人类型不在 `DungeonRunManager` 上单独配置，而是从 `UDungeonGenerationSettings.EnemySpawnPool` 中按权重和深度筛选。
- 当前 PCG 不依赖 UE PCG 插件；UE PCG Graph 后续更适合用于墙体、装饰和场景资产散布。

## GridSettings 配置

类：`UGridSettings`

路径：`Source/Chessboard_Roguelike/Public/Grid/GridSettings.h`

可修改参数：

- `Width`: 棋盘宽度，默认 `10`。
- `Height`: 棋盘高度，默认 `10`。
- `TileSize`: 格子中心间距，默认 `200.0`。
- `Origin`: `GridCoord(0,0)` 对应的世界坐标。
- `TileMesh`: 棋盘格视觉用 Static Mesh；不影响移动合法性。
- `InitialTileOverrides`: 手动指定某些格子的初始地块。

`InitialTileOverrides` 单项参数：

- `GridCoord`: 要覆盖的格子坐标。
- `TileType`: `Minimal`、`Construct`、`Acid`、`Obstacle`。
- `CellRole`: 地图形态语义，当前支持 `Open`、`Room`、`Corridor`、`Wall`、`Start`、`Exit`。
- `bWalkable`: 是否可通行；`Obstacle` 会强制不可通行。
- `bConvertible`: 是否允许后续地块效果把该格转换为其他类型。
- `RegionId`: 房间或区域编号，默认 `INDEX_NONE`。
- `Depth`: 区域深度，可用于后续难度分层和房间激活。

修改 `Width`、`Height`、`TileSize` 后，`GenerateGrid()` 会按新设置重建 `Tiles` 和 ISM 实例。

## Tile 数据

结构：`FTileData`

关键字段：

- `GridCoord`: 逻辑坐标。角色真实位置以它为准。
- `TileType`: 地块类型。
- `CellRole`: 地图形态语义，用于区分房间、走廊、墙、起点和出口。
- `RegionId`: 生成房间或区域 ID。
- `Depth`: 生成深度或房间层级。
- `OccupantType`: 占据类型。
- `OccupantActor`: 当前占据 Actor。
- `bWalkable`: 是否可通行。
- `bConvertible`: 是否可被地块效果转换。

地块类型 `ETileType`：

- `Minimal`: 默认地块。
- `Construct`: 构成地块。
- `Acid`: 酸性地块。
- `Obstacle`: 障碍地块。

地图形态 `EGridCellRole`：

- `Open`: 普通开放格。
- `Room`: 房间地面。
- `Corridor`: 走廊地面。
- `Wall`: 墙或不可通行边界。
- `Start`: 起点区域。
- `Exit`: 出口或目标区域。

占据类型 `EGridOccupantType`：

- `Empty`
- `Player`
- `Enemy`
- `Obstacle`

## GridManager 可调用接口

类：`AGridManager`

路径：`Source/Chessboard_Roguelike/Public/Grid/GridManager.h`

核心变量：

- `GridSettings`: 当前棋盘配置。
- `TileISM`: 棋盘格视觉实例组件。
- `Tiles`: `TMap<FIntPoint, FTileData>`，棋盘逻辑数据源。
- `CurrentWidth`: 当前已生成布局宽度。
- `CurrentHeight`: 当前已生成布局高度。
- `OnTileTypeChanged`: 地块类型改变事件。

常用函数：

- `GenerateGrid()`: 按 `GridSettings` 重建棋盘数据和视觉实例。
- `ApplyTileLayout(TileLayout, LayoutWidth, LayoutHeight)`: 应用完整生成布局，并重建 `Tiles` 与 ISM 视觉实例。
- `IsValidCoord(Coord)`: 判断坐标是否在棋盘中。
- `IsWalkable(Coord)`: 判断格子是否可通行。
- `IsOccupied(Coord)`: 判断格子是否已被占据。
- `GetTileData(Coord, OutTileData)`: 读取格子数据副本。
- `SetTileType(Coord, NewTileType)`: 修改地块类型，不修改占据状态。
- `IsTileConvertible(Coord)`: 判断地块是否可转换。
- `GridToWorld(Coord)`: 逻辑坐标转世界坐标。
- `WorldToGrid(WorldLocation)`: 世界坐标转逻辑坐标。
- `TryOccupyTile(Coord, Occupant, OccupantType)`: 初始占据某格。
- `ClearOccupant(Coord)`: 清空某格占据。
- `RequestMove(Unit, FromCoord, ToCoord)`: 原子移动请求。
- `GetTileInstanceIndex(Coord, OutInstanceIndex)`: 获取坐标对应的 ISM 实例下标。
- `RefreshTileInstanceVisual(Coord, NewTileType)`: 蓝图可重写的视觉刷新钩子。

`ApplyTileLayout()` 使用约定：

- `TileLayout` 的元素类型为 `FGridTileLayoutData`。
- PCG、关卡初始化器或调试工具应通过该入口写入整张地图，不应直接修改 `Tiles`。
- `GridSettings` 仍需存在，并提供 `TileSize`、`Origin`、`TileMesh` 等坐标和视觉参数。
- `Wall` 或 `Obstacle` 会被视为不可通行，并以 `Obstacle` 占据类型写入。
- 应用成功后会清空旧格子、旧占据状态和旧 ISM 实例。

## DungeonGenerationSettings 配置

类：`UDungeonGenerationSettings`

路径：`Source/Chessboard_Roguelike/Public/PCG/DungeonGenerationSettings.h`

核心参数：

- `Seed`: 地牢生成种子。
- `Width` / `Height`: 生成布局尺寸。
- `Axiom`: L-System 初始字符串。
- `Iterations`: L-System 展开次数。
- `ProductionRules`: L-System 生产规则，键为单字符名称，例如 `F`。
- `MinRoomRadius` / `MaxRoomRadius`: 房间半径范围。
- `MaxRoomCount`: 最大房间数量。
- `BoundaryNoise`: 房间不规则边界扰动强度。
- `CorridorWidth`: 走廊宽度。
- `CorridorWanderChance`: 走廊崎岖偏移概率，范围 `0-100`。
- `MaxGenerationAttempts`: 生成失败后的最大重试次数。
- `StartSafetyRadius`: 起点安全区半径。
- `ConstructTileChance`: 可通行格变为构成地块的概率。
- `AcidTileChance`: 可通行格变为酸性地块的概率。
- `EnemySpawnCount`: 敌人候选生成数量。
- `EnemySpawnPool`: 敌人类型池。每项包含 `EnemyClass`、`Weight`、`MinDepth` 和 `MaxDepth`，运行时会按候选点深度筛选并按权重随机选择。
- `EventCandidateCount`: 事件候选生成数量。
- `RewardCandidateCount`: 奖励候选生成数量。

L-System 符号：

- `F`: 前进并创建房间。
- `B`: 创建分支房间，当前等价于 `F`，预留给后续分支权重。
- `E`: 创建出口候选房间，当前最终出口选择深度最高房间。
- `R`: 保留当前房间节点。
- `+`: 右转。
- `-`: 左转。
- `[`: 保存当前生成状态。
- `]`: 恢复最近保存的生成状态。

## 生成布局数据

结构：`FGeneratedDungeonLayout`

关键字段：

- `Width` / `Height`: 生成结果尺寸。
- `Seed`: 本次生成实际使用的种子。重试时会基于配置种子递增。
- `Tiles`: `FGridTileLayoutData` 数组，可直接传入 `ApplyTileLayout()`。
- `Rooms`: 生成房间节点。
- `Connections`: 房间连接边和走廊路径。
- `StartCoord`: 玩家出生坐标。
- `ExitCoord`: 出口坐标。
- `EnemySpawnCandidates`: 敌人出生候选点。
- `EventSpawnCandidates`: 事件候选点。
- `RewardSpawnCandidates`: 奖励候选点。

`FDungeonSpawnCandidate` 包含 `Coord`、`RegionId` 和 `Depth`，用于后续刷怪、事件、奖励系统按区域和深度做筛选。

`RequestMove()` 规则：

- 目标坐标必须存在。
- 目标格必须可通行。
- 目标格不能被占据。
- 源格必须由传入的 `Unit` 占据。
- 全部检查通过后才会清空源格并占据目标格。
- 非法移动返回 `false`，不会修改任何占据状态。

`OnTileTypeChanged` 事件：

- 触发时机：`SetTileType()` 成功改变地块类型后。
- 参数：`TileCoord`、`NewTileType`。
- 用途：蓝图或材质表现层监听后刷新单个格子的外观。

视觉说明：

- `TileISM->PerInstanceCustomData[0]` 存储地块类型数值。
- 当前映射：`Minimal=0`、`Construct=1`、`Acid=2`、`Obstacle=3`。
- 材质可以读取该值决定颜色或图案。

## Pawn 移动接口

类：`AGridPawn`

路径：`Source/Chessboard_Roguelike/Public/Player/GridPawn.h`

可修改参数：

- `StartGridCoord`: 起点格，默认 `(0,0)`。
- `bAutoInitializeOnBeginPlay`: 是否 BeginPlay 自动查找 Manager 并初始化。
- `MoveDuration`: 视觉插值时间，默认 `0.15` 秒。

只读状态：

- `CurrentGridCoord`: 当前逻辑坐标。
- `bIsMoving`: 是否处于视觉移动插值中。
- `GridManager`: 当前使用的 GridManager。
- `TurnManager`: 当前使用的 TurnManager。

可调用函数：

- `InitializeOnGrid(InGridManager, InTurnManager, InStartCoord)`: 初始化到指定格子并占据起点。
- `TryMove(Direction)`: 请求四方向单格移动。
- `StartVisualMove(From, To)`: 开始视觉插值。
- `FinishVisualMove()`: 结束视觉插值并精确对齐目标格中心。

`TryMove(Direction)` 要求：

- `GridManager` 有效。
- `TurnManager` 有效。
- `TurnManager->CanAcceptPlayerInput()` 为 `true`。
- `bIsMoving == false`。
- `Direction` 必须是四方向之一，即 `Abs(X) + Abs(Y) == 1`。
- 目标格存在、可通行、未被占据。

成功移动后：

- 调用 `TurnManager->BeginPlayerAction()`。
- 调用 `GridManager->RequestMove()`。
- 更新 `CurrentGridCoord`。
- 调用 `TileEffectResolverComponent->ResolveTileEnterEffect()`。
- 调用 `TurnManager->AddStep()`。
- 开始视觉插值。
- 插值结束后调用 `TurnManager->EndPlayerAction()`。

非法移动后：

- 不改变 `CurrentGridCoord`。
- 不改变格子占据状态。
- 不增加 `StepCount`。
- 不进入视觉插值。

## PlayerController 输入配置

类：`AGridPlayerController`

路径：`Source/Chessboard_Roguelike/Public/Player/GridPlayerController.h`

可配置字段：

- `GridMovementMappingContext`
- `MoveUpAction`
- `MoveDownAction`
- `MoveLeftAction`
- `MoveRightAction`
- `PlayerAttributeHUDClass`

Enhanced Input 行为：

- BeginPlay 添加 `GridMovementMappingContext`。
- `SetupInputComponent()` 绑定四个 InputAction。
- 使用 `ETriggerEvent::Started`，每次按下触发一次。

当前 C++ 方向映射：

- `MoveUp()` -> `FIntPoint(0, 1)`
- `MoveDown()` -> `FIntPoint(0, -1)`
- `MoveLeft()` -> `FIntPoint(1, 0)`
- `MoveRight()` -> `FIntPoint(-1, 0)`

注意：当前 Left/Right 与常见 WASD 需求相反。如果要恢复常规方向，请在 `GridPlayerController.cpp` 中改为：

```cpp
void AGridPlayerController::MoveLeft()
{
	RequestPawnMove(FIntPoint(-1, 0));
}

void AGridPlayerController::MoveRight()
{
	RequestPawnMove(FIntPoint(1, 0));
}
```

## TurnManager 回合锁

类：`ATurnManager`

路径：`Source/Chessboard_Roguelike/Public/Core/TurnManager.h`

状态枚举 `ETurnState`：

- `Initializing`: 初始化中。
- `PlayerInput`: 允许玩家输入。
- `PlayerActionResolve`: 玩家行动结算中，输入锁定。
- `EnemyTurnResolve`: 预留敌人回合。
- `MapResolve`: 预留地图结算。

变量：

- `CurrentTurnState`: 当前回合状态。
- `StepCount`: 成功合法移动步数。

可调用函数：

- `CanAcceptPlayerInput()`: 只有 `PlayerInput` 返回 `true`。
- `SetTurnState(NewState)`: 手动设置状态。
- `BeginPlayerAction()`: 切到 `PlayerActionResolve`。
- `EndPlayerAction()`: 切回 `PlayerInput`。
- `AddStep()`: 步数加一。

使用约定：

- 合法移动成功后才调用 `AddStep()`。
- 非法移动不调用 `BeginPlayerAction()`，也不增加步数。
- 视觉移动期间依靠 `PlayerActionResolve` 和 Pawn 的 `bIsMoving` 双重锁输入。

## 地块属性效果

类：`UTileEffectResolverComponent`

路径：`Source/Chessboard_Roguelike/Public/Grid/TileEffectResolverComponent.h`

作用：

- 只处理“玩家成功进入地块后”的属性变化。
- 不负责移动合法性、战斗、AI 或 HUD。

可配置字段：

- `TileAttributeEffectConfig`: 可选配置资产；未设置时使用代码默认值。
- `GridManager`: 当前使用的 GridManager。

可调用函数：

- `ResolveTileEnterEffect(PlayerActor, TileCoord)`: 处理进入格子的效果。
- `SetGridManager(InGridManager)`: 显式设置 GridManager，避免运行时搜索。

行为：

- `Construct` 地块：增加 Construct，减少 Acid。
- `Acid` 地块：增加 Acid，减少 Construct。
- `Minimal` 和 `Obstacle` 不触发属性变化。
- 如果配置允许消耗地块，且目标格 `bConvertible == true`，进入后调用 `SetTileType()` 把地块变为配置中的结果类型。

## 地块属性效果配置

类：`UTileAttributeEffectConfig`

路径：`Source/Chessboard_Roguelike/Public/Data/TileAttributeEffectConfig.h`

资产：`Content/Data/DA_TileAttributeEffectConfig.uasset`

可修改参数：

- `ConstructTile_ConstructDelta`: 进入 Construct 地块时 Construct 变化量。
- `ConstructTile_AcidDelta`: 进入 Construct 地块时 Acid 变化量。
- `AcidTile_ConstructDelta`: 进入 Acid 地块时 Construct 变化量。
- `AcidTile_AcidDelta`: 进入 Acid 地块时 Acid 变化量。
- `MinAttributeValue`: 配置预留，目前属性组件实际使用自身最小值 `0`。
- `MaxAttributeValue`: 配置预留，目前属性组件实际使用自身最大值。
- `bConsumeTileAfterEnter`: 进入 Construct/Acid 后是否消耗地块。
- `ConsumedTileResultType`: 消耗后变成的地块类型，默认 `Minimal`。

## 玩家属性组件

类：`UPlayerAttributeComponent`

路径：`Source/Chessboard_Roguelike/Public/Player/PlayerAttributeComponent.h`

私有但可在蓝图 Details 中编辑的值：

- `CurrentHealth`
- `MaxHealth`
- `ConstructValue`
- `AcidValue`
- `MaxConstructValue`
- `MaxAcidValue`

可调用函数：

- `ApplyHealthDamage(DamageAmount)`: 扣减 HP。
- `Heal(HealAmount)`: 恢复 HP。
- `GetCurrentHealth()`
- `GetMaxHealth()`
- `GetHealthRatio()`: 返回 0 到 1。
- `ApplyTileAttributeDelta(ConstructDelta, AcidDelta)`: 同时应用两个属性变化。
- `AddConstructValue(Delta)`: 修改 Construct。
- `AddAcidValue(Delta)`: 修改 Acid。
- `GetConstructValue()`
- `GetAcidValue()`
- `GetMaxConstructValue()`
- `GetMaxAcidValue()`
- `GetConstructRatio()`: 返回 0 到 1。
- `GetAcidRatio()`: 返回 0 到 1。
- `IsConstructValueMaxed()`
- `IsAcidValueMaxed()`
- `IsDefeated()`

事件：

- `OnPlayerAttributeChanged(NewConstructValue, NewAcidValue)`
- `OnPlayerHealthChanged(NewHealth, MaxHealth)`
- `OnPlayerDefeated()`

触发时机：

- `ApplyTileAttributeDelta()` 后，任一属性的最终值发生变化才广播。
- `ApplyHealthDamage()` 或 `Heal()` 后，HP 的最终值发生变化才广播。
- HP 从正数降到 0 时广播 `OnPlayerDefeated()`。
- 属性值会 Clamp 到 `[0, MaxValue]`。

## 属性 HUD

类：`UPlayerAttributeHUDWidget`

路径：`Source/Chessboard_Roguelike/Public/UI/PlayerAttributeHUDWidget.h`

可绑定控件名：

- `HealthText`
- `HealthProgressBar`
- `ConstructText`
- `AcidText`
- `ConstructProgressBar`
- `AcidProgressBar`

可调用函数：

- `InitializeFromAttributeComponent(InAttributeComponent)`: 绑定属性组件。
- `RefreshAttributeDisplay()`: 手动刷新显示。

行为：

- Controller 默认尝试加载 `/Game/UI/WBP_PlayerAttributeHUD`。
- 如果没有该 Widget Blueprint，则使用原生 fallback Widget Tree。
- HUD 只读属性组件，不写 gameplay 数据。
- HUD 监听 `OnPlayerAttributeChanged`，不使用 Tick 轮询。
- HUD 监听 `OnPlayerHealthChanged` 刷新 HP 文本和进度条。

## 常见修改方式

修改棋盘大小：

1. 打开 `DA_GridSettings_Prototype`。
2. 修改 `Width`、`Height`、`TileSize`。
3. 重新进入 PIE，或在 GridManager 上调用 `GenerateGrid()`。

添加障碍格：

1. 打开 `DA_GridSettings_Prototype`。
2. 在 `InitialTileOverrides` 添加一项。
3. 设置 `GridCoord`。
4. 设置 `TileType = Obstacle`。
5. `bWalkable` 保持 `false` 或任意值；Obstacle 会强制不可通行。

添加 Construct/Acid 测试格：

1. 在 `InitialTileOverrides` 添加坐标。
2. 设置 `TileType = Construct` 或 `Acid`。
3. 设置 `bWalkable = true`。
4. 设置 `bConvertible = true`，进入后可被消耗。

修改移动速度：

1. 打开 `BP_GridPawn`。
2. 修改 `MoveDuration`。

修改起点：

1. 打开 `BP_GridPawn`。
2. 修改 `StartGridCoord`。
3. 确保该格存在、可通行、未被占据。

修改输入：

1. 打开 `IMC_GridMovement`。
2. 修改 `W/S/A/D` 或添加其他按键。
3. 如果新增动作，需要在 `BP_GridPlayerController` 中指定对应 InputAction，并在 C++ 增加绑定函数。

修改地块效果数值：

1. 打开 `DA_TileAttributeEffectConfig`。
2. 修改 Construct/Acid 的 Delta。
3. 如需接到 Pawn，确保 `BP_GridPawn` 的 `TileEffectResolverComponent` 引用了该配置资产。

自定义地块视觉：

1. 在 `BP_GridManager` 重写 `RefreshTileInstanceVisual(Coord, NewTileType)`。
2. 或在棋盘材质中读取 `PerInstanceCustomData[0]`。
3. 使用 `GetTileInstanceIndex()` 找到单个格子的 ISM 实例下标。

## 调试建议

- 检查 `GridManager.Tiles.Num()` 是否等于 `Width * Height`。
- 检查 Pawn 的 `CurrentGridCoord` 是否与 Actor Location 对应。
- 检查 TurnManager 的 `CurrentTurnState` 是否在移动后回到 `PlayerInput`。
- 检查非法移动时 `StepCount` 是否不变。
- 检查目标格 `OccupantActor` 是否指向玩家 Pawn。
- 检查 `GridMovementMappingContext` 和四个 InputAction 是否在 `BP_GridPlayerController` 中赋值。

## 当前边界

已实现：

- 固定尺寸棋盘生成。
- 逻辑坐标和世界坐标转换。
- WASD 单格移动。
- 占据系统。
- 基础回合输入锁。
- 可选 Construct/Acid 地块属性效果。
- 简单属性 HUD。

未实现：

- 敌人 AI。
- 敌人移动。
- 战斗和攻击。
- 多房间。
- PCG 地图。
- 完整 HUD/美术表现。
- 存档。
- 联机同步。
