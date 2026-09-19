#include "WasamiSaveGame.h"

#include "Kismet/GameplayStatics.h"

const FString UWasamiSaveGame::SlotName(TEXT("structSlot"));

UWasamiSaveGame* UWasamiSaveGame::Erase(const FString& Slot)
{
	// @39493: a new SaveSlot and a new structSlot, each written. This game's one save stands for both.
	UWasamiSaveGame* Save = Cast<UWasamiSaveGame>(UGameplayStatics::CreateSaveGameObject(StaticClass()));
	UGameplayStatics::SaveGameToSlot(Save, Slot, UserIndex);
	return Save;
}
