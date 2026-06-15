# 教学关卡系统说明

本文档说明当前教学关卡的实现方式、资产配置、运行入口和验证流程。教学关卡用于稳定演示核心规则，因此不走 `ULSystemDungeonGenerator` 的 PCG 路径。

## 目标

- 每个教学关卡使用固定 `10 x 10` 棋盘。
- 地块类型、玩家出生点、敌人坐标、敌人阵营、敌人行为类型、击杀阈值和固定拾取物全部由 DataAsset 固定配置。
- 运行时仍复用 `AGridManager`、`AGridPawn`、`AGridEnemyPawn`、`AGridEnemyManager`、`AGridPickupManager`、`ATurnManager` 和现有战斗/能量/拾取规则。
- PCG 常规关卡路径保持默认不变。

## 关键资产

| 资产 | 用途 |
| --- | --- |
| `Content/Data/Tutorial/DA_TutorialLevelSet.uasset` | 6 个教学关卡的固定布局、固定敌人和固定拾取物配置 |
| `Content/Maps/Tutorial/L_Tutorial_01_TileAttributes.umap` | 教学 1：地块属性变化 |
| `Content/Maps/Tutorial/L_Tutorial_02_EnemyKills.umap` | 教学 2：敌人击杀 |
| `Content/Maps/Tutorial/L_Tutorial_03_ConversionEnergy.umap` | 教学 3：地块转换能量 |
| `Content/Maps/Tutorial/L_Tutorial_04_RangedFriendlyFire.umap` | 教学 4：远程敌人和友伤 |
| `Content/Maps/Tutorial/L_Tutorial_05_FactionSuppression.umap` | 教学 5：阵营压制系统 |
| `Content/Maps/Tutorial/L_Tutorial_06_HealthAndTransform.umap` | 教学 6：生命恢复物和变身效果 |

## C++ 类型

### EDungeonRunGenerationMode

位置：`Source/Chessboard_Roguelike/Public/PCG/DungeonRunManager.h`

| 值 | 说明 |
| --- | --- |
| `Procedural` | 默认模式，调用 `ULSystemDungeonGenerator::GenerateDungeonLayout()` 生成 PCG 地图 |
| `TutorialFixed` | 教学模式，跳过 PCG，读取 `UTutorialLevelSet` 中的固定关卡定义 |

### UTutorialLevelSet

位置：

- `Source/Chessboard_Roguelike/Public/Tutorial/TutorialLevelSet.h`
- `Source/Chessboard_Roguelike/Private/Tutorial/TutorialLevelSet.cpp`

字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `TutorialLevels` | `TArray<FTutorialLevelDefinition>` | 教学关卡列表 |

`UTutorialLevelSet` 构造函数内置了 6 个默认教学关卡。编辑器中的 `DA_TutorialLevelSet` 会基于这些默认数据创建，并额外绑定具体敌人和拾取物蓝图类。

### FTutorialLevelDefinition

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `LevelId` | `FName` | 关卡标识 |
| `Width` / `Height` | `int32` | 当前要求固定为 `10 x 10` |
| `StartCoord` | `FIntPoint` | 玩家出生坐标 |
| `ExitCoord` | `FIntPoint` | 预留出口坐标 |
| `Tiles` | `TArray<FGridTileLayoutData>` | 100 个固定地块布局数据 |
| `Enemies` | `TArray<FTutorialEnemySpawnData>` | 固定敌人生成列表 |
| `Pickups` | `TArray<FTutorialPickupSpawnData>` | 固定拾取物生成列表 |

### FTutorialEnemySpawnData

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `Coord` | `FIntPoint` | 敌人生成坐标 |
| `EnemyClass` | `TSubclassOf<AGridEnemyPawn>` | 敌人蓝图类 |
| `Faction` | `EEnemyFaction` | 构成或酸性阵营 |
| `BehaviorType` | `EEnemyBehaviorType` | 近战或远程 |
| `KillThreshold` | `int32` | 单次击杀阈值 |
| `MaxRangedAttackDistance` | `int32` | 远程攻击最大距离；近战敌人保持 `0` |

### FTutorialPickupSpawnData

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `Coord` | `FIntPoint` | 拾取物生成坐标 |
| `PickupClass` | `TSubclassOf<AGridPickupActor>` | 生命恢复物或变身棋子蓝图类 |

## DungeonRunManager 接入

教学关卡 Map 中的 `BP_DungeonRunManager` 已配置为：

| 属性 | 值 |
| --- | --- |
| `GenerationMode` | `TutorialFixed` |
| `TutorialLevelSet` | `DA_TutorialLevelSet` |
| `TutorialLevelIndex` | 对应关卡索引 `0-5` |
| `bSpawnEnemies` | `false` |
| `bSpawnPickups` | `false` |

`bSpawnEnemies` 在教学 Map 中保持 `false`，因为敌人由 `FTutorialLevelDefinition.Enemies` 固定生成，不从 PCG 候选点和 `DungeonGenerationSettings.EnemySpawnPool` 抽取。

运行流程：

1. `ADungeonRunManager::GenerateAndInitializeRun()` 根据 `GenerationMode` 判断运行模式。
2. `TutorialFixed` 模式调用 `GenerateTutorialRun()`。
3. `ApplyTutorialLevel()` 校验 `Width == 10`、`Height == 10`、`Tiles.Num() == 100`。
4. 调用 `GridManager->ApplyTileLayout(Tiles, 10, 10)` 应用固定地图。
5. 调用 `InitializePlayerAtCoord(StartCoord)` 初始化玩家。
6. `SpawnTutorialEnemies()` 使用 deferred spawn 生成固定敌人。
7. `SpawnTutorialPickups()` 使用 deferred spawn 生成固定拾取物；没有配置拾取物的关卡会直接跳过。
8. 初始化 `EnemyManager` 并将回合状态设置为 `PlayerInput`。

## 教学关卡内容

| 索引 | Map | LevelId | 内容 |
| --- | --- | --- | --- |
| `0` | `L_Tutorial_01_TileAttributes` | `Tutorial_01_TileAttributes` | 无敌人，使用构成、酸性和极简地块让玩家熟悉踩地块后的属性变化 |
| `1` | `L_Tutorial_02_EnemyKills` | `Tutorial_02_EnemyKills` | 固定生成构成近战和酸性近战敌人，引导玩家收集足够属性后击杀 |
| `2` | `L_Tutorial_03_ConversionEnergy` | `Tutorial_03_ConversionEnergy` | 近处 1 阈值酸性敌人，远处 3 阈值构成和酸性敌人；地图仅有一个构成地块，引导玩家通过击杀获得转换能量 |
| `3` | `L_Tutorial_04_RangedFriendlyFire` | `Tutorial_04_RangedFriendlyFire` | 在近战敌人基础上加入构成远程和酸性远程敌人，用于演示远程瞄准、攻击线和跨阵营友伤 |
| `4` | `L_Tutorial_05_FactionSuppression` | `Tutorial_05_FactionSuppression` | 阵营压制系统。`(0,0)` 到 `(9,1)` 为酸性地块，酸性近战敌人在 `(0,2)`；`(0,8)` 到 `(9,9)` 为构成地块，构成近战敌人在 `(0,9)` |
| `5` | `L_Tutorial_06_HealthAndTransform` | `Tutorial_06_HealthAndTransform` | 生命恢复物和变身效果。玩家从 `(1,1)` 出生，地图中固定放置 3 个生命恢复物和 Knight、Bishop、Rook、Queen 四种变身棋子，让玩家熟悉踩格拾取、恢复和变身库存机制 |

## 敌人蓝图绑定

`DA_TutorialLevelSet` 中的敌人类已绑定到现有蓝图：

| 用途 | 蓝图类 |
| --- | --- |
| 构成近战 | `Content/Blueprints/Enemies/ConstructEnemies/BP_ConstructEnemy_Melee.uasset` |
| 酸性近战 | `Content/Blueprints/Enemies/AcidEnemies/BP_AcidEnemy_Melee.uasset` |
| 构成远程 | `Content/Blueprints/Enemies/ConstructEnemies/BP_ConstructEnemy_Ranged.uasset` |
| 酸性远程 | `Content/Blueprints/Enemies/AcidEnemies/BP_AcidEnemy_Ranged.uasset` |

如果某个教学敌人的 `EnemyClass` 未配置，C++ 会回退到 `AGridEnemyPawn::StaticClass()`，但正式教学资产应始终绑定具体敌人蓝图，保证表现一致。

## 拾取物蓝图绑定

第 6 关的 `Pickups` 已绑定到现有拾取物蓝图：

| 用途 | 坐标 | 蓝图类 |
| --- | --- | --- |
| 生命恢复物 | `(2,1)`、`(1,3)`、`(7,7)` | `Content/Blueprints/Pickups/BP_HealthPickup.uasset` |
| Knight 变身棋子 | `(3,2)` | `Content/Blueprints/Pickups/Transform/BP_TransformPiece_Knight.uasset` |
| Bishop 变身棋子 | `(5,2)` | `Content/Blueprints/Pickups/Transform/BP_TransformPiece_Bishop.uasset` |
| Rook 变身棋子 | `(3,5)` | `Content/Blueprints/Pickups/Transform/BP_TransformPiece_Rook.uasset` |
| Queen 变身棋子 | `(6,6)` | `Content/Blueprints/Pickups/Transform/BP_TransformPiece_Queen.uasset` |

## 自动化脚本

| 脚本 | 用途 |
| --- | --- |
| `Tools/CreateTutorialAssets.py` | 创建或更新 `DA_TutorialLevelSet`，复制 5 张教学 Map |
| `Tools/AddTutorialLevel06.py` | 只追加或更新第 6 关 DataAsset 条目，并创建/配置 `L_Tutorial_06_HealthAndTransform`，不会重建前 5 关 |
| `Tools/ConfigureTutorialMap.py` | 打开当前 Map 后设置 `DungeonRunManager` 的教学模式、DataAsset 和索引 |
| `Tools/ValidateTutorialMap.py` | 验证当前 Map 的 `DungeonRunManager` 教学配置 |
| `Tools/ValidateTutorialDataAsset.py` | 验证 DataAsset 中存在第 6 关，并检查第 6 关 10x10 布局、起终点和拾取物蓝图绑定 |

如果前 5 关已经在编辑器中手动调整，新增第 6 关时应使用 `Tools/AddTutorialLevel06.py`，不要重新运行 `Tools/CreateTutorialAssets.py`，以免批量脚本重建旧教学关卡资产。

示例验证命令：

```powershell
$env:CHESSBOARD_TUTORIAL_INDEX='0'
& 'D:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe' `
  'F:\Documents\Unreal Projects\Chessboard_Roguelike\Chessboard_Roguelike.uproject' `
  /Game/Maps/Tutorial/L_Tutorial_01_TileAttributes `
  -ExecutePythonScript='F:\Documents\Unreal Projects\Chessboard_Roguelike\Tools\ValidateTutorialMap.py' `
  -unattended -nop4 -nullrhi
```

DataAsset 验证：

```powershell
& 'D:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe' `
  'F:\Documents\Unreal Projects\Chessboard_Roguelike\Chessboard_Roguelike.uproject' `
  -ExecutePythonScript='F:\Documents\Unreal Projects\Chessboard_Roguelike\Tools\ValidateTutorialDataAsset.py' `
  -unattended -nop4 -nullrhi
```

## 调整教学关卡

调整现有教学关卡时优先修改 `DA_TutorialLevelSet`：

1. 打开 `Content/Data/Tutorial/DA_TutorialLevelSet.uasset`。
2. 在 `TutorialLevels` 中选择对应索引。
3. 修改 `Tiles` 中的地块类型，或修改 `Enemies` 中的坐标、敌人类、阵营、行为类型和击杀阈值。
   如关卡使用固定拾取物，也可以修改 `Pickups` 中的坐标和拾取物蓝图类。
4. 保存 DataAsset。
5. 打开对应教学 Map 运行验证。

如果需要继续新增教学关卡：

1. 在 `DA_TutorialLevelSet.TutorialLevels` 中添加一个 `FTutorialLevelDefinition`。
2. 保持 `Width = 10`、`Height = 10`、`Tiles.Num() = 100`。
3. 新建或复制一个 Map，并把场景中的 `DungeonRunManager.TutorialLevelIndex` 指向新索引。
4. 运行 `ValidateTutorialMap.py` 和 `ValidateTutorialDataAsset.py`。

## 调试点

- 地图未变为 10x10：检查 `DungeonRunManager.GenerationMode` 是否为 `TutorialFixed`。
- 敌人没有生成：检查 `DA_TutorialLevelSet` 中对应关卡的 `Enemies` 数组，以及敌人坐标是否可通行、未被玩家占据。
- 敌人表现不对：检查 `EnemyClass` 是否绑定到正确蓝图，而不是回退到原生 `AGridEnemyPawn`。
- 教学 Map 仍然出现 PCG 随机内容：检查是否打开了 `Content/Maps/Tutorial` 下的 Map，以及 `GenerationMode` 是否被改回 `Procedural`。
