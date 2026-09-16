#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WasamiGameMode.generated.h"

/**
 * The game's mode: spawns the player (AWasamiPlayerCharacter) at the level's player start and keeps what the tablet's
 * band shows, after Dark Deception's BP_DD_GameMode (pak_reference).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWasamiGameMode();

	/**
	 * Current Objective: the tablet's band shows it in upper case. The hospital's Zone 1 sets it to COLLECT ALL SHARDS
	 * once the player is on their way (pak_reference_2's 06_Hospital_Zone_01, @2293).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FText CurrentObjective;
};
