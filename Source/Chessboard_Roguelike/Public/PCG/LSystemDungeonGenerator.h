#pragma once

#include "CoreMinimal.h"
#include "PCG/DungeonGenerationSettings.h"
#include "PCG/DungeonLayoutTypes.h"
#include "UObject/Object.h"
#include "LSystemDungeonGenerator.generated.h"

UCLASS(BlueprintType)
class CHESSBOARD_ROGUELIKE_API ULSystemDungeonGenerator : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PCG|Dungeon")
	static bool GenerateDungeonLayout(const UDungeonGenerationSettings* Settings, FGeneratedDungeonLayout& OutLayout);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PCG|Dungeon")
	static bool ValidateConnectivity(const FGeneratedDungeonLayout& Layout);
};
