#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RangedAttackTelegraphComponent.generated.h"

class AGridManager;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

UCLASS(ClassGroup=(Enemy), BlueprintType, meta=(BlueprintSpawnableComponent))
class CHESSBOARD_ROGUELIKE_API URangedAttackTelegraphComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URangedAttackTelegraphComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ranged Telegraph")
	TObjectPtr<UStaticMesh> TelegraphTileMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged Telegraph")
	TObjectPtr<UMaterialInterface> TelegraphMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ranged Telegraph")
	FName TelegraphComponentName = TEXT("RangedAttackTelegraphISM");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ranged Telegraph", meta = (ClampMin = "0.0"))
	float ZOffset = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ranged Telegraph", meta = (ClampMin = "0.0"))
	float ScaleMultiplier = 0.92f;

	UFUNCTION(BlueprintCallable, Category = "Ranged Telegraph")
	void ShowTelegraph(AGridManager* GridManager, const TArray<FIntPoint>& Tiles);

	UFUNCTION(BlueprintCallable, Category = "Ranged Telegraph")
	void ClearTelegraph();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ranged Telegraph")
	bool IsShowingTelegraph() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ranged Telegraph")
	TArray<FIntPoint> GetTelegraphedTiles() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> TelegraphISM;

	UPROPERTY(Transient)
	TArray<FIntPoint> TelegraphedTiles;

	void EnsureTelegraphComponent(AGridManager* GridManager);
	FVector GetInstanceScale(AGridManager* GridManager, UStaticMesh* Mesh) const;
};
