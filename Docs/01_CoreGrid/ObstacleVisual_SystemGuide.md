# Obstacle 视觉系统技术说明

本文档说明 Obstacle 地块的运行时视觉生成方案，包括 C++ 类职责、生成过滤规则、墙面邻接规则、`PerInstanceCustomData` 写入规则和材质接入约定。

## 目标

Obstacle 的逻辑阻挡仍然由 `AGridManager::Tiles` 管理。视觉系统只负责根据现有棋盘数据生成表现层，包括：

- 随时间做波浪趋势上下浮动的硬立方体。
- 根据相邻地块类型生成墙面构成主义涂鸦或全息 Glitch 投影。
- 只为玩家可能看到或接近的 Obstacle 生成视觉实例，避免大面积不可达墙体拖慢性能。

## 关键 C++ 类型

| 类型 | 路径 | 职责 |
| --- | --- | --- |
| `AGridManager` | `Source/Chessboard_Roguelike/Public/Grid/GridManager.h` | 棋盘逻辑真源，持有 `Tiles`、`TileISM`、`ObstacleCubeISM`、`ObstacleFaceISM` 和 `ObstacleVisualComponent`。 |
| `UGridObstacleVisualComponent` | `Source/Chessboard_Roguelike/Public/Grid/GridObstacleVisualComponent.h` | Obstacle 视觉生成算法组件。它不再创建 SceneComponent 子组件，只操作 `AGridManager` 持有的 ISM。 |
| `UObstacleVisualConfig` | `Source/Chessboard_Roguelike/Public/Grid/ObstacleVisualConfig.h` | Obstacle 视觉配置 DataAsset，保存 Mesh、材质、波浪参数、墙面生成开关和性能过滤半径。 |

当前组件层级为：

```text
AGridManager
├─ SceneRoot
├─ TileISM
├─ ObstacleCubeISM
├─ ObstacleFaceISM
└─ ObstacleVisualComponent
```

`ObstacleCubeISM` 和 `ObstacleFaceISM` 直接由 `AGridManager` 创建，避免在 `UGridObstacleVisualComponent` 内部再创建 SceneComponent 子组件，从而规避 UE 默认子对象模板和实例组件混合 attach 时的 `Template Mismatch during attachment` ensure。

## 生成入口

Obstacle 视觉在以下路径中重建：

- `AGridManager::GenerateGrid()`
- `AGridManager::ApplyTileLayout(...)`
- `AGridManager::SetTileType(...)`
- `AGridManager::SetTileBlockingType(...)`

`SetTileType()` 只修改地块类型，不同步阻挡状态。运行时创建或移除 Obstacle 时应优先使用 `SetTileBlockingType()`，它会同步：

- `TileType`
- `bWalkable`
- `bConvertible`
- `OccupantType`
- `OccupantActor`
- Tile 和 Obstacle 视觉实例

## 性能过滤规则

`UGridObstacleVisualComponent::RebuildFromGrid()` 会遍历所有 `ETileType::Obstacle` 地块，但并不是每个 Obstacle 都生成视觉实例。

只有当 Obstacle 周围曼哈顿距离 `NearbyWalkableTileManhattanRadius` 范围内存在可移动地块时，才生成方块和墙面。

默认半径：

```cpp
NearbyWalkableTileManhattanRadius = 3;
```

可移动地块判断：

```cpp
CandidateTile->bWalkable && CandidateTile->TileType != ETileType::Obstacle
```

该规则只检查地形可通行性，不检查玩家、敌人等临时占位。这样可以让视觉生成结果稳定，不随单位移动频繁重建。

扫描范围是曼哈顿菱形，例如半径 3 会检查：

```text
      3
    2 3 2
  1 2 3 2 1
0 1 2 X 2 1 0
  1 2 3 2 1
    2 3 2
      3
```

其中 `X` 是当前 Obstacle，数字表示到中心的曼哈顿距离。

## 墙面生成规则

每个可生成视觉的 Obstacle 会检查 4 邻域：

```cpp
( 1,  0)
(-1,  0)
( 0,  1)
( 0, -1)
```

墙面规则：

| 邻居地块 | 墙面结果 |
| --- | --- |
| `Construct` | 生成 `ConstructGraffiti` 墙面。 |
| `Acid` | 生成 `HologramGlitch` 墙面。 |
| `Minimal` | 生成 `NeutralWarning` 墙面，前提是 `bGenerateNeutralFaces = true`。 |
| `Obstacle` | 不生成内部墙面。 |
| 邻居不存在 | 生成 `NeutralWarning` 边界墙面，前提是 `bGenerateBoundaryFaces = true`。 |

墙面的位置基于当前 Obstacle 格子中心、格子尺寸和方向计算。墙面不参与碰撞，碰撞和通行仍由棋盘逻辑层判断。

## ObstacleCubeISM Custom Data

`ObstacleCubeISM` 使用 3 个 `PerInstanceCustomData` float：

| Index | 含义 | 说明 |
| --- | --- | --- |
| `0` | `CoordX` | 当前 Obstacle 的棋盘 X 坐标。 |
| `1` | `CoordY` | 当前 Obstacle 的棋盘 Y 坐标。 |
| `2` | `StablePhase` | 根据格子坐标 hash 得到的稳定相位，范围约为 `0..2π`。 |

材质可使用 `CoordX`、`CoordY` 和 `StablePhase` 做波浪相位、颜色扰动、噪声偏移或局部 Glitch 动画。

## ObstacleFaceISM Custom Data

`ObstacleFaceISM` 使用 6 个 `PerInstanceCustomData` float：

| Index | 含义 | 说明 |
| --- | --- | --- |
| `0` | `FaceStyle` | 墙面视觉类型。 |
| `1` | `CoordX` | 当前 Obstacle 的棋盘 X 坐标。 |
| `2` | `CoordY` | 当前 Obstacle 的棋盘 Y 坐标。 |
| `3` | `DirectionX` | 墙面朝向 X。 |
| `4` | `DirectionY` | 墙面朝向 Y。 |
| `5` | `StablePhase` | 与当前 Obstacle 坐标绑定的稳定动画相位。 |

`FaceStyle` 映射：

| 数值 | 枚举 | 用途 |
| --- | --- | --- |
| `0` | `None` | 不应生成该面，材质侧通常不用处理。 |
| `1` | `ConstructGraffiti` | 构成主义涂鸦、红黑米色块面、警示形状。 |
| `2` | `HologramGlitch` | 全息 Glitch、扫描线、霓虹投影。 |
| `3` | `NeutralWarning` | 普通边界警示、编号、弱装饰面。 |

方向映射：

| 墙面方向 | `DirectionX` | `DirectionY` |
| --- | ---: | ---: |
| 右侧面 | `1` | `0` |
| 左侧面 | `-1` | `0` |
| 上侧面 | `0` | `1` |
| 下侧面 | `0` | `-1` |

注意：`CoordX` 和 `CoordY` 是当前 Obstacle 的坐标，不是邻居地块坐标。

## 动态材质参数

`UGridObstacleVisualComponent` 会向 Cube 和 Face 的动态材质实例写入以下 scalar 参数：

| 参数 | 来源 | 用途 |
| --- | --- | --- |
| `WaveAmplitude` | `VisualConfig.WaveSettings.Amplitude` | 上下浮动幅度。 |
| `WaveSpeed` | `VisualConfig.WaveSettings.Speed` | 动画速度。 |
| `WaveX` | `VisualConfig.WaveSettings.WaveX` | X 坐标相位影响。 |
| `WaveY` | `VisualConfig.WaveSettings.WaveY` | Y 坐标相位影响。 |
| `ObstacleHeight` | `VisualConfig.ObstacleHeight` | Obstacle 视觉高度。 |

材质中的典型波浪公式：

```text
WaveZ = sin(Time * WaveSpeed + CoordX * WaveX + CoordY * WaveY + StablePhase) * WaveAmplitude
```

如果材质需要在 Pixel Shader 阶段读取 `PerInstanceCustomData`，建议先通过 `VertexInterpolator` 从顶点阶段传递到像素阶段。

## DataAsset 配置

推荐创建或维护：

```text
Content/Data/Grid/DA_ObstacleVisualConfig.uasset
```

常用字段：

| 字段 | 说明 |
| --- | --- |
| `ObstacleCubeMesh` | Obstacle 方块实例使用的 Static Mesh。 |
| `FacePlaneMesh` | 墙面投影或涂鸦使用的 Plane Mesh。 |
| `CubeMaterial` | 方块材质。 |
| `FaceMaterial` | 墙面材质。 |
| `ObstacleHeight` | 方块和墙面高度。 |
| `FaceSurfaceOffset` | 墙面离方块外表面的偏移，避免 Z-fighting。 |
| `bEnableObstacleCollision` | 是否给视觉方块启用碰撞。玩法阻挡不依赖该碰撞。 |
| `bGenerateBoundaryFaces` | 是否为地图边界生成中性墙面。 |
| `bGenerateNeutralFaces` | 是否为 `Minimal` 邻居生成中性墙面。 |
| `NearbyWalkableTileManhattanRadius` | 可移动地块邻近过滤半径，默认 `3`。 |
| `WaveSettings` | 浮动幅度、速度和坐标相位参数。 |

## 蓝图接入注意事项

`BP_GridManager` 中应保留以下 inherited 组件：

- `TileISM`
- `ObstacleCubeISM`
- `ObstacleFaceISM`
- `ObstacleVisualComponent`

如果之前使用 Live Coding 或 Hot Reload 后保存过蓝图，Components 面板中可能残留旧的 Obstacle 子组件。若出现重复组件或启动时 `SceneComponent.cpp` 的 `Template Mismatch during attachment` ensure：

1. 打开 `BP_GridManager`。
2. 检查 Components 面板。
3. 删除手动添加或残留的旧 Obstacle 组件。
4. 保留 C++ inherited 的 `ObstacleCubeISM`、`ObstacleFaceISM` 和 `ObstacleVisualComponent`。
5. Compile 并 Save 蓝图。
6. 关闭并重新打开编辑器，确认 ensure 不再触发。

## 调试建议

- 如果完全不生成 Obstacle 视觉，先检查 `ObstacleVisualComponent.VisualConfig` 是否已设置。
- 如果只生成部分 Obstacle，检查 `NearbyWalkableTileManhattanRadius` 是否过小，以及附近是否存在 `bWalkable=true` 的非 Obstacle 地块。
- 如果墙面样式不对，检查相邻地块的 `TileType`，墙面规则只看当前 Obstacle 的 4 邻域。
- 如果材质无法读取 custom data，确认 ISM 的 `NumCustomDataFloats` 分别为 Cube `3`、Face `6`。
- 如果运行时把地块变成 Obstacle 后仍可移动，确认调用的是 `SetTileBlockingType()`，而不是只改类型的 `SetTileType()`。
