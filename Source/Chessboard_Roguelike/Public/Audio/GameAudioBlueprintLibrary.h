#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameAudioBlueprintLibrary.generated.h"

class UGameAudioSubsystem;

UCLASS()
class CHESSBOARD_ROGUELIKE_API UGameAudioBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game Audio", meta = (WorldContext = "WorldContextObject"))
	static UGameAudioSubsystem* GetGameAudioSubsystem(const UObject* WorldContextObject);
};
