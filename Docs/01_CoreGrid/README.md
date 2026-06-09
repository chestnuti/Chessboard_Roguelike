# 核心棋盘与回合

本模块记录棋盘原型的基础运行链路，包括棋盘生成、格子数据、玩家移动、输入配置和回合锁。

## 文档

| 文档 | 说明 |
| --- | --- |
| [GridPrototype_UserManual.md](GridPrototype_UserManual.md) | 棋盘移动原型使用手册，覆盖 GridSettings、Tile 数据、GridManager 接口、Pawn 移动接口、PlayerController 输入、TurnManager 回合锁和基础调试建议。 |

## 相关模块

- [地块属性与转换能量](../02_TileAttributesAndEnergy/README.md)：地块类型变化、属性效果和转换能量依赖核心棋盘坐标与地块接口。
- [可拾取道具系统](../02_TileAttributesAndEnergy/Pickup_SystemGuide.md)：拾取物使用棋盘坐标和奖励候选点生成，但不占用 GridManager 的 Occupant 状态。
- [战斗与敌人](../03_CombatAndEnemies/README.md)：攻击和敌人行动依赖棋盘占格、移动和回合调度。
- [教学关卡](../05_TutorialLevels/README.md)：固定 10x10 教学地图复用 `AGridManager::ApplyTileLayout()` 和回合初始化流程。
