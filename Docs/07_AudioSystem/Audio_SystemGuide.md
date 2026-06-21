# 音频系统技术说明

## 目标

音频系统统一管理游戏 BGM、玩家事件音效、敌人事件音效、关卡流程音效和 UI 音效。调用方只需要发出“播放某类事件音效”或“切换某类 BGM”的请求，具体音频资源由 DataAsset 配置，避免资源引用散落在关卡蓝图、角色蓝图和战斗逻辑中。

当前实现支持：

- 主菜单 BGM 与关卡 BGM 分离配置。
- BGM 切换使用淡出淡入，BGM 播放结束后会自动循环。
- 所有事件音效使用持久 AudioComponent 播放，不会因为关卡切换而被 Flush 掐断。
- 玩家、敌人、关卡流程、UI 事件音效统一由 `UGameAudioSubsystem` 播放。
- 每个事件音效槽位使用 `FGameSoundSet`，可配置多个 `USoundBase`，触发时随机选择一个有效音效播放。
- 玩家“准备使用能量”音效支持手动开始和停止，适合长按空格蓄力/准备音效。
- 每类敌人可以指定独立的 `UEnemyAudioProfileDataAsset`。

## 核心类型

| 类型 | 路径 | 职责 |
| --- | --- | --- |
| `UGameAudioSubsystem` | `Source/Chessboard_Roguelike/Public/Audio/GameAudioSubsystem.h` | 全局音频服务，管理 BGM、事件音效和可控音效。 |
| `UGameAudioSettingsDataAsset` | `Source/Chessboard_Roguelike/Public/Audio/GameAudioSettingsDataAsset.h` | 全局音频配置，包含 BGM、玩家音效、关卡音效和 UI 音效。 |
| `UEnemyAudioProfileDataAsset` | `Source/Chessboard_Roguelike/Public/Audio/EnemyAudioProfileDataAsset.h` | 敌人音频配置，按敌人类型配置近战/远程攻击、瞄准和死亡音效。 |
| `FGameSoundSet` | `Source/Chessboard_Roguelike/Public/Audio/GameAudioTypes.h` | 随机音效池，内部持有 `TArray<TObjectPtr<USoundBase>> Sounds`。 |
| `AGameAudioBootstrapActor` | `Source/Chessboard_Roguelike/Public/Audio/GameAudioBootstrapActor.h` | 关卡内音频初始化入口，用于设置音频配置并播放初始 BGM。 |
| `UGameAudioBlueprintLibrary` | `Source/Chessboard_Roguelike/Public/Audio/GameAudioBlueprintLibrary.h` | 蓝图辅助函数，提供获取 `UGameAudioSubsystem` 的入口。 |

## 随机音效池

事件音效槽位使用 `FGameSoundSet`：

```cpp
USTRUCT(BlueprintType)
struct FGameSoundSet
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Audio")
    TArray<TObjectPtr<USoundBase>> Sounds;
};
```

每次播放事件音效时，`UGameAudioSubsystem` 会过滤空元素，然后从有效音效中随机选择一个播放。`Sounds` 中可以混用 `SoundWave`、`SoundCue` 或 `MetaSound`。

## BGM 管理

BGM 使用单个 `USoundBase` 槽位：

- `MainMenuBGM`
- `LevelBGM`

主要接口：

```cpp
void PlayMainMenuBGM(float FadeTime = -1.0f);
void PlayLevelBGM(float FadeTime = -1.0f);
void PlayBGM(USoundBase* Music, float FadeTime = -1.0f);
void StopBGM(float FadeTime = -1.0f);
```

当 `FadeTime < 0` 时，系统使用 `UGameAudioSettingsDataAsset::DefaultBGMFadeTime`。BGM 由 `UGameAudioSubsystem` 持有，不挂在关卡 Actor 生命周期上。

## 玩家音效

| 事件 | 触发位置 | 音效槽 |
| --- | --- | --- |
| 玩家攻击 | `AGridPawn::ResolveEnemyMeleeAttack` | `PlayerAudio.Attack` |
| 玩家击杀 | `AGridPawn::ResolveEnemyMeleeAttack` 击杀成功时 | `PlayerAudio.Kill` |
| 玩家变身 | `AGridPawn::TryTransformMoveToCoord` 应用变身视觉前 | `PlayerAudio.Transform` |
| 获得转换能量 | `UConversionEnergyComponent::GrantConversionEnergy` | `PlayerAudio.GainEnergy` |
| 准备使用转换能量 | 蓝图手动调用 | `PlayerAudio.PrepareUseEnergy` |
| 使用转换能量 | `UConversionEnergyComponent::ConsumeConversionEnergy` | `PlayerAudio.UseEnergy` |
| 切换转换能量类型 | `UConversionEnergyComponent::SetHeldConversionEnergyType` 成功切换后 | `PlayerAudio.SwitchEnergy` |
| 获得属性值 | `UPlayerAttributeComponent::ApplyTileAttributeDelta` 属性增加时 | `PlayerAudio.GainAttribute` |
| 属性满值 | `UPlayerAttributeComponent::ApplyTileAttributeDelta` 首次达到满值时 | `PlayerAudio.AttributeFull` |
| 玩家死亡 | `UPlayerAttributeComponent::ApplyHealthDamage` 生命降至 0 时 | `PlayerAudio.Death` |

### 准备使用能量音效

`PrepareUseEnergy` 是一个可控音效槽位，适合长按空格准备使用能量时播放。它不会自动由 C++ 输入逻辑触发，需要在蓝图中按状态调用：

```text
Space Pressed
-> Get Game Audio Subsystem
-> Start Player Prepare Use Energy SFX
```

```text
Space Released / Canceled
-> Get Game Audio Subsystem
-> Stop Player Prepare Use Energy SFX
```

如果成功释放能量，可以先停止准备音，再播放真正的释放音：

```text
Stop Player Prepare Use Energy SFX
-> Play Player Use Energy SFX
```

相关接口：

```cpp
void StartPlayerPrepareUseEnergySFX(float FadeInTime = 0.0f, bool bRestartIfPlaying = false);
void StopPlayerPrepareUseEnergySFX(float FadeOutTime = 0.0f);
bool IsPlayerPrepareUseEnergySFXPlaying() const;
```

说明：

- `FadeInTime` 大于 0 时会淡入播放。
- `FadeOutTime` 大于 0 时会淡出停止。
- `bRestartIfPlaying=false` 时，长按过程中重复调用 Start 不会重播。
- 如果希望准备音一直持续，请在音频资源自身使用循环设置，例如循环 `SoundCue` 或 `MetaSound`。

## 敌人音效

| 事件 | 触发位置 | 音效槽 |
| --- | --- | --- |
| 近战敌人攻击 | `AGridEnemyPawn::ExecuteBasicTurn_Implementation` 邻接攻击前 | `AudioProfile.MeleeAttack` |
| 近战敌人死亡 | `AGridEnemyPawn::Kill` | `AudioProfile.MeleeDeath` |
| 远程敌人瞄准 | `AGridEnemyPawn::EnterRangedAimMode` | `AudioProfile.RangedAim` |
| 远程敌人攻击 | `AGridEnemyPawn::ResolvePendingRangedAttack` 和 `AGridEnemyManager::ResolvePendingRangedAttacks` | `AudioProfile.RangedAttack` |
| 远程敌人死亡 | `AGridEnemyPawn::Kill` | `AudioProfile.RangedDeath` |

每个 `AGridEnemyPawn` 都有 `AudioProfile` 字段。不同敌人蓝图可以指定不同的敌人音频 Profile。

## 关卡流程音效

| 事件 | 触发位置 | 音效槽 |
| --- | --- | --- |
| 关卡开始 | `ADungeonRunManager::StartLevel` 成功生成并初始化后 | `LevelAudio.LevelStart` |
| 关卡切换 | `ADungeonRunManager::StartLevel` 目标层数变化时 | `LevelAudio.LevelTransition` |
| 关卡胜利 | `ATurnManager::SetTurnState(ETurnState::Victory)` | `LevelAudio.LevelVictory` |
| 关卡失败 | `ATurnManager::SetTurnState(ETurnState::Defeat)` | `LevelAudio.LevelFailed` |

## UI 音效

Widget 蓝图可以在按钮事件中调用：

```text
Get Game Audio Subsystem
-> Play UI Hover SFX
```

```text
Get Game Audio Subsystem
-> Play UI Click SFX
```

推荐绑定：

| UI 事件 | 音效槽 |
| --- | --- |
| `OnHovered` | `UIAudio.Hover` |
| `OnClicked` | `UIAudio.Click` |

## 配置流程

1. 打开 `/Game/Data/Audio/DA_GameAudioSettings`。
2. 给 `MainMenuBGM` 和 `LevelBGM` 指定 BGM 资源。
3. 展开 `PlayerAudio`、`LevelAudio`、`UIAudio`。
4. 在每个事件槽位的 `Sounds` 数组中添加一个或多个 `USoundBase`。
5. 若需要长按空格准备音，配置 `PlayerAudio.PrepareUseEnergy`。
6. 若需要切换能量类型反馈，配置 `PlayerAudio.SwitchEnergy`。
7. 打开 `/Game/Data/Audio/Enemy` 下的敌人 Profile。
8. 为每个敌人事件槽位的 `Sounds` 数组添加对应音效。
9. 确认主菜单地图引用 `BP_GameAudioBootstrap_MainMenu`。
10. 确认关卡地图引用 `BP_GameAudioBootstrap_Level`。
11. 确认敌人蓝图上的 `AudioProfile` 指向对应敌人 Profile。

## 蓝图访问

任意蓝图可以通过以下方式访问音频系统：

```text
Get Game Audio Subsystem
-> 调用 PlayPlayerAttackSFX / PlayLevelVictorySFX / PlayUIClickSFX 等接口
```

设计上建议只让高层系统或明确事件源调用音频接口：

- 玩家组件调用玩家事件音效。
- 敌人 Pawn 调用敌人事件音效。
- `DungeonRunManager` 和 `TurnManager` 调用关卡流程音效。
- Widget 调用 UI 音效。
- 玩家输入蓝图调用 `StartPlayerPrepareUseEnergySFX` 和 `StopPlayerPrepareUseEnergySFX`。

## 注意事项

- `Sounds` 数组为空时不会播放音效。
- 事件音效会跨关卡切换继续播放，播放完成后由 `UGameAudioSubsystem` 自动清理组件。
- 世界音效需要提供合理世界坐标。
- 如果需要音量分类、静音、混音器或设置菜单，后续应接入 `SoundClass`、`SoundMix` 或项目统一设置系统。
- 如果需要权重随机或避免连续重复，建议扩展 `FGameSoundSet`，不要在调用点手写随机逻辑。
