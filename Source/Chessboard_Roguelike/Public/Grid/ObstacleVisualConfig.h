#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ObstacleVisualConfig.generated.h"

class UMaterialInterface;
class UStaticMesh;

UENUM(BlueprintType)
enum class EObstacleFaceVisualStyle : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	ConstructGraffiti = 1 UMETA(DisplayName = "Construct Graffiti"),
	HologramGlitch = 2 UMETA(DisplayName = "Hologram Glitch"),
	NeutralWarning = 3 UMETA(DisplayName = "Neutral Warning")
};

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FObstacleWaveSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "0.0"))
	float Amplitude = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "0.0"))
	float Speed = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	float WaveX = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	float WaveY = 0.35f;
};

UCLASS(BlueprintType)
class CHESSBOARD_ROGUELIKE_API UObstacleVisualConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstacle")
	TObjectPtr<UStaticMesh> ObstacleCubeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstacle")
	TObjectPtr<UStaticMesh> FacePlaneMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material")
	TObjectPtr<UMaterialInterface> CubeMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material")
	TObjectPtr<UMaterialInterface> FaceMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstacle", meta = (ClampMin = "1.0"))
	float ObstacleHeight = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstacle", meta = (ClampMin = "0.0"))
	float FaceSurfaceOffset = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstacle")
	bool bEnableObstacleCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstacle")
	bool bGenerateBoundaryFaces = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstacle")
	bool bGenerateNeutralFaces = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Optimization", meta = (ClampMin = "0"))
	int32 NearbyWalkableTileManhattanRadius = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	FObstacleWaveSettings WaveSettings;
};
