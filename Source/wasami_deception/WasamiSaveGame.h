#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WasamiSaveGame.generated.h"

/** Enum_Collectables (pak_reference_2): which of the extras screen's lists an extra is in. */
UENUM(BlueprintType)
enum class EWasamiCollectableType : uint8
{
	ArtGallery UMETA(DisplayName = "Art Gallery"),
	Diary,
	Sound,
	Movie
};

/** Struct_Collectable: an extra a secret file unlocks, by its list (Type) and its ID in it. */
USTRUCT(BlueprintType)
struct FWasamiCollectableEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable")
	EWasamiCollectableType Type = EWasamiCollectableType::ArtGallery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable")
	int32 ID = 0;
};

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
 * slot (BP_DD_SaveGame, SaveSlot), Last Checkpoint Warning and the extras unlocked (Extras_Art, Extras_SFX). Only
 * Erase empties the extras: dying, RESTART and the level's end empty the hospital's entry alone, as they do the
 * original's levelStruct, and nothing the hospital runs writes the original's SaveSlot back from an older copy
 * (BP_DD_GameMode writes its Global Save Instance only in Check For Save, when it has just made it, and in Erase Save
 * Files).
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** The original's slot name for the level save. */
	static const FString SlotName;

	static constexpr int32 UserIndex = 0;

	/**
	 * BP_DD_GameMode's Erase Save Files: a new save (no progress, no warning) written to Slot in place of the one there.
	 * Returns it.
	 */
	static UWasamiSaveGame* Erase(const FString& Slot);

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Save")
	FWasamiLevelProgress Hospital;

	/** Whether LAST CHECKPOINT has already warned that it rules out an S rank. */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Save")
	bool bLastCheckpointWarning = false;

	/** Extras_Art: the Art Gallery's pictures unlocked, by their IDs. */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Save")
	TArray<int32> ExtrasArt;

	/** Extras_SFX: the Sound Archive's tracks unlocked, by their IDs. */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Save")
	TArray<int32> ExtrasSFX;

	/**
	 * BP_Collectable's Unlock, for one entry: an Art Gallery one's ID AddUnique'd to ExtrasArt, a Sound one's to
	 * ExtrasSFX; a Diary's and a Movie's do nothing (the original's switch has no case for them). Returns whether it
	 * went into a list (the original writes the save after those two only).
	 */
	bool Unlock(const FWasamiCollectableEntry& Entry);

	/** Whether the extra is unlocked: an Art Gallery's in ExtrasArt, a Sound's in ExtrasSFX; the others never. */
	bool IsUnlocked(const FWasamiCollectableEntry& Entry) const;
};
