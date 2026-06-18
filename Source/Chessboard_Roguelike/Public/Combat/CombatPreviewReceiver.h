#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatPreviewTypes.h"
#include "UObject/Interface.h"
#include "CombatPreviewReceiver.generated.h"

UINTERFACE(BlueprintType)
class CHESSBOARD_ROGUELIKE_API UCombatPreviewReceiver : public UInterface
{
	GENERATED_BODY()
};

class CHESSBOARD_ROGUELIKE_API ICombatPreviewReceiver
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat Preview")
	void UpdateCombatPreview(const FCombatPreviewState& PreviewState);
};
