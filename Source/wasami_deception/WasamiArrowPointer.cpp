#include "WasamiArrowPointer.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "WasamiAssets.h"
#include "WasamiGameMode.h"
#include "WasamiPlayerCharacter.h"
#include "WasamiShard.h"
#include "WasamiZoneFlow.h"
#include "WasamiZoneShardChecker.h"

namespace
{
	// BP_ArrowPointer's Plane_GEN_VARIABLE.
	const FRotator ArrowPlaneRotation(0., 90.00023651123047, 0.);
	const FVector ArrowPlaneScale(1.5, 1.5, 1.);
	const FName ArrowColorName(TEXT("Color"));

	// Smooth Rotation: RInterpTo's and FInterpTo's speeds, MapRangeClamped(horizontal distance, 0, 3000, 7, 4) and
	// Select(mapZoomedOut?, 10, 0).
	constexpr float ArrowTurnSpeed = 20.f;
	constexpr float ArrowSizeSpeed = 5.f;
	const FVector2D ArrowDistanceRange(0., 3000.);
	const FVector2D ArrowSizeRange(7., 4.);
	constexpr double ArrowZoomedOutSize = 10.;

	// FindClosestShard's Closest Distance to begin with.
	constexpr double ArrowFarthestShard = 99999.;
}

// BP_DD_PlayerCharacter's BP_ArrowPointer_GEN_VARIABLE (a ChildActorComponent on CharacterMesh0).
const FVector AWasamiArrowPointer::PlayerRelativeLocation(0., 0., 2000.);
const FVector AWasamiArrowPointer::PlayerRelativeScale(5., 5., 1.);

AWasamiArrowPointer::AWasamiArrowPointer()
{
	// The original ticks, doing nothing; its work is on the timers.
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	// The engine's own content, which the pipeline never rebuilds, so it may load here.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	Plane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plane"));
	Plane->SetupAttachment(DefaultSceneRoot);
	Plane->SetStaticMesh(PlaneMesh.Object);
	Plane->SetRelativeRotation(ArrowPlaneRotation);
	Plane->SetRelativeScale3D(ArrowPlaneScale);
	Plane->SetCastShadow(false);
	// The original leaves the component's default (blocking, with the Plane's box): a board up to 25 m wide moving
	// 20 m over the player, in the way of whatever is on a floor above. Here it is only a mark for the map.
	Plane->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Plane->SetCanEverAffectNavigation(false);

	ArrowMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/Special/M_Arrow_Inst")));
}

void AWasamiArrowPointer::BeginPlay()
{
	Super::BeginPlay();
	Plane->SetMaterial(0, ArrowMaterial.LoadSynchronous());

	FTimerManager& Timers = GetWorldTimerManager();
	Timers.SetTimer(SetRotationTimer, this, &AWasamiArrowPointer::SetRotation, RotationRate, true);
	Timers.SetTimer(SmoothRotationTimer, this, &AWasamiArrowPointer::SmoothRotation, RotationRate, true);
	Timers.SetTimer(FindObjectTimer, this, &AWasamiArrowPointer::FindObject, FindObjectRate, true);

	// The original casts GetPlayerCharacter(0), the one carrying it; the actor it is a child of comes first here (a
	// player spawned in a test's world has no controller).
	AWasamiPlayerCharacter* Character = Cast<AWasamiPlayerCharacter>(GetParentActor());
	if (!Character)
	{
		Character = Cast<AWasamiPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	}
	if (Character)
	{
		Player = Character;
		PlayerCamera = Character->GetCamera();
	}
}

void AWasamiArrowPointer::ChangeColor(const FLinearColor& NewColor)
{
	Color = NewColor;
	Plane->SetVectorParameterValueOnMaterials(ArrowColorName, FVector(NewColor));
}

void AWasamiArrowPointer::TakeUpZone()
{
	const AWasamiGameMode* Mode = GetWorld()->GetAuthGameMode<AWasamiGameMode>();
	const AWasamiZoneFlow* Flow = Mode ? Mode->GetZoneFlow() : nullptr;
	if (!Flow)
	{
		return;
	}
	bShards = Flow->IsArrowOnShards();
	if (!bShards)
	{
		Target = Flow->GetArrowTarget();
	}
	const TOptional<FLinearColor> Given = Flow->GetArrowColor();
	if (Given.IsSet() && !(Color.IsSet() && Color->Equals(*Given, 0.f)))
	{
		ChangeColor(*Given);
	}
}

AActor* AWasamiArrowPointer::ZoneShards(TArray<AActor*>& Shards) const
{
	Shards.Reset();
	AWasamiPlayerCharacter* Character = Player.Get();
	if (!Character)
	{
		return nullptr;
	}
	TArray<AActor*> Checkers;
	Character->GetOverlappingActors(Checkers, AWasamiZoneShardChecker::StaticClass());
	AActor* Checker = Checkers.IsValidIndex(0) ? Checkers[0] : nullptr;
	if (IsValid(Checker))
	{
		Checker->GetOverlappingActors(Shards, AWasamiShard::StaticClass());
	}
	return Checker;
}

void AWasamiArrowPointer::FindObject()
{
	if (!Player.IsValid())
	{
		return;
	}
	TakeUpZone();
	if (!bShards)
	{
		Plane->SetVisibility(IsValid(Target.Get()), false);
		return;
	}
	TArray<AActor*> Shards;
	if (!IsValid(ZoneShards(Shards)) || Shards.Num() >= ShardsPointedBelow)
	{
		Plane->SetVisibility(false, false);
		return;
	}
	// None left in the box (or the first gone): shown, at whatever it pointed at before.
	if (Shards.Num() > 0 && IsValid(Shards[0]))
	{
		Target = FindClosestShard();
	}
	Plane->SetVisibility(true, false);
}

AActor* AWasamiArrowPointer::FindClosestShard() const
{
	const AWasamiPlayerCharacter* Character = Player.Get();
	TArray<AActor*> Shards;
	ZoneShards(Shards);
	AActor* Closest = nullptr;
	double ClosestDistance = ArrowFarthestShard;
	for (AActor* Shard : Shards)
	{
		const double Distance = Shard->GetDistanceTo(Character);
		if (Distance < ClosestDistance)
		{
			Closest = Shard;
			ClosestDistance = Distance;
		}
	}
	return Closest;
}

void AWasamiArrowPointer::SetRotation()
{
	const AActor* Pointed = Target.Get();
	const UCameraComponent* Camera = PlayerCamera.Get();
	if (!IsValid(Pointed) || !Camera)
	{
		return;
	}
	// MoreporkFunctions' FindLookAtRotation (YawOnly): both points on the ground plane.
	const FVector From = Camera->GetComponentLocation();
	const FVector To = Pointed->GetActorLocation();
	TargetRotation = UKismetMathLibrary::FindLookAtRotation(FVector(From.X, From.Y, 0.), FVector(To.X, To.Y, 0.));
}

void AWasamiArrowPointer::SmoothRotation()
{
	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaSeconds, ArrowTurnSpeed));
	const AActor* Pointed = Target.Get();
	if (!IsValid(Pointed))
	{
		return;
	}
	const AWasamiPlayerCharacter* Character = Player.Get();
	const double Size = FMath::GetMappedRangeValueClamped(ArrowDistanceRange, ArrowSizeRange,
		Pointed->GetHorizontalDistanceTo(this)) + (Character && Character->IsMapZoomedOut() ? ArrowZoomedOutSize : 0.);
	const double X = FMath::FInterpTo(GetActorScale3D().X, Size, DeltaSeconds, ArrowSizeSpeed);
	SetActorScale3D(FVector(X, X, 1.));
}
