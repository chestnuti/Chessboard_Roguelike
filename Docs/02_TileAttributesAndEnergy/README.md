# 地块属性与转换能量

本模块记录地块属性变化、玩家属性值、压制状态、击杀能量和地块转换相关系统。

## 文档

| 文档 | 说明 |
| --- | --- |
| [TileAttributeEffect_SystemGuide.md](TileAttributeEffect_SystemGuide.md) | 地块属性变化与双属性值系统说明，覆盖地块类型、玩家属性组件、地块效果解析、配置资产、GridManager 接口、材质刷新和属性 HUD。 |
| [SuppressionAndConversionEnergy_SystemGuide.md](SuppressionAndConversionEnergy_SystemGuide.md) | 压制状态与地块转换能量系统说明，覆盖玩家满值压制、敌人阵营判定、击杀能量事件、蓝图推荐实现和 3x3 地块转换。 |

## 相关模块

- [核心棋盘与回合](../01_CoreGrid/README.md)：地块转换和属性效果依赖棋盘坐标、地块数据和玩家当前位置。
- [战斗与敌人](../03_CombatAndEnemies/README.md)：压制、击杀能量和敌人阵营判定与战斗和敌人行动紧密相关。
