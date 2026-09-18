#include "WasamiTestInteractable.h"

#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"

AWasamiTestInteractable::AWasamiTestInteractable()
{
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->InitBoxExtent(FVector(50.f));
	Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Box->ComponentTags.Add(TEXT("interact"));
	RootComponent = Box;
}

void AWasamiTestInteractable::InteractWithObject_Implementation(AActor* Interactee)
{
	++InteractCount;
	LastInteractee = Interactee;
}
