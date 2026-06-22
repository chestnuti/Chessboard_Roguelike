#include "Audio/GameAudioSubsystem.h"

#include "Audio/EnemyAudioProfileDataAsset.h"
#include "Audio/GameAudioSettingsDataAsset.h"
#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UGameAudioSubsystem::Deinitialize()
{
	if (CurrentBGMComponent)
	{
		CurrentBGMComponent->OnAudioFinished.RemoveDynamic(this, &UGameAudioSubsystem::HandleBGMFinished);
		CurrentBGMComponent->Stop();
		CurrentBGMComponent = nullptr;
	}

	for (UAudioComponent* AudioComponent : ActivePersistentSFXComponents)
	{
		if (!AudioComponent)
		{
			continue;
		}

		AudioComponent->OnAudioFinished.RemoveDynamic(this, &UGameAudioSubsystem::HandlePersistentSFXFinished);
		AudioComponent->Stop();
		AudioComponent->DestroyComponent();
	}
	ActivePersistentSFXComponents.Reset();

	PlayerPrepareUseEnergyComponent = nullptr;
	CurrentBGM = nullptr;
	AudioSettings = nullptr;

	Super::Deinitialize();
}

void UGameAudioSubsystem::SetAudioSettings(UGameAudioSettingsDataAsset* InAudioSettings)
{
	AudioSettings = InAudioSettings;
}

UGameAudioSettingsDataAsset* UGameAudioSubsystem::GetAudioSettings() const
{
	return AudioSettings;
}

void UGameAudioSubsystem::SetBGMVolume(float InVolume)
{
	BGMVolume = FMath::Clamp(InVolume, 0.0f, 1.0f);
	if (CurrentBGMComponent)
	{
		CurrentBGMComponent->SetVolumeMultiplier(BGMVolume);
	}
}

float UGameAudioSubsystem::GetBGMVolume() const
{
	return BGMVolume;
}

void UGameAudioSubsystem::SetSFXVolume(float InVolume)
{
	SFXVolume = FMath::Clamp(InVolume, 0.0f, 1.0f);
	for (UAudioComponent* AudioComponent : ActivePersistentSFXComponents)
	{
		if (AudioComponent)
		{
			AudioComponent->SetVolumeMultiplier(SFXVolume);
		}
	}
}

float UGameAudioSubsystem::GetSFXVolume() const
{
	return SFXVolume;
}

void UGameAudioSubsystem::PlayMainMenuBGM(float FadeTime)
{
	PlayBGM(AudioSettings ? AudioSettings->MainMenuBGM.Get() : nullptr, FadeTime);
}

void UGameAudioSubsystem::PlayLevelBGM(float FadeTime)
{
	PlayBGM(AudioSettings ? AudioSettings->LevelBGM.Get() : nullptr, FadeTime);
}

void UGameAudioSubsystem::PlayBGM(USoundBase* Music, float FadeTime)
{
	if (!Music || CurrentBGM == Music)
	{
		if (Music && CurrentBGMComponent && !CurrentBGMComponent->IsPlaying())
		{
			CurrentBGMComponent->Play(0.0f);
		}
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float ActualFadeTime = ResolveFadeTime(FadeTime);
	if (CurrentBGMComponent)
	{
		CurrentBGMComponent->OnAudioFinished.RemoveDynamic(this, &UGameAudioSubsystem::HandleBGMFinished);
		CurrentBGMComponent->bAutoDestroy = true;
		CurrentBGMComponent->FadeOut(ActualFadeTime, 0.0f);
		CurrentBGMComponent = nullptr;
	}

	CurrentBGMComponent = UGameplayStatics::CreateSound2D(
		World,
		Music,
		BGMVolume,
		1.0f,
		0.0f,
		nullptr,
		true,
		false);

	if (!CurrentBGMComponent)
	{
		CurrentBGM = nullptr;
		return;
	}

	CurrentBGM = Music;
	CurrentBGMComponent->OnAudioFinished.AddDynamic(this, &UGameAudioSubsystem::HandleBGMFinished);
	CurrentBGMComponent->FadeIn(ActualFadeTime);
}

void UGameAudioSubsystem::StopBGM(float FadeTime)
{
	if (!CurrentBGMComponent)
	{
		CurrentBGM = nullptr;
		return;
	}

	CurrentBGMComponent->OnAudioFinished.RemoveDynamic(this, &UGameAudioSubsystem::HandleBGMFinished);
	CurrentBGMComponent->bAutoDestroy = true;
	CurrentBGMComponent->FadeOut(ResolveFadeTime(FadeTime), 0.0f);
	CurrentBGMComponent = nullptr;
	CurrentBGM = nullptr;
}

USoundBase* UGameAudioSubsystem::GetCurrentBGM() const
{
	return CurrentBGM;
}

void UGameAudioSubsystem::HandleBGMFinished()
{
	if (!CurrentBGM || !CurrentBGMComponent)
	{
		return;
	}

	CurrentBGMComponent->Play(0.0f);
}

void UGameAudioSubsystem::HandlePersistentSFXFinished()
{
	PruneFinishedPersistentSFXComponents();
}

void UGameAudioSubsystem::PlayPlayerAttackSFX()
{
	if (AudioSettings)
	{
		Play2DSFX(AudioSettings->PlayerAudio.Attack);
	}
}

void UGameAudioSubsystem::PlayPlayerKillSFX()
{
	if (AudioSettings)
	{
		Play2DSFX(AudioSettings->PlayerAudio.Kill);
	}
}

void UGameAudioSubsystem::PlayPlayerTransformSFX()
{
	if (AudioSettings)
	{
		Play2DSFX(AudioSettings->PlayerAudio.Transform);
	}
}

void UGameAudioSubsystem::PlayPlayerGainEnergySFX()
{
	if (AudioSettings)
	{
		Play2DSFX(AudioSettings->PlayerAudio.GainEnergy);
	}
}

void UGameAudioSubsystem::StartPlayerPrepareUseEnergySFX(float FadeInTime, bool bRestartIfPlaying)
{
	if (!AudioSettings)
	{
		return;
	}

	if (PlayerPrepareUseEnergyComponent && PlayerPrepareUseEnergyComponent->IsPlaying())
	{
		if (!bRestartIfPlaying)
		{
			return;
		}

		StopPlayerPrepareUseEnergySFX(0.0f);
	}

	USoundBase* Sound = SelectRandomSound(AudioSettings->PlayerAudio.PrepareUseEnergy);
	UAudioComponent* AudioComponent = CreatePersistent2DSFXComponent(Sound);
	if (!AudioComponent)
	{
		return;
	}

	PlayerPrepareUseEnergyComponent = AudioComponent;
	TrackPersistentSFXComponent(AudioComponent);

	if (FadeInTime > 0.0f)
	{
		AudioComponent->FadeIn(FadeInTime);
	}
	else
	{
		AudioComponent->Play(0.0f);
	}
}

void UGameAudioSubsystem::StopPlayerPrepareUseEnergySFX(float FadeOutTime)
{
	UAudioComponent* AudioComponent = PlayerPrepareUseEnergyComponent;
	if (!AudioComponent)
	{
		return;
	}

	PlayerPrepareUseEnergyComponent = nullptr;

	if (FadeOutTime > 0.0f && AudioComponent->IsPlaying())
	{
		AudioComponent->FadeOut(FadeOutTime, 0.0f);
		return;
	}

	AudioComponent->OnAudioFinished.RemoveDynamic(this, &UGameAudioSubsystem::HandlePersistentSFXFinished);
	ActivePersistentSFXComponents.Remove(AudioComponent);
	AudioComponent->Stop();
	AudioComponent->DestroyComponent();
}

bool UGameAudioSubsystem::IsPlayerPrepareUseEnergySFXPlaying() const
{
	return PlayerPrepareUseEnergyComponent && PlayerPrepareUseEnergyComponent->IsPlaying();
}

void UGameAudioSubsystem::PlayPlayerUseEnergySFX()
{
	if (AudioSettings)
	{
		Play2DSFX(AudioSettings->PlayerAudio.UseEnergy);
	}
}

void UGameAudioSubsystem::PlayPlayerSwitchEnergySFX()
{
	if (AudioSettings)
	{
		Play2DSFX(AudioSettings->PlayerAudio.SwitchEnergy);
	}
}

void UGameAudioSubsystem::PlayPlayerPickupItemSFX()
{
	if (AudioSettings)
	{
		Play2DSFX(AudioSettings->PlayerAudio.PickupItem);
	}
}

void UGameAudioSubsystem::PlayPlayerGainAttributeSFX()
{
	if (AudioSettings)
	{
		Play2DSFX(AudioSettings->PlayerAudio.GainAttribute);
	}
}

void UGameAudioSubsystem::PlayPlayerAttributeFullSFX()
{
	if (AudioSettings)
	{
		Play2DSFX(AudioSettings->PlayerAudio.AttributeFull);
	}
}

void UGameAudioSubsystem::PlayPlayerDeathSFX()
{
	if (AudioSettings)
	{
		Play2DSFX(AudioSettings->PlayerAudio.Death);
	}
}

void UGameAudioSubsystem::PlayEnemyMeleeAttackSFX(UEnemyAudioProfileDataAsset* AudioProfile, FVector WorldLocation)
{
	if (AudioProfile)
	{
		PlayWorldSFX(AudioProfile->MeleeAttack, WorldLocation);
	}
}

void UGameAudioSubsystem::PlayEnemyMeleeDeathSFX(UEnemyAudioProfileDataAsset* AudioProfile, FVector WorldLocation)
{
	if (AudioProfile)
	{
		PlayWorldSFX(AudioProfile->MeleeDeath, WorldLocation);
	}
}

void UGameAudioSubsystem::PlayEnemyRangedAimSFX(UEnemyAudioProfileDataAsset* AudioProfile, FVector WorldLocation)
{
	if (AudioProfile)
	{
		PlayWorldSFX(AudioProfile->RangedAim, WorldLocation);
	}
}

void UGameAudioSubsystem::PlayEnemyRangedAttackSFX(UEnemyAudioProfileDataAsset* AudioProfile, FVector WorldLocation)
{
	if (AudioProfile)
	{
		PlayWorldSFX(AudioProfile->RangedAttack, WorldLocation);
	}
}

void UGameAudioSubsystem::PlayEnemyRangedDeathSFX(UEnemyAudioProfileDataAsset* AudioProfile, FVector WorldLocation)
{
	if (AudioProfile)
	{
		PlayWorldSFX(AudioProfile->RangedDeath, WorldLocation);
	}
}

void UGameAudioSubsystem::PlayLevelStartSFX()
{
	if (AudioSettings)
	{
		Play2DSFX(AudioSettings->LevelAudio.LevelStart);
	}
}

void UGameAudioSubsystem::PlayLevelFailedSFX()
{
	if (AudioSettings)
	{
		Play2DSFX(AudioSettings->LevelAudio.LevelFailed);
	}
}

void UGameAudioSubsystem::PlayLevelVictorySFX()
{
	if (AudioSettings)
	{
		Play2DSFX(AudioSettings->LevelAudio.LevelVictory);
	}
}

void UGameAudioSubsystem::PlayLevelTransitionSFX()
{
	if (AudioSettings)
	{
		Play2DSFX(AudioSettings->LevelAudio.LevelTransition);
	}
}

void UGameAudioSubsystem::PlayUIHoverSFX()
{
	if (AudioSettings)
	{
		Play2DSFX(AudioSettings->UIAudio.Hover);
	}
}

void UGameAudioSubsystem::PlayUIClickSFX()
{
	if (AudioSettings)
	{
		Play2DSFX(AudioSettings->UIAudio.Click);
	}
}

float UGameAudioSubsystem::ResolveFadeTime(float FadeTime) const
{
	if (FadeTime >= 0.0f)
	{
		return FadeTime;
	}

	return AudioSettings ? FMath::Max(0.0f, AudioSettings->DefaultBGMFadeTime) : 1.0f;
}

USoundBase* UGameAudioSubsystem::SelectRandomSound(const FGameSoundSet& SoundSet) const
{
	TArray<USoundBase*> ValidSounds;
	ValidSounds.Reserve(SoundSet.Sounds.Num());

	for (USoundBase* Sound : SoundSet.Sounds)
	{
		if (Sound)
		{
			ValidSounds.Add(Sound);
		}
	}

	if (ValidSounds.IsEmpty())
	{
		return nullptr;
	}

	return ValidSounds[FMath::RandRange(0, ValidSounds.Num() - 1)];
}

UAudioComponent* UGameAudioSubsystem::CreatePersistent2DSFXComponent(USoundBase* Sound)
{
	UWorld* World = GetWorld();
	if (!Sound || !World)
	{
		return nullptr;
	}

	return UGameplayStatics::CreateSound2D(
		World,
		Sound,
		SFXVolume,
		1.0f,
		0.0f,
		nullptr,
		true,
		false);
}

void UGameAudioSubsystem::Play2DSFX(const FGameSoundSet& SoundSet)
{
	USoundBase* Sound = SelectRandomSound(SoundSet);
	UAudioComponent* AudioComponent = CreatePersistent2DSFXComponent(Sound);
	if (AudioComponent)
	{
		TrackPersistentSFXComponent(AudioComponent);
		AudioComponent->Play(0.0f);
	}
}

void UGameAudioSubsystem::PlayWorldSFX(const FGameSoundSet& SoundSet, FVector WorldLocation)
{
	USoundBase* Sound = SelectRandomSound(SoundSet);
	UWorld* World = GetWorld();
	if (!Sound || !World)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UObject* ComponentOuter = GameInstance ? static_cast<UObject*>(GameInstance) : GetTransientPackage();
	UAudioComponent* AudioComponent = NewObject<UAudioComponent>(ComponentOuter);
	if (!AudioComponent)
	{
		return;
	}

	AudioComponent->SetSound(Sound);
	AudioComponent->SetWorldLocation(WorldLocation);
	AudioComponent->SetVolumeMultiplier(SFXVolume);
	AudioComponent->SetPitchMultiplier(1.0f);
	AudioComponent->bAutoDestroy = false;
	AudioComponent->bStopWhenOwnerDestroyed = false;
	AudioComponent->bIgnoreForFlushing = true;
	AudioComponent->bAllowSpatialization = true;
	AudioComponent->bIsUISound = false;
	AudioComponent->RegisterComponentWithWorld(World);
	TrackPersistentSFXComponent(AudioComponent);
	AudioComponent->Play(0.0f);
}

void UGameAudioSubsystem::TrackPersistentSFXComponent(UAudioComponent* AudioComponent)
{
	if (!AudioComponent)
	{
		return;
	}

	AudioComponent->OnAudioFinished.RemoveDynamic(this, &UGameAudioSubsystem::HandlePersistentSFXFinished);
	AudioComponent->OnAudioFinished.AddDynamic(this, &UGameAudioSubsystem::HandlePersistentSFXFinished);
	ActivePersistentSFXComponents.Add(AudioComponent);
}

void UGameAudioSubsystem::PruneFinishedPersistentSFXComponents()
{
	for (int32 Index = ActivePersistentSFXComponents.Num() - 1; Index >= 0; --Index)
	{
		UAudioComponent* AudioComponent = ActivePersistentSFXComponents[Index];
		if (AudioComponent && AudioComponent->IsPlaying())
		{
			continue;
		}

		if (AudioComponent)
		{
			if (AudioComponent == PlayerPrepareUseEnergyComponent)
			{
				PlayerPrepareUseEnergyComponent = nullptr;
			}

			AudioComponent->OnAudioFinished.RemoveDynamic(this, &UGameAudioSubsystem::HandlePersistentSFXFinished);
			AudioComponent->DestroyComponent();
		}

		ActivePersistentSFXComponents.RemoveAtSwap(Index);
	}
}
