#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "RunRecordSaveGame.generated.h"

UCLASS()
class CHESSBOARD_ROGUELIKE_API URunRecordSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Run Record")
	int32 SchemaVersion = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Run Record")
	int32 HighestLevelReached = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run Record")
	int32 BestTimeToHighestLevelSeconds = 0;
};
