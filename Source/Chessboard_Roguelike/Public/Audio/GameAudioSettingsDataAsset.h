#pragma once

#include "CoreMinimal.h"
#include "Audio/GameAudioTypes.h"
#include "Engine/DataAsset.h"
#include "GameAudioSettingsDataAsset.generated.h"

class USoundBase;

UCLASS(BlueprintType)
class CHESSBOARD_ROGUELIKE_API UGameAudioSettingsDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM")
	TObjectPtr<USoundBase> MainMenuBGM = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM")
	TObjectPtr<USoundBase> LevelBGM = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM", meta = (ClampMin = "0.0"))
	float DefaultBGMFadeTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Audio")
	FPlayerAudioSet PlayerAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Audio")
	FLevelAudioSet LevelAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI Audio")
	FUIAudioSet UIAudio;
};
