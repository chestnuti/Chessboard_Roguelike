# UI 菜单系统技术说明

## 目标

UI 菜单系统负责提供两个运行时菜单：

- Setting 菜单：可从主菜单或 Pause 菜单打开，提供 BGM 与 SFX 两个音量滑条。
- Pause 菜单：正式关和教学关中由 ESC 触发，覆盖当前画面，并提供 Resume、Back、Setting、Main Menu、Quit Game 入口。

当前实现遵循“Widget 不直接引用 `AGridPlayerController`”的原则。Widget 只广播事件，调用方负责监听事件、关闭菜单、切换输入模式或执行关卡流程。

## 核心类型

| 类型或资产 | 路径 | 职责 |
| --- | --- | --- |
| `USettingsMenuWidget` | `Source/Chessboard_Roguelike/Public/UI/SettingsMenuWidget.h` | Setting 菜单基类，绑定音量滑条和返回按钮。 |
| `WBP_SettingsMenu` | `Content/UI/WBP_SettingsMenu.uasset` | Setting 菜单 UMG 资产。 |
| `UPauseMenuWidget` | `Source/Chessboard_Roguelike/Public/UI/PauseMenuWidget.h` | Pause 菜单基类，绑定按钮并广播请求事件。 |
| `WBP_PauseMenu` | `Content/UI/WBP_PauseMenu.uasset` | Pause 菜单 UMG 资产。 |
| `UPlayerAttributeHUDWidget` | `Source/Chessboard_Roguelike/Public/UI/PlayerAttributeHUDWidget.h` | 当前主菜单 HUD 基类，提供 `SettingButton` 和 `OpenSettingsMenu()`。 |
| `AGridPlayerController` | `Source/Chessboard_Roguelike/Public/Player/GridPlayerController.h` | 创建 Pause 菜单并把 Widget 事件转发为 Controller 可监听事件。 |
| `UGameAudioSubsystem` | `Source/Chessboard_Roguelike/Public/Audio/GameAudioSubsystem.h` | 保存并应用 BGM/SFX 音量。 |

## Setting 菜单

`USettingsMenuWidget` 的绑定控件如下：

| 控件名 | 类型 | 说明 |
| --- | --- | --- |
| `BGMSlider` | `USlider` | 控制 `UGameAudioSubsystem::SetBGMVolume()`。 |
| `SFXSlider` | `USlider` | 控制 `UGameAudioSubsystem::SetSFXVolume()`。 |
| `BGMVolumeText` | `UTextBlock` | 显示 BGM 百分比。 |
| `SFXVolumeText` | `UTextBlock` | 显示 SFX 百分比。 |
| `BackButton` | `UButton` | 点击后广播 `OnSettingsBackRequested`。 |

Setting 菜单不主动关闭自己。调用方应监听 `OnSettingsBackRequested`，然后决定是关闭设置菜单、返回 Pause 菜单，还是回到主菜单视图。

音量范围使用 `0.0` 到 `1.0`。菜单构造时会调用 `RefreshFromAudioSettings()`，从 `UGameAudioSubsystem` 读取当前值并同步滑条显示。

## 主菜单接入

当前主菜单基类是 `UPlayerAttributeHUDWidget`，提供以下 Setting 相关接口和绑定：

| 名称 | 类型 | 说明 |
| --- | --- | --- |
| `SettingButton` | `UButton` | 主菜单上的设置按钮，UMG 中同名控件会自动绑定。 |
| `SettingsMenuClass` | `TSubclassOf<USettingsMenuWidget>` | 默认查找 `/Game/UI/WBP_SettingsMenu`。 |
| `OpenSettingsMenu()` | `UFUNCTION` | 创建并显示 Setting 菜单。 |
| `CloseSettingsMenu()` | `UFUNCTION` | 移除当前 Setting 菜单。 |

如果主菜单点击 Setting 没有弹出菜单，优先检查可见按钮是否真的属于 `WBP_PlayerAttributeHUD`，并且控件名是否为 `SettingButton`。如果实际按钮位于关卡蓝图或另一个 Widget 中，需要在对应蓝图里直接调用 `OpenSettingsMenu()`，或把该按钮移动/重命名到 `WBP_PlayerAttributeHUD` 的 Widget Tree 中。

## Pause 菜单

`UPauseMenuWidget` 的绑定控件如下：

| 控件名 | 类型 | 说明 |
| --- | --- | --- |
| `ResumeButton` | `UButton` | 点击后关闭 Pause 菜单并广播 `OnResumeRequested`。 |
| `BackButton` | `UButton` | 点击后广播 `OnBackRequested`。 |
| `Back` | `UButton` | 兼容已有 WBP 命名，同样广播 `OnBackRequested`。 |
| `SettingButton` | `UButton` | 点击后广播 `OnSettingsRequested`。 |
| `MainMenuButton` | `UButton` | 点击后关闭 Pause 菜单并广播 `OnMainMenuRequested`。 |
| `QuitGameButton` | `UButton` | 点击后广播 `OnQuitGameRequested`。 |

`UPauseMenuWidget` 对外广播以下事件：

| 事件 | 说明 |
| --- | --- |
| `OnResumeRequested` | 请求恢复游戏。当前只表示按钮意图。 |
| `OnBackRequested` | 请求返回上一级或关闭当前 Pause 菜单。 |
| `OnSettingsRequested` | 请求打开 Setting 菜单。 |
| `OnMainMenuRequested` | 请求返回主菜单。当前未实现实际跳转。 |
| `OnQuitGameRequested` | 请求退出游戏。当前未实现实际退出。 |

## Grid Player Controller 事件链

`AGridPlayerController::ShowPauseMenu()` 会创建 `PauseMenuClass`，并把 `UPauseMenuWidget` 的事件绑定到 Controller 内部处理函数，再转发为 Controller 上的 BlueprintAssignable 事件：

| Widget 事件 | Controller 事件 |
| --- | --- |
| `OnResumeRequested` | `OnPauseResumeRequested` |
| `OnBackRequested` | `OnPauseBackRequested` |
| `OnSettingsRequested` | `OnPauseSettingsRequested` |
| `OnMainMenuRequested` | `OnPauseMainMenuRequested` |
| `OnQuitGameRequested` | `OnPauseQuitGameRequested` |

蓝图如果希望在 Grid Player Controller 中读取 Pause 菜单点击结果，应监听 Controller 上的事件，而不是让 Widget 直接拿 Controller 引用。

重要限制：如果蓝图直接 `Create Widget WBP_PauseMenu` 并 `Add To Viewport`，就会绕过 `AGridPlayerController::ShowPauseMenu()` 中的绑定逻辑，Controller 的 `OnPauseSettingsRequested` 等事件不会自动触发。此时应改为调用 `ShowPauseMenu()`，或在蓝图创建 Widget 后手动绑定 Widget 事件。

## 当前 Resume 状态

当前项目还没有完整的“关卡 Resume”流程。已有内容是：

- `ResumeButton` 绑定和 `OnPauseResumeRequested` 事件入口。
- `ClosePauseMenu()` 可移除 Pause 菜单。
- 蓝图中存在暂停输入相关资源和变量迹象。

尚未形成完整闭环的内容包括：

- `SetGamePaused(true/false)` 或等价的全局暂停/恢复。
- 恢复后输入模式切回游戏输入。
- 恢复后隐藏鼠标、解除 UI Focus。
- Pause 与 Setting 叠层之间的统一返回策略。

因此当前 `ResumeButton` 更准确地说是“恢复请求事件”，还不是完整关卡恢复功能。正式接入时建议由 `AGridPlayerController` 或关卡流程控制蓝图统一处理暂停状态、输入模式和菜单关闭。

## 音量持久化

BGM/SFX 音量当前保存在 `UGameAudioSubsystem` 的运行时字段中，并立即应用到当前 BGM 与事件音效组件。尚未写入 SaveGame 或用户设置文件。需要跨启动保存时，应在 Setting 菜单滑条变化或返回时写入统一设置系统，并在音频初始化时恢复。

## 蓝图排查清单

1. 主菜单 Setting 不弹出：确认可见按钮是 `WBP_PlayerAttributeHUD` 内名为 `SettingButton` 的控件，或确认实际按钮调用了 `OpenSettingsMenu()`。
2. Pause 菜单 Setting 事件不触发：确认 Pause 菜单由 `AGridPlayerController::ShowPauseMenu()` 创建，或蓝图手动绑定了 `UPauseMenuWidget::OnSettingsRequested`。
3. BackButton 不触发：确认 UMG 控件名为 `BackButton` 或兼容名 `Back`，且 Widget 父类为 `UPauseMenuWidget`。
4. 音量滑条无效果：确认当前世界可以访问 `UGameAudioSubsystem`，并且 BGM/SFX 通过该 Subsystem 播放。
5. Resume 只关菜单不恢复游戏：这是当前实现边界，需要后续补充暂停状态和输入模式恢复逻辑。

