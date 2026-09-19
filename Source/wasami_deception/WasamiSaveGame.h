#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WasamiSaveGame.generated.h"

/**
 * One level's progress, the fields of Dark Deception's DD_LevelStructureyyy (pak_reference_2) that the hospital writes.
 * The original keeps eleven of them in BP_DD_levelStructSave's levelStruct; the hospital's is index 5.
 */
USTRUCT(BlueprintType)
struct FWasamiLevelProgress
{
	GENERATED_BODY()

	/** The last checkpoint passed: 4 to 6 in Zone 1, 7 to 10 in Zone 2, 0 for none. */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 LevelCheckpoint = 0;

	/** Deaths in the level; the death screen adds one. */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 Deaths = 0;

	/** Seconds played up to the last checkpoint. */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Save")
	float Time = 0.f;

	/** Shards collected in a row; a death sets it back to 0. */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 CurrentStreak = 0;

	/** The best streak milestone reached, as an Enum_ShardStreaks value: 0 none, 1..10 for 20..1000 in a row. */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Save")
	uint8 Streak = 0;

	/** The red shards taken (the level clear screen shows only how many, of 2). */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Save")
	TArray<int32> BonusShards;

	/** The secrets found (the level clear screen shows only how many, of 4). */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Save")
	TArray<int32> Secrets;
};

/**
 * The game's one save: the hospital's entry of BP_DD_levelStructSave (slot structSlot) and, from the original's other
 * slot (BP_DD_SaveGame, SaveSlot), Last Checkpoint Warning.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** The original's slot name for the level save. */
	static const FString SlotName;

	static constexpr int32 UserIndex = 0;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Save")
	FWasamiLevelProgress Hospital;

	/** Whether LAST CHECKPOINT has already warned that it rules out an S rank. */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Save")
	bool bLastCheckpointWarning = false;
};
