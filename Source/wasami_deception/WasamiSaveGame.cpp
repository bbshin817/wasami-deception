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

bool UWasamiSaveGame::Unlock(const FWasamiCollectableEntry& Entry)
{
	// Unlock's SwitchEnum on Type: 0 → Extras_Art, 2 → Extras_SFX (Array_AddUnique), the rest to the loop's next.
	switch (Entry.Type)
	{
	case EWasamiCollectableType::ArtGallery:
		ExtrasArt.AddUnique(Entry.ID);
		return true;
	case EWasamiCollectableType::Sound:
		ExtrasSFX.AddUnique(Entry.ID);
		return true;
	default:
		return false;
	}
}

bool UWasamiSaveGame::IsUnlocked(const FWasamiCollectableEntry& Entry) const
{
	switch (Entry.Type)
	{
	case EWasamiCollectableType::ArtGallery:
		return ExtrasArt.Contains(Entry.ID);
	case EWasamiCollectableType::Sound:
		return ExtrasSFX.Contains(Entry.ID);
	default:
		return false;
	}
}
