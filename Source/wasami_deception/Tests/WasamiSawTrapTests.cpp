#include "Misc/AutomationTest.h"
#include "../WasamiGameMode.h"
#include "../WasamiHitFX.h"
#include "../WasamiPlayerCharacter.h"
#include "../WasamiSaveGame.h"
#include "../WasamiSawTrap.h"
#include "Animation/AnimSequence.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FString SawTrapTestSlotName(TEXT("WasamiTest_SawTrap"));

	/** The ticks' length. */
	constexpr float SawTrapStep = 0.01f;
	/** How late a timer may be seen (it goes off on the first tick past its time; a chain falls behind a tick each). */
	constexpr float SawTrapLate = 0.03f;

	/** A kind of trap and what the original gives it. */
	struct FSawTrapKind
	{
		TSubclassOf<AWasamiSawTrap> Class;
		const TCHAR* Mesh;
		double Scale;
		FVector BoxScale;
		const TCHAR* Bone;
	};

	TArray<FSawTrapKind> SawTrapKinds()
	{
		return {
			{AWasamiSawTrap::StaticClass(), TEXT("hospital_sawTrap_medium_01_anim"), 1.5, FVector(1.2858065, 0.0514925, 1.3441961), TEXT("Saw")},
			{AWasamiSawTrapShort01::StaticClass(), TEXT("hospital_sawTrap_short_01_anim"), 1.8, FVector(1.3157035, 0.0514925, 1.3740931), TEXT("blade")},
			{AWasamiSawTrapShort02::StaticClass(), TEXT("hospital_sawTrap_short_02_anim"), 1.8, FVector(1.1945093, 0.0514925, 1.2528989), TEXT("blade")},
			{AWasamiSawTrapLong01::StaticClass(), TEXT("hospital_sawTrap_long_01_anim"), 1., FVector(5.3825216, 0.0514925, 5.4409113), TEXT("Saw")},
		};
	}

	void AdvanceSawTrapWorld(FTestWorldWrapper& Wrapper, float Seconds)
	{
		for (float Left = Seconds; Left > 1e-4f; Left -= SawTrapStep)
		{
			Wrapper.TickTestWorld(FMath::Min(Left, SawTrapStep));
		}
	}

	bool SawTrapSameTurn(const FRotator& A, const FRotator& B)
	{
		return A.Quaternion().AngularDistance(B.Quaternion()) < 1e-3;
	}

	void SawTrapTeleport(AActor* Actor, const FVector& Where)
	{
		Actor->SetActorLocation(Where, false, nullptr, ETeleportType::TeleportPhysics);
	}

	TArray<AWasamiHitFX*> SawTrapHitFlashes(UWorld* World)
	{
		TArray<AWasamiHitFX*> Flashes;
		for (TActorIterator<AWasamiHitFX> It(World); It; ++It)
		{
			Flashes.Add(*It);
		}
		return Flashes;
	}

	/** The player: player 0's character without being possessed (so it makes no widgets), not falling. */
	AWasamiPlayerCharacter* SpawnSawTrapPlayer(UWorld* World, const FVector& Where)
	{
		AWasamiPlayerCharacter* Player = World->SpawnActor<AWasamiPlayerCharacter>(Where, FRotator::ZeroRotator);
		APlayerController* Controller = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (!Player || !Controller)
		{
			return nullptr;
		}
		Controller->SetPawn(Player);
		Player->GetCharacterMovement()->DisableMovement();
		return UGameplayStatics::GetPlayerCharacter(World, 0) == Player ? Player : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiSawTrapActorTest, "Wasami.SawTrap.Actor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiSawTrapActorTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	const TArray<FSawTrapKind> Kinds = SawTrapKinds();
	TArray<AWasamiSawTrap*> Traps;
	TArray<FVector> Starts;
	for (int32 Index = 0; Index < Kinds.Num(); ++Index)
	{
		const FSawTrapKind& Kind = Kinds[Index];
		const FString What = FString(Kind.Mesh) + TEXT(": ");
		// Turned, as most placed ones are: collision attach keeps the actor's turn.
		const FRotator Turn(0., 90., 0.);
		AWasamiSawTrap* Trap = World->SpawnActor<AWasamiSawTrap>(Kind.Class, FVector(Index * 5000., 0., 0.), Turn);
		if (!TestNotNull(*(What + TEXT("the trap")), Trap))
		{
			return false;
		}
		Traps.Add(Trap);

		// The mesh, its animation (on its own), at the class's size, 50 m away at most, no collision.
		const USkeletalMeshComponent* Mesh = Trap->GetMesh();
		TestTrue(What + TEXT("the mesh"), Mesh->GetSkeletalMeshAsset() && Mesh->GetSkeletalMeshAsset()->GetName() == Kind.Mesh);
		TestTrue(What + TEXT("its animation, on its own"), Mesh->GetAnimationMode() == EAnimationMode::AnimationSingleNode
			&& Mesh->AnimationData.AnimToPlay && Mesh->AnimationData.AnimToPlay->GetName() == FString(Kind.Mesh) + TEXT("_Anim"));
		TestTrue(What + TEXT("its size"), Mesh->GetRelativeScale3D().Equals(FVector(Kind.Scale), 1e-6));
		TestEqual(What + TEXT("drawn to 5000"), Mesh->LDMaxDrawDistance, 5000.f);
		TestTrue(What + TEXT("no collision"), Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
			&& !Mesh->GetGenerateOverlapEvents());
		TestTrue(What + TEXT("sawSocket on the blade"), Mesh->DoesSocketExist(AWasamiSawTrap::SawSocket)
			&& Mesh->GetSocketBoneName(AWasamiSawTrap::SawSocket) == FName(Kind.Bone));

		// collision attach on the socket, turned as every placed one's (a roll of −90°: the actor's turn kept).
		const USceneComponent* Attach = Trap->GetCollisionAttach();
		TestTrue(What + TEXT("collision attach on sawSocket"), Attach->GetAttachParent() == Mesh
			&& Attach->GetAttachSocketName() == AWasamiSawTrap::SawSocket);
		TestTrue(What + TEXT("snapped to it"), Attach->GetRelativeLocation().IsNearlyZero(1e-3));
		TestTrue(What + TEXT("a roll of -90 from it, as the original's"), SawTrapSameTurn(Attach->GetRelativeRotation(), FRotator(0., 0., -90.)));
		TestTrue(What + TEXT("turned as the actor"), SawTrapSameTurn(Attach->GetComponentRotation(), Turn));

		// Box: about the blade, overlapping pawns, out of the navigation.
		const UBoxComponent* Box = Trap->GetBox();
		TestTrue(What + TEXT("Box on collision attach"), Box->GetAttachParent() == Attach);
		TestTrue(What + TEXT("Box's size"), Box->GetRelativeScale3D().Equals(Kind.BoxScale, 1e-6)
			&& Box->GetScaledBoxExtent().Equals(32. * Kind.BoxScale * Kind.Scale, 1e-3));
		TestTrue(What + TEXT("Box overlaps pawns"), Box->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics
			&& Box->GetCollisionObjectType() == ECC_WorldDynamic && Box->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap);
		TestTrue(What + TEXT("and ignores the world and the traces"), Box->GetCollisionResponseToChannel(ECC_WorldStatic) == ECR_Ignore
			&& Box->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Ignore
			&& Box->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Ignore
			&& Box->GetCollisionResponseToChannel(ECC_Camera) == ECR_Ignore
			&& Box->GetCollisionResponseToChannel(ECC_PhysicsBody) == ECR_Ignore);
		TestTrue(What + TEXT("hit events"), Box->BodyInstance.bNotifyRigidBodyCollision);
		TestFalse(What + TEXT("not in the navigation"), Box->IsNavigationRelevant());

		// The whine, on Box.
		const UAudioComponent* Audio = Trap->GetAudio();
		TestTrue(What + TEXT("the whine on Box"), Audio->GetAttachParent() == Box);
		TestTrue(What + TEXT("SFX_Matron_SawLoop"), Audio->Sound && Audio->Sound->GetName() == TEXT("SFX_Matron_SawLoop"));
		TestEqual(What + TEXT("at 0.5"), Audio->VolumeMultiplier, 0.5f);
		TestTrue(What + TEXT("its pitch from 1 to 1.2"), Audio->PitchMultiplier >= AWasamiSawTrap::MinPitch
			&& Audio->PitchMultiplier <= AWasamiSawTrap::MaxPitch);
		const FSoundAttenuationSettings& Attenuation = Audio->AttenuationOverrides;
		TestTrue(What + TEXT("natural, occluded"), Audio->bOverrideAttenuation
			&& Attenuation.DistanceAlgorithm == EAttenuationDistanceModel::NaturalSound && Attenuation.bEnableOcclusion
			&& Attenuation.bUseComplexCollisionForOcclusion && Attenuation.OcclusionLowPassFilterFrequency == 4000.f);
		const bool bShort01 = Kind.Class == AWasamiSawTrapShort01::StaticClass();
		TestTrue(What + TEXT("the occlusion's volume and time"), bShort01
			? Attenuation.OcclusionVolumeAttenuation == 0.2f && Attenuation.OcclusionInterpolationTime == 1.f
			: Attenuation.OcclusionVolumeAttenuation == 0.3f && Attenuation.OcclusionInterpolationTime == 0.3f);
		TestTrue(What + TEXT("the falloff"), bShort01
			? Attenuation.FalloffDistance == 1500.f && Attenuation.AttenuationShapeExtents.X == 100.f
			: Attenuation.FalloffDistance == FSoundAttenuationSettings().FalloffDistance
				&& Attenuation.AttenuationShapeExtents.X == FSoundAttenuationSettings().AttenuationShapeExtents.X);
		Starts.Add(Box->GetComponentLocation());
	}

	// short01's light: over the trap, a cold white, no shadows.
	if (const AWasamiSawTrapShort01* Short01 = Cast<AWasamiSawTrapShort01>(Traps[1]))
	{
		const UPointLightComponent* Light = Short01->GetPointLight();
		TestTrue(TEXT("short01's light on root, 114 cm up"), Light->GetAttachParent() == Short01->GetRoot()
			&& Light->GetRelativeLocation().Equals(FVector(0., 0., 113.619), 1e-3));
		TestTrue(TEXT("a cold white, 1750, unitless"), Light->LightColor == FColor(255, 254, 251, 255) && Light->Intensity == 1750.f
			&& Light->IntensityUnits == ELightUnits::Unitless);
		TestTrue(TEXT("its reach"), Light->AttenuationRadius == 250.f && Light->SoftSourceRadius == 100.f
			&& Light->MaxDrawDistance == 3000.f && Light->MaxDistanceFadeRange == 1500.f);
		TestFalse(TEXT("no shadows"), Light->CastShadows);
	}
	else
	{
		AddError(TEXT("the second kind is short01"));
	}

	// The blades move, and Box with each.
	AdvanceSawTrapWorld(Wrapper, 1.f);
	for (int32 Index = 0; Index < Traps.Num(); ++Index)
	{
		const FString What = FString(Kinds[Index].Mesh) + TEXT(": ");
		const FVector Now = Traps[Index]->GetBox()->GetComponentLocation();
		TestTrue(What + TEXT("Box on the blade"), Now.Equals(Traps[Index]->GetMesh()->GetSocketLocation(AWasamiSawTrap::SawSocket), 0.01));
		TestTrue(What + TEXT("moved in 1 s"), FVector::Dist(Now, Starts[Index]) > 5.);
		TestFalse(What + TEXT("nobody caught"), Traps[Index]->HasCaughtPlayer());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiSawTrapCatchTest, "Wasami.SawTrap.Catch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiSawTrapCatchTest::RunTest(const FString& Parameters)
{
	// Each case in a world of its own (a death pauses it), the game mode writing to the test slot. The test world has no
	// local player, so neither the black screen nor the death screen is made: CreateWidget reports the controller for
	// each.
	AddExpectedError(TEXT("PlayerController_0"), EAutomationExpectedErrorFlags::Contains, 4);
	for (const bool bStill : {true, false})
	{
		const FString What = bStill ? TEXT("standing still: ") : TEXT("walking in: ");
		FTestWorldWrapper Wrapper;
		if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
		{
			Wrapper.ForwardErrorMessages(this);
			return false;
		}
		UWorld* World = Wrapper.GetTestWorld();
		AWasamiGameMode* Mode = World->GetAuthGameMode<AWasamiGameMode>();
		const TSubclassOf<AWasamiSawTrap> Class = bStill ? AWasamiSawTrap::StaticClass() : AWasamiSawTrapShort01::StaticClass();
		AWasamiSawTrap* Trap = World->SpawnActor<AWasamiSawTrap>(Class, FVector::ZeroVector, FRotator::ZeroRotator);
		const FVector Away(0., 3000., 0.);
		ACharacter* Walker = World->SpawnActor<ACharacter>(Away, FRotator::ZeroRotator);
		AWasamiPlayerCharacter* Player = SpawnSawTrapPlayer(World, Away + FVector(0., 1000., 0.));
		if (!TestNotNull(TEXT("the project's game mode"), Mode) || !TestNotNull(TEXT("the trap"), Trap)
			|| !TestNotNull(TEXT("someone else"), Walker) || !TestNotNull(TEXT("the player"), Player))
		{
			return false;
		}
		Mode->SaveSlotName = SawTrapTestSlotName;
		Walker->GetCharacterMovement()->DisableMovement();
		UBoxComponent* Box = Trap->GetBox();

		// Anyone else coming into Box is let be.
		SawTrapTeleport(Walker, Box->GetComponentLocation() + FVector(0., 500., 0.));
		Walker->SetActorLocation(Box->GetComponentLocation(), true);
		TestTrue(What + TEXT("someone else in Box"), Box->IsOverlappingActor(Walker));
		TestFalse(What + TEXT("is let be"), Trap->HasCaughtPlayer());
		SawTrapTeleport(Walker, Away);

		if (bStill)
		{
			// The blade comes to the player: where Box was, farthest from where it is after 3 s, before a loop is out
			// (the animation's 8.6 s).
			TArray<FVector> Path;
			for (int32 Step = 0; Step < 60; ++Step)
			{
				AdvanceSawTrapWorld(Wrapper, 0.05f);
				Path.Add(Box->GetComponentLocation());
			}
			const FVector Now = Box->GetComponentLocation();
			FVector Farthest = Now;
			for (const FVector& Where : Path)
			{
				Farthest = FVector::Dist(Where, Now) > FVector::Dist(Farthest, Now) ? Where : Farthest;
			}
			TestTrue(What + TEXT("the blade goes 2 m and more"), FVector::Dist(Farthest, Now) > 200.);
			SawTrapTeleport(Player, Farthest);
			TestFalse(What + TEXT("clear of the blade at first"), Box->IsOverlappingActor(Player) || Trap->HasCaughtPlayer());
			for (float Left = 9.f; Left > 0.f && !Trap->HasCaughtPlayer(); Left -= SawTrapStep)
			{
				AdvanceSawTrapWorld(Wrapper, SawTrapStep);
			}
		}
		else
		{
			// The player walks into it.
			SawTrapTeleport(Player, Box->GetComponentLocation() + FVector(0., 500., 0.));
			TestFalse(What + TEXT("not before"), Trap->HasCaughtPlayer());
			Player->SetActorLocation(Box->GetComponentLocation(), true);
		}
		TestTrue(What + TEXT("caught"), Trap->HasCaughtPlayer());

		// The whine gone, the player stopped, the hit's flash shaking 1.
		TestFalse(What + TEXT("the whine gone"), IsValid(Trap->GetAudio()));
		TestFalse(What + TEXT("the player stopped"), Player->bCanMove);
		TArray<AWasamiHitFX*> Flashes = SawTrapHitFlashes(World);
		TestTrue(What + TEXT("the hit's flash, shaking 1"), Flashes.Num() == 1 && Flashes[0]->ShakeScale == AWasamiSawTrap::HitShakeScale);

		// Once only.
		Trap->CatchPlayer();
		TestEqual(What + TEXT("one flash"), SawTrapHitFlashes(World).Num(), 1);

		// The black screen 0.1 s on, dead 0.3 s after it.
		AdvanceSawTrapWorld(Wrapper, AWasamiSawTrap::BlackScreenDelay + AWasamiSawTrap::DeathDelay - 0.05f);
		TestTrue(What + TEXT("not dead at 0.35 s"), Mode->IsDeathOpen());
		AdvanceSawTrapWorld(Wrapper, 0.05f + SawTrapLate);
		TestFalse(What + TEXT("dead 0.4 s on"), Mode->IsDeathOpen());
	}
	UGameplayStatics::DeleteGameInSlot(SawTrapTestSlotName, UWasamiSaveGame::UserIndex);
	return true;
}

#endif
