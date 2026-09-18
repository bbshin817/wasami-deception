#include "WasamiMapTextureMultiFloor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "WasamiMapArea.h"
#include "WasamiShard.h"

const FName AWasamiMapTextureMultiFloor::TextureParameter(TEXT("Texture"));

AWasamiMapTextureMultiFloor::AWasamiMapTextureMultiFloor()
{
	// Movable (the original's is a static mesh actor's static plane): its material changes as the game goes, it is in
	// no baked lighting (only the capture's base colour draws it), and the level build can put it in again without
	// leaving the lighting unbuilt.
	GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
}

void AWasamiMapTextureMultiFloor::BeginPlay()
{
	Super::BeginPlay();
	DynamicMat = GetStaticMeshComponent()->CreateDynamicMaterialInstance(0);
	GetWorldTimerManager().SetTimer(CheckMapTimer, this, &AWasamiMapTextureMultiFloor::CheckMap, CheckMapRate, true);
}

void AWasamiMapTextureMultiFloor::CheckMap()
{
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!IsValid(Player))
	{
		return;
	}
	TArray<AActor*> Areas;
	Player->GetOverlappingActors(Areas, AWasamiMapArea::StaticClass());
	if (!Areas.IsValidIndex(0) || !IsValid(Areas[0]))
	{
		return;
	}
	CurrentMapArea = Cast<AWasamiMapArea>(Areas[0]);
	// Map_Find: no texture for a box that is not in Map.
	const TObjectPtr<UTexture>* Found = Map.Find(CurrentMapArea);
	if (DynamicMat)
	{
		DynamicMat->SetTextureParameterValue(TextureParameter, Found ? Found->Get() : nullptr);
	}

	// Every shard's mark hidden, then the box's shown.
	TArray<AActor*> Shards;
	UGameplayStatics::GetAllActorsOfClass(this, AWasamiShard::StaticClass(), Shards);
	for (AActor* Actor : Shards)
	{
		if (UStaticMeshComponent* Plane = CastChecked<AWasamiShard>(Actor)->GetPlane())
		{
			Plane->SetVisibility(false, false);
		}
	}
	Shards.Reset();
	CurrentMapArea->GetShards(Shards);
	for (AActor* Actor : Shards)
	{
		if (UStaticMeshComponent* Plane = CastChecked<AWasamiShard>(Actor)->GetPlane())
		{
			Plane->SetVisibility(true, false);
		}
	}
}
