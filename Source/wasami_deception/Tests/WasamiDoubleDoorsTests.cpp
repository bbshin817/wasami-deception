#include "Misc/AutomationTest.h"
#include "../WasamiDoubleDoors.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Seconds of play in ticks of 0.1 s at most (as the zone flow's tests). */
	void AdvanceDoorsWorld(FTestWorldWrapper& Wrapper, float Seconds)
	{
		Wrapper.TickTestWorld(0.f);
		for (float Left = Seconds; Left > 1e-4f; Left -= 0.1f)
		{
			Wrapper.TickTestWorld(FMath::Min(Left, 0.1f));
		}
	}

	float Yaw(const UStaticMeshComponent* Door)
	{
		return static_cast<float>(Door->GetRelativeRotation().Yaw);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiDoubleDoorsTest, "Wasami.DoubleDoors.Actor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiDoubleDoorsTest::RunTest(const FString& Parameters)
{
	// The timelines' Float track (the export's keys and tangents): past 1 at 0.69 s, back to 1 at the end.
	TestEqual(TEXT("shut at the start"), AWasamiDoubleDoors::EvaluateSwing(0.f), 0.f);
	TestEqual(TEXT("0.95 at 0.55 s"), AWasamiDoubleDoors::EvaluateSwing(0.5519317984580994f), 0.9510974884033203f, 1e-5f);
	TestEqual(TEXT("1.04 at 0.69 s"), AWasamiDoubleDoors::EvaluateSwing(0.6852617859840393f), 1.0404237508773804f, 1e-5f);
	TestEqual(TEXT("1 at 1 s"), AWasamiDoubleDoors::EvaluateSwing(1.f), 1.f, 1e-5f);
	TestTrue(TEXT("rising at 0.3 s"), AWasamiDoubleDoors::EvaluateSwing(0.3f) > 0.1f && AWasamiDoubleDoors::EvaluateSwing(0.3f) < 0.9f);

	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiDoubleDoors* Doors = World->SpawnActor<AWasamiDoubleDoors>(FVector::ZeroVector, FRotator::ZeroRotator);
	// A nurse or the player, away from the boxes (the tests walk them in and out).
	ACharacter* Walker = World->SpawnActor<ACharacter>(FVector(0., 0., -50000.), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the doors"), Doors) || !TestNotNull(TEXT("a character"), Walker))
	{
		return false;
	}
	UStaticMeshComponent* Door = Doors->GetStaticMesh();
	UStaticMeshComponent* Door1 = Doors->GetStaticMesh1();
	TestTrue(TEXT("interact"), Doors->ActorHasTag(TEXT("interact")));
	TestEqual(TEXT("90°"), Doors->OpenAmount, 90.f);
	TestTrue(TEXT("StaticMesh at x −200"), Door->GetRelativeLocation().Equals(FVector(-200., 0., 0.)));
	TestTrue(TEXT("StaticMesh1 at x +200"), Door1->GetRelativeLocation().Equals(FVector(200., 0., 0.)));
	TestTrue(TEXT("the doors block"), Door->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics
		&& Door->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);
	TestTrue(TEXT("the front box, 400 × 180 cm in front"), Doors->GetFrontEnter()->GetScaledBoxExtent().Equals(FVector(200.279, 90.239, 32.), 1e-2)
		&& Doors->GetFrontEnter()->GetRelativeLocation().Equals(FVector(0., -150., 50.)));
	TestTrue(TEXT("the back box behind"), Doors->GetBackEnter()->GetScaledBoxExtent().Equals(FVector(200.279, 95.114, 32.), 1e-2)
		&& Doors->GetBackEnter()->GetRelativeLocation().Equals(FVector(0., 150., 50.)));
	TestTrue(TEXT("Leave over both"), Doors->GetLeave()->GetScaledBoxExtent().Equals(FVector(200.279, 386.087, 32.), 1e-2));
	TestTrue(TEXT("the boxes overlap pawns"), Doors->GetLeave()->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap);

	// Only a character opens them.
	Doors->NotifyFrontEnter(Doors);
	TestFalse(TEXT("not by anything else"), Doors->AnySideOpen());

	// From the front: both doors swing toward the back (StaticMesh by +90°, StaticMesh1 by −90°) in 1 s.
	Doors->NotifyFrontEnter(Walker);
	TestTrue(TEXT("open from the front"), Doors->IsOpenFront() && Doors->IsOpening());
	TestEqual(TEXT("from shut"), Yaw(Door), 0.f, 1e-3f);
	// The tick of nothing is the world's MinUndilatedFrameTime (0.5 ms), which puts the swing 0.1° on.
	AdvanceDoorsWorld(Wrapper, 0.5f);
	TestEqual(TEXT("halfway at 0.5 s"), Yaw(Door), 90.f * AWasamiDoubleDoors::EvaluateSwing(0.5005f), 0.01f);
	TestEqual(TEXT("the other the other way"), Yaw(Door1), -90.f * AWasamiDoubleDoors::EvaluateSwing(0.5005f), 0.01f);
	AdvanceDoorsWorld(Wrapper, 0.6f);
	TestFalse(TEXT("done by 1 s"), Doors->IsOpening());
	TestEqual(TEXT("open 90°"), Yaw(Door), 90.f, 0.01f);
	Doors->NotifyBackEnter(Walker);
	TestFalse(TEXT("open is open"), Doors->IsOpenBack());

	// Walking out of Leave (no player left in it) swings them shut, from where they are.
	Doors->NotifyLeave(Walker);
	TestTrue(TEXT("shutting"), !Doors->AnySideOpen() && Doors->IsClosing());
	TestEqual(TEXT("from open"), Yaw(Door), 90.f, 0.01f);
	AdvanceDoorsWorld(Wrapper, 1.1f);
	TestFalse(TEXT("shut by 1 s"), Doors->IsClosing());
	TestEqual(TEXT("shut"), Yaw(Door), 0.f, 0.01f);

	// From the back: the other way round.
	Doors->NotifyBackEnter(Walker);
	TestTrue(TEXT("open from the back"), Doors->IsOpenBack());
	AdvanceDoorsWorld(Wrapper, 1.1f);
	TestEqual(TEXT("StaticMesh at −90°"), Yaw(Door), -90.f, 0.01f);
	TestEqual(TEXT("StaticMesh1 at +90°"), Yaw(Door1), 90.f, 0.01f);
	Doors->ForceClose();
	TestTrue(TEXT("Force Close"), !Doors->AnySideOpen() && Doors->IsClosing());
	AdvanceDoorsWorld(Wrapper, 1.1f);
	TestEqual(TEXT("shut again"), Yaw(Door), 0.f, 0.01f);

	// Locked, walking in only rattles them, at most once in 2 s; Unlock opens them for the one in front.
	Doors->Lock();
	TestTrue(TEXT("locked"), Doors->bLocked);
	Doors->NotifyFrontEnter(Walker);
	TestFalse(TEXT("they hold"), Doors->AnySideOpen());
	TestTrue(TEXT("a rattle"), Doors->IsLockedSoundBlocked());
	AdvanceDoorsWorld(Wrapper, AWasamiDoubleDoors::LockedSoundDelay + 0.1f);
	TestFalse(TEXT("ready to rattle again 2 s on"), Doors->IsLockedSoundBlocked());
	Doors->Unlock();
	TestTrue(TEXT("Unlock opens them for the one in front"), Doors->IsOpenFront());

	// Lock with the doors open and a character that walked out of Leave: shut.
	Doors->Lock();
	TestTrue(TEXT("Lock shuts them"), !Doors->AnySideOpen() && Doors->IsClosing());
	AdvanceDoorsWorld(Wrapper, 1.1f);
	Doors->OpenFront();
	TestFalse(TEXT("Open Front, locked: nothing"), Doors->AnySideOpen());
	Doors->bLocked = false;
	Doors->OpenFront();
	TestTrue(TEXT("unlocked: open from the front"), Doors->IsOpenFront());
	AdvanceDoorsWorld(Wrapper, 1.1f);
	TestEqual(TEXT("open"), Yaw(Door), 90.f, 0.01f);

	// Update Animation Speed: twice as fast.
	Doors->UpdateAnimationSpeed(2.f);
	Doors->ForceClose();
	AdvanceDoorsWorld(Wrapper, 0.55f);
	TestFalse(TEXT("shut in 0.5 s at twice the speed"), Doors->IsClosing());
	TestEqual(TEXT("shut"), Yaw(Door), 0.f, 0.01f);
	return true;
}

#endif
