#include "WasamiEnemySentry.h"

#include "Components/CapsuleComponent.h"
#include "Components/ChildActorComponent.h"
#include "Kismet/KismetMathLibrary.h"

const FVector AWasamiEnemySentry::ViewconeLocation(0., 0., 72.);
const FRotator AWasamiEnemySentry::ViewconeRotation(-19.999958038330078, 0., 0.);

AWasamiEnemySentry::AWasamiEnemySentry()
{
	bAggressiveIdle = true;

	Viewcone = CreateDefaultSubobject<UChildActorComponent>(TEXT("Viewcone"));
	Viewcone->SetupAttachment(GetCapsuleComponent());
	Viewcone->SetChildActorClass(AWasamiViewconeNurse::StaticClass());
	Viewcone->SetRelativeLocationAndRotation(ViewconeLocation, ViewconeRotation);

	JumpDownSpot = CreateDefaultSubobject<USceneComponent>(TEXT("JumpDownSpot"));
	JumpDownSpot->SetupAttachment(GetCapsuleComponent());
}

AWasamiViewcone* AWasamiEnemySentry::GetViewcone() const
{
	return IsValid(Viewcone) ? Cast<AWasamiViewcone>(Viewcone->GetChildActor()) : nullptr;
}

void AWasamiEnemySentry::Activate()
{
	if (AWasamiViewcone* Cone = GetViewcone())
	{
		Cone->Offset = Offset;
		Cone->Initialize();
	}
}

void AWasamiEnemySentry::PlayerSpotted_Implementation()
{
	if (bSpottedClosed)
	{
		return;
	}
	bSpottedClosed = true;
	// The 06_NurseAlert achievement (Set Struct Achievement) is left out.
	if (IsValid(Viewcone))
	{
		Viewcone->DestroyComponent();
	}
	bChasing = true;
	const FVector Way = UKismetMathLibrary::GetDirectionUnitVector(GetActorLocation(), JumpDownSpot->GetComponentLocation());
	LaunchCharacter(FVector(Way.X * JumpSpeed, Way.Y * JumpSpeed, JumpUpSpeed), true, true);
	// The nurse's BeginPlay (Super's: CanSpawn, the random point and Make Choice), then the tick's gate opened.
	Super::BeginNurse();
}
