# 构成 / 酸性棋盘式肉鸽战术游戏
## 技术需求与实现文档

版本：0.4
来源：基于 [GameDesignDoc_完整.md](GameDesignDoc_完整.md)
用途：技术策划拆解、程序开发对齐、原型验证与迭代评审

---

## 1. 文档目标

本文档将 GDD 中的玩法概念拆解为可实现、可验证、可调参的技术需求，并给出推荐实现方案。目标是让程序、美术、关卡与策划在同一套规则下协作，避免“设计可读但实现不可落地”的问题。

### 1.1 核心交付物

- 明确游戏的系统边界与模块划分
- 明确地图、回合、角色、敌人、战利品、PCG 的实现规则
- 明确关键数据结构、事件流与状态流转
- 明确需要参数化调优的数值项
- 明确原型阶段的最小可玩范围与验证重点

### 1.2 非目标

- 不展开完整剧情与世界观脚本
- 不定义最终美术资产制作规范
- 不定义上线运营与商业化系统
- 不定义完整数值平衡，只保留可配置项与测试方向

---

## 2. 需求总览

### 2.1 游戏类型

- 步进式回合制战术游戏
- Roguelite 单局驱动
- 网格地图策略解谜

### 2.2 核心玩法目标

- 玩家通过移动、攻击和地形重塑进行策略决策
- 构成值与酸性值形成双资源博弈
- 敌人类型与地块类型形成明确相克关系
- 地图本身既是战场，也是资源系统

### 2.3 技术实现目标

- 支持种子驱动的可重复生成
- 支持单步驱动的回合推进
- 支持双属性伤害、单次判定和免疫规则
- 支持地块转换能量的获取与消耗
- 支持敌人 AI、远程蓄力与友伤判定

---

## 3. 系统架构建议

### 3.1 推荐模块划分

- `GameFlow`：对局状态机、回合推进、胜负判定
- `MapSystem`：网格地图、地块状态、占据状态、地形转换
- `PCGSystem`：种子生成、分区、地块分布、可达性修正
- `PlayerSystem`：移动、攻击、属性值、单个能量槽
- `TransformSystem`：变身棋子拾取、背包堆叠、轮盘选择、特殊移动、形态视觉切换
- `EnemySystem`：敌人数据、AI 行为、伤害判定、死亡逻辑
- `CombatSystem`：攻击结算、免疫、压制、友伤、退回规则
- `UISystem`：HUD、提示、能量显示、状态反馈
- `VFXSFXSystem`：移动、攻击、压制、地块转换表现

### 3.2 数据驱动原则

建议所有数值和配置项尽量采用 DataAsset、DataTable 或可序列化配置文件承载，避免硬编码。核心原因是本项目规则较多且平衡项密集，必须支持快速调参。

---

## 4. 核心游戏循环实现

### 4.1 单步循环

一次玩家输入构成一个“步”，其标准执行顺序如下：

1. 校验玩家输入是否合法
2. 执行玩家默认移动、近战攻击或已确认的变身特殊移动
3. 结算玩家本步产生的地块/资源变化
4. 触发敌人逐个行动
5. 结算敌人移动、攻击、远程蓄力与死亡
6. 结算地图状态更新与持续效果
7. 若跨越房间边界，则执行房间激活与场景切换结算
8. 检查胜负条件
9. 刷新 UI 和反馈

### 4.2 状态机建议

建议对局使用以下状态：

- `Initializing`：初始化地图和角色
- `PlayerInput`：等待玩家输入
- `PlayerActionResolve`：执行玩家动作
- `EnemyTurnResolve`：执行敌人动作
- `MapResolve`：地图状态结算
- `CheckEndCondition`：检查胜负
- `Victory`：胜利结算
- `Defeat`：失败结算

### 4.3 关键技术要求

- 同一步中禁止重复触发输入
- 所有行动必须可回放、可追踪
- 每个结算环节必须具备明确前后状态
- 单步内的视觉表现可以延迟，但逻辑结算必须确定

---

## 5. 地图与 PCG 技术需求

### 5.1 地图基础数据

每个格子至少包含以下字段：

- 坐标 `X/Y`
- 地块类型：构成主义、酸性、极简主义
- 占据状态：空、玩家、敌人、障碍物
- 可选环境状态：陷阱、增益、阻挡、持续效果
- 是否可通行
- 是否可被转换

### 5.2 地块状态规则

- 构成主义地块：玩家进入时增加构成值，并减少酸性值
- 酸性地块：玩家进入时增加酸性值，并减少构成值
- 极简主义地块：不提供属性变化
- 地块被转换后应保留基础可通行规则，除非未来扩展特殊地块

### 5.3 PCG 生成流程

推荐生成流程如下：

1. 根据种子生成基础随机噪声
2. 生成地图分区或房间结构
3. 通过 Cellular Automata 或平滑规则修整地块分布
4. 进行起点安全区清理
5. 摆放敌人、障碍和可选事件
6. 使用 Flood Fill 或寻路校验可达性
7. 若不可达则局部修正或重生成

### 5.4 生成约束

- 起点周围必须有安全可行动区域
- 必须保证存在至少一条到敌人的可达路径
- 构成与酸性地块都应有有效分布，不可单边缺失
- 敌人分布应分层，不应在出生点附近过度堆叠
- 地图生成结果必须支持种子复现

### 5.5 PCG 验证指标

- 可达率
- 重生成次数
- 起点安全率
- 两类地块覆盖率
- 平均敌人可接触步数

### 5.6 当前工程实现：L-System 地牢生成管线

当前工程已经完成第一版可运行的 PCG 地图生成与运行时接入，重点是把“生成算法”“棋盘运行时数据”和“关卡初始化流程”分开：

- `AGridManager` 仍然是运行时棋盘权威数据源，但不再只支持固定矩形默认生成。
- `AGridManager::ApplyTileLayout(TileLayout, LayoutWidth, LayoutHeight)` 提供统一入口，用于把程序生成的布局写入 `Tiles` 并重建 ISM 视觉实例。
- `TileISM` 为每个格子实例写入 4 个 `PerInstanceCustomData`：`[0] = TileType`、`[1] = GridCoord.X`、`[2] = GridCoord.Y`、`[3] = bPlayerCanMoveNext`。棋盘材质应通过该通道读取地块类型、逻辑坐标和玩家下一步可移动格标记，避免用世界坐标反推格子位置。
- `FGridTileLayoutData` 是 PCG 到 GridManager 的轻量数据载体，生成器应输出该结构数组，而不是直接修改 `Tiles`。
- `EGridCellRole` 用于描述地图形态语义，包括 `Open`、`Room`、`Corridor`、`Wall`、`Start`、`Exit`。它与 `ETileType` 分离，避免把房间/走廊/墙体语义混入构成、酸性、极简、障碍等地块属性。
- `FTileData` 新增 `CellRole`、`RegionId`、`Depth`，用于记录房间节点、区域层级和后续房间激活/难度分层。
- `UDungeonGenerationSettings` 是独立的地牢生成 DataAsset，承载 Seed、地图尺寸、L-System axiom/rules、迭代次数、房间半径、边界噪声、房间间隔、走廊宽度、崎岖概率、地块属性概率、出生候选数量、最大生成尝试次数和起点安全半径。
- `ULSystemDungeonGenerator::GenerateDungeonLayout()` 负责根据配置生成 `FGeneratedDungeonLayout`，其中包含格子布局、房间节点、连接边、起点、出口和出生候选点。
- `ULSystemDungeonGenerator::ValidateConnectivity()` 使用 Flood Fill 校验所有房间中心从起点可达。
- `ADungeonRunManager::GenerateAndInitializeRun()` 是运行时接入口，执行“生成布局 -> 应用到 GridManager -> 初始化玩家 -> 可选生成敌人 -> 可选生成拾取物 -> 初始化 EnemyManager -> 设置 PlayerInput”。

当前阶段不依赖 UE PCG 插件。原因是本项目的首要需求是回合制逻辑格、可达性保证和种子复现；UE PCG Graph 更适合后续生成墙体 Mesh、装饰和场景资产散布。

#### 5.6.1 L-System 符号约定

当前生成器支持以下符号：

- `F`：向当前方向前进，并创建下一个房间节点。
- `B`：创建分支房间节点，语义上等价于一次前进建房，保留给后续分支权重调参。
- `E`：创建出口候选房间节点，当前最终出口仍选择深度最高房间。
- `R`：保留当前房间节点，不前进；用于明确 axiom 中的房间语义。
- `+`：向右旋转当前方向。
- `-`：向左旋转当前方向。
- `[`：保存当前 turtle 状态。
- `]`：恢复最近保存的 turtle 状态。

#### 5.6.2 生成阶段

`ULSystemDungeonGenerator` 当前按以下阶段生成：

1. `BuildRoomGraph`：展开 L-System 字符串，生成房间节点和连接关系。新房间会按半径、`BoundaryNoise` 和 `RoomSeparation` 检查与已有房间的中心距离；如果多次尝试后仍找不到非重叠位置，该节点会被跳过。
2. `RasterizeLayout`：把房间节点雕刻为不规则房间，把连接关系雕刻为崎岖走廊。房间格发生局部重叠时，Tile 优先保留较低 `Depth` 的房间归属；走廊不会覆盖已有房间、起点或出口的 `RegionId` 和 `Depth`。
3. `AssignTileTypes`：在可通行格上分配 `Minimal`、`Construct`、`Acid`，墙体固定为 `Obstacle`。
4. `BuildSpawnCandidates`：先从起点执行 Flood Fill 得到可达格集合，再生成敌人、事件、奖励候选点；候选点必须可达，并避开起点安全区、墙、出口中心和不可通行格。
5. `ValidateConnectivity`：从起点 Flood Fill，确保所有房间中心可达。

生成器失败时会按 `MaxGenerationAttempts` 递增尝试种子重生成；所有随机路径必须使用 `FRandomStream`，禁止使用全局随机函数。

该深度归属规则用于避免抽象 L-System 深层房间在边界夹取或分支回绕后覆盖近起点房间，导致同一个视觉房间内出现高低混杂的 `Depth`。

#### 5.6.3 运行时接入

`ADungeonRunManager` 用于在关卡中串联 PCG 与现有棋盘玩法：

- `DungeonGenerationSettings`：地牢生成配置 DataAsset。
- `GenerationMode`：运行生成模式。`Procedural` 使用 PCG；`TutorialFixed` 跳过 PCG，使用固定教学关卡数据。
- `TutorialLevelSet`：教学关卡 DataAsset；未显式指定时可使用 C++ 默认教学配置。
- `TutorialLevelIndex`：当前教学关卡索引。
- `GridManager`、`TurnManager`、`PlayerPawn`、`EnemyManager`、`PickupManager`：可手动指定，也可通过 `bAutoFindReferences` 自动查找。
- `bGenerateOnBeginPlay`：BeginPlay 时自动生成并初始化。
- `bInitializePlayer`：将玩家初始化到 `LastGeneratedLayout.StartCoord`。
- `bSpawnEnemies`：根据 `EnemySpawnCandidates` 生成敌人；敌人类型从 `DungeonGenerationSettings.EnemySpawnPool` 中按深度筛选并按权重选择，生成后按池条目的阈值规则写入敌人 `KillThreshold`。
- `bSpawnPickups`：根据 `RewardSpawnCandidates` 生成拾取物；道具类型从 `DungeonGenerationSettings.PickupSpawnPool` 中按深度筛选并按权重选择。
- `LastGeneratedLayout`：保存最近一次生成结果，供蓝图调试和后续系统读取。

`GenerationMode == TutorialFixed` 时，`GenerateAndInitializeRun()` 调用 `GenerateTutorialRun()`，不执行 `ULSystemDungeonGenerator::GenerateDungeonLayout()`。教学模式会校验关卡定义必须是 `10 x 10` 且包含 100 个 `FGridTileLayoutData`，然后调用 `AGridManager::ApplyTileLayout()` 应用固定布局，并根据 `FTutorialEnemySpawnData` 逐个生成固定敌人。

当前教学资产位于：

- `Content/Data/Tutorial/DA_TutorialLevelSet.uasset`
- `Content/Maps/Tutorial/L_Tutorial_01_TileAttributes.umap`
- `Content/Maps/Tutorial/L_Tutorial_02_EnemyKills.umap`
- `Content/Maps/Tutorial/L_Tutorial_03_ConversionEnergy.umap`
- `Content/Maps/Tutorial/L_Tutorial_04_RangedFriendlyFire.umap`
- `Content/Maps/Tutorial/L_Tutorial_05_FactionSuppression.umap`

敌人生成使用 deferred spawn，先关闭敌人的自动网格初始化，再根据候选点深度计算 `KillThreshold`，最后调用 `InitializeOnGrid(GridManager, CandidateCoord)`，避免敌人先占用默认 `StartGridCoord`。

`EnemySpawnPool` 单项字段：

- `EnemyClass`：被生成的敌人 Pawn 类。
- `Weight`：同一候选深度下的相对抽取权重。
- `MinDepth` / `MaxDepth`：该敌人类型允许出现的候选深度范围。
- `KillThresholdOverride`：大于 `0` 时覆盖敌人类默认击杀阈值；等于 `0` 时保留敌人蓝图或 C++ 默认值。
- `KillThresholdBonusPerDepth`：按候选点 `Depth` 累加的击杀阈值加成。

最终敌人击杀阈值公式：

```text
FinalKillThreshold = Max(1, BaseKillThreshold + Candidate.Depth * KillThresholdBonusPerDepth)
```

其中 `BaseKillThreshold` 优先取 `KillThresholdOverride`，未配置覆盖时取敌人实例自身的 `KillThreshold`。

拾取物生成使用 deferred spawn，从 `RewardSpawnCandidates` 中取可通行、未被占据、未已有道具的坐标。生成后调用 `InitializeOnGrid(GridManager, CandidateCoord)`，并注册到 `AGridPickupManager`。拾取物不写入 `FTileData.OccupantType`，因此不会阻挡玩家移动。

`PickupSpawnPool` 单项字段：

- `PickupClass`：被生成的 `AGridPickupActor` 或其子类。
- `Weight`：同一候选深度下的相对抽取权重。
- `MinDepth` / `MaxDepth`：该道具类型允许出现的候选深度范围。

首个具体道具为 `AGridHealthPickupActor` / `BP_HealthPickup`，默认调用 `UPlayerAttributeComponent::Heal(1)` 回复 1 点 HP。

---

## 6. 玩家系统技术需求

### 6.1 基础属性

- HP：默认 3
- 构成值：0 到 10
- 酸性值：0 到 10

当前工程实现：

- `UPlayerAttributeComponent` 持有 `CurrentHealth`、`MaxHealth`、构成值与酸性值。
- HP 变化通过 `ApplyHealthDamage()` 和 `Heal()` 处理，并广播 `OnPlayerHealthChanged`。
- HP 从正数降至 0 时广播 `OnPlayerDefeated`，敌方回合管理器会将回合状态切换到 `Defeat`。

### 6.2 移动规则

- 玩家可在四个方向移动
- 目标格为空时执行位移
- 目标格为敌人时进入攻击判定
- 目标格不可通行时移动失败不消耗步数
- 若玩家主动向敌人格移动但未击杀目标，则该次动作消耗步数并退回原格

### 6.3 属性变化规则

- 进入构成主义地块：构成值 +1，酸性值 -1
- 进入酸性地块：酸性值 +1，构成值 -1
- 受到构成伤害：构成值 -1
- 受到酸性伤害：酸性值 -1
- 任一数值下限为 0，上限为 10

### 6.4 压制状态

当某一属性值达到 10 时，对应阵营敌人进入压制状态：

- 构成值满：构成主义敌人不再主动攻击玩家
- 酸性值满：酸性敌人不再主动攻击玩家

技术上建议以敌人状态标记实现，而非在玩家身上写死免疫逻辑，这样后续可扩展为“临时压制”“持续若干回合压制”等规则。压制状态与免疫效果可叠加，不互斥。

### 6.5 变身棋子系统

变身棋子是一次性战术移动资源，用于在默认 WASD 四向单格移动之外提供特殊走法。

- 变身棋子由地图中的可拾取道具提供，拾取后进入玩家变身背包，同类棋子堆叠计数。
- 背包通过 4 槽轮盘呈现，槽位分别对应 `Knight`、`Bishop`、`Rook`、`Queen`。
- 默认状态下玩家仍使用 WASD 移动；只有按住 G 打开轮盘并点击棋子槽位后，才进入变身目标选择。
- 进入目标选择时立即切换玩家棋子模型或材质；此阶段不消耗库存。
- 左键点击合法目标并成功开始移动后，消耗 1 个对应变身棋子。
- 按 ESC 或右键短按取消变身时不消耗资源，并恢复默认棋子模型。
- 变身移动或变身击杀完成后，玩家自动恢复默认棋子模型，并按一次玩家行动推进回合。
- `Knight` 形态按 8 个 L 形偏移跳跃，允许越过中间格。
- `Bishop` 形态沿 4 个斜向移动任意距离，不可穿越障碍或敌人。
- `Rook` 形态沿上下左右 4 向移动任意距离，不可穿越障碍或敌人。
- `Queen` 形态沿 8 向移动任意距离，阻挡与击杀限制同 `Rook` 和 `Bishop`。
- 长距离形态扫描方向时，遇到首个可击杀敌人可生成击杀目标，并停止继续扫描该方向。
- 目标选择阶段启用边缘滚屏，支持查看默认视角外的合法目标；右键拖动相机时暂停边缘滚屏和左键确认。
- 鼠标悬停合法目标时写入全局材质参数，供目标格材质显示 hover 状态。

---

## 7. 战斗与伤害结算

### 7.1 攻击类型定义

玩家攻击为双属性输出：

- 构成伤害 = 当前构成值
- 酸性伤害 = 当前酸性值

### 7.2 免疫规则

- 构成主义敌人免疫构成伤害
- 酸性敌人免疫酸性伤害

### 7.3 单次判定规则

敌人不进行伤害累计。必须满足以下条件之一才会死亡：

- 受到的有效单次对应属性伤害大于等于其 HP 阈值
- 或被不同阵营敌人直接击杀

这意味着敌人的 HP 不是传统意义上的累积血量，而是单次击杀阈值。

### 7.4 玩家攻击退回规则

当玩家移动到敌人所在格并发起近战攻击时：

- 若击杀成功，玩家占据该格
- 若未击杀成功，玩家退回原格并消耗该步

### 7.5 友伤规则

- 敌人不主动攻击同阵营敌人
- 不同阵营敌人之间的友伤必须在伤害结算层统一处理，避免 AI 层与战斗层重复判断
- 远程敌人攻击到的非同阵营敌人直接死亡，不进入HP阈值判定
- 非同阵营远程敌人若在同一结算窗口内互相攻击，则双方同时死亡，不分行动优先级
- 不同阵营的近战敌人若移动到同一格，则按当前格地块类型结算：
	- 构成主义地块：击杀酸性敌人
	- 酸性地块：击杀构成主义敌人
	- 极简主义地块：击杀HP较低的敌人
	- 极简主义地块且HP相同：双方同时死亡
- 压制状态不影响敌人的免疫判定，压制与免疫可同时成立

### 7.6 结算顺序建议

建议结算顺序如下：

1. 判定有效伤害类型
2. 应用免疫
3. 判定是否触发直接死亡或同步死亡（远程互击、近战同格冲突）
4. 计算单次阈值是否达成
5. 触发死亡、掉落与占位清理
6. 再处理移动占位变化

### 7.7 当前工程实现备注

玩家攻击敌人：

- `AGridPawn::TryMove()` 在目标格为敌人时进入近战结算。
- `UCombatResolverComponent::ResolvePlayerMeleeAttack()` 读取玩家双属性值，应用敌人阵营免疫，并返回 `FCombatResolveResult`。
- 若本次单次有效伤害达到 `KillThreshold`，敌人死亡并清空占位，玩家进入敌人格。
- 若未击杀，玩家播放失败前冲/退回表现，格子占位不变化，但本步仍消耗。

敌人攻击玩家：

- 默认 `AGridEnemyPawn::ExecuteBasicTurn()` 在敌人与玩家相邻时先调用 `ApplyMeleeAttackDamage()`，再触发 `ExecuteMeleeAttack()` 给蓝图播放表现。
- `UCombatResolverComponent::ResolveEnemyMeleeAttack()` 负责应用 `FEnemyAttackDamage`，修改玩家 HP 与对应属性值。
- 默认 `AttackDamage = 1`；`bApplyFactionAttributeDamage = true` 时，构成敌人扣玩家构成值，酸性敌人扣玩家酸性值。
- 如果玩家 HP 归零，`AGridEnemyManager::ExecuteEnemyTurn()` 中断后续敌人行动，并设置 `ETurnState::Defeat`。

敌人友伤：

- `UCombatResolverComponent::ResolveEnemyMeleeCollision()` 负责结算两个敌人在同一目标格发生的近战冲突，只返回 `FEnemyFriendlyFireResolveResult`，不直接修改 Actor 或格子。
- `AGridEnemyPawn::TryMoveToGridCoord()` 在目标格被敌人占据时调用近战友伤结算；同阵营不造成伤害，异阵营按目标格地块类型决定死亡结果。
- 目标死亡且攻击者存活时，攻击者通过 `AGridManager::RequestMove()` 进入目标格；攻击者死亡时清空自身源格；双方死亡时清空双方占据。
- `UCombatResolverComponent::ResolveEnemyRangedFriendlyFire()` 提供远程友伤判定；敌方回合中的 pending ranged 由 `AGridEnemyManager::ResolvePendingRangedAttacks()` 批量收集命中并统一应用死亡。
- `AGridEnemyPawn::ApplyRangedFriendlyFireDamage()` 保留给蓝图或其他单发远程逻辑使用，会对异阵营目标执行直接击杀和占位清理。
- 友伤表现入口为 `AGridEnemyPawn::OnFriendlyFireResolved()`。当前已实现基础直接结算、远程敌人两阶段瞄准/开火，以及远程同窗口互击同步死亡调度；跨阵营清除链的队列化表现仍属于后续扩展。

---

## 8. 敌人系统技术需求

### 8.1 敌人基础类型

- 阵营类型：构成主义、酸性
- 行为类型：近战、远程

### 8.2 敌人基础属性

建议最少包含：

- HP 阈值
- 视野或攻击范围
- 移动优先级
- 是否远程
- 蓄力状态
- 当前目标
- 是否处于压制状态
- HP 阈值可由 `EnemySpawnPool` 单项配置随房间深度递增；不同敌人类型可以有不同基础阈值和深度成长值。

### 8.3 敌人行动规则

玩家每步结束后，每个存活敌人按顺序行动一次，典型选择为：

- 可攻击则攻击
- 不可攻击则移动
- 远程敌人优先与玩家在 X/Y 轴对齐；同轴且中间无 Obstacle 时进入瞄准模式，下一次敌方回合开始时结算锁定攻击线

当前工程默认近战 AI：

- 敌人与玩家曼哈顿距离为 1 时执行近战攻击。
- 敌人被玩家当前满值属性压制时，不执行主动攻击。
- 不相邻时通过四方向 A* 计算到玩家格的路径，并尝试移动到路径上的下一格。
- 若玩家 HP 在敌方回合中归零，本轮敌人行动立即停止。

### 8.4 远程敌人状态机

推荐状态：

- `Idle`
- `AimingRangedAttack`

基础行为：

- 当远程敌人与玩家处于同一 X 轴或 Y 轴，且中间无 `Obstacle` 时，进入 `AimingRangedAttack` 并显示锁定攻击线
- 锁定攻击线从敌人相邻格开始，沿瞄准方向延伸到地图边界或 `Obstacle` 前一格
- 下一次敌方回合开始时优先批量结算上一回合锁定的攻击线，结算后清除瞄准状态与攻击范围提示
- 同一结算窗口内的跨阵营远程互击会先收集命中结果，再统一应用死亡，因此双方同时死亡，不受敌人遍历顺序影响
- 若玩家已经离开锁定攻击线，则不会受到伤害
- 若无法瞄准玩家，远程敌人优先移动到能与玩家同 X/Y 轴且无遮挡的位置；没有可用目标时再回退到基础靠近逻辑

### 8.5 AI 选择逻辑

最小可用方案建议采用优先级驱动，而非复杂行为树：

- 近战敌人优先攻击相邻玩家，否则朝玩家最短路径移动
- 远程敌人优先进入瞄准/结算流程，否则寻找对齐位置
- 远程单位优先维持射击节奏

若使用行为树，也应保持节点简洁，避免原型阶段过度工程化。

### 8.6 路径寻路要求

- 敌人移动建议使用 A* 或 BFS
- 需要避让不可通行格与占据格
- 必须支持局部路径重算
- 寻路失败时应回退到原地或执行待机

---

## 9. 地块转换能量系统

### 9.1 获取规则

- 击杀敌人后，获得地块转换能量
- 能量本身不绑定转换类型，类型只在使用时由玩家决定

### 9.2 存储规则

- 仅能存储 1 个能量
- 使用后消耗

### 9.3 使用规则

使用能量时，由玩家通过 UI 触发，选择想转换的类型，消耗当前持有能量，并将以玩家当前格为中心的 3×3 区域转换为对应类型地块。
也就是说，能量只提供一次转换机会，不预设具体转换结果。

### 9.4 技术实现建议

- 转换能量应作为独立资源对象管理
- 使用前仅需检查是否持有能量，无需管理队列
- 地块转换应走统一的地图修改接口，确保同步 UI、可达性与 VFX
- PCG 起点区域 `Start` 保持 `Minimal`，但允许转换；出口 `Exit` 和墙体/障碍仍不可转换。

### 9.5 风险点

- 3×3 转换可能破坏地图可读性，需保留最小比例的极简地块作为缓冲
- 转换过强会削弱地形策略，因此建议后续通过使用时机、步数成本或冷却控制节奏

---

## 10. UI 与反馈需求

### 10.1 HUD 必备信息

- HP
- 构成值进度
- 酸性值进度
- 当前持有的地块转换能量状态
- 变身棋子库存计数
- 当前回合提示
- 敌人压制状态提示

当前工程实现：

- `UPlayerAttributeHUDWidget` 可绑定 `HealthText`、`HealthProgressBar`、`ConstructText`、`AcidText`、`ConstructProgressBar`、`AcidProgressBar`。
- HUD 监听 `OnPlayerAttributeChanged` 和 `OnPlayerHealthChanged`，不使用 Tick 轮询。
- 如果 Widget Blueprint 没有设计器树，C++ 会创建包含 HP、构成值、酸性值的 fallback Widget Tree。

### 10.2 战斗反馈

- 移动时播放简短位移动画和音效
- 攻击时播放冲击反馈
- 未击杀退回时必须有明显失败反馈
- 压制状态需有专门高亮表现

### 10.3 地图反馈

- 地块转换必须有显著视觉切换
- 玩家回到输入阶段时，应高亮下一步可移动到的空白相邻格；玩家动作和敌人回合解析期间应清空该提示，避免显示过期移动目标。
- 变身目标选择阶段应高亮所有合法移动或击杀目标，并在鼠标悬停格显示额外 hover 反馈。
- 远距离变身目标选择时，边缘滚屏必须保持目标格、鼠标射线和点击确认一致。
- 属性变化应有即时数字或条形反馈
- 远程敌人瞄准回合需要明确提示

### 10.4 可用性要求

- 玩家必须能快速识别地块类型
- 玩家必须能快速识别下一步可以移动到哪些格子
- 玩家必须能区分当前属性成长方向
- 玩家必须能识别敌人类型与压制状态

---

## 11. 技术数据结构建议

### 11.1 核心枚举

- `ETileType`：构成主义、酸性、极简主义、障碍、特殊
- `EGridCellRole`：开放格、房间、走廊、墙、起点、出口
- `EOccupantType`：空、玩家、敌人、物件
- `EEnemyFaction`：构成主义、酸性
- `EEnemyBehavior`：近战、远程
- `ETurnState`：初始化、玩家输入、玩家动作结算、敌人行动、地图结算、终局检查、胜利、失败
- `EResourceType`：构成能量、酸性能量
- `EChessTransformType`：无、骑士、教皇、战车、皇后
- `ETransformTargetType`：移动目标、击杀目标
- `EPlayerControlMode`：默认 WASD、变身轮盘、变身目标选择

### 11.2 核心结构体

- `FTileData`：地块类型、地图形态语义、占据状态、坐标、可通行性、转换性、区域 ID、深度
- `FGridTileLayoutData`：PCG 或手工布局写入 GridManager 前使用的轻量格子描述
- `FLSystemDungeonRoomNode`：L-System 地牢房间节点，包含房间 ID、中心点、半径、深度和房间格子
- `FLSystemDungeonConnection`：房间连接边，包含起止房间和走廊路径
- `FDungeonSpawnCandidate`：生成后的候选出生/事件/奖励坐标，包含坐标、区域 ID 和深度
- `FGeneratedDungeonLayout`：一次地牢生成的完整输出，包含尺寸、Seed、格子布局、房间、连接、起点、出口和候选点
- `FTutorialEnemySpawnData`：教学关卡固定敌人描述，包含坐标、敌人类、阵营、行为类型、击杀阈值和远程攻击距离
- `FTutorialLevelDefinition`：教学关卡固定布局描述，包含关卡 ID、10x10 尺寸、玩家起点、出口、100 个地块和固定敌人列表
- `FDungeonPickupSpawnEntry`：拾取物生成池条目，包含道具类、权重和深度范围
- `FPlayerState`：HP、构成值、酸性值、当前持有能量、位置
- `FEnemyState`：阵营、行为类型、HP阈值、状态机、位置、是否压制
- `FEnemyAttackDamage`：敌人对玩家造成的 HP 伤害、构成值变化、酸性值变化
- `FEnemyAttackResolveResult`：敌人攻击玩家后的伤害是否生效、玩家是否失败、剩余 HP
- `FTurnContext`：当前步数、动作来源、事件列表
- `UDungeonGenerationSettings`：地牢生成 DataAsset，承载种子、L-System、房间、走廊和可达性约束
- `UTutorialLevelSet`：教学关卡 DataAsset，承载固定教学布局和固定敌人配置
- `ADungeonRunManager`：运行时关卡初始化 Actor，支持 PCG 模式和固定教学模式，负责生成或读取布局、应用棋盘、初始化玩家、可选敌人、固定教学敌人和可选拾取物
- `AGridPickupActor`：按棋盘坐标放置的可拾取道具基类，具体效果由子类或蓝图覆写
- `AGridPickupManager`：按坐标注册、查询和结算拾取物的运行时管理器
- `FTransformMoveTarget`：变身目标格、目标类型、目标敌人引用、世界坐标
- `UChessPieceFormData`：变身形态 DataAsset，包含形态类型、显示名、图标、模型、材质、移动方向、最大距离、是否跳跃、是否可击杀
- `UPlayerTransformInventoryComponent`：玩家变身背包组件，负责拾取、堆叠、查询、消耗和刷新事件
- `AGridTransformPiecePickupActor`：地图变身道具 Actor，负责提供具体形态并在拾取后写入背包
- `UTransformWheelWidget`：G 键轮盘 UI，负责展示 4 个槽位、库存数量和槽位点击选择

### 11.3 存档建议

单局存档最少记录：

- 种子
- 当前地图格状态
- 玩家状态
- 敌人状态
- 当前步数
- 当前能量

---

## 12. 事件流与模块通信

### 12.1 事件建议

- `OnPlayerMoved`
- `OnPlayerAttacked`
- `OnEnemyMoved`
- `OnEnemyAttacked`
- `OnPlayerHealthChanged`
- `OnPlayerDefeated`
- `OnEnemyMeleeAttackResolved`
- `OnEnemyFriendlyFireResolved`
- `OnTileChanged`
- `OnValueChanged`
- `OnEnemyKilled`
- `OnEnergyGained`
- `OnEnergyConsumed`
- `OnTransformInventoryChanged`
- `OnTransformSelectionStarted`
- `OnTransformSelectionCancelled`
- `OnTransformVisualStateChanged`
- `OnTransformMoveCommitted`
- `OnTransformMoveFinished`
- `OnTurnEnded`
- `OnVictory`
- `OnDefeat`

### 12.2 通信原则

- 逻辑层只发事件，不直接控制 UI
- UI 层只订阅状态变化，不反向修改战斗结果
- 地图变更必须由统一接口执行，避免多处写格子

---

## 13. 开发里程碑建议

### 13.1 原型阶段

目标：验证核心循环是否成立。

- 完成棋盘地图基础生成
- 完成玩家移动、攻击和退回
- 完成双属性值变化
- 完成敌人最小 AI
- 完成单次阈值击杀
- 完成地块转换能量的获取与使用

### 13.2 垂直切片阶段

目标：验证一局完整体验。

- 完成 PCG 可达性校验
- 完成远程敌人蓄力逻辑
- 完成友伤清除链表现与压制状态反馈
- 完成基础 UI 和反馈
- 完成变身棋子拾取、背包、轮盘选择、目标高亮、边缘滚屏和一次性特殊移动
- 完成 5 个固定教学关卡 Map 与教学 DataAsset
- 完成房间级胜负流转
- 完成多房间激活与无缝推进

### 13.3 内容扩展阶段

目标：增加可重复游玩性。

- 扩展敌人种类
- 扩展环境事件
- 扩展地图规则与特殊地块
- 扩展关卡层级与难度曲线

---

## 14. 风险与验证点

### 14.1 主要风险

- 单次阈值击杀可能导致数值平衡极端化
- 双属性系统理解成本高，需强 UI 引导
- 3×3 转换可能过强，破坏地图策略
- PCG 若不可达率过高，会影响生成稳定性
- 敌人友伤规则若过强，可能让玩家策略单一化

### 14.2 必测问题

- 玩家是否能在 3 分钟内理解构成与酸性的区别
- 单步循环是否足够流畅且没有卡顿感
- 地块转换是否会导致地图失真或失去挑战
- 远程敌人的蓄力是否清晰且可被反制
- 地图是否能稳定生成可达解

---

## 15. 平衡参数占位表

建议以下参数全部外置配置，后续由数值策划调优：

- 玩家 HP
- 初始构成值
- 初始酸性值
- 构成/酸性地块增减值
- 敌人 HP 阈值
- 敌人移动优先级
- 远程敌人蓄力回合数
- 转换能量存储上限
- 3×3 转换范围
- 变身道具掉落概率
- 各变身棋子库存上限或推荐持有量
- 各变身形态移动方向、最大距离和跳跃规则
- 变身目标选择边缘滚屏速度、触发边距和最大相机偏移
- 地图生成中地块比例
- 起点安全区半径

---

## 16. 最小可玩版本定义

### 16.1 MVP 必须包含

- 1 张可生成的网格地图
- 2 类地块
- 2 类敌人阵营
- 近战攻击与退回
- 双属性值与压制机制
- 1 套转换能量机制
- 1 套基础变身棋子机制，至少包含拾取、库存、轮盘选择、目标选择、取消和消耗规则
- 胜负判定
- 基础 UI

### 16.2 MVP 可暂缓

- 复杂环境事件
- 进阶音画演出
- 完整数值平衡
- 大量敌人变体

---

## 17. 实现顺序建议

1. 先完成地图数据与玩家移动
2. 再完成属性变化与敌人占格
3. 再完成攻击、退回与死亡判定
4. 再完成地块转换能量系统
5. 再完成敌人 AI 和远程蓄力
6. 再完成 PCG 可达性校验
7. 最后补 UI、反馈与调参工具

---

## 18. 已确认规则

- 玩家因障碍或墙体导致的移动失败不消耗步数；玩家主动向敌人格移动但未击杀目标时，消耗步数并退回原格。
- 目标格已有敌人时仅触发攻击，不允许穿越。
- 默认状态下玩家使用 WASD 四向单格移动；变身棋子仅影响一次被选择的特殊移动。
- 按住 G 打开变身轮盘，点击有库存且有合法目标的槽位后进入目标选择；进入目标选择会立即切换棋子模型。
- 变身目标选择阶段按 ESC 或右键短按取消，不消耗对应棋子并恢复默认模型。
- 变身移动或击杀成功开始后才消耗 1 个对应棋子；移动完成后恢复默认模型并推进回合。
- `Rook` 和 `Queen` 等远距离形态不可跨越敌人或障碍，但可以选择击杀路径上遇到的首个合法敌人。
- 敌人 HP 阈值通过 `EnemySpawnPool` 单项配置与候选点深度计算，越靠后的房间可以生成更高 `KillThreshold` 的敌人。
- 远程敌人采用四向直线范围攻击，若玩家不在攻击线内则追逐玩家。
- 地块转换不会覆盖障碍物与特殊事件。
- 压制状态与免疫效果可叠加，被压制敌人仍保持对应免疫。
- 关卡采用多房间无缝推进，敌人在房间激活后参与行动，未清理完的敌人可继续追逐玩家进入新房间。
- 教学关卡固定使用 `10 x 10` 地图，不调用 PCG；敌人由 `DA_TutorialLevelSet` 的固定列表生成。

---

## 19. 结论

这个项目的技术关键不在于单点战斗实现，而在于三层系统的联动：

- 地图是资源系统
- 属性是输出与压制系统
- 敌人是反向约束系统

只要这三层在单步回合中稳定闭环，后续再扩展美术表现、敌人变体与关卡内容都会比较顺。
