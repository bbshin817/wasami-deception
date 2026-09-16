#include "WasamiGameMode.h"

#include "WasamiPlayerCharacter.h"

AWasamiGameMode::AWasamiGameMode()
{
	DefaultPawnClass = AWasamiPlayerCharacter::StaticClass();
	CurrentObjective = NSLOCTEXT("Wasami", "ObjectiveCollectAllShards", "Collect all shards");
}
