#include "Misc/AutomationTest.h"
#include "../WasamiLift.h"
#include "../WasamiPlayerCharacter.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Seconds of play in ticks of 0.05 s at most (the lifts move 13–27 cm a tick). */
	void AdvanceLiftWorld(FTestWorldWrapper& Wrapper, float Seconds)
	{
		Wrapper.TickTestWorld(0.f);
		for (float Left = Seconds; Left > 1e-4f; Left -= 0.05f)
		{
			Wrapper.TickTestWorld(FMath::Min(Left, 0.05f));
		}
	}

	const FVector FarAway(5000., 0., 100.);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiLiftTest, "Wasami.Lift.Actor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiLiftTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiLift* Lift = World->SpawnActor<AWasamiLift>(FVector::ZeroVector, FRotator::ZeroRotator);
	// No controller: the player stays where it is put. The walker falls and rides as a controlled character does.
	AWasamiPlayerCharacter* Player = World->SpawnActor<AWasamiPlayerCharacter>(FarAway, FRotator::ZeroRotator);
	ACharacter* Walker = World->SpawnActor<ACharacter>(FVector(0., -5000., 100.), FRotator::ZeroRotator);
	AActor* Listener = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the lift"), Lift) || !TestNotNull(TEXT("the player"), Player) || !TestNotNull(TEXT("a character"), Walker)
		|| !TestNotNull(TEXT("a listener"), Listener))
	{
		return false;
	}
	Walker->GetCharacterMovement()->bRunPhysicsWithNoController = true;
	Walker->GetCharacterMovement()->SetMovementMode(MOVE_None);

	// BP_06_LiftBase's parts; BP_06_Lift takes LiftCollision1 away when play begins.
	TestEqual(TEXT("Top Location 535"), Lift->TopLocation, 535.f);
	TestTrue(TEXT("the floor's box, 291 × 294 × 76 cm"), Lift->GetLiftCollision()->GetScaledBoxExtent().Equals(FVector(145.675, 146.756, 38.084), 1e-2)
		&& Lift->GetLiftCollision()->GetAttachParent() == Lift->GetLiftMesh());
	TestTrue(TEXT("it blocks"), Lift->GetLiftCollision()->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block
		&& Lift->GetLiftCollision()->GetCollisionObjectType() == ECC_WorldStatic);
	TestTrue(TEXT("the mesh does not"), Lift->GetLiftMesh()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	TestTrue(TEXT("the box over it overlaps pawns"), Lift->GetLiftCollisionOverlap()->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap
		&& Lift->GetLiftCollisionOverlap()->GetRelativeLocation().Equals(FVector(0., 0., 63.5114), 1e-3));
	TestTrue(TEXT("the box under it, 611 cm deep"), Lift->GetBottomCollision()->GetScaledBoxExtent().Equals(FVector(145.675, 146.756, 305.564), 1e-2)
		&& Lift->GetBottomCollision()->GetRelativeLocation().Equals(FVector(0., 0., -288.75), 1e-3));
	TestNull(TEXT("no LiftCollision1 in play"), Lift->GetLiftCollision1());
	TestFalse(TEXT("the clunk waits"), Lift->GetAudio()->bAutoActivate);
	TestTrue(TEXT("the loop at 0.7 and pitch 1.5"), !Lift->GetMovementAudio()->bAutoActivate
		&& FMath::IsNearlyEqual(Lift->GetMovementAudio()->VolumeMultiplier, 0.7f) && FMath::IsNearlyEqual(Lift->GetMovementAudio()->PitchMultiplier, 1.5f));

	// Nobody on it: it stays down.
	AdvanceLiftWorld(Wrapper, 0.3f);
	TestEqual(TEXT("down"), Lift->GetHeight(), 0.f);
	TestFalse(TEXT("still"), Lift->IsMoving() || Lift->IsMovementSoundOn());

	// A character on it: up at 267.5 cm/s (Top Location × 0.5), carrying it; the loop plays while it moves.
	Walker->SetActorLocation(FVector(0., 0., 140.), false, nullptr, ETeleportType::TeleportPhysics);
	Walker->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	AdvanceLiftWorld(Wrapper, 0.1f);
	TestTrue(TEXT("a character on top"), Lift->IsCharacterOnTop());
	const float Start = Lift->GetHeight();
	AdvanceLiftWorld(Wrapper, 1.f);
	TestEqual(TEXT("267.5 cm in a second"), Lift->GetHeight() - Start, 267.5f, 15.f);
	TestTrue(TEXT("moving, with the loop"), Lift->IsMoving() && Lift->IsMovementSoundOn());
	AdvanceLiftWorld(Wrapper, 1.2f);
	TestEqual(TEXT("at the top"), Lift->GetHeight(), 535.f);
	TestFalse(TEXT("stopped, the loop off"), Lift->IsMoving() || Lift->IsMovementSoundOn());
	TestTrue(TEXT("it carried the character"), Walker->GetActorLocation().Z > 535. + 38. + 70.);

	// Off it: back down at 535 cm/s.
	Walker->GetCharacterMovement()->SetMovementMode(MOVE_None);
	Walker->SetActorLocation(FVector(0., -5000., 100.), false, nullptr, ETeleportType::TeleportPhysics);
	AdvanceLiftWorld(Wrapper, 0.5f);
	TestEqual(TEXT("halfway down in 0.5 s"), Lift->GetHeight(), 267.5f, 30.f);
	AdvanceLiftWorld(Wrapper, 0.6f);
	TestEqual(TEXT("down in 1 s"), Lift->GetHeight(), 0.f);

	// Player Overlap: the player only.
	FScriptDelegate Heard;
	Heard.BindUFunction(Listener, TEXT("K2_DestroyActor"));
	Lift->OnPlayerOverlap.Add(Heard);
	Lift->NotifyOverlap(Walker);
	TestTrue(TEXT("not for another character"), IsValid(Listener));
	Lift->NotifyOverlap(Player);
	TestFalse(TEXT("Player Overlap for the player"), IsValid(Listener));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiCornerLiftTest, "Wasami.Lift.Corner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiCornerLiftTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiCornerLift* Lift = World->SpawnActor<AWasamiCornerLift>(FVector::ZeroVector, FRotator::ZeroRotator);
	AWasamiPlayerCharacter* Player = World->SpawnActor<AWasamiPlayerCharacter>(FarAway, FRotator::ZeroRotator);
	// The first player controller's character, without possessing it (so it stays where it is put).
	APlayerController* Controller = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the lift"), Lift) || !TestNotNull(TEXT("the player"), Player) || !TestNotNull(TEXT("a controller"), Controller))
	{
		return false;
	}
	Controller->SetPawn(Player);
	if (!TestTrue(TEXT("the player is the player"), UGameplayStatics::GetPlayerCharacter(World, 0) == Player))
	{
		return false;
	}
	TestTrue(TEXT("LiftCollision1 stays at the top"), Lift->GetLiftCollision1() != nullptr
		&& Lift->GetLiftCollision1()->GetRelativeLocation().Equals(FVector(0., 0., 535.))
		&& Lift->GetLiftCollision1()->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);

	// It waits on the player's floor: the bottom, then the top (higher than 610 cm), at 535 cm/s with nobody on it.
	AdvanceLiftWorld(Wrapper, 0.3f);
	TestEqual(TEXT("down with the player below"), Lift->GetHeight(), 0.f);
	Player->SetActorLocation(FVector(5000., 0., 700.));
	AdvanceLiftWorld(Wrapper, 0.5f);
	TestEqual(TEXT("halfway up in 0.5 s"), Lift->GetHeight(), 267.5f, 30.f);
	AdvanceLiftWorld(Wrapper, 0.6f);
	TestEqual(TEXT("up with the player above"), Lift->GetHeight(), 535.f);
	Player->SetActorLocation(FarAway);
	AdvanceLiftWorld(Wrapper, 1.1f);
	TestEqual(TEXT("down again"), Lift->GetHeight(), 0.f);

	// Walking onto it starts Double Check; walking off before 1 s cancels it.
	Player->SetActorLocation(FVector(0., 0., 130.));
	AdvanceLiftWorld(Wrapper, 0.05f);
	TestTrue(TEXT("Double Check waits"), Lift->IsDoubleCheckPending());
	AdvanceLiftWorld(Wrapper, 0.5f);
	Player->SetActorLocation(FarAway);
	AdvanceLiftWorld(Wrapper, 0.05f);
	TestFalse(TEXT("cancelled"), Lift->IsDoubleCheckPending());
	AdvanceLiftWorld(Wrapper, 1.f);
	TestFalse(TEXT("nothing forced"), Lift->bPlayerForceMovement);
	TestEqual(TEXT("still down"), Lift->GetHeight(), 0.f);

	// Standing on it for a second: sent to the other floor (up), at 267.5 cm/s with the player on it.
	Player->SetActorLocation(FVector(0., 0., 130.));
	AdvanceLiftWorld(Wrapper, 1.1f);
	TestTrue(TEXT("forced up"), Lift->bPlayerForceMovement && Lift->bGoUp);
	const float Start = Lift->GetHeight();
	AdvanceLiftWorld(Wrapper, 0.2f);
	TestEqual(TEXT("rising at 267.5 cm/s"), Lift->GetHeight() - Start, 53.5f, 14.f);
	TestTrue(TEXT("moving"), Lift->IsMoving());

	// Off it: no longer forced, back to the player's floor.
	Player->SetActorLocation(FarAway);
	AdvanceLiftWorld(Wrapper, 0.05f);
	TestFalse(TEXT("not forced once off"), Lift->bPlayerForceMovement);
	AdvanceLiftWorld(Wrapper, 0.5f);
	TestEqual(TEXT("down"), Lift->GetHeight(), 0.f);
	return true;
}

#endif
