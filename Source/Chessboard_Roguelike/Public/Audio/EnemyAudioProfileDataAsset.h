#pragma once

#include "CoreMinimal.h"
#include "Audio/GameAudioTypes.h"
#include "Engine/DataAsset.h"
#include "EnemyAudioProfileDataAsset.generated.h"

UCLASS(BlueprintType)
class CHESSBOARD_ROGUELIKE_API UEnemyAudioProfileDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Audio|Melee")
	FGameSoundSet MeleeAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Audio|Melee")
	FGameSoundSet MeleeDeath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Audio|Ranged")
	FGameSoundSet RangedAim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Audio|Ranged")
	FGameSoundSet RangedAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Audio|Ranged")
	FGameSoundSet RangedDeath;
};
