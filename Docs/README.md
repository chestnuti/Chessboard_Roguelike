# 技术文档索引

本目录按功能模块组织项目技术说明。优先从模块索引进入，再阅读具体系统文档。

## 模块导航

| 模块 | 入口 | 内容范围 |
| --- | --- | --- |
| 核心棋盘与回合 | [01_CoreGrid](01_CoreGrid/README.md) | 棋盘生成、格子数据、移动接口、输入、回合锁 |
| 地块属性与转换能量 | [02_TileAttributesAndEnergy](02_TileAttributesAndEnergy/README.md) | 地块属性效果、玩家双属性值、HUD、压制状态、击杀能量、3x3 地块转换、可拾取道具 |
| 战斗与敌人 | [03_CombatAndEnemies](03_CombatAndEnemies/README.md) | 近战攻击、敌人属性、免疫与阈值击杀、敌人管理器、敌方行动调度、Combat Preview |
| 项目规划 | [04_ProjectPlanning](04_ProjectPlanning/README.md) | GDD、技术实现需求、开发任务清单、阶段划分、里程碑 |
| 教学关卡 | [05_TutorialLevels](05_TutorialLevels/README.md) | 固定 10x10 教学地图、教学 DataAsset、5 张 Map 资产和验证脚本 |
| 变身棋子系统 | [06_TransformSystem](06_TransformSystem/README.md) | 变身棋子背包、4 槽轮盘、鼠标选格移动、边缘滚屏、一次性变身消耗规则 |
| 音频系统 | [07_AudioSystem](07_AudioSystem/README.md) | BGM 淡入淡出、玩家/敌人/关卡/UI 音效、DataAsset 配置、随机音效池 |

## 推荐阅读路径

1. 先读 [核心棋盘与回合](01_CoreGrid/README.md)，理解棋盘、移动和回合基础。
2. 再读 [地块属性与转换能量](02_TileAttributesAndEnergy/README.md)，理解地块变化和属性资源。
3. 接着读 [战斗与敌人](03_CombatAndEnemies/README.md)，理解攻击、敌人行动和相关边界。
4. 需要验证规则教学时，读 [教学关卡](05_TutorialLevels/README.md)，了解固定地图和教学资产。
5. 需要接入 BGM 或事件音效时，读 [音频系统](07_AudioSystem/README.md)，了解配置资产和触发点。
6. 最后读 [项目规划](04_ProjectPlanning/README.md)，对照 GDD、技术实现需求、当前功能和后续任务。

## 维护约定

- 新增技术说明时，优先归入已有功能模块。
- 跨模块系统放在主要责任模块中，并在相关模块索引中加入反向链接。
- 文档内引用其他文档时使用相对链接，避免目录移动后链接失效。
