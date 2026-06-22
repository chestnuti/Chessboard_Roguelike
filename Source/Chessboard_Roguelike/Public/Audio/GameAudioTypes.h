#pragma once

#include "CoreMinimal.h"
#include "GameAudioTypes.generated.h"

class USoundBase;

UENUM(BlueprintType)
enum class EGameBGMType : uint8
{
	None,
	MainMenu,
	Level
};

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FGameSoundSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Audio")
	TArray<TObjectPtr<USoundBase>> Sounds;
};

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FPlayerAudioSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Audio")
	FGameSoundSet Attack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Audio")
	FGameSoundSet Kill;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Audio")
	FGameSoundSet Transform;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Audio")
	FGameSoundSet GainEnergy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Audio")
	FGameSoundSet PrepareUseEnergy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Audio")
	FGameSoundSet UseEnergy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Audio")
	FGameSoundSet SwitchEnergy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Audio")
	FGameSoundSet PickupItem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Audio")
	FGameSoundSet GainAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Audio")
	FGameSoundSet AttributeFull;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Audio")
	FGameSoundSet Death;
};

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FLevelAudioSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Audio")
	FGameSoundSet LevelStart;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Audio")
	FGameSoundSet LevelFailed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Audio")
	FGameSoundSet LevelVictory;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Audio")
	FGameSoundSet LevelTransition;
};

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FUIAudioSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI Audio")
	FGameSoundSet Hover;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI Audio")
	FGameSoundSet Click;
};
