#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RunRecordBlueprintLibrary.generated.h"

USTRUCT(BlueprintType)
struct FRunRecordStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Run Record")
	bool bHasRecord = false;

	UPROPERTY(BlueprintReadOnly, Category = "Run Record")
	int32 HighestLevelReached = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run Record")
	int32 BestTimeToHighestLevelSeconds = 0;
};

UCLASS()
class CHESSBOARD_ROGUELIKE_API URunRecordBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Run Record", meta = (WorldContext = "WorldContextObject"))
	static FRunRecordStats LoadRunRecordStats(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Run Record", meta = (WorldContext = "WorldContextObject"))
	static bool RecordRunProgress(const UObject* WorldContextObject, int32 LevelReached, int32 ElapsedSeconds);

	UFUNCTION(BlueprintCallable, Category = "Run Record", meta = (WorldContext = "WorldContextObject"))
	static bool ClearRunRecord(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Run Record")
	static FText FormatRunRecordTime(int32 ElapsedSeconds);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Run Record")
	static FText FormatRunRecordSummary(const FRunRecordStats& Stats);

private:
	static const TCHAR* SaveSlotName;
	static constexpr int32 SaveUserIndex = 0;
};
