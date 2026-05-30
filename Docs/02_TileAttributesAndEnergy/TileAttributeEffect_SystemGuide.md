# 地块属性变化与双属性值系统说明

本文档说明任务 4 功能的实现方式、可在蓝图中调用或修改的事件、值、参数，以及地块材质刷新的配置约定。该系统只处理玩家成功移动后的地块属性效果，不包含攻击、击杀、敌人 AI、地块转换能量、PCG 或多房间逻辑。

## 功能流程

运行时流程如下：

1. 玩家通过 `AGridPawn::TryMove()` 请求向相邻格移动。
2. `AGridManager::RequestMove()` 校验目标格是否存在、可通行、未被占据。
3. 移动成功后，Pawn 更新 `CurrentGridCoord`。
4. Pawn 调用 `UTileEffectResolverComponent::ResolveTileEnterEffect()`。
5. Resolver 读取目标格 `FTileData.TileType`。
6. 如果是 `Construct` 或 `Acid`，Resolver 修改玩家 `UPlayerAttributeComponent`。
7. 如果该格 `bConvertible == true`，Resolver 调用 `AGridManager::SetTileType()` 把该格转为配置的消耗结果，默认是 `Minimal`。
8. `SetTileType()` 更新 `PerInstanceCustomData[0]`，调用 `RefreshTileInstanceVisual()`，并广播 `OnTileTypeChanged`。
9. HUD 监听 `OnPlayerAttributeChanged`，刷新属性文本和进度条。

非法移动、撞墙、目标格被占据、移动到 `Obstacle` 都不会触发属性变化，也不会消耗地块。

## 地块类型

枚举：`ETileType`

路径：`Source/Chessboard_Roguelike/Public/Grid/GridTypes.h`

| 值 | 含义 | 材质 CustomData |
| --- | --- | --- |
| `Minimal` | 默认地块，不修改属性 | `0.0` |
| `Construct` | 构成地块，进入后增加构成值 | `1.0` |
| `Acid` | 酸性地块，进入后增加酸性值 | `2.0` |
| `Obstacle` | 障碍地块，不可通行 | `3.0` |

说明：

- 枚举值显式固定，避免已有资产序列化后顺序变化。
- `PerInstanceCustomData[0]` 使用同一套数值映射，用于地块材质区分视觉。

## 地块数据

结构体：`FTileData`

路径：`Source/Chessboard_Roguelike/Public/Grid/GridTypes.h`

| 字段 | 类型 | 用途 |
| --- | --- | --- |
| `GridCoord` | `FIntPoint` | 地块逻辑坐标 |
| `TileType` | `ETileType` | 当前地块类型 |
| `OccupantType` | `EGridOccupantType` | 当前占据类型 |
| `OccupantActor` | `TWeakObjectPtr<AActor>` | 当前占据 Actor |
| `bWalkable` | `bool` | 是否可通行 |
| `bConvertible` | `bool` | 是否允许被地块效果转换 |

`bConvertible == false` 时，即使玩家进入 `Construct` 或 `Acid` 地块，也只修改属性，不会把地块消耗为 `Minimal`。

## GridSettings 可配置项

类：`UGridSettings`

路径：`Source/Chessboard_Roguelike/Public/Grid/GridSettings.h`

资产示例：`Content/Data/Grid/DA_GridSettings_Prototype.uasset`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `Width` | `int32` | 棋盘宽度 |
| `Height` | `int32` | 棋盘高度 |
| `TileSize` | `float` | 格子中心间距 |
| `Origin` | `FVector` | `(0,0)` 对应世界位置 |
| `TileMesh` | `UStaticMesh*` | 所有地块使用的网格体 |
| `InitialTileOverrides` | `TArray<FGridInitialTileOverride>` | 手动指定初始特殊地块 |

`FGridInitialTileOverride` 字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `GridCoord` | `FIntPoint` | 要覆盖的格子坐标 |
| `TileType` | `ETileType` | 初始地块类型 |
| `bWalkable` | `bool` | 是否可通行；`Obstacle` 会强制不可通行 |
| `bConvertible` | `bool` | 是否允许被消耗转换 |

用途示例：

- 添加一个 Construct 测试格：`GridCoord=(1,0)`，`TileType=Construct`，`bWalkable=true`，`bConvertible=true`。
- 添加一个 Acid 测试格：`GridCoord=(0,1)`，`TileType=Acid`，`bWalkable=true`，`bConvertible=true`。
- 添加障碍：`TileType=Obstacle`。

## 玩家属性组件

类：`UPlayerAttributeComponent`

路径：

- `Source/Chessboard_Roguelike/Public/Player/PlayerAttributeComponent.h`
- `Source/Chessboard_Roguelike/Private/Player/PlayerAttributeComponent.cpp`

默认挂载位置：`AGridPawn`

### 可读取或编辑的值

这些值在 C++ 中为私有字段，支持 Blueprint 读取；UI 不应直接修改这些值。

| 字段 | 默认值 | 说明 |
| --- | --- | --- |
| `ConstructValue` | `0` | 当前构成值 |
| `AcidValue` | `0` | 当前酸性值 |
| `MaxConstructValue` | `10` | 构成值上限 |
| `MaxAcidValue` | `10` | 酸性值上限 |

所有属性变化都会 Clamp 到 `[0, MaxValue]`。

### 蓝图可调用函数

| 函数 | 参数 | 返回值 | 说明 |
| --- | --- | --- | --- |
| `ApplyTileAttributeDelta` | `ConstructDelta`, `AcidDelta` | 无 | 同时修改构成值和酸性值，并 Clamp |
| `AddConstructValue` | `Delta` | 无 | 修改构成值 |
| `AddAcidValue` | `Delta` | 无 | 修改酸性值 |
| `GetConstructValue` | 无 | `int32` | 当前构成值 |
| `GetAcidValue` | 无 | `int32` | 当前酸性值 |
| `GetMaxConstructValue` | 无 | `int32` | 构成值上限 |
| `GetMaxAcidValue` | 无 | `int32` | 酸性值上限 |
| `GetConstructRatio` | 无 | `float` | `ConstructValue / MaxConstructValue` |
| `GetAcidRatio` | 无 | `float` | `AcidValue / MaxAcidValue` |
| `IsConstructValueMaxed` | 无 | `bool` | 构成值是否达到上限 |
| `IsAcidValueMaxed` | 无 | `bool` | 酸性值是否达到上限 |

### 事件

事件：`OnPlayerAttributeChanged`

参数：

- `NewConstructValue`
- `NewAcidValue`

触发条件：

- `ApplyTileAttributeDelta()` 执行后，最终 Clamp 后的任一属性值发生变化。

推荐用途：

- HUD 监听该事件刷新文本和进度条。
- 需要响应属性变化的视觉或音效逻辑监听该事件。

不推荐用途：

- 不要在 UI 中直接修改属性。
- 不要用 Tick 每帧轮询属性。

### 蓝图获取当前属性值

在任意蓝图中：

```text
Get Player Pawn
-> Get Component by Class (PlayerAttributeComponent)
-> Get Construct Value / Get Acid Value
```

在 `BP_GridPawn` 内部：

```text
PlayerAttributeComponent
-> Get Construct Value
-> Get Acid Value
```

## 地块效果解析组件

类：`UTileEffectResolverComponent`

路径：

- `Source/Chessboard_Roguelike/Public/Grid/TileEffectResolverComponent.h`
- `Source/Chessboard_Roguelike/Private/Grid/TileEffectResolverComponent.cpp`

默认挂载位置：`AGridPawn`

### 可配置字段

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `TileAttributeEffectConfig` | `UTileAttributeEffectConfig*` | 可选数值配置资产 |
| `GridManager` | `AGridManager*` | 当前网格管理器 |

`AGridPawn::InitializeOnGrid()` 会调用 `SetGridManager()`，把当前使用的 `AGridManager` 传给 Resolver。

### 蓝图可调用函数

| 函数 | 参数 | 说明 |
| --- | --- | --- |
| `ResolveTileEnterEffect` | `PlayerActor`, `TileCoord` | 结算玩家进入目标格后的地块效果 |
| `SetGridManager` | `InGridManager` | 显式设置当前 GridManager |

### 地块效果规则

| 进入地块 | Construct 变化 | Acid 变化 | 地块是否消耗 |
| --- | --- | --- | --- |
| `Construct` | `+1` | `-1` | 可转换时变为 `Minimal` |
| `Acid` | `-1` | `+1` | 可转换时变为 `Minimal` |
| `Minimal` | `0` | `0` | 不变 |
| `Obstacle` | `0` | `0` | 不变 |

如果绑定了 `TileAttributeEffectConfig`，实际变化值来自配置资产。

## 地块效果配置资产

类：`UTileAttributeEffectConfig`

路径：

- `Source/Chessboard_Roguelike/Public/Data/TileAttributeEffectConfig.h`
- `Source/Chessboard_Roguelike/Private/Data/TileAttributeEffectConfig.cpp`

资产：`Content/Data/DA_TileAttributeEffectConfig.uasset`

| 字段 | 默认值 | 说明 |
| --- | --- | --- |
| `ConstructTile_ConstructDelta` | `1` | 进入 Construct 时构成值变化 |
| `ConstructTile_AcidDelta` | `-1` | 进入 Construct 时酸性值变化 |
| `AcidTile_ConstructDelta` | `-1` | 进入 Acid 时构成值变化 |
| `AcidTile_AcidDelta` | `1` | 进入 Acid 时酸性值变化 |
| `MinAttributeValue` | `0` | 预留字段；当前 Clamp 由属性组件自身执行 |
| `MaxAttributeValue` | `10` | 预留字段；当前 Clamp 由属性组件自身执行 |
| `bConsumeTileAfterEnter` | `true` | 进入 Construct/Acid 后是否消耗地块 |
| `ConsumedTileResultType` | `Minimal` | 消耗后变成的地块类型 |

## GridManager 接口

类：`AGridManager`

路径：

- `Source/Chessboard_Roguelike/Public/Grid/GridManager.h`
- `Source/Chessboard_Roguelike/Private/Grid/GridManager.cpp`

### 关键变量

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `GridSettings` | `UGridSettings*` | 当前棋盘配置 |
| `TileISM` | `UInstancedStaticMeshComponent*` | 所有地块的实例化静态网格体组件 |
| `Tiles` | `TMap<FIntPoint, FTileData>` | 棋盘逻辑数据 |
| `TileInstanceIndices` | `TMap<FIntPoint, int32>` | 坐标到 ISM InstanceIndex 的运行时映射 |

`TileInstanceIndices` 是 `Transient`，运行时由 `GenerateGrid()` 重建。

### 蓝图可调用函数

| 函数 | 参数 | 返回值 | 说明 |
| --- | --- | --- | --- |
| `GenerateGrid` | 无 | 无 | 根据 `GridSettings` 重建棋盘数据和 ISM 实例 |
| `IsValidCoord` | `Coord` | `bool` | 坐标是否在棋盘内 |
| `IsWalkable` | `Coord` | `bool` | 坐标是否可通行 |
| `IsOccupied` | `Coord` | `bool` | 坐标是否被占据 |
| `GetTileData` | `Coord`, `OutTileData` | `bool` | 获取地块数据副本 |
| `SetTileType` | `Coord`, `NewTileType` | `bool` | 只修改地块类型，不修改占据状态 |
| `RefreshTileInstanceVisual` | `Coord`, `NewTileType` | 无 | 蓝图可重写的单格视觉刷新钩子 |
| `GetTileInstanceIndex` | `Coord`, `OutInstanceIndex` | `bool` | 获取坐标对应的 ISM 实例下标 |
| `IsTileConvertible` | `Coord` | `bool` | 是否允许被地块效果转换 |
| `GridToWorld` | `Coord` | `FVector` | 逻辑坐标转世界坐标 |
| `WorldToGrid` | `WorldLocation` | `FIntPoint` | 世界坐标转逻辑坐标 |
| `TryOccupyTile` | `Coord`, `Occupant`, `OccupantType` | `bool` | 初始占据某格 |
| `ClearOccupant` | `Coord` | 无 | 清空某格占据 |
| `RequestMove` | `Unit`, `FromCoord`, `ToCoord` | `bool` | 原子移动请求 |

### 地块类型变化事件

事件：`OnTileTypeChanged`

参数：

- `TileCoord`
- `NewTileType`

触发条件：

- `SetTileType()` 成功改变地块类型后广播。

推荐用途：

- 播放地块变化特效。
- 打印调试信息。
- 触发额外蓝图表现。

注意：

- `SetTileType()` 已经会在 C++ 中写入 `PerInstanceCustomData[0]`。
- 如果蓝图重写 `RefreshTileInstanceVisual()`，需要调用 `Call Parent Function`，否则只会执行蓝图里的额外逻辑。
- 不需要在 `OnTileTypeChanged` 中再次调用 `RefreshTileInstanceVisual()`；保留也可以，但不是必须。

## 地块材质刷新

当前系统使用一个 `TileISM` 渲染所有地块。不要用 `Set Material` 给单个格子换材质，因为 `Set Material` 会影响整个 ISM 组件。

推荐方式是使用一个主材质读取：

```text
PerInstanceCustomData[0]
```

代码写入位置：

- `GenerateGrid()`：创建地块时写入初始值。
- `SetTileType()`：地块类型变化时写入新值。
- `ApplyTileInstanceCustomData()`：实际执行 `SetCustomDataValue()`。

材质数值映射：

| CustomData[0] | 地块类型 |
| --- | --- |
| `0.0` | `Minimal` |
| `1.0` | `Construct` |
| `2.0` | `Acid` |
| `3.0` | `Obstacle` |

### 材质设置建议

1. 打开 `DA_GridSettings_Prototype`。
2. 找到 `TileMesh`。
3. 打开该 Static Mesh 资源。
4. 在 Static Mesh 的 `Material Slots` 中，把 `Element 0` 设置为读取 `PerInstanceCustomData[0]` 的主材质或材质实例。
5. 在材质中根据 `PerInstanceCustomData[0]` 选择颜色、贴图或效果。

如果 `PerInstanceCustomData[0]` 用于 `Base Color`、`Emissive Color` 等像素阶段输入，通常需要通过 `VertexInterpolator` 把值从顶点阶段传到像素阶段。

### 蓝图获取格子 InstanceIndex

在 `BP_GridManager` 中：

```text
Coord
-> Get Tile Instance Index
-> Branch(Return Value)
-> 使用 Out Instance Index
```

如果你手动在蓝图中设置 Custom Data：

```text
TileISM -> Set Custom Data Value
Instance Index = Out Instance Index
Custom Data Index = 0
Custom Data Value = 0/1/2/3
Mark Render State Dirty = true
```

正常情况下不需要手动设置；C++ 已经在地块生成和地块类型变化时自动写入。

## 属性 HUD

类：`UPlayerAttributeHUDWidget`

路径：

- `Source/Chessboard_Roguelike/Public/UI/PlayerAttributeHUDWidget.h`
- `Source/Chessboard_Roguelike/Private/UI/PlayerAttributeHUDWidget.cpp`

资产：`Content/UI/WBP_PlayerAttributeHUD.uasset`

### 可绑定控件名

| 控件名 | 类型 | 说明 |
| --- | --- | --- |
| `ConstructText` | `TextBlock` | 显示 `Construct: 当前值 / 最大值` |
| `AcidText` | `TextBlock` | 显示 `Acid: 当前值 / 最大值` |
| `ConstructProgressBar` | `ProgressBar` | 显示构成值比例 |
| `AcidProgressBar` | `ProgressBar` | 显示酸性值比例 |

### 蓝图可调用函数

| 函数 | 参数 | 说明 |
| --- | --- | --- |
| `InitializeFromAttributeComponent` | `InAttributeComponent` | 绑定属性组件并刷新 |
| `RefreshAttributeDisplay` | 无 | 手动刷新 HUD |

HUD 行为：

- `NativeConstruct()` 时尝试从 `GetOwningPlayerPawn()` 获取 `UPlayerAttributeComponent`。
- 监听 `OnPlayerAttributeChanged`。
- 属性变化时刷新文本和进度条。
- 不使用 Tick。
- 不写入玩家属性。

## 常见调试点

### 事件触发但材质不变

检查顺序：

1. Output Log 是否有 `ApplyTileInstanceCustomData failed`。
2. `TileInstanceIndices.Num()` 是否等于 `Tiles.Num()`。
3. `TileISM.NumCustomDataFloats` 是否至少为 `1`。
4. 材质是否真的挂在 `TileMesh` 的 `Element 0`。
5. 材质是否读取 `PerInstanceCustomData[0]`。
6. 如果用于像素阶段，是否经过 `VertexInterpolator`。
7. 蓝图是否重写了 `RefreshTileInstanceVisual()` 且没有 `Call Parent Function`。

### 属性变化但 HUD 不变

检查顺序：

1. Pawn 上是否有 `PlayerAttributeComponent`。
2. Controller 是否创建了 `WBP_PlayerAttributeHUD`。
3. HUD 是否成功绑定属性组件。
4. 是否监听了 `OnPlayerAttributeChanged`。
5. 属性是否真的发生变化；Clamp 后无变化时不会广播。

### Construct/Acid 只生效一次

这是预期行为。进入后地块会被消耗为 `Minimal`，再次进入时不会继续增加属性。

如果希望某个地块不被消耗：

```text
InitialTileOverrides
-> 对应格子
-> bConvertible = false
```

或者在 `DA_TileAttributeEffectConfig` 中设置：

```text
bConsumeTileAfterEnter = false
```

## 当前边界

本系统不处理：

- 攻击判定
- 敌人死亡
- 压制 AI
- 地块转换能量
- 3x3 地块转换
- PCG 地图生成
- 多房间系统

这些功能应在后续任务中单独实现，避免和地块属性结算逻辑耦合。
