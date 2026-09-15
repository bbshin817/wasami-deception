#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WasamiGameMode.generated.h"

/** The game's mode: spawns the player (AWasamiPlayerCharacter) at the level's player start. */
UCLASS()
class WASAMI_DECEPTION_API AWasamiGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWasamiGameMode();
};
