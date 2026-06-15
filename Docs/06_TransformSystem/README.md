# 变身棋子系统

本模块记录玩家拾取、储存并消耗变身棋子的技术实现。变身棋子是一次性战术移动资源：默认状态下玩家仍使用 WASD 四向移动；按住 G 打开 4 槽轮盘，点击棋子后进入鼠标选格模式，成功执行一次变身移动后消耗 1 个对应棋子并恢复默认形态。

## 文档

| 文档 | 说明 |
| --- | --- |
| [Transform_SystemGuide.md](Transform_SystemGuide.md) | 变身棋子数据、背包、拾取物、轮盘 UI、目标选择、边缘滚屏和取消规则的完整技术说明。 |

## 相关模块

- [核心棋盘与回合](../01_CoreGrid/README.md)：变身移动最终仍通过 `AGridPawn`、`AGridManager` 和 `ATurnManager` 结算。
- [地块属性与转换能量](../02_TileAttributesAndEnergy/README.md)：拾取物系统负责把变身棋子加入玩家背包。
- [战斗与敌人](../03_CombatAndEnemies/README.md)：变身移动选择敌人格时复用现有近战结算与击杀流程。
