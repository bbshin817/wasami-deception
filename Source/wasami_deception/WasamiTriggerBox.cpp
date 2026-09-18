#include "WasamiTriggerBox.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

AWasamiTriggerBox::AWasamiTriggerBox()
{
	PrimaryActorTick.bCanEverTick = false;

	// Box_GEN_VARIABLE: a UBoxComponent at its defaults (32 cm half extent, hidden in game, query only) whose collision is
	// Custom — a WorldStatic object that overlaps pawns and ignores the engine's other channels. The project's Teleport
	// channel is not listed, so it keeps its default (Overlap), which leaves the teleport's trace alone.
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetCollisionObjectType(ECC_WorldStatic);
	Box->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Destructible, ECR_Ignore);
	SetRootComponent(Box);
	// The class's empty Cube (a StaticMeshComponent without a mesh) and its Debug print are left out.
}

void AWasamiTriggerBox::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	if (OtherActor && OtherActor == UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		NotifyPlayerOverlap(true);
	}
}

void AWasamiTriggerBox::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);
	if (OtherActor && OtherActor == UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		NotifyPlayerOverlap(false);
	}
}

void AWasamiTriggerBox::NotifyPlayerOverlap(bool bBegin)
{
	// ReceiveActorBeginOverlap (@244) and ReceiveActorEndOverlap (@369): each closes its own DoOnce whether or not it
	// fires, so the way End Overlap does not ask for is spent on the first pass as well.
	bool& bClosed = bBegin ? bBeginClosed : bEndClosed;
	if (bClosed)
	{
		return;
	}
	bClosed = true;
	if (bBegin != bEndOverlap)
	{
		bFired = true;
		OnTrigger.Broadcast();
	}
}
