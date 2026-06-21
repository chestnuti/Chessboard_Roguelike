#include "Audio/GameAudioBlueprintLibrary.h"

#include "Audio/GameAudioSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

UGameAudioSubsystem* UGameAudioBlueprintLibrary::GetGameAudioSubsystem(const UObject* WorldContextObject)
{
	const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	return GameInstance ? GameInstance->GetSubsystem<UGameAudioSubsystem>() : nullptr;
}
