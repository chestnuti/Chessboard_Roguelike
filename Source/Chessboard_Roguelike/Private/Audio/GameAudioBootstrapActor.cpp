#include "Audio/GameAudioBootstrapActor.h"

#include "Audio/GameAudioSubsystem.h"
#include "Engine/GameInstance.h"

AGameAudioBootstrapActor::AGameAudioBootstrapActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AGameAudioBootstrapActor::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	UGameAudioSubsystem* AudioSubsystem = GameInstance ? GameInstance->GetSubsystem<UGameAudioSubsystem>() : nullptr;
	if (!AudioSubsystem)
	{
		return;
	}

	if (AudioSettings)
	{
		AudioSubsystem->SetAudioSettings(AudioSettings);
	}

	switch (InitialBGM)
	{
	case EGameBGMType::MainMenu:
		AudioSubsystem->PlayMainMenuBGM(InitialBGMFadeTime);
		break;
	case EGameBGMType::Level:
		AudioSubsystem->PlayLevelBGM(InitialBGMFadeTime);
		break;
	default:
		break;
	}

	if (bPlayLevelStartSFX)
	{
		AudioSubsystem->PlayLevelStartSFX();
	}
}
