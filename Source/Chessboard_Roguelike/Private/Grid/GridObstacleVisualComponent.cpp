#include "Grid/GridObstacleVisualComponent.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Grid/GridManager.h"
#include "Grid/GridSettings.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridObstacleVisual, Log, All);

namespace
{
	constexpr int32 CubeCoordXCustomDataIndex = 0;
	constexpr int32 CubeCoordYCustomDataIndex = 1;
	constexpr int32 CubePhaseCustomDataIndex = 2;
	constexpr int32 CubeCustomDataFloatCount = 3;

	constexpr int32 FaceStyleCustomDataIndex = 0;
	constexpr int32 FaceCoordXCustomDataIndex = 1;
	constexpr int32 FaceCoordYCustomDataIndex = 2;
	constexpr int32 FaceDirectionXCustomDataIndex = 3;
	constexpr int32 FaceDirectionYCustomDataIndex = 4;
	constexpr int32 FacePhaseCustomDataIndex = 5;
	constexpr int32 FaceCustomDataFloatCount = 6;

	float FaceStyleToCustomDataValue(EObstacleFaceVisualStyle Style)
	{
		return static_cast<float>(Style);
	}
}

UGridObstacleVisualComponent::UGridObstacleVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGridObstacleVisualComponent::OnRegister()
{
	Super::OnRegister();

	if (AGridManager* GridManager = Cast<AGridManager>(GetOwner()))
	{
		ConfigureComponents(GridManager);
	}
}

void UGridObstacleVisualComponent::RebuildFromGrid(AGridManager* GridManager)
{
	ClearObstacleVisuals(GridManager);
	ConfigureComponents(GridManager);

	if (!GridManager || !VisualConfig)
	{
		return;
	}

	if (!GridManager->GridSettings)
	{
		UE_LOG(LogGridObstacleVisual, Warning, TEXT("RebuildFromGrid skipped: GridManager has no GridSettings."));
		return;
	}

	if (!VisualConfig->ObstacleCubeMesh)
	{
		UE_LOG(LogGridObstacleVisual, Verbose, TEXT("RebuildFromGrid skipped: ObstacleCubeMesh is not assigned."));
		return;
	}

	for (const TPair<FIntPoint, FTileData>& TilePair : GridManager->Tiles)
	{
		const FTileData& TileData = TilePair.Value;
		if (TileData.TileType != ETileType::Obstacle)
		{
			continue;
		}

		if (!HasNearbyWalkableTile(GridManager, TileData.GridCoord))
		{
			continue;
		}

		AddObstacleCube(GridManager, TileData);
		AddObstacleFaces(GridManager, TileData);
	}
}

void UGridObstacleVisualComponent::RefreshObstacleAtCoord(AGridManager* GridManager, FIntPoint Coord)
{
	// Obstacle faces depend on neighboring tile types, so a full visual rebuild keeps dynamic terrain edits correct.
	RebuildFromGrid(GridManager);
}

bool UGridObstacleVisualComponent::GetCubeInstanceIndex(const FIntPoint& Coord, int32& OutInstanceIndex) const
{
	if (const int32* InstanceIndex = CubeInstanceIndices.Find(Coord))
	{
		OutInstanceIndex = *InstanceIndex;
		return true;
	}

	OutInstanceIndex = INDEX_NONE;
	return false;
}

void UGridObstacleVisualComponent::ClearObstacleVisuals(AGridManager* GridManager)
{
	CubeInstanceIndices.Empty();

	if (GridManager && GridManager->ObstacleCubeISM)
	{
		GridManager->ObstacleCubeISM->ClearInstances();
		GridManager->ObstacleCubeISM->NumCustomDataFloats = CubeCustomDataFloatCount;
	}

	if (GridManager && GridManager->ObstacleFaceISM)
	{
		GridManager->ObstacleFaceISM->ClearInstances();
		GridManager->ObstacleFaceISM->NumCustomDataFloats = FaceCustomDataFloatCount;
	}
}

void UGridObstacleVisualComponent::ConfigureComponents(AGridManager* GridManager)
{
	if (!GridManager)
	{
		return;
	}

	if (GridManager->ObstacleCubeISM)
	{
		GridManager->ObstacleCubeISM->NumCustomDataFloats = CubeCustomDataFloatCount;
		GridManager->ObstacleCubeISM->SetStaticMesh(VisualConfig ? VisualConfig->ObstacleCubeMesh : nullptr);
		GridManager->ObstacleCubeISM->SetCollisionEnabled(VisualConfig && VisualConfig->bEnableObstacleCollision
			? ECollisionEnabled::QueryAndPhysics
			: ECollisionEnabled::NoCollision);

		if (VisualConfig && VisualConfig->CubeMaterial)
		{
			CubeMaterialInstance = GridManager->ObstacleCubeISM->CreateDynamicMaterialInstance(0, VisualConfig->CubeMaterial);
		}
	}

	if (GridManager->ObstacleFaceISM)
	{
		GridManager->ObstacleFaceISM->NumCustomDataFloats = FaceCustomDataFloatCount;
		GridManager->ObstacleFaceISM->SetStaticMesh(VisualConfig ? VisualConfig->FacePlaneMesh : nullptr);
		GridManager->ObstacleFaceISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (VisualConfig && VisualConfig->FaceMaterial)
		{
			FaceMaterialInstance = GridManager->ObstacleFaceISM->CreateDynamicMaterialInstance(0, VisualConfig->FaceMaterial);
		}
	}

	ApplyWaveMaterialParameters();
}

void UGridObstacleVisualComponent::ApplyWaveMaterialParameters()
{
	if (!VisualConfig)
	{
		return;
	}

	const auto ApplyParameters = [this](UMaterialInstanceDynamic* MaterialInstance)
	{
		if (!MaterialInstance || !VisualConfig)
		{
			return;
		}

		MaterialInstance->SetScalarParameterValue(TEXT("WaveAmplitude"), VisualConfig->WaveSettings.Amplitude);
		MaterialInstance->SetScalarParameterValue(TEXT("WaveSpeed"), VisualConfig->WaveSettings.Speed);
		MaterialInstance->SetScalarParameterValue(TEXT("WaveX"), VisualConfig->WaveSettings.WaveX);
		MaterialInstance->SetScalarParameterValue(TEXT("WaveY"), VisualConfig->WaveSettings.WaveY);
		MaterialInstance->SetScalarParameterValue(TEXT("ObstacleHeight"), VisualConfig->ObstacleHeight);
	};

	ApplyParameters(CubeMaterialInstance);
	ApplyParameters(FaceMaterialInstance);
}

void UGridObstacleVisualComponent::AddObstacleCube(const AGridManager* GridManager, const FTileData& TileData)
{
	if (!GridManager || !GridManager->GridSettings || !VisualConfig || !GridManager->ObstacleCubeISM || !VisualConfig->ObstacleCubeMesh)
	{
		return;
	}

	const float TileSize = GridManager->GridSettings->TileSize;
	const FVector DesiredSize(TileSize, TileSize, VisualConfig->ObstacleHeight);
	const FVector Scale = GetMeshScaleForDesiredSize(VisualConfig->ObstacleCubeMesh, DesiredSize);
	const FVector Location = GridManager->GridToWorld(TileData.GridCoord) + FVector(0.f, 0.f, VisualConfig->ObstacleHeight * 0.5f);

	UInstancedStaticMeshComponent* CubeISM = GridManager->ObstacleCubeISM;
	const int32 InstanceIndex = CubeISM->AddInstance(FTransform(FRotator::ZeroRotator, Location, Scale), true);
	CubeInstanceIndices.Add(TileData.GridCoord, InstanceIndex);

	const float Phase = GetStablePhase(TileData.GridCoord);
	const bool bWroteCoordX = CubeISM->SetCustomDataValue(InstanceIndex, CubeCoordXCustomDataIndex, static_cast<float>(TileData.GridCoord.X), false);
	const bool bWroteCoordY = CubeISM->SetCustomDataValue(InstanceIndex, CubeCoordYCustomDataIndex, static_cast<float>(TileData.GridCoord.Y), false);
	const bool bWrotePhase = CubeISM->SetCustomDataValue(InstanceIndex, CubePhaseCustomDataIndex, Phase, true);
	if (!bWroteCoordX || !bWroteCoordY || !bWrotePhase)
	{
		UE_LOG(LogGridObstacleVisual, Warning, TEXT("Failed to write cube custom data for obstacle (%d,%d)."),
			TileData.GridCoord.X, TileData.GridCoord.Y);
	}
}

void UGridObstacleVisualComponent::AddObstacleFaces(const AGridManager* GridManager, const FTileData& TileData)
{
	if (!GridManager || !VisualConfig || !GridManager->ObstacleFaceISM || !VisualConfig->FacePlaneMesh)
	{
		return;
	}

	static const FIntPoint Directions[] =
	{
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

	for (const FIntPoint& Direction : Directions)
	{
		const FIntPoint NeighborCoord = TileData.GridCoord + Direction;
		ETileType NeighborType = ETileType::Minimal;
		const bool bHasNeighbor = GetTileTypeAt(GridManager, NeighborCoord, NeighborType);

		if (!bHasNeighbor)
		{
			if (VisualConfig->bGenerateBoundaryFaces)
			{
				AddFaceInstance(GridManager, TileData, Direction, EObstacleFaceVisualStyle::NeutralWarning);
			}
			continue;
		}

		const EObstacleFaceVisualStyle Style = GetFaceStyleForNeighbor(NeighborType);
		if (Style == EObstacleFaceVisualStyle::None)
		{
			continue;
		}

		if (Style == EObstacleFaceVisualStyle::NeutralWarning && !VisualConfig->bGenerateNeutralFaces)
		{
			continue;
		}

		AddFaceInstance(GridManager, TileData, Direction, Style);
	}
}

void UGridObstacleVisualComponent::AddFaceInstance(
	const AGridManager* GridManager,
	const FTileData& TileData,
	const FIntPoint& Direction,
	EObstacleFaceVisualStyle Style)
{
	if (!GridManager || !GridManager->GridSettings || !VisualConfig || !GridManager->ObstacleFaceISM || !VisualConfig->FacePlaneMesh)
	{
		return;
	}

	const float TileSize = GridManager->GridSettings->TileSize;
	const float HalfTileSize = TileSize * 0.5f;
	const FVector OutwardDirection(static_cast<float>(Direction.X), static_cast<float>(Direction.Y), 0.f);
	const FVector FaceTangent(-OutwardDirection.Y, OutwardDirection.X, 0.f);
	const FRotator Rotation = FRotationMatrix::MakeFromXZ(FaceTangent.GetSafeNormal(), OutwardDirection.GetSafeNormal()).Rotator();

	const FVector DesiredSize(TileSize, VisualConfig->ObstacleHeight, 1.f);
	const FVector Scale = GetMeshScaleForDesiredSize(VisualConfig->FacePlaneMesh, DesiredSize);
	const FVector Location = GridManager->GridToWorld(TileData.GridCoord)
		+ OutwardDirection * (HalfTileSize + VisualConfig->FaceSurfaceOffset)
		+ FVector(0.f, 0.f, VisualConfig->ObstacleHeight * 0.5f);

	UInstancedStaticMeshComponent* FaceISM = GridManager->ObstacleFaceISM;
	const int32 InstanceIndex = FaceISM->AddInstance(FTransform(Rotation, Location, Scale), true);
	const float Phase = GetStablePhase(TileData.GridCoord);

	const bool bWroteStyle = FaceISM->SetCustomDataValue(InstanceIndex, FaceStyleCustomDataIndex, FaceStyleToCustomDataValue(Style), false);
	const bool bWroteCoordX = FaceISM->SetCustomDataValue(InstanceIndex, FaceCoordXCustomDataIndex, static_cast<float>(TileData.GridCoord.X), false);
	const bool bWroteCoordY = FaceISM->SetCustomDataValue(InstanceIndex, FaceCoordYCustomDataIndex, static_cast<float>(TileData.GridCoord.Y), false);
	const bool bWroteDirectionX = FaceISM->SetCustomDataValue(InstanceIndex, FaceDirectionXCustomDataIndex, static_cast<float>(Direction.X), false);
	const bool bWroteDirectionY = FaceISM->SetCustomDataValue(InstanceIndex, FaceDirectionYCustomDataIndex, static_cast<float>(Direction.Y), false);
	const bool bWrotePhase = FaceISM->SetCustomDataValue(InstanceIndex, FacePhaseCustomDataIndex, Phase, true);
	if (!bWroteStyle || !bWroteCoordX || !bWroteCoordY || !bWroteDirectionX || !bWroteDirectionY || !bWrotePhase)
	{
		UE_LOG(LogGridObstacleVisual, Warning, TEXT("Failed to write face custom data for obstacle (%d,%d)."),
			TileData.GridCoord.X, TileData.GridCoord.Y);
	}
}

bool UGridObstacleVisualComponent::HasNearbyWalkableTile(const AGridManager* GridManager, const FIntPoint& Coord) const
{
	if (!GridManager || !VisualConfig)
	{
		return false;
	}

	const int32 Radius = FMath::Max(0, VisualConfig->NearbyWalkableTileManhattanRadius);
	for (int32 DeltaY = -Radius; DeltaY <= Radius; ++DeltaY)
	{
		const int32 RemainingDistance = Radius - FMath::Abs(DeltaY);
		for (int32 DeltaX = -RemainingDistance; DeltaX <= RemainingDistance; ++DeltaX)
		{
			if (DeltaX == 0 && DeltaY == 0)
			{
				continue;
			}

			const FIntPoint CandidateCoord = Coord + FIntPoint(DeltaX, DeltaY);
			const FTileData* CandidateTile = GridManager->Tiles.Find(CandidateCoord);
			if (!CandidateTile)
			{
				continue;
			}

			if (CandidateTile->bWalkable && CandidateTile->TileType != ETileType::Obstacle)
			{
				return true;
			}
		}
	}

	return false;
}

bool UGridObstacleVisualComponent::GetTileTypeAt(const AGridManager* GridManager, const FIntPoint& Coord, ETileType& OutTileType) const
{
	if (!GridManager)
	{
		return false;
	}

	const FTileData* TileData = GridManager->Tiles.Find(Coord);
	if (!TileData)
	{
		return false;
	}

	OutTileType = TileData->TileType;
	return true;
}

EObstacleFaceVisualStyle UGridObstacleVisualComponent::GetFaceStyleForNeighbor(ETileType NeighborType) const
{
	switch (NeighborType)
	{
	case ETileType::Construct:
		return EObstacleFaceVisualStyle::ConstructGraffiti;
	case ETileType::Acid:
		return EObstacleFaceVisualStyle::HologramGlitch;
	case ETileType::Minimal:
		return EObstacleFaceVisualStyle::NeutralWarning;
	case ETileType::Obstacle:
	default:
		return EObstacleFaceVisualStyle::None;
	}
}

FVector UGridObstacleVisualComponent::GetMeshScaleForDesiredSize(const UStaticMesh* Mesh, const FVector& DesiredSize) const
{
	if (!Mesh)
	{
		return FVector::OneVector;
	}

	const FVector MeshSize = Mesh->GetBounds().BoxExtent * 2.f;
	FVector Scale = FVector::OneVector;
	if (MeshSize.X > KINDA_SMALL_NUMBER)
	{
		Scale.X = DesiredSize.X / MeshSize.X;
	}
	if (MeshSize.Y > KINDA_SMALL_NUMBER)
	{
		Scale.Y = DesiredSize.Y / MeshSize.Y;
	}
	if (MeshSize.Z > KINDA_SMALL_NUMBER)
	{
		Scale.Z = DesiredSize.Z / MeshSize.Z;
	}

	return Scale;
}

float UGridObstacleVisualComponent::GetStablePhase(const FIntPoint& Coord) const
{
	const int32 Hash = HashCombine(::GetTypeHash(Coord.X), ::GetTypeHash(Coord.Y));
	return static_cast<float>(Hash % 10000) * 0.00062831853f;
}
