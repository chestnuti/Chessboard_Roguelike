# 音频系统

本模块记录游戏 BGM、事件音效、敌人音效配置和 UI 音效接入方式。

## 文档

| 文档 | 说明 |
| --- | --- |
| [Audio_SystemGuide.md](Audio_SystemGuide.md) | 音频系统技术说明，覆盖 `UGameAudioSubsystem`、音频 DataAsset、随机音效池、BGM 淡入淡出、玩家/敌人/关卡/UI 音效触发点和配置流程。 |

## 相关模块

- [核心棋盘与回合](../01_CoreGrid/README.md)：胜利、失败等回合状态变化会触发关卡流程音效。
- [战斗与敌人](../03_CombatAndEnemies/README.md)：玩家攻击、击杀、敌人攻击、瞄准和死亡会触发战斗音效。
- [地块属性与转换能量](../02_TileAttributesAndEnergy/README.md)：能量获得、能量使用、属性增长和属性满值会触发玩家资源音效。
- [变身棋子系统](../06_TransformSystem/README.md)：变身移动会触发玩家变身音效。
