#include "WasamiGameMode.h"

#include "WasamiPlayerCharacter.h"

AWasamiGameMode::AWasamiGameMode()
{
	DefaultPawnClass = AWasamiPlayerCharacter::StaticClass();
}
