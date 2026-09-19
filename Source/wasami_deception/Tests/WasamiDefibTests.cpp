#include "Misc/AutomationTest.h"
#include "../WasamiDefib.h"
#include "../WasamiGameMode.h"
#include "../WasamiHitFX.h"
#include "../WasamiSaveGame.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FString DefibTestSlotName(TEXT("WasamiTest_Defib"));

	// Where the player stands (the defibrillator at the origin): in the gap between the stands, in Sphere clear of Box
	// (its 32 cm half-depth and the capsule's 34 cm), and far away.
	const FVector DefibGap(0., 0., 165.);
	const FVector DefibNear(400., 0., 165.);
	const FVector DefibAway(5000., 0., 165.);

	/** The ticks' length. */
	constexpr float DefibStep = 0.01f;
	/**
	 * How late a fire may be seen: a timer goes off on the first tick past its time, and one set as another goes off
	 * counts from that tick, so a chain of them falls behind by up to a tick each.
	 */
	constexpr float DefibLate = 0.11f;

	/** A test world's clock and what the defibrillator did in it. */
	struct FDefibClock
	{
		float Time = 0.f;
		/** When Firing turned on. */
		TArray<float> Fires;
	};

	/** Seconds of play in ticks of DefibStep, noting when Defib starts firing. */
	void AdvanceDefib(FTestWorldWrapper& Wrapper, const AWasamiDefib* Defib, FDefibClock& Clock, float Seconds)
	{
		bool bWasFiring = Defib->IsFiring();
		for (float Left = Seconds; Left > 1e-4f; Left -= DefibStep)
		{
			const float Step = FMath::Min(Left, DefibStep);
			Wrapper.TickTestWorld(Step);
			Clock.Time += Step;
			if (Defib->IsFiring() && !bWasFiring)
			{
				Clock.Fires.Add(Clock.Time);
			}
			bWasFiring = Defib->IsFiring();
		}
	}

	/** Play until Defib fires (at most Seconds). */
	void AdvanceUntilFiring(FTestWorldWrapper& Wrapper, const AWasamiDefib* Defib, FDefibClock& Clock, float Seconds)
	{
		for (float Left = Seconds; Left > 1e-4f && !Defib->IsFiring(); Left -= DefibStep)
		{
			AdvanceDefib(Wrapper, Defib, Clock, DefibStep);
		}
	}

	/** The player: possessed by the first player controller, held where it is put (moved by teleports). */
	ACharacter* SpawnDefibPlayer(UWorld* World)
	{
		ACharacter* Player = World->SpawnActor<ACharacter>(DefibAway, FRotator::ZeroRotator);
		APlayerController* Controller = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (!Player || !Controller)
		{
			return nullptr;
		}
		Controller->Possess(Player);
		Player->GetCharacterMovement()->DisableMovement();
		return UGameplayStatics::GetPlayerCharacter(World, 0) == Player ? Player : nullptr;
	}

	void Teleport(AActor* Actor, const FVector& Where)
	{
		Actor->SetActorLocation(Where, false, nullptr, ETeleportType::TeleportPhysics);
	}

	/** The same rotation (straight down, the yaw and roll may be shared out otherwise). */
	bool SameTurn(const FRotator& A, const FRotator& B)
	{
		return A.Quaternion().AngularDistance(B.Quaternion()) < 1e-4;
	}

	TArray<AWasamiHitFX*> HitFlashes(UWorld* World)
	{
		TArray<AWasamiHitFX*> Flashes;
		for (TActorIterator<AWasamiHitFX> It(World); It; ++It)
		{
			Flashes.Add(*It);
		}
		return Flashes;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiDefibActorTest, "Wasami.Defib.Actor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiDefibActorTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiDefib* Defib = World->SpawnActor<AWasamiDefib>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the defibrillator"), Defib))
	{
		return false;
	}
	TestTrue(TEXT("Activated"), Defib->bActivated);

	// The stands, 4 m apart facing each other, blocking everything.
	const UStaticMeshComponent* Stand01 = Defib->GetDefibrillator01();
	const UStaticMeshComponent* Stand02 = Defib->GetDefibrillator02();
	TestEqual(TEXT("the original's names"), Stand01->GetName(), FString(TEXT("hospital_defibrillator_01")));
	TestTrue(TEXT("the first at y −200, 180 times"), Stand01->GetRelativeLocation().Equals(FVector(0., -200., 0.), 1e-3)
		&& Stand01->GetRelativeScale3D().Equals(FVector(180.)));
	TestTrue(TEXT("the second at y +200, turned round"), Stand02->GetRelativeLocation().Equals(FVector(0., 200., 0.), 0.01)
		&& SameTurn(Stand02->GetRelativeRotation(), FRotator(0., 180., 0.)));
	TestTrue(TEXT("BlockAll"), Stand01->GetCollisionProfileName() == UCollisionProfile::BlockAll_ProfileName);
	TestTrue(TEXT("blocking the player"), Stand02->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block
		&& Stand02->GetCollisionObjectType() == ECC_WorldStatic);
	TestNull(TEXT("the mesh is the level build's"), Stand01->GetStaticMesh().Get());

	// The sparks at each stand's paddles, waiting for Fire.
	const UParticleSystemComponent* Sparks = Defib->GetParticleSystem();
	const UParticleSystemComponent* Sparks1 = Defib->GetParticleSystem1();
	TestTrue(TEXT("ParticleSystem at the first"), Sparks->GetRelativeLocation().Equals(FVector(0., -160., 165.), 1e-3)
		&& SameTurn(Sparks->GetRelativeRotation(), FRotator(-90., -66.5713, 156.5714)));
	TestTrue(TEXT("ParticleSystem1 at the second"), Sparks1->GetRelativeLocation().Equals(FVector(0., 165., 165.), 0.01)
		&& SameTurn(Sparks1->GetRelativeRotation(), FRotator(-90., -15.9454, 285.9450)));
	TestTrue(TEXT("not started"), !Sparks->bAutoActivate && !Sparks->IsActive() && !Sparks1->IsActive());
	TestFalse(TEXT("nor ticking"), Sparks->PrimaryComponentTick.bStartWithTickEnabled);
	TestTrue(TEXT("P_06_Defib on both"), Sparks->Template && Sparks->Template->GetName() == TEXT("P_06_Defib")
		&& Sparks1->Template == Sparks->Template);

	// Box over the gap, Sphere 10 m around: only pawns, out of the navigation.
	const UBoxComponent* Box = Defib->GetBox();
	TestTrue(TEXT("Box 64 × 383 × 258 cm"), Box->GetScaledBoxExtent().Equals(FVector(32., 191.3698, 128.8946), 1e-3));
	TestTrue(TEXT("165 cm up"), Box->GetRelativeLocation().Equals(FVector(0., 0., 165.)));
	TestTrue(TEXT("overlapping pawns only"), Box->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap
		&& Box->GetCollisionResponseToChannel(ECC_WorldStatic) == ECR_Ignore
		&& Box->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Ignore
		&& Box->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Ignore);
	const USphereComponent* Sphere = Defib->GetSphere();
	TestEqual(TEXT("Sphere 10 m"), Sphere->GetUnscaledSphereRadius(), 1000.f);
	TestTrue(TEXT("overlapping pawns"), Sphere->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap);
	TestFalse(TEXT("Box out of the navigation"), Box->CanEverAffectNavigation());
	TestFalse(TEXT("Sphere too"), Sphere->CanEverAffectNavigation());

	// The zap at 0.6 through MonkeyAttenuation, played by Charge only.
	const UAudioComponent* Zap = Defib->GetZap();
	TestEqual(TEXT("the original's name"), Zap->GetName(), FString(TEXT("DD_TT_Defibrillator_Zap")));
	TestEqual(TEXT("0.6"), Zap->VolumeMultiplier, 0.6f);
	TestFalse(TEXT("not by itself"), Zap->bAutoActivate);
	TestTrue(TEXT("DD_TT_Defibrillator_Zap"), Zap->Sound && Zap->Sound->GetName() == TEXT("DD_TT_Defibrillator_Zap"));
	TestTrue(TEXT("MonkeyAttenuation"), Zap->AttenuationSettings && Zap->AttenuationSettings->GetName() == TEXT("MonkeyAttenuation"));
	TestFalse(TEXT("nothing going at first"), Defib->IsLooping() || Defib->IsCharging() || Defib->IsFiring() || Defib->HasHitPlayer());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiDefibChargeTest, "Wasami.Defib.Charge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiDefibChargeTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiDefib* Defib = World->SpawnActor<AWasamiDefib>(FVector::ZeroVector, FRotator::ZeroRotator);
	const FVector Elsewhere(0., 20000., 0.);
	AWasamiDefib* Off = World->SpawnActor<AWasamiDefib>(Elsewhere, FRotator::ZeroRotator);
	ACharacter* Player = SpawnDefibPlayer(World);
	ACharacter* Walker = World->SpawnActor<ACharacter>(-DefibAway, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the defibrillator"), Defib) || !TestNotNull(TEXT("another"), Off)
		|| !TestNotNull(TEXT("the player"), Player) || !TestNotNull(TEXT("a walker"), Walker))
	{
		return false;
	}
	Walker->GetCharacterMovement()->DisableMovement();
	Off->bActivated = false;
	FDefibClock Clock;

	// Only the player starts it.
	Teleport(Walker, DefibNear);
	TestFalse(TEXT("not another pawn"), Defib->IsLooping() || Defib->IsCharging());
	Teleport(Walker, -DefibAway);

	// The player in Sphere: the charge at once, the sparks 2.25 s on for 1 s, the player clear of them.
	Teleport(Player, DefibNear);
	TestTrue(TEXT("Looping"), Defib->IsLooping());
	TestTrue(TEXT("charging"), Defib->IsCharging());
	TestTrue(TEXT("the fire 2.25 s on"), FMath::IsNearlyEqual(Defib->GetTimeToFire(), AWasamiDefib::ChargeDelay, 1e-3f));
	AdvanceDefib(Wrapper, Defib, Clock, AWasamiDefib::ChargeDelay - 0.05f);
	TestFalse(TEXT("not yet firing"), Defib->IsFiring());
	AdvanceUntilFiring(Wrapper, Defib, Clock, 0.05f + DefibLate);
	TestTrue(TEXT("firing 2.25 s on"), Defib->IsFiring());
	TestTrue(TEXT("the next fire 3.5 s on"), FMath::IsNearlyEqual(Defib->GetTimeToFire(),
		AWasamiDefib::RechargeDelay + AWasamiDefib::ChargeDelay, DefibLate));
	TestTrue(TEXT("both sparks going"), Defib->GetParticleSystem()->IsActive() && Defib->GetParticleSystem1()->IsActive());
	TestFalse(TEXT("the player clear of Box"), Defib->HasHitPlayer());
	AdvanceDefib(Wrapper, Defib, Clock, AWasamiDefib::FiringTime - 0.05f);
	TestTrue(TEXT("still firing"), Defib->IsFiring());
	AdvanceDefib(Wrapper, Defib, Clock, 0.05f + DefibLate);
	TestFalse(TEXT("for 1 s"), Defib->IsFiring());

	// Staying, every 3.5 s; walking out at 8 s, the charge under way still fires, and none after it.
	AdvanceDefib(Wrapper, Defib, Clock, 8.f - Clock.Time);
	Teleport(Player, DefibAway);
	TestFalse(TEXT("not Looping once out"), Defib->IsLooping());
	TestTrue(TEXT("still charging"), Defib->IsCharging());
	TestTrue(TEXT("its fire still coming"), Defib->GetTimeToFire() > 0.f);
	AdvanceDefib(Wrapper, Defib, Clock, 14.f - Clock.Time);
	const float Period = AWasamiDefib::ChargeDelay + AWasamiDefib::RechargeDelay;
	TestEqual(TEXT("three fires"), Clock.Fires.Num(), 3);
	for (int32 Index = 0; Index < Clock.Fires.Num(); ++Index)
	{
		const float Due = AWasamiDefib::ChargeDelay + Index * Period;
		TestTrue(FString::Printf(TEXT("fire %d at %.2f s (due %.2f s)"), Index, Clock.Fires[Index], Due),
			Clock.Fires[Index] >= Due - 1e-3f && Clock.Fires[Index] <= Due + DefibLate);
	}
	TestFalse(TEXT("then at rest"), Defib->IsCharging() || Defib->IsFiring());
	TestEqual(TEXT("no fire coming"), Defib->GetTimeToFire(), -1.f);
	TestFalse(TEXT("never hit"), Defib->HasHitPlayer());

	// Coming back charges it at once; walking in and out again while it charges starts nothing more.
	Clock.Fires.Reset();
	const float Back = Clock.Time;
	Teleport(Player, DefibNear);
	TestTrue(TEXT("charging again"), Defib->IsLooping() && Defib->IsCharging());
	AdvanceDefib(Wrapper, Defib, Clock, 1.f);
	Teleport(Player, DefibAway);
	Teleport(Player, DefibNear);
	AdvanceDefib(Wrapper, Defib, Clock, Period + AWasamiDefib::ChargeDelay + DefibLate);
	TestEqual(TEXT("two fires"), Clock.Fires.Num(), 2);
	TestTrue(TEXT("on the first's time"), Clock.Fires.Num() == 2 && FMath::IsNearlyEqual(Clock.Fires[0] - Back,
		AWasamiDefib::ChargeDelay, DefibLate) && FMath::IsNearlyEqual(Clock.Fires[1] - Clock.Fires[0], Period, DefibLate));

	// Not Activated, it does nothing.
	Teleport(Player, Elsewhere + DefibNear);
	AdvanceDefib(Wrapper, Off, Clock, AWasamiDefib::ChargeDelay + 0.5f);
	TestFalse(TEXT("not Activated: nothing"), Off->IsLooping() || Off->IsCharging() || Off->IsFiring());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiDefibHitTest, "Wasami.Defib.Hit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiDefibHitTest::RunTest(const FString& Parameters)
{
	// Each case in a world of its own (a death pauses it), the game mode writing to the test slot. The test world has no
	// local player, so the death screen is not made: CreateWidget reports the controller once in each.
	AddExpectedError(TEXT("PlayerController_0"), EAutomationExpectedErrorFlags::Contains, 2);
	for (const bool bInGap : {true, false})
	{
		const FString What = bInGap ? TEXT("in the gap: ") : TEXT("walking in: ");
		FTestWorldWrapper Wrapper;
		if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
		{
			Wrapper.ForwardErrorMessages(this);
			return false;
		}
		UWorld* World = Wrapper.GetTestWorld();
		AWasamiGameMode* Mode = World->GetAuthGameMode<AWasamiGameMode>();
		AWasamiDefib* Defib = World->SpawnActor<AWasamiDefib>(FVector::ZeroVector, FRotator::ZeroRotator);
		ACharacter* Player = SpawnDefibPlayer(World);
		if (!TestNotNull(TEXT("the project's game mode"), Mode) || !TestNotNull(TEXT("the defibrillator"), Defib)
			|| !TestNotNull(TEXT("the player"), Player))
		{
			return false;
		}
		Mode->SaveSlotName = DefibTestSlotName;
		FDefibClock Clock;

		if (bInGap)
		{
			// Standing between the stands: nothing until it fires, then hit.
			Teleport(Player, DefibGap);
			TestTrue(What + TEXT("charging"), Defib->IsCharging());
			TestFalse(What + TEXT("not hit before the sparks"), Defib->HasHitPlayer());
			AdvanceDefib(Wrapper, Defib, Clock, AWasamiDefib::ChargeDelay - 0.05f);
			TestFalse(What + TEXT("still not"), Defib->HasHitPlayer() || HitFlashes(World).Num() > 0);
			AdvanceUntilFiring(Wrapper, Defib, Clock, 0.05f + DefibLate);
		}
		else
		{
			// Near: walking in after the sparks is safe; while they last, hit at once.
			Teleport(Player, DefibNear);
			AdvanceDefib(Wrapper, Defib, Clock, AWasamiDefib::ChargeDelay + AWasamiDefib::FiringTime + DefibLate);
			TestFalse(What + TEXT("the sparks over"), Defib->IsFiring());
			Teleport(Player, DefibGap);
			TestFalse(What + TEXT("walking in after them is safe"), Defib->HasHitPlayer());
			Teleport(Player, DefibNear);
			AdvanceUntilFiring(Wrapper, Defib, Clock, AWasamiDefib::RechargeDelay + AWasamiDefib::ChargeDelay);
			TestTrue(What + TEXT("firing again"), Defib->IsFiring());
			TestFalse(What + TEXT("the player clear of them"), Defib->HasHitPlayer());
			Teleport(Player, DefibGap);
		}
		TestTrue(What + TEXT("hit"), Defib->HasHitPlayer());
		TArray<AWasamiHitFX*> Flashes = HitFlashes(World);
		TestTrue(What + TEXT("the hit's flash, shaking 5"), Flashes.Num() == 1 && Flashes[0]->ShakeScale == AWasamiDefib::HitShakeScale);

		// Once only.
		Defib->PlayerHit();
		Defib->Fire();
		TestEqual(What + TEXT("one flash"), HitFlashes(World).Num(), 1);

		// Dead 0.2 s on.
		TestTrue(What + TEXT("not at once"), Mode->IsDeathOpen());
		AdvanceDefib(Wrapper, Defib, Clock, AWasamiDefib::DeathDelay + DefibLate);
		TestFalse(What + TEXT("dead 0.2 s on"), Mode->IsDeathOpen());
	}
	UGameplayStatics::DeleteGameInSlot(DefibTestSlotName, UWasamiSaveGame::UserIndex);
	return true;
}

#endif
