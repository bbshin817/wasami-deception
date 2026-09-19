#include "Misc/AutomationTest.h"
#include "../WasamiPlayerCharacter.h"
#include "../WasamiPowerComponent.h"
#include "../WasamiSpeedBarrier.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"
#include "WasamiTestListener.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Where the player (a 50 cm capsule) stands before a barrier at (0, Y, 0) turned toward +X, at the class's size:
	// Box reaches x −103.9 to 100.9 cm, Box1 −24.6 to 10.3.
	constexpr double SpeedBarrierFront = 250.;   // clear of Box
	constexpr double SpeedBarrierInBox = 100.;   // in Box, clear of Box1
	constexpr double SpeedBarrierBehind = -250.; // through both, if nothing stopped the player
	constexpr double SpeedBarrierFar = 1500.;

	/** The player moved with a sweep (overlaps and a blocking hit, as walking into it), and what it hit. */
	FHitResult SweepTo(AActor* Player, double X, double Y)
	{
		FHitResult Hit;
		Player->SetActorLocation(FVector(X, Y, 0.), true, &Hit, ETeleportType::None);
		return Hit;
	}

	void PutAt(AActor* Player, double X, double Y)
	{
		Player->SetActorLocation(FVector(X, Y, 0.), false, nullptr, ETeleportType::TeleportPhysics);
	}

	void AdvanceSpeedBarrierWorld(FTestWorldWrapper& Wrapper, float Seconds)
	{
		for (float Left = Seconds; Left > 1e-4f; Left -= 0.01f)
		{
			Wrapper.TickTestWorld(FMath::Min(Left, 0.01f));
		}
	}

	bool SameTurn(const FRotator& A, const FRotator& B)
	{
		return A.Quaternion().AngularDistance(B.Quaternion()) < 1e-4;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiSpeedBarrierActorTest, "Wasami.SpeedBarrier.Actor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiSpeedBarrierActorTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiSpeedBarrier* Barrier = World->SpawnActor<AWasamiSpeedBarrier>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the barrier"), Barrier))
	{
		return false;
	}
	TestTrue(TEXT("Can Be Destroyed?"), Barrier->bCanBeDestroyed);
	TestTrue(TEXT("the root at 3.2"), Barrier->GetRootComponent()->GetRelativeScale3D().Equals(FVector(3.2), 1e-5));

	// The planes: the engine's Plane standing up, the back one 3 cm behind and larger; they block nothing.
	const UStaticMeshComponent* Front = Barrier->GetStaticMesh1();
	const UStaticMeshComponent* Back = Barrier->GetStaticMesh();
	TestTrue(TEXT("the engine's Plane"), Front->GetStaticMesh() && Front->GetStaticMesh()->GetName() == TEXT("Plane")
		&& Back->GetStaticMesh() == Front->GetStaticMesh());
	TestTrue(TEXT("the front standing up"), SameTurn(Front->GetRelativeRotation(), FRotator(0., 90., -90.)));
	TestTrue(TEXT("the back 3.04 behind, 1.071 times"), Back->GetRelativeLocation().Equals(FVector(-3.0358, 0., 0.), 1e-3)
		&& Back->GetRelativeScale3D().Equals(FVector(1.0710336, 1.0710336, 1.), 1e-5));
	TestTrue(TEXT("no collision"), Front->GetCollisionEnabled() == ECollisionEnabled::NoCollision
		&& Back->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	TestTrue(TEXT("out of the navigation"), !Front->CanEverAffectNavigation() && !Back->CanEverAffectNavigation());
	TestTrue(TEXT("the materials are the level build's"), Front->OverrideMaterials.IsEmpty() && Back->OverrideMaterials.IsEmpty());

	// Box overlaps (1 m deep each way), Box1 blocks pawns only, in queries.
	const UBoxComponent* Box = Barrier->GetBox();
	const UBoxComponent* Box1 = Barrier->GetBox1();
	TestTrue(TEXT("Box 205 × 343 × 368 cm"), Box->GetScaledBoxExtent().Equals(FVector(102.4, 171.569, 184.008), 0.01)
		&& Box->GetRelativeLocation().Equals(FVector(-0.4631, 0., 0.), 1e-3));
	TestTrue(TEXT("Box overlaps pawns"), Box->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap
		&& Box->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
	TestTrue(TEXT("Box1 35 × 361 × 361 cm"), Box1->GetScaledBoxExtent().Equals(FVector(17.4229, 180.652, 180.652), 0.01)
		&& Box1->GetRelativeLocation().Equals(FVector(-2.2415, 0., 0.), 1e-3));
	TestTrue(TEXT("Box1 blocks pawns in queries"), Box1->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block
		&& Box1->GetCollisionEnabled() == ECollisionEnabled::QueryOnly && Box1->GetCollisionObjectType() == ECC_WorldStatic);
	TestTrue(TEXT("and ignores the world and the traces"), Box1->GetCollisionResponseToChannel(ECC_WorldStatic) == ECR_Ignore
		&& Box1->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Ignore
		&& Box1->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Ignore
		&& Box1->GetCollisionResponseToChannel(ECC_Camera) == ECR_Ignore
		&& Box1->GetCollisionResponseToChannel(ECC_PhysicsBody) == ECR_Ignore);
	TestTrue(TEXT("neither in the navigation"), !Box->CanEverAffectNavigation() && !Box1->CanEverAffectNavigation());

	// A red light and the hum.
	const UPointLightComponent* Light = Barrier->GetPointLight();
	TestTrue(TEXT("a red light"), Light->LightColor == FColor(255, 0, 0, 255) && Light->Intensity == 2500.f
		&& Light->IntensityUnits == ELightUnits::Unitless && Light->AttenuationRadius == 400.f);
	TestTrue(TEXT("its radii"), FMath::IsNearlyEqual(Light->SourceRadius, 124.1857f, 1e-3f)
		&& FMath::IsNearlyEqual(Light->SoftSourceRadius, 42.5886f, 1e-3f));
	const UAudioComponent* Audio = Barrier->GetAudio();
	TestTrue(TEXT("Barrier_Loop"), Audio->Sound && Audio->Sound->GetName() == TEXT("Barrier_Loop"));
	TestEqual(TEXT("at 0.3"), Audio->VolumeMultiplier, 0.3f);
	TestTrue(TEXT("natural, over 2000 cm"), Audio->bOverrideAttenuation
		&& Audio->AttenuationOverrides.DistanceAlgorithm == EAttenuationDistanceModel::NaturalSound
		&& Audio->AttenuationOverrides.FalloffDistance == 2000.f);
	TestFalse(TEXT("whole"), Barrier->HasShattered());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiSpeedBarrierBreakTest, "Wasami.SpeedBarrier.Break",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiSpeedBarrierBreakTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	constexpr double LaneA = 0.;
	constexpr double LaneB = 3000.;
	AWasamiSpeedBarrier* Barrier = World->SpawnActor<AWasamiSpeedBarrier>(FVector(0., LaneA, 0.), FRotator::ZeroRotator);
	AWasamiSpeedBarrier* Kept = World->SpawnActor<AWasamiSpeedBarrier>(FVector(0., LaneB, 0.), FRotator::ZeroRotator);
	AWasamiPlayerCharacter* Player = World->SpawnActor<AWasamiPlayerCharacter>(FVector(SpeedBarrierFar, LaneA, 0.), FRotator::ZeroRotator);
	// The first player controller's character without possessing it (so the speed boost makes no widgets).
	APlayerController* Controller = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the barrier"), Barrier) || !TestNotNull(TEXT("another"), Kept)
		|| !TestNotNull(TEXT("the player"), Player) || !TestNotNull(TEXT("a controller"), Controller))
	{
		return false;
	}
	Controller->SetPawn(Player);
	Player->GetCharacterMovement()->DisableMovement();
	UWasamiPowerComponent* Powers = Player->GetPowers();
	if (!TestTrue(TEXT("the player is the player"), UGameplayStatics::GetPlayerCharacter(World, 0) == Player)
		|| !TestTrue(TEXT("the left socket is the speed boost"), Powers->GetSocketPower(true) == EWasamiPower::SpeedBoost))
	{
		return false;
	}
	Kept->bCanBeDestroyed = false;
	const TStrongObjectPtr<UWasamiTestListener> Listener(NewObject<UWasamiTestListener>());
	Barrier->OnBarrierDestroyed.AddDynamic(Listener.Get(), &UWasamiTestListener::Hear);
	Kept->OnBarrierDestroyed.AddDynamic(Listener.Get(), &UWasamiTestListener::Hear);

	// Without Can Be Destroyed?, running into Box1 with the speed boost only stops the player.
	PutAt(Player, SpeedBarrierInBox, LaneB);
	TestTrue(TEXT("in the other's Box"), Kept->GetBox()->IsOverlappingActor(Player));
	Powers->UsePower(true);
	if (!TestTrue(TEXT("the speed boost on"), Powers->IsUsingPower(EWasamiPower::SpeedBoost)))
	{
		return false;
	}
	FHitResult Hit = SweepTo(Player, SpeedBarrierBehind, LaneB);
	TestTrue(TEXT("Box1 stops the player"), Hit.bBlockingHit && Hit.GetComponent() == Kept->GetBox1()
		&& Player->GetActorLocation().X > 50.);
	TestTrue(TEXT("not broken: Can Be Destroyed? is false"), IsValid(Kept) && !Kept->HasShattered());

	// Without the speed boost, the player comes into Box and is stopped by Box1.
	Powers->ResetPowers();
	TestFalse(TEXT("the speed boost off"), Powers->IsUsingPower(EWasamiPower::SpeedBoost));
	AdvanceSpeedBarrierWorld(Wrapper, 0.6f);   // the 0.5 s before a power can be used again
	PutAt(Player, SpeedBarrierFront, LaneA);
	TestFalse(TEXT("clear of Box"), Barrier->GetBox()->IsOverlappingActor(Player));
	Hit = SweepTo(Player, SpeedBarrierBehind, LaneA);
	// At Box1's face and the capsule's radius (60.3), and the sweep's pull-back (61.75 here).
	TestTrue(TEXT("stopped by Box1"), Hit.bBlockingHit && Hit.GetComponent() == Barrier->GetBox1()
		&& Player->GetActorLocation().X > 60. && Player->GetActorLocation().X < 63.);
	TestTrue(TEXT("in Box"), Barrier->GetBox()->IsOverlappingActor(Player));
	TestTrue(TEXT("whole"), IsValid(Barrier) && !Barrier->HasShattered());

	// With it, running into Box1 shatters it: Destroyed, and the actor gone.
	Powers->UsePower(true);
	if (!TestTrue(TEXT("the speed boost on again"), Powers->IsUsingPower(EWasamiPower::SpeedBoost)))
	{
		return false;
	}
	TestTrue(TEXT("nothing until the player moves"), IsValid(Barrier) && !Barrier->HasShattered());
	Hit = SweepTo(Player, SpeedBarrierBehind, LaneA);
	TestTrue(TEXT("shattered by the hit"), Barrier->HasShattered() && !IsValid(Barrier));
	TestEqual(TEXT("Destroyed once"), Listener->Count, 1);

	// Coming into Box with it shatters one without Can Be Destroyed? too.
	PutAt(Player, SpeedBarrierFar, LaneB);
	SweepTo(Player, SpeedBarrierInBox, LaneB);
	TestTrue(TEXT("shattered by the overlap"), Kept->HasShattered() && !IsValid(Kept));
	TestEqual(TEXT("Destroyed again"), Listener->Count, 2);

	// Nothing more once broken.
	Barrier->BreakIfBoosting();
	TestEqual(TEXT("once each"), Listener->Count, 2);
	return true;
}

#endif
