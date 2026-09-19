#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WasamiTitleGameMode.generated.h"

/**
 * The title's mode, which its level (L_Title) sets in its World Settings: nobody to play, and when play begins what
 * Dark Deception's TitleScreen level does in its Blueprint (ReceiveBeginPlay @4390): the game instance reset (the
 * shards collected forgotten, 3 lives) and the title screen at Z 1. The zones' mode (AWasamiGameMode) is not used here:
 * its BeginPlay readies a zone (the save's checkpoint, the player's start, the zone's flow, the fade from black).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiTitleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWasamiTitleGameMode();

	/** The title has no pawn: the player is only the screen's. */
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;

protected:
	virtual void BeginPlay() override;
};
