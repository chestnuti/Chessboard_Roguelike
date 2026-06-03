# 战斗与敌人

本模块记录玩家近战攻击、敌人属性、免疫与阈值击杀、敌人对玩家造成伤害、敌人跨阵营友伤、敌方行动调度和敌人管理器。

## 文档

| 文档 | 说明 |
| --- | --- |
| [CombatAndEnemy_SystemGuide.md](CombatAndEnemy_SystemGuide.md) | 近战攻击、远程敌人瞄准攻击、敌人阈值击杀与跨阵营友伤系统说明，覆盖玩家攻击、敌人攻击玩家、敌人基础类型、伤害结构、CombatResolverComponent、GridPawn 战斗入口和调试点。 |
| [EnemyManager_SystemGuide.md](EnemyManager_SystemGuide.md) | 敌人管理器系统说明，覆盖 GridEnemyManager、EnemyPawn 行动接口、远程攻击调度、蓝图 AI 覆写、友伤接入、失败中断、关卡使用方式和当前 AI 规则。 |
| [CombatCamera_SystemGuide.md](CombatCamera_SystemGuide.md) | 敌人死亡时的战斗相机聚焦系统说明，覆盖死亡事件、EnemyManager 转发、PlayerController 相机组件和调试点。 |

## 相关模块

- [核心棋盘与回合](../01_CoreGrid/README.md)：敌人占格、移动和行动回合依赖棋盘与 TurnManager。
- [地块属性与转换能量](../02_TileAttributesAndEnergy/README.md)：战斗击杀会触发地块转换能量，敌人行动会受玩家属性满值压制影响。
