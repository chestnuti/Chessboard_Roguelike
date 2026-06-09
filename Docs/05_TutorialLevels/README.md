# 教学关卡

本模块记录固定教学关卡系统。教学关卡不使用 PCG 生成地图，而是通过 `UTutorialLevelSet` DataAsset 提供 10x10 固定地块布局、玩家出生点和固定敌人配置。

## 文档

| 文档 | 说明 |
| --- | --- |
| [TutorialLevels_SystemGuide.md](TutorialLevels_SystemGuide.md) | 教学关卡系统说明，覆盖固定布局数据、DungeonRunManager 教学模式、5 个 Map 资产、DataAsset 配置、敌人蓝图绑定和验证脚本。 |

## 相关模块

- [核心棋盘与回合](../01_CoreGrid/README.md)：教学关卡复用 `AGridManager::ApplyTileLayout()`、玩家初始化和回合状态。
- [地块属性与转换能量](../02_TileAttributesAndEnergy/README.md)：关卡 1 和关卡 3 分别验证地块属性变化与转换能量。
- [战斗与敌人](../03_CombatAndEnemies/README.md)：关卡 2 和关卡 4 验证敌人免疫、阈值击杀、远程攻击与友伤。
