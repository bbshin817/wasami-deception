#include "WasamiMapArea.h"

#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "WasamiShard.h"

AWasamiMapArea::AWasamiMapArea()
{
	PrimaryActorTick.bCanEverTick = false;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	RootComponent = Box;
	// Box_GEN_VARIABLE: a custom profile on the shape's own object type (world dynamic).
	Box->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	Box->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AWasamiMapArea::GetShards(TArray<AActor*>& Shards) const
{
	Box->GetOverlappingActors(Shards, AWasamiShard::StaticClass());
}
