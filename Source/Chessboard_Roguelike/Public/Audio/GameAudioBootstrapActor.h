#pragma once

#include "CoreMinimal.h"
#include "Audio/GameAudioTypes.h"
#include "GameFramework/Actor.h"
#include "GameAudioBootstrapActor.generated.h"

class UGameAudioSettingsDataAsset;

UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API AGameAudioBootstrapActor : public AActor
{
	GENERATED_BODY()

public:
	AGameAudioBootstrapActor();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Audio")
	TObjectPtr<UGameAudioSettingsDataAsset> AudioSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Audio")
	EGameBGMType InitialBGM = EGameBGMType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Audio", meta = (ClampMin = "-1.0"))
	float InitialBGMFadeTime = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Audio")
	bool bPlayLevelStartSFX = false;
};
