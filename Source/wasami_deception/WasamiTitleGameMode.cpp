#include "WasamiTitleGameMode.h"

#include "WasamiGameInstance.h"
#include "WasamiTitleScreenWidget.h"

AWasamiTitleGameMode::AWasamiTitleGameMode()
{
	DefaultPawnClass = nullptr;
}

bool AWasamiTitleGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return false;
}

void AWasamiTitleGameMode::BeginPlay()
{
	Super::BeginPlay();
	// Used Hard Respawn? and Hard Check Point belong to the entrance, which this game does not have. Then the game
	// mode's Reset Game Instance(True): the shards collected forgotten and the lives reset (the rest of what it empties
	// is not in this game).
	if (UWasamiGameInstance* Instance = GetGameInstance<UWasamiGameInstance>())
	{
		Instance->ForgetCollectedShards();
		Instance->ResetLives();
	}
	// Create(UMG_TitleScreen), AddToViewport(1). SaveSlot's read for the level and the achievements is not copied (the
	// screen reads the save itself).
	UWasamiTitleScreenWidget::Show(this);
}
