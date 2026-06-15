# 地块属性与转换能量

本模块记录地块属性变化、玩家属性值、压制状态、击杀能量和地块转换相关系统。

## 文档

| 文档 | 说明 |
| --- | --- |
| [TileAttributeEffect_SystemGuide.md](TileAttributeEffect_SystemGuide.md) | 地块属性变化与双属性值系统说明，覆盖地块类型、玩家属性组件、地块效果解析、配置资产、GridManager 接口、材质刷新和属性 HUD。 |
| [SuppressionAndConversionEnergy_SystemGuide.md](SuppressionAndConversionEnergy_SystemGuide.md) | 压制状态与地块转换能量系统说明，覆盖玩家满值压制、敌人阵营判定、击杀能量事件、蓝图推荐实现和 3x3 地块转换。 |
| [Pickup_SystemGuide.md](Pickup_SystemGuide.md) | 可拾取道具系统说明，覆盖回血道具、PickupManager、PCG 奖励候选生成、蓝图资产和后续道具扩展方式。 |
| [../06_TransformSystem/Transform_SystemGuide.md](../06_TransformSystem/Transform_SystemGuide.md) | 变身棋子系统说明，覆盖变身背包、4 槽轮盘、鼠标选格、边缘滚屏和变身拾取物接入。 |

## 相关模块

- [核心棋盘与回合](../01_CoreGrid/README.md)：地块转换和属性效果依赖棋盘坐标、地块数据和玩家当前位置。
- [战斗与敌人](../03_CombatAndEnemies/README.md)：压制、击杀能量和敌人阵营判定与战斗和敌人行动紧密相关。
- [教学关卡](../05_TutorialLevels/README.md)：关卡 1 验证地块属性变化，关卡 3 验证击杀获得转换能量和地块转换，关卡 5 验证阵营压制系统。
- [变身棋子系统](../06_TransformSystem/README.md)：变身棋子由拾取物加入背包，但移动规则和轮盘交互由独立变身模块维护。
