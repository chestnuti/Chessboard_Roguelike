#include "Enemy/RangedAttackTelegraphComponent.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Grid/GridManager.h"
#include "Grid/GridSettings.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogRangedAttackTelegraph, Log, All);

URangedAttackTelegraphComponent::URangedAttackTelegraphComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URangedAttackTelegraphComponent::BeginPlay()
{
	Super::BeginPlay();
}

void URangedAttackTelegraphComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearTelegraph();
	Super::EndPlay(EndPlayReason);
}

void URangedAttackTelegraphComponent::ShowTelegraph(AGridManager* GridManager, const TArray<FIntPoint>& Tiles)
{
	TelegraphedTiles = Tiles;
	EnsureTelegraphComponent(GridManager);

	if (!TelegraphISM)
	{
		return;
	}

	TelegraphISM->ClearInstances();

	UStaticMesh* Mesh = TelegraphTileMesh.Get();
	if (!Mesh && GridManager && GridManager->GridSettings)
	{
		Mesh = GridManager->GridSettings->TileMesh;
	}

	if (!Mesh)
	{
		TelegraphISM->SetHiddenInGame(true);
		UE_LOG(LogRangedAttackTelegraph, Verbose, TEXT("ShowTelegraph skipped visuals: no mesh assigned."));
		return;
	}

	TelegraphISM->SetStaticMesh(Mesh);
	if (TelegraphMaterial)
	{
		TelegraphISM->SetMaterial(0, TelegraphMaterial);
	}

	const FVector InstanceScale = GetInstanceScale(GridManager, Mesh);
	for (const FIntPoint& Tile : TelegraphedTiles)
	{
		if (!GridManager || !GridManager->IsValidCoord(Tile))
		{
			continue;
		}

		const FVector WorldLocation = GridManager->GridToWorld(Tile) + FVector(0.f, 0.f, ZOffset);
		TelegraphISM->AddInstance(FTransform(FRotator::ZeroRotator, WorldLocation, InstanceScale), true);
	}

	TelegraphISM->SetHiddenInGame(TelegraphISM->GetInstanceCount() <= 0);
}

void URangedAttackTelegraphComponent::ClearTelegraph()
{
	TelegraphedTiles.Reset();
	if (TelegraphISM)
	{
		TelegraphISM->ClearInstances();
		TelegraphISM->SetHiddenInGame(true);
	}
}

bool URangedAttackTelegraphComponent::IsShowingTelegraph() const
{
	return !TelegraphedTiles.IsEmpty();
}

TArray<FIntPoint> URangedAttackTelegraphComponent::GetTelegraphedTiles() const
{
	return TelegraphedTiles;
}

void URangedAttackTelegraphComponent::EnsureTelegraphComponent(AGridManager* GridManager)
{
	if (TelegraphISM)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TelegraphISM = NewObject<UInstancedStaticMeshComponent>(Owner, TelegraphComponentName);
	if (!TelegraphISM)
	{
		return;
	}

	TelegraphISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TelegraphISM->SetMobility(EComponentMobility::Movable);
	TelegraphISM->SetHiddenInGame(true);
	Owner->AddInstanceComponent(TelegraphISM);
	TelegraphISM->RegisterComponent();

	USceneComponent* AttachParent = GridManager ? GridManager->GetRootComponent() : Owner->GetRootComponent();
	if (AttachParent)
	{
		TelegraphISM->AttachToComponent(AttachParent, FAttachmentTransformRules::KeepWorldTransform);
	}
}

FVector URangedAttackTelegraphComponent::GetInstanceScale(AGridManager* GridManager, UStaticMesh* Mesh) const
{
	if (!GridManager || !GridManager->GridSettings || !Mesh)
	{
		return FVector(ScaleMultiplier);
	}

	const float TileSize = GridManager->GridSettings->TileSize;
	const FVector MeshSize = Mesh->GetBounds().BoxExtent * 2.f;
	FVector InstanceScale = FVector(ScaleMultiplier);
	if (MeshSize.X > KINDA_SMALL_NUMBER)
	{
		InstanceScale.X = (TileSize / MeshSize.X) * ScaleMultiplier;
	}
	if (MeshSize.Y > KINDA_SMALL_NUMBER)
	{
		InstanceScale.Y = (TileSize / MeshSize.Y) * ScaleMultiplier;
	}

	return InstanceScale;
}
