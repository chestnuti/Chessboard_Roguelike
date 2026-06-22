# 变身棋子系统技术说明

## 设计目标

变身棋子系统提供一次性战术移动资源。玩家在默认状态下继续使用 WASD 四向移动；拾取变身棋子后，棋子进入 4 槽变身背包并可堆叠。玩家按住 G 打开轮盘，点击轮盘上的棋子后进入鼠标选格模式。选格模式中合法目标会高亮，鼠标移动到屏幕边缘会自动滚动相机。玩家左键点击合法格后执行一次变身移动，成功开始行动后消耗 1 个对应棋子；ESC 或右键短按取消，不消耗资源。

## C++ 类型

### 变身类型与目标

路径：

- `Source/Chessboard_Roguelike/Public/Transform/ChessTransformTypes.h`

主要类型：

| 类型 | 说明 |
| --- | --- |
| `EChessTransformType` | `Knight`、`Bishop`、`Rook`、`Queen` 和 `None`。 |
| `ETransformTargetType` | 目标格用途：`Move` 或 `Capture`。 |
| `FTransformMoveTarget` | 保存目标坐标和目标用途，供 UI、高亮和点击校验使用。 |

### 棋子形态配置

路径：

- `Source/Chessboard_Roguelike/Public/Data/ChessPieceFormData.h`
- `Source/Chessboard_Roguelike/Private/Data/ChessPieceFormData.cpp`

`UChessPieceFormData` 是 DataAsset，字段如下：

| 字段 | 说明 |
| --- | --- |
| `TransformType` | 背包消耗和轮盘槽位使用的形态类型。 |
| `DisplayName` | UI 显示名。 |
| `Icon` | 轮盘图标。 |
| `FormMesh` / `FormMaterial` | 执行变身移动时临时替换的玩家表现，可为空。 |
| `MoveDirections` | 可扫描方向或跳跃偏移。 |
| `MaxRange` | 非跳跃形态的最大扫描距离。 |
| `bCanJump` | 骑士类移动使用 true，可忽略中间格。 |
| `bCanCaptureEnemy` | 是否允许把敌人格作为合法目标。 |

推荐创建 4 个 DataAsset：

| 形态 | `MoveDirections` | `MaxRange` | `bCanJump` |
| --- | --- | ---: | --- |
| Knight | `(1,2),(2,1),(2,-1),(1,-2),(-1,-2),(-2,-1),(-2,1),(-1,2)` | 1 | true |
| Bishop | `(1,1),(1,-1),(-1,1),(-1,-1)` | 99 | false |
| Rook | `(1,0),(-1,0),(0,1),(0,-1)` | 99 | false |
| Queen | 战车 4 向 + 教皇 4 斜向 | 99 | false |

非跳跃形态会沿方向逐格扫描：空格加入移动目标；遇到首个敌人时，如果 `bCanCaptureEnemy=true`，加入捕获目标并停止该方向；遇到障碍、墙、其他占用或越界时停止该方向。

### 变身背包组件

路径：

- `Source/Chessboard_Roguelike/Public/Player/PlayerTransformInventoryComponent.h`
- `Source/Chessboard_Roguelike/Private/Player/PlayerTransformInventoryComponent.cpp`

`UPlayerTransformInventoryComponent` 挂在 `AGridPawn` 上，使用 `TMap<EChessTransformType, int32>` 保存堆叠数量。

主要 API：

| 函数 | 说明 |
| --- | --- |
| `AddTransformPiece(Type, Amount)` | 增加指定棋子数量。 |
| `ConsumeTransformPiece(Type, Amount)` | 成功消耗时返回 true。 |
| `GetTransformPieceCount(Type)` | 查询指定棋子数量。 |
| `CanConsumeTransformPiece(Type, Amount)` | 轮盘置灰和点击前校验使用。 |
| `GetTransformStacks()` | 返回当前库存拷贝。 |

事件：

| 事件 | 说明 |
| --- | --- |
| `OnTransformInventoryChanged` | 任意库存变化时广播。 |
| `OnTransformInventorySlotChanged` | 单个类型变化时广播类型和新数量。 |

### 变身拾取物

路径：

- `Source/Chessboard_Roguelike/Public/Pickup/GridTransformPiecePickupActor.h`
- `Source/Chessboard_Roguelike/Private/Pickup/GridTransformPiecePickupActor.cpp`

`AGridTransformPiecePickupActor` 继承现有 `AGridPickupActor`。字段：

| 字段 | 说明 |
| --- | --- |
| `TransformType` | 拾取后加入背包的棋子类型。 |
| `Amount` | 本次拾取增加数量，默认 1。 |

拾取流程沿用现有 `AGridPickupManager`：玩家进入道具格后调用 `ApplyPickupEffect()`，成功时把棋子加入 `TransformInventoryComponent` 并销毁拾取物。

### 玩家 Pawn 接口

路径：

- `Source/Chessboard_Roguelike/Public/Player/GridPawn.h`
- `Source/Chessboard_Roguelike/Private/Player/GridPawn.cpp`

新增组件：

| 字段 | 说明 |
| --- | --- |
| `TransformInventoryComponent` | 玩家变身棋子背包。 |

新增 API：

| 函数 | 说明 |
| --- | --- |
| `BuildLegalTransformTargets(FormData)` | 根据当前坐标、棋子配置和棋盘状态生成合法目标。 |
| `TryTransformMoveToCoord(TargetCoord, FormData)` | 校验目标后执行一次变身移动。 |

默认 `TryMove(Direction)` 仍然只接受曼哈顿距离为 1 的四向移动。变身移动和默认移动共享 `TryResolveMoveOrAttackToCoord()`，因此行动成功后仍会执行：

1. `GridManager::RequestMove()` 或敌人攻击结算。
2. 地块进入效果。
3. 拾取物结算。
4. `TurnManager->AddStep()`。
5. 敌人回合推进。

如果 `FormMesh` 或 `FormMaterial` 已配置，玩家会在轮盘选择棋子成功并进入目标选择时立即临时替换外观。玩家取消目标选择时会立刻恢复默认外观；玩家确认目标并成功开始变身移动后，会在移动表现结束时恢复默认外观。

变身外观状态也会通知蓝图：

| 接口 | 说明 |
| --- | --- |
| `bIsTransformVisualActive` | 当前是否处于变身外观状态。 |
| `ActiveTransformVisualForm` | 当前正在显示的变身形态 DataAsset。 |
| `OnTransformVisualStateChanged(FormData, bIsTransformed)` | 动态委托；`true` 表示进入变身外观，`false` 表示恢复默认外观。 |
| `OnTransformVisualApplied(FormData)` | BlueprintImplementableEvent，进入变身外观后触发。 |
| `OnTransformVisualRestored(FormData)` | BlueprintImplementableEvent，恢复默认外观后触发。 |

如果只需要简单模型切换，直接在对应 `UChessPieceFormData` 配置 `FormMesh` / `FormMaterial` 即可。如果需要切换多个组件、播放动画、生成特效或替换棋子 Actor，建议在 `BP_GridPawn` 中实现 `OnTransformVisualApplied` 和 `OnTransformVisualRestored`，或绑定 `OnTransformVisualStateChanged`。`ApplyTransformVisual(FormData)` 和 `RestoreDefaultPawnVisual()` 也暴露给蓝图，便于特殊流程手动控制外观状态。

## 轮盘 UI 与输入状态机

### Widget 基类

路径：

- `Source/Chessboard_Roguelike/Public/UI/TransformWheelWidget.h`
- `Source/Chessboard_Roguelike/Private/UI/TransformWheelWidget.cpp`

`UTransformWheelWidget` 是轮盘 WBP 的 C++ 基类。蓝图建议继承它；如果 WBP 中存在指定名称的控件，C++ 会自动绑定按钮、图标、名称和数量。如果 WBP 为空，C++ 会生成一套仅用于调试的默认文字按钮。

| 函数 / 事件 | 说明 |
| --- | --- |
| `InitializeWheel(Controller)` | Controller 创建 Widget 后调用。 |
| `RefreshFromInventory(Inventory)` | 打开轮盘时刷新 4 个槽位数量。 |
| `RequestSelectTransform(FormData)` | 槽位点击时调用，转发到 Controller。 |
| `OnWheelInitialized` | 蓝图初始化事件。 |
| `OnInventoryRefreshed` | 蓝图刷新槽位显示事件。 |

推荐 WBP 树和控件命名：

```text
WBP_TransformWheel
  Root Canvas
    WheelPanel
      Slot_Knight        (Button)
        Icon_Knight      (Image)
        NameText_Knight  (TextBlock, optional)
        CountText_Knight (TextBlock)
      Slot_Bishop        (Button)
        Icon_Bishop
        NameText_Bishop
        CountText_Bishop
      Slot_Rook          (Button)
        Icon_Rook
        NameText_Rook
        CountText_Rook
      Slot_Queen         (Button)
        Icon_Queen
        NameText_Queen
        CountText_Queen
```

槽位规则：

- 库存大于 0：正常亮度，可点击。
- 库存等于 0：置灰，不可点击。
- 点击槽位只进入目标选择，不消耗资源。
- `Icon_*` 会自动读取对应 `UChessPieceFormData.Icon`。
- `NameText_*` 会自动显示 `UChessPieceFormData.DisplayName`，可不放。
- `CountText_*` 会自动显示对应库存数量。
- `Image` 建议设置为 `Hit Test Invisible`，避免挡住按钮点击。
- 这些控件在 C++ 中使用 `BindWidgetOptional + BlueprintReadOnly` 暴露。修改 C++ 后需要重新编译并重开或刷新 `WBP_TransformWheel`，才能在事件图表的继承变量中看到它们。
- 点击轮盘槽位调用变身请求后，会在 `TransformWheelOpenSuppressDurationAfterSelectionRequest` 时间内暂时忽略 G 键打开输入，避免 UI 点击和输入焦点变化造成轮盘闪回。

### Controller 状态机

路径：

- `Source/Chessboard_Roguelike/Public/Player/GridPlayerController.h`
- `Source/Chessboard_Roguelike/Private/Player/GridPlayerController.cpp`

状态：

| 状态 | 说明 |
| --- | --- |
| `DefaultWASD` | 默认状态，WASD 移动有效。 |
| `TransformWheel` | G 键按住时显示轮盘，WASD 禁用。 |
| `TransformTargeting` | 已选择棋子，显示合法目标，鼠标选格，边缘滚屏启用。 |

新增输入动作字段：

| 字段 | 建议绑定 |
| --- | --- |
| `OpenTransformWheelAction` | G，Started 打开，Completed/Canceled 关闭。 |
| `TransformLeftClickAction` | 左键，目标选择阶段点击合法格。 |
| `TransformCancelAction` | ESC，取消轮盘或目标选择。 |
| `TransformRightMouseAction` | 右键，短按取消，拖动不取消。 |

状态流：

```text
DefaultWASD
  G Started -> TransformWheel

TransformWheel
  点击有库存槽位 -> TransformTargeting
  G Released / ESC -> DefaultWASD

TransformTargeting
  左键点击合法格 -> 执行变身移动 -> 消耗库存 -> DefaultWASD
  ESC / 右键短按 -> 取消 -> DefaultWASD
  右键拖动 -> 相机拖动，不取消
```

取消不会消耗棋子。消耗发生在 `TryTransformMoveToCoord()` 成功开始行动之后。

## 边缘滚屏

边缘滚屏只在 `TransformTargeting` 状态启用。Controller 每帧读取鼠标屏幕位置：

- 靠近左边缘：相机向棋盘左侧移动。
- 靠近右边缘：相机向棋盘右侧移动。
- 靠近上边缘：相机向棋盘上方移动。
- 靠近下边缘：相机向棋盘下方移动。

相关参数位于 `AGridPlayerController`：

| 参数 | 说明 |
| --- | --- |
| `EdgeScrollZoneSize` | 屏幕边缘触发宽度，默认 32 px。 |
| `EdgeScrollSpeed` | 滚屏速度，默认 1200。 |
| `EdgeScrollGridPaddingTiles` | 相机可超出棋盘边界的格数，默认 2。 |

相机平移入口位于 `UCombatCameraDirectorComponent::PanCameraByScreenDirection()`。它把屏幕方向转换成 SpringArm 的水平 `RightVector` / `ForwardVector`，并根据 `AGridManager` 的棋盘范围夹取 `TargetOffset`，避免镜头滚出棋盘过远。

进入 `TransformTargeting` 时，Controller 会调用 `BeginTransformTargetingCameraSession()` 记录当前 SpringArm `TargetOffset`。取消目标选择会立即调用 `RestoreTransformTargetingCameraSession()`。成功执行变身移动时，恢复会延迟到 `AGridPawn::bIsMoving == false` 之后再执行，避免逻辑坐标已到目标格但视觉移动仍在起点时错误计算相机中心。

如果蓝图中已有右键拖动相机逻辑，可以在拖动开始 / 结束时分别调用：

```text
AGridPlayerController::NotifyTransformCameraDragStarted()
AGridPlayerController::NotifyTransformCameraDragEnded()
```

这样 C++ 目标选择会在拖动期间暂停边缘滚屏和左键确认。

## 材质悬停目标格

变身目标选择阶段，`AGridPlayerController` 会把鼠标当前指向的合法目标格写入 Material Parameter Collection。默认优先使用 Controller 的 `MouseHoverGridParameterCollection`；如果未配置，则自动复用 Pawn 的 `PlayerGridParameterCollection`，也就是当前项目中的 `Content/Materials/MPC/MPC_PlayerPosition.uasset`。

需要在 MPC 中保留以下 Scalar 参数：

| 参数 | 含义 |
| --- | --- |
| `MouseHoverGridX` | 鼠标当前指向目标格的逻辑 X 坐标 |
| `MouseHoverGridY` | 鼠标当前指向目标格的逻辑 Y 坐标 |
| `bHasMouseHoverGrid` | 当前是否有有效悬停目标，`1.0` 表示有效，`0.0` 表示无效 |

Controller 只在 `TransformTargeting` 状态写入有效悬停值。离开目标选择、取消变身、完成点击移动、右键拖动相机或鼠标不在合法目标格上时，`bHasMouseHoverGrid` 会被写为 `0.0`。默认 `bMouseHoverRequiresPendingTransformTarget = true`，因此只有当前变身形态的合法目标格才会触发悬停高亮；如需材质读取任意鼠标所在格，可在 `BP_GridPlayerController` 中关闭该选项。

棋盘材质中推荐逻辑：

```text
TileX = PerInstanceCustomData[1]
TileY = PerInstanceCustomData[2]
CanMoveOrCapture = PerInstanceCustomData[3]

HoverX = MPC.MouseHoverGridX
HoverY = MPC.MouseHoverGridY
HasHover = MPC.bHasMouseHoverGrid

IsHoverTile =
  HasHover
  * Equal(TileX, HoverX)
  * Equal(TileY, HoverY)
```

如果只想让合法目标格显示悬停效果，可再乘以 `CanMoveOrCapture`。如果参数接入 `Base Color`、`Emissive Color` 或 `Opacity` 等像素阶段输入，`PerInstanceCustomData` 通常需要先经过 `VertexInterpolator`。

## 蓝图接线清单

1. 创建 4 个 `UChessPieceFormData` DataAsset：Knight、Bishop、Rook、Queen。
2. 创建 `BP_TransformPiecePickup_*`，父类选择 `AGridTransformPiecePickupActor`，配置 `TransformType` 和表现 Mesh。
3. 在 `DungeonGenerationSettings.PickupSpawnPool` 中加入变身拾取物蓝图。
4. 创建 `WBP_TransformWheel`，父类选择 `UTransformWheelWidget`。
5. 在 `BP_GridPlayerController` 中设置：
   - `TransformWheelWidgetClass = WBP_TransformWheel`
   - `OpenTransformWheelAction`
   - `TransformLeftClickAction`
   - `TransformCancelAction`
   - `TransformRightMouseAction`
   - 可选：`MouseHoverGridParameterCollection = MPC_PlayerPosition`；未设置时会自动复用 Pawn 的 `PlayerGridParameterCollection`
6. 在 `WBP_TransformWheel` 中按推荐名称创建 4 个槽位按钮、图标和数量文本。C++ 会自动按 `BP_GridPlayerController.TransformWheelForms` 的顺序绑定到 Knight、Bishop、Rook、Queen。
7. 在 4 个 `UChessPieceFormData` 中配置 `Icon`，轮盘会自动刷新按钮图标、数量和可点击状态。

## 调试建议

- 轮盘打不开：检查 `OpenTransformWheelAction` 是否加入当前 Mapping Context，`TransformWheelWidgetClass` 是否配置。
- 槽位点击无效：检查对应库存是否大于 0，FormData 的 `TransformType` 是否与库存类型一致。
- 没有合法目标：检查 FormData 的 `MoveDirections`、`MaxRange`、`bCanJump`、`bCanCaptureEnemy`。
- 鼠标点击格子不准：当前使用鼠标射线与玩家所在棋盘平面求交，不依赖地块碰撞；检查相机是否过于水平，或棋盘是否不在统一 Z 平面。
- 目标选择时镜头不滚动：检查 `EdgeScrollZoneSize`、`EdgeScrollSpeed`，以及是否处于 `TransformTargeting` 状态。
- 右键拖动误取消：增大 `RightMouseDragCancelThreshold`，或在蓝图拖动逻辑中调用拖动通知函数。

## Combat Preview 接入

变身目标选择阶段会复用战斗预览系统。进入 `TransformTargeting` 后，`AGridPlayerController` 会把 `PendingTransformTargets` 中的 `Capture` 目标交给 `AGridPawn::RefreshTransformCombatPreview()`：

```cpp
GridPawn->RefreshTransformCombatPreview(PendingTransformTargets, GridPawn->CurrentGridCoord);
```

当鼠标悬停在合法目标格上时，Controller 会把悬停格作为 `PreviewPlayerCoord` 再次刷新：

```cpp
GridPawn->RefreshTransformCombatPreview(PendingTransformTargets, HoverCoord);
```

这样敌人蓝图可以同时显示：

- 该敌人是否会被当前变身形态击杀。
- 如果玩家移动到悬停格，该敌人下一回合是否可以攻击玩家。

敌人表现层通过 `ICombatPreviewReceiver::UpdateCombatPreview(FCombatPreviewState)` 接收状态。完整接口说明见 [Combat Preview 系统说明](../03_CombatAndEnemies/CombatPreview_SystemGuide.md)。
