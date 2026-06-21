#include "Save/RunRecordBlueprintLibrary.h"

#include "Kismet/GameplayStatics.h"
#include "Save/RunRecordSaveGame.h"

const TCHAR* URunRecordBlueprintLibrary::SaveSlotName = TEXT("ChessboardRoguelikeRunRecord");

FRunRecordStats URunRecordBlueprintLibrary::LoadRunRecordStats(const UObject* WorldContextObject)
{
	FRunRecordStats Stats;

	USaveGame* LoadedSave = UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex);
	const URunRecordSaveGame* RunRecordSave = Cast<URunRecordSaveGame>(LoadedSave);
	if (!RunRecordSave || RunRecordSave->SchemaVersion <= 0 || RunRecordSave->HighestLevelReached <= 0)
	{
		return Stats;
	}

	Stats.bHasRecord = true;
	Stats.HighestLevelReached = RunRecordSave->HighestLevelReached;
	Stats.BestTimeToHighestLevelSeconds = FMath::Max(0, RunRecordSave->BestTimeToHighestLevelSeconds);
	return Stats;
}

bool URunRecordBlueprintLibrary::RecordRunProgress(const UObject* WorldContextObject, int32 LevelReached, int32 ElapsedSeconds)
{
	if (LevelReached <= 0 || ElapsedSeconds < 0)
	{
		return false;
	}

	URunRecordSaveGame* RunRecordSave = Cast<URunRecordSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));
	if (!RunRecordSave)
	{
		RunRecordSave = Cast<URunRecordSaveGame>(
			UGameplayStatics::CreateSaveGameObject(URunRecordSaveGame::StaticClass()));
	}

	if (!RunRecordSave)
	{
		return false;
	}

	const bool bReachedFartherLevel = LevelReached > RunRecordSave->HighestLevelReached;
	const bool bMatchedBestLevelFaster =
		LevelReached == RunRecordSave->HighestLevelReached
		&& (RunRecordSave->BestTimeToHighestLevelSeconds <= 0 || ElapsedSeconds < RunRecordSave->BestTimeToHighestLevelSeconds);

	if (!bReachedFartherLevel && !bMatchedBestLevelFaster)
	{
		return true;
	}

	RunRecordSave->SchemaVersion = 1;
	RunRecordSave->HighestLevelReached = LevelReached;
	RunRecordSave->BestTimeToHighestLevelSeconds = ElapsedSeconds;
	return UGameplayStatics::SaveGameToSlot(RunRecordSave, SaveSlotName, SaveUserIndex);
}

bool URunRecordBlueprintLibrary::ClearRunRecord(const UObject* WorldContextObject)
{
	if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
	{
		return true;
	}

	return UGameplayStatics::DeleteGameInSlot(SaveSlotName, SaveUserIndex);
}

FText URunRecordBlueprintLibrary::FormatRunRecordTime(int32 ElapsedSeconds)
{
	const int32 SafeElapsedSeconds = FMath::Max(0, ElapsedSeconds);
	const int32 Minutes = SafeElapsedSeconds / 60;
	const int32 Seconds = SafeElapsedSeconds % 60;
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds));
}

FText URunRecordBlueprintLibrary::FormatRunRecordSummary(const FRunRecordStats& Stats)
{
	if (!Stats.bHasRecord)
	{
		return NSLOCTEXT("RunRecord", "NoRunRecordSummary", "Best Run: None");
	}

	return FText::Format(
		NSLOCTEXT("RunRecord", "RunRecordSummaryFormat", "Best Run: Level {0} in {1}"),
		FText::AsNumber(Stats.HighestLevelReached),
		FormatRunRecordTime(Stats.BestTimeToHighestLevelSeconds));
}
