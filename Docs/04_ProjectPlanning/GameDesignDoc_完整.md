<a id="english-version"></a>
<p align="right"><a href="#中文版本"><strong>中文</strong></a></p>

# Construct & Corrode - Gameplay Design Document

Version: 0.5  
Date: 2026-06-26  
Document focus: Gameplay vision, player experience, and run flow.

---

## High Concept

Construct & Corrode is a step-based roguelite tactics game played on a chessboard dungeon. The player reads the board, moves one tile at a time, gathers opposing Construct and Corrode power from terrain, prepares single-action threshold kills, manipulates enemy friendly fire, and spends rare chess-piece transformations to break the normal movement rules at decisive moments.

The game is designed around one question: **what is the most valuable next step?** Every move changes position, resource state, enemy timing, and future kill potential.

## Genre And Experience

- **Genre**: step-based turn tactics, roguelite, grid strategy, light puzzle tactics.
- **Session shape**: short tactical floors chained into a run.
- **Core objective**: clear every enemy on the current floor.
- **Failure condition**: player HP reaches 0.
- **Primary feeling**: deliberate pressure, readable danger, and the satisfaction of turning a hostile board into a planned clearing chain.

## Player Fantasy

The player is not a brute-force fighter. They are a board tactician caught between two opposed forces:

- **Construct**: structure, geometry, order, hard edges, controlled growth.
- **Corrode**: acid, erosion, saturation, unstable bursts, destructive flow.

The player survives by absorbing both forces, deciding when to lean into one, when to pivot, and when to let enemies destroy each other.

## Design Pillars

1. **Every step is a decision**  
   Default movement is four-directional and tile-by-tile. Since valid actions advance the turn, even a small repositioning choice carries tactical cost.

2. **Terrain is the resource economy**  
   Tiles are not passive floor art. Construct, Corrode, minimal, and blocked tiles define movement routes, attribute growth, combat readiness, and local board control.

3. **Kills are prepared, not chipped down**  
   Enemies do not use gradual HP attrition. The player must meet the correct kill threshold in one action, creating a rhythm of preparation, prediction, and release.

4. **Enemy factions are part of the puzzle**  
   Opposing enemy factions can damage or eliminate each other through ranged lines and collision rules. A dangerous enemy can become a tool if the player reshapes the situation.

5. **Special movement is a tactical spike**  
   Chess-piece transformations are rare one-use movement options. They create breakthrough turns without replacing the tension of ordinary grid movement.

6. **Rules must be readable at a glance**  
   The player should be able to understand what will happen next through tile colors, enemy faction language, attack previews, movement highlights, and clear feedback.

## Core Gameplay Loop

### Moment-To-Moment Loop

1. Read the board: terrain types, enemy faction, enemy behavior, attack lines, safe tiles, and current attributes.
2. Choose an action: move, attack, use conversion energy, or spend a chess-piece transformation.
3. Resolve the player action: collect terrain power, trigger pickups, attempt a threshold kill, or reposition.
4. Resolve enemy intent: melee enemies close in or strike; ranged enemies aim or fire.
5. Update the board: threats, available moves, attributes, remaining enemies, and run state become readable again.

### Floor Loop

1. Enter a tactical floor.
2. Build Construct or Corrode values through terrain movement.
3. Prepare kill thresholds against enemies of the opposing faction.
4. Use friendly fire, suppression, conversion, and transformations to remove enemy clusters.
5. Clear all enemies to advance deeper into the run.

### Run Loop

1. Start with low resources and a readable board.
2. Clear floors while enemy density and threshold pressure increase.
3. Find pickups that restore HP or grant chess-piece transformations.
4. Push for a deeper floor and a faster run record.
5. On defeat, the run ends and the player can try again with better tactical understanding.

## Board Reading

The board is the game's main language. A good turn begins with reading five layers:

| Layer | Player question |
| --- | --- |
| Terrain | Which tiles grow Construct or Corrode, and which routes will become neutral after use? |
| Enemy faction | Which damage type can kill this enemy, and which damage type is ignored? |
| Enemy behavior | Is this enemy melee pressure, ranged pressure, or a friendly-fire opportunity? |
| Timing | Will this move trigger a safe enemy turn, an attack, or a setup for the next turn? |
| Breakthrough options | Is this the moment to spend conversion energy or a chess-piece transformation? |

The intended player rhythm is: **observe, predict, commit, resolve, re-read**.

## Terrain And Attribute Economy

The player has two opposed attributes:

| Attribute | Role |
| --- | --- |
| Construct | Damage source against Corrode enemies and suppression route against Construct enemies. |
| Corrode | Damage source against Construct enemies and suppression route against Corrode enemies. |

Terrain drives these values:

| Tile type | Gameplay meaning |
| --- | --- |
| Construct tile | Increases Construct and lowers Corrode, then becomes minimal. |
| Corrode tile | Increases Corrode and lowers Construct, then becomes minimal. |
| Minimal tile | Neutral space that supports movement without changing attributes. |
| Blocked tile | Shapes routes, sight lines, and enemy approach patterns. |

This creates a soft route-planning puzzle. The player is not simply moving toward an enemy; they are choosing a path that builds the correct future damage.

## Combat Grammar

### Dual Damage

Each player attack carries both current attribute values:

- Construct damage equals the current Construct value.
- Corrode damage equals the current Corrode value.

### Faction Immunity

| Target | Ignores | Can be killed by |
| --- | --- | --- |
| Construct enemy | Construct damage | Corrode damage reaching the kill threshold. |
| Corrode enemy | Corrode damage | Construct damage reaching the kill threshold. |

### Threshold Kill Rule

Enemies are defeated only when the correct non-immune damage reaches or exceeds their kill threshold in a single action. If the threshold is not met, the attack does not partially weaken the target.

The purpose is to make attacks feel like tactical executions rather than repeated damage trades. The player should ask: **am I ready to strike, or should I spend one more turn shaping the board?**

## Suppression

When one of the player's attributes reaches its maximum, enemies of the matching faction become suppressed and stop actively attacking.

| Full attribute | Suppressed faction |
| --- | --- |
| Full Construct | Construct enemies |
| Full Corrode | Corrode enemies |

Suppression is a tempo tool. It lets the player change the threat structure for a short tactical window, but it does not remove faction immunity. A suppressed enemy may be safer, but it still requires the correct opposing damage type to kill.

## Enemy Design

Enemies are defined by two readable axes:

| Axis | Options |
| --- | --- |
| Faction | Construct or Corrode |
| Behavior | Melee or ranged |

### Melee Enemies

Melee enemies pressure space. They move toward the player and attack when adjacent. Their role is to make local positioning meaningful and to punish careless routes.

### Ranged Enemies

Ranged enemies pressure lines. They threaten straight lanes, aim before firing, and create turns where the player must decide whether to dodge, bait, or redirect the shot.

### Friendly Fire And Collision

Opposing factions can destroy each other:

- A ranged shot that hits an enemy of the opposing faction kills that enemy.
- Opposing ranged enemies can eliminate each other if their attacks line up in the same resolution window.
- Opposing melee enemies that collide can resolve based on the terrain they meet on: Construct terrain favors Construct, Corrode terrain favors Corrode, and minimal terrain favors the lower-threshold enemy or destroys both if tied.

This makes enemy placement a resource. The player is rewarded for seeing enemies not only as threats, but as pieces in a clearing chain.

## Conversion Energy

Conversion energy is a limited board rewrite tool gained through kills.

| Rule | Design purpose |
| --- | --- |
| Earned by killing enemies | Rewards successful setup with a new tactical option. |
| Limited storage | Encourages use instead of hoarding. |
| Converts a 3x3 area around the player | Creates a local, readable burst of terrain control. |

The player uses conversion energy to:

- create missing attribute resources,
- set up a future threshold kill,
- alter melee collision outcomes,
- create suppression routes,
- recover tactical space when the board is running dry.

## Chess-Piece Transformations

Chess-piece transformations are consumable movement forms. They are designed as rare tactical breakthroughs, not as the default way to move.

| Form | Movement fantasy | Tactical use |
| --- | --- | --- |
| Knight | Leap in an L shape | Jump over pressure, reach a precise target, or escape boxed routes. |
| Bishop | Slide diagonally | Cut through diagonal lanes and attack from unexpected angles. |
| Rook | Slide orthogonally | Cross long straight paths and punish open lanes. |
| Queen | Slide in any of eight directions | Spend a powerful option for maximum reach and flexibility. |

The transformation flow should feel like a moment of commitment:

1. The player opens the transformation selection.
2. The chosen form previews legal targets.
3. The player confirms a move or capture.
4. The form is spent and normal movement resumes.

## Pickups

Pickups are simple, readable rewards that support the run without diluting the tactical board focus.

| Pickup | Role |
| --- | --- |
| HP recovery | Extends the run after costly decisions or enemy pressure. |
| Chess-piece form | Adds one tactical breakthrough option to the player's inventory. |

Pickups should appear as meaningful route incentives. A pickup is strongest when reaching it creates an interesting positioning question.

## Difficulty And Progression

The run grows through board pressure rather than through hidden complexity.

| Progression lever | Gameplay effect |
| --- | --- |
| More rooms and routes | Increases planning space and navigation pressure. |
| More enemies | Creates denser threat reading and more friendly-fire opportunities. |
| More pickups | Offers recovery and transformation choices as pressure rises. |
| Higher kill thresholds | Demands more deliberate terrain routing and attribute preparation. |

The intended difficulty curve asks players to improve at reading patterns, sequencing actions, and preserving breakthrough tools.

## Tutorial Flow

Tutorial levels are fixed tactical lessons. They introduce one concept at a time, then ask the player to perform it through normal play.

| Tutorial | Learning goal |
| --- | --- |
| 1 | Terrain changes and minimal tiles. |
| 2 | Threshold kills and faction immunity. |
| 3 | Kill reward into 3x3 terrain conversion. |
| 4 | Ranged aim lines and cross-faction friendly fire. |
| 5 | Full-attribute suppression. |
| 6 | HP recovery, transformation pickup, selection, and special movement. |

The onboarding goal is not to explain every rule in text. It is to make the player *perform* the rule in a controlled space, then recognize it during a run.

## Feedback And Readability

The game should constantly answer: **what can I do, what will happen, and why did that happen?**

| Feedback need | Presentation goal |
| --- | --- |
| Available movement | Highlight legal reachable tiles during player input. |
| Attack result | Preview whether the current attack can kill the target. |
| Enemy threat | Show melee danger and ranged aim lines clearly. |
| Attribute state | Keep HP, Construct, Corrode, conversion energy, and transformation inventory visible. |
| Turn rhythm | Make player action, enemy action, and board update feel distinct. |
| Transformation targeting | Preview legal destinations and danger after the move. |

Good feedback should reduce rule confusion while preserving decision pressure.

## Audio Direction

Audio supports the tactical rhythm of the board.

- Movement should feel precise and step-based.
- Attribute gain should make terrain feel valuable.
- Threshold kills should feel decisive.
- Failed attacks should clearly communicate that the player was not ready.
- Conversion and transformation should sound like rare, intentional tools.
- Enemy aim and ranged fire should create readable anticipation.

The goal is not constant noise. The goal is to sonify tactical beats: **prepare, confirm, strike, convert, survive**.

## Visual Direction

### Construct

Construct visuals should emphasize geometry, hard edges, modular shapes, structured lines, and controlled space.

### Corrode

Corrode visuals should emphasize saturation, fluid motion, erosion, noise, asymmetry, and unstable edges.

### Chess Identity

Player and transformation forms should remain readable from a top-down tactical view. Silhouette clarity matters more than fine detail.

## Current Gameplay Scope

The current gameplay design baseline includes:

- step-based four-direction movement,
- Construct, Corrode, minimal, and blocked terrain,
- dual attributes and suppression,
- threshold kills and faction immunity,
- melee enemies and ranged enemies,
- friendly fire and opposing-faction collision,
- 3x3 terrain conversion energy,
- HP recovery and chess-piece pickups,
- Knight, Bishop, Rook, and Queen transformations,
- procedural run floors,
- six fixed tutorial levels,
- combat preview, HUD feedback, menu flow, audio feedback, and run records.

Future design space includes fuller narrative framing, permanent progression, build-defining relics, shops, elite enemies, bosses, and more varied floor objectives.

## Design Summary

Construct & Corrode is about turning movement into preparation. The player wins by reading terrain, building the correct attribute, predicting enemy behavior, and choosing the one step that transforms the board from danger into opportunity.

---

<a id="中文版本"></a>
<p align="right"><a href="#english-version"><strong>English</strong></a></p>

# Construct & Corrode / 构蚀棋域 - 游戏玩法设计文档

版本：0.5  
日期：2026-06-26  
文档重点：玩法愿景、玩家体验与 Gameplay 流程。

---

## 核心概念

Construct & Corrode 是一款步进式 Roguelite 战术游戏。玩家在棋盘地牢中阅读局势，逐格移动，从地形中获取相互对立的构成与蚀变力量，准备单次阈值击杀，引导敌人友伤，并在关键时刻消耗稀缺的棋子变身来突破默认移动规则。

游戏围绕一个问题展开：**下一步最有价值的行动是什么？** 每一次移动都会改变站位、资源状态、敌人节奏和后续击杀可能。

## 类型与体验

- **类型**：步进式回合战术、Roguelite、网格策略、轻解谜战术。
- **单局形态**：短小战术楼层串联成一次 Run。
- **核心目标**：清除当前楼层内全部敌人。
- **失败条件**：玩家 HP 降至 0。
- **核心感受**：有压力但可读、每一步都有取舍，并能把危险棋盘转化为计划好的清场链。

## 玩家幻想

玩家不是依靠蛮力推进的战士，而是夹在两股对立力量之间的棋盘战术师：

- **构成**：结构、几何、秩序、硬边、受控增长。
- **蚀变**：酸蚀、侵蚀、高饱和、不稳定爆发、破坏性流动。

玩家通过吸收两种力量、判断何时专注一边、何时切换方向、何时让敌人互相摧毁来生存。

## 设计支柱

1. **每一步都是决策**  
   默认移动是四方向单格移动。有效行动会推进回合，因此一次看似简单的换位也有战术成本。

2. **地形就是资源经济**  
   地块不是被动背景。构成、蚀变、极简与障碍共同决定路线、属性成长、击杀准备和局部控场。

3. **击杀来自准备，而不是刮血**  
   敌人不走连续扣血。玩家必须在一次行动中达到正确的击杀阈值，形成“准备、预判、释放”的节奏。

4. **敌人阵营也是谜题的一部分**  
   对立阵营敌人可以通过远程攻击线和同格冲突互相击杀。危险敌人也能成为玩家清场的工具。

5. **特殊移动是战术爆点**  
   棋子变身是稀缺的一次性移动资源。它制造关键突破，但不取代普通网格移动带来的紧张感。

6. **规则必须一眼可读**  
   玩家应能通过地块颜色、敌人阵营语言、攻击预览、移动高亮和清晰反馈理解接下来会发生什么。

## 核心 Gameplay 循环

### 单步循环

1. 阅读棋盘：地形类型、敌人阵营、敌人行为、攻击线、安全格和当前属性。
2. 选择行动：移动、攻击、使用转换能量，或消耗棋子变身。
3. 结算玩家行动：获得地形力量，触发拾取物，尝试阈值击杀，或重新站位。
4. 结算敌人意图：近战敌人逼近或攻击，远程敌人瞄准或开火。
5. 更新棋盘：威胁、可移动格、属性、剩余敌人和 Run 状态重新变得可读。

### 楼层循环

1. 进入一个战术楼层。
2. 通过地形移动积累构成或蚀变值。
3. 针对敌对阵营准备击杀阈值。
4. 利用友伤、压制、地形转换和棋子变身清理敌群。
5. 清除全部敌人后进入更深层的 Run。

### Run 循环

1. 从低资源和可读棋盘开始。
2. 在敌人密度和阈值压力逐渐提高的楼层中推进。
3. 获取恢复 HP 或提供棋子变身的拾取物。
4. 追求更深楼层和更快通关记录。
5. 失败后结束本次 Run，并用更好的战术理解再次尝试。

## 棋盘阅读

棋盘是游戏的主要语言。一个好回合来自对五层信息的阅读：

| 层级 | 玩家问题 |
| --- | --- |
| 地形 | 哪些地块能增长构成或蚀变？哪些路线使用后会变为中性？ |
| 敌人阵营 | 这个敌人能被哪种伤害击杀？会免疫哪种伤害？ |
| 敌人行为 | 这是近战压力、远程压力，还是友伤机会？ |
| 节奏 | 这一步会触发安全敌方回合、敌人攻击，还是下一回合的准备？ |
| 突破选项 | 现在是否值得消耗转换能量或棋子变身？ |

预期的玩家节奏是：**观察、预判、承诺、结算、重新阅读**。

## 地形与属性经济

玩家拥有两条相互对立的属性：

| 属性 | 作用 |
| --- | --- |
| 构成 | 击杀蚀变敌人的伤害来源，也是压制构成敌人的路线。 |
| 蚀变 | 击杀构成敌人的伤害来源，也是压制蚀变敌人的路线。 |

地形驱动这些数值：

| 地块类型 | 玩法含义 |
| --- | --- |
| 构成地块 | 增加构成、降低蚀变，随后变为极简地块。 |
| 蚀变地块 | 增加蚀变、降低构成，随后变为极简地块。 |
| 极简地块 | 不改变属性的中性移动空间。 |
| 障碍地块 | 塑造路线、视线和敌人接近路径。 |

这形成了柔性的路线规划谜题。玩家不是单纯走向敌人，而是在选择一条能够构建未来击杀条件的路径。

## 战斗语法

### 双伤害

玩家每次攻击同时携带当前两条属性值：

- 构成伤害等于当前构成值。
- 蚀变伤害等于当前蚀变值。

### 阵营免疫

| 目标 | 免疫 | 可被击杀方式 |
| --- | --- | --- |
| 构成敌人 | 构成伤害 | 蚀变伤害达到击杀阈值。 |
| 蚀变敌人 | 蚀变伤害 | 构成伤害达到击杀阈值。 |

### 阈值击杀规则

只有当正确的非免疫伤害在一次行动中达到或超过击杀阈值时，敌人才会被击败。若未达到阈值，本次攻击不会让敌人被部分削弱。

这个规则的目的，是让攻击成为战术处决，而不是反复换血。玩家需要问自己：**现在已经准备好出手了吗，还是应该再用一回合塑造棋盘？**

## 压制

当玩家某一属性达到满值时，对应阵营敌人会进入压制状态并停止主动攻击。

| 满值属性 | 被压制阵营 |
| --- | --- |
| 构成满值 | 构成敌人 |
| 蚀变满值 | 蚀变敌人 |

压制是一种节奏工具。它能在短时间内改变威胁结构，但不会移除阵营免疫。被压制的敌人更安全，但仍需要正确的对立伤害才能击杀。

## 敌人设计

敌人由两个可读维度构成：

| 维度 | 选项 |
| --- | --- |
| 阵营 | 构成或蚀变 |
| 行为 | 近战或远程 |

### 近战敌人

近战敌人制造空间压力。它们靠近玩家，并在相邻时攻击。它们的作用是让局部站位变得重要，并惩罚粗心路线。

### 远程敌人

远程敌人制造线性压力。它们威胁直线通道，先瞄准再开火，让玩家决定闪避、诱导，还是利用这条攻击线。

### 友伤与冲突

对立阵营敌人可以互相摧毁：

- 远程攻击命中对立阵营敌人时，会击杀该敌人。
- 对立阵营远程敌人的攻击若在同一结算窗口对齐，可以互相击杀。
- 对立阵营近战敌人发生同格冲突时，根据相遇地形结算：构成地形偏向构成，蚀变地形偏向蚀变，极简地形偏向击杀阈值更低的一方，若相同则双方死亡。

这让敌人分布本身成为资源。玩家会因为不仅把敌人看作威胁，也把它们看作清场链中的棋子而获得奖励。

## 地形转换能量

地形转换能量是通过击杀获得的有限棋盘改写工具。

| 规则 | 设计目的 |
| --- | --- |
| 通过击杀获得 | 用新的战术选项奖励成功准备。 |
| 存储量有限 | 鼓励使用，而不是囤积。 |
| 转换玩家周围 3x3 区域 | 制造局部、可读的地形控制爆发。 |

玩家使用地形转换能量来：

- 创造缺失的属性资源，
- 准备未来的阈值击杀，
- 改变近战敌人冲突结果，
- 创造压制路线，
- 在棋盘资源枯竭时恢复策略空间。

## 棋子变身

棋子变身是消耗型移动形态。它们被设计成稀缺的战术突破，而不是默认移动方式。

| 形态 | 移动幻想 | 战术用途 |
| --- | --- | --- |
| Knight | L 形跳跃 | 越过压力、抵达精确目标、逃离封锁路线。 |
| Bishop | 斜线滑行 | 切入斜向通道，从意外角度攻击。 |
| Rook | 直线滑行 | 穿越长直路线，惩罚开阔通道。 |
| Queen | 八方向滑行 | 消耗强力选项，获得最大范围与灵活性。 |

变身流程应像一次明确的承诺：

1. 玩家打开变身选择。
2. 选择的形态预览合法目标。
3. 玩家确认移动或捕获。
4. 消耗该形态并恢复普通移动。

## 拾取物

拾取物是简单、可读的奖励，用于支持 Run，而不稀释棋盘战术焦点。

| 拾取物 | 作用 |
| --- | --- |
| HP 恢复 | 在高代价决策或敌人压力后延长 Run。 |
| 棋子形态 | 为玩家库存增加一次战术突破机会。 |

拾取物应作为有意义的路线诱因。一个拾取物最有价值的时刻，是到达它本身就形成一个有趣站位问题的时候。

## 难度与推进

Run 的成长来自棋盘压力，而不是隐藏复杂度。

| 推进变量 | 玩法效果 |
| --- | --- |
| 更多房间与路线 | 增加规划空间和导航压力。 |
| 更多敌人 | 提高威胁阅读密度，也创造更多友伤机会。 |
| 更多拾取物 | 在压力上升时提供恢复与变身选择。 |
| 更高击杀阈值 | 要求更有意识的地形路线与属性准备。 |

预期难度曲线要求玩家逐渐更擅长阅读模式、编排行动顺序，并保留关键突破工具。

## 教学流程

教学关卡是固定的战术课程。它们每次介绍一个概念，并要求玩家通过正常玩法完成它。

| 教学 | 学习目标 |
| --- | --- |
| 1 | 地块变化与极简地块。 |
| 2 | 阈值击杀与阵营免疫。 |
| 3 | 击杀奖励与 3x3 地形转换。 |
| 4 | 远程瞄准线与跨阵营友伤。 |
| 5 | 满属性压制。 |
| 6 | HP 恢复、变身拾取、变身选择与特殊移动。 |

教学目标不是用文本解释所有规则，而是让玩家在受控空间中亲自执行规则，并在正式 Run 中认出它。

## 反馈与可读性

游戏应持续回答：**我能做什么、会发生什么、刚才为什么发生？**

| 反馈需求 | 呈现目标 |
| --- | --- |
| 可移动范围 | 在玩家输入阶段高亮合法可达地块。 |
| 攻击结果 | 预览当前攻击是否能击杀目标。 |
| 敌人威胁 | 清晰显示近战危险与远程瞄准线。 |
| 属性状态 | 保持 HP、构成、蚀变、转换能量和变身库存可见。 |
| 回合节奏 | 让玩家行动、敌人行动和棋盘更新具有清晰区分。 |
| 变身目标 | 预览合法目标与移动后的危险。 |

好的反馈应降低规则困惑，同时保留决策压力。

## 音频方向

音频用于支撑棋盘的战术节奏。

- 移动应精确、明确、具有步进感。
- 属性增长应让地形显得有价值。
- 阈值击杀应果断有力。
- 攻击失败应清楚表达玩家尚未准备好。
- 地形转换与棋子变身应听起来像稀缺且有意图的工具。
- 敌人瞄准与远程攻击应制造可读的预兆。

目标不是持续制造噪音，而是为战术节点配音：**准备、确认、击杀、转换、生存**。

## 视觉方向

### 构成

构成视觉强调几何、硬边、模块化形状、结构线条和受控空间。

### 蚀变

蚀变视觉强调高饱和、流动、侵蚀、噪声、非对称和不稳定边界。

### 棋子识别

玩家和变身形态需要在俯视战术视角下保持可读。轮廓清晰度优先于细节复杂度。

## 当前玩法范围

当前玩法设计基线包含：

- 步进式四方向移动，
- 构成、蚀变、极简和障碍地形，
- 双属性与压制，
- 阈值击杀与阵营免疫，
- 近战敌人与远程敌人，
- 友伤与对立阵营冲突，
- 3x3 地形转换能量，
- HP 恢复与棋子拾取，
- Knight、Bishop、Rook、Queen 四种变身，
- 程序化 Run 楼层，
- 六个固定教学关卡，
- 战斗预览、HUD 反馈、菜单流程、音频反馈和 Run 记录。

后续设计空间包括更完整的叙事包装、永久成长、定义构筑的遗物、商店、精英敌人、Boss，以及更多样的楼层目标。

## 设计总结

Construct & Corrode 的核心是把移动变成准备。玩家通过阅读地形、构建正确属性、预判敌人行为，并选择最关键的一步，把危险棋盘转化为战术机会。
