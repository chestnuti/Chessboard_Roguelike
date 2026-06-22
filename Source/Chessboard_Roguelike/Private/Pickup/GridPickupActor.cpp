#include "Pickup/GridPickupActor.h"

#include "Audio/GameAudioSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Grid/GridManager.h"
#include "Player/GridPawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridPickupActor, Log, All);

AGridPickupActor::AGridPickupActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(SceneRoot);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGridPickupActor::InitializeOnGrid(AGridManager* InGridManager, FIntPoint InGridCoord)
{
	GridManager = InGridManager;
	GridCoord = InGridCoord;
	bCollected = false;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(false);

	if (!GridManager)
	{
		UE_LOG(LogGridPickupActor, Warning, TEXT("InitializeOnGrid failed for %s: GridManager is null."), *GetName());
		return;
	}

	SetActorLocation(GridManager->GridToWorld(GridCoord) + FVector(0.f, 0.f, VisualZOffset));
}

FIntPoint AGridPickupActor::GetGridCoord() const
{
	return GridCoord;
}

bool AGridPickupActor::TryCollect(AGridPawn* PlayerPawn)
{
	if (!CanCollect(PlayerPawn))
	{
		return false;
	}

	const bool bEffectApplied = ApplyPickupEffect(PlayerPawn);
	const bool bShouldConsume = (bEffectApplied && bConsumeOnSuccessfulEffect) || (!bEffectApplied && bConsumeWhenEffectFails);
	if (!bShouldConsume)
	{
		return false;
	}

	bCollected = true;
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UGameAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UGameAudioSubsystem>())
		{
			AudioSubsystem->PlayPlayerPickupItemSFX();
		}
	}

	OnGridPickupCollected.Broadcast(this, PlayerPawn, bEffectApplied);
	OnCollected(PlayerPawn, bEffectApplied);
	Destroy();
	return true;
}

bool AGridPickupActor::CanCollect_Implementation(AGridPawn* PlayerPawn) const
{
	return !bCollected && IsValid(PlayerPawn);
}

bool AGridPickupActor::ApplyPickupEffect_Implementation(AGridPawn* PlayerPawn)
{
	return false;
}
