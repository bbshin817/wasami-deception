#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "../WasamiBossAnimInstance.h"
#include "../WasamiEnemySentry.h"
#include "../WasamiMatron.h"
#include "../WasamiViewcone.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** The imported clips' lengths (30 fps frames, implementation record 17). */
	TArray<float> BossLengths()
	{
		return {339.f / 30.f, 120.f / 30.f, 156.f / 30.f};
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiMatronAnimTest, "Wasami.Matron.Anim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiMatronAnimTest::RunTest(const FString& Parameters)
{
	using namespace WasamiBossAnim;
	TestEqual(TEXT("a change is the transition clip until 0.05 s is left, then 0.2 s"), ChangeTime, 0.95f, 1e-6f);

	FWasamiBossAnimState State;
	State.Init(BossLengths());
	State.Update(true, false, 0.1f);
	TestEqual(TEXT("the first update takes no change: Idle"), State.GetClipWeight(WasamiBossClip::Idle), 1.f);
	TestEqual(TEXT("its clip moves on"), State.GetClipTime(WasamiBossClip::Idle), 0.1f, 1e-6f);

	// bAlert: to Alert over 0.95 s (HermiteCubic: half way at half the time), Alert from its start.
	State.Update(true, false, ChangeTime / 2.f);
	TestTrue(TEXT("changing to Alert"), State.IsAlert());
	TestEqual(TEXT("half way"), State.GetClipWeight(WasamiBossClip::Alert), 0.5f, 1e-4f);
	TestEqual(TEXT("Alert from its start"), State.GetClipTime(WasamiBossClip::Alert), ChangeTime / 2.f, 1e-5f);
	State.Update(false, false, ChangeTime / 4.f);
	TestTrue(TEXT("a change runs to its end"), State.IsAlert());
	State.Update(false, false, ChangeTime / 4.f);
	TestEqual(TEXT("in Alert"), State.GetClipWeight(WasamiBossClip::Alert), 1.f);
	TestEqual(TEXT("Idle is left"), State.GetClipWeight(WasamiBossClip::Idle), 0.f);
	const float IdleLeft = State.GetClipTime(WasamiBossClip::Idle);

	// Then back to Idle, which starts over.
	State.Update(false, false, 0.1f);
	TestFalse(TEXT("back to Idle once the change ended"), State.IsAlert());
	TestEqual(TEXT("Idle starts over"), State.GetClipTime(WasamiBossClip::Idle), 0.1f, 1e-5f);
	TestTrue(TEXT("(not where it was left)"), !FMath::IsNearlyEqual(IdleLeft, 0.1f));
	State.Update(false, false, 1.f);
	TestEqual(TEXT("in Idle"), State.GetClipWeight(WasamiBossClip::Idle), 1.f);
	State.Update(false, false, 11.3f);
	TestEqual(TEXT("Idle loops"), State.GetClipTime(WasamiBossClip::Idle), 1.1f, 1e-3f);

	// Detected: in over 0.25 s (Cubic), over everything, and it stays on its last pose.
	TestTrue(TEXT("Detected plays"), State.PlayDetected());
	TestTrue(TEXT("and is playing"), State.IsPlayingDetected());
	State.Update(true, false, 0.125f);
	TestEqual(TEXT("half in"), State.GetClipWeight(WasamiBossClip::Detected), 0.5f, 1e-4f);
	TestEqual(TEXT("from its start"), State.GetClipTime(WasamiBossClip::Detected), 0.125f, 1e-5f);
	State.Update(true, false, 0.125f);
	TestEqual(TEXT("all in"), State.GetClipWeight(WasamiBossClip::Detected), 1.f);
	TestEqual(TEXT("nothing under it shows"), State.GetClipWeight(WasamiBossClip::Idle) + State.GetClipWeight(WasamiBossClip::Alert), 0.f);
	State.Update(true, false, 10.f);
	TestEqual(TEXT("held at its end"), State.GetClipTime(WasamiBossClip::Detected), 156.f / 30.f, 1e-4f);
	TestEqual(TEXT("still over everything"), State.GetClipWeight(WasamiBossClip::Detected), 1.f);

	// The LookAt: bSpotted, in over 0.5 s (Cubic), out at once.
	TestEqual(TEXT("no LookAt before"), State.GetLookAtAlpha(), 0.f);
	State.Update(true, true, 0.25f);
	TestEqual(TEXT("half in"), State.GetLookAtAlpha(), 0.5f, 1e-4f);
	State.Update(true, true, 0.25f);
	TestEqual(TEXT("all in"), State.GetLookAtAlpha(), 1.f);
	State.Update(true, false, 0.01f);
	TestEqual(TEXT("out at once"), State.GetLookAtAlpha(), 0.f);

	// A missing Detected never plays; a missing Alert leaves Idle's the whole pose.
	FWasamiBossAnimState Missing;
	Missing.Init({11.3f, 0.f, 0.f});
	TestFalse(TEXT("no Detected to play"), Missing.PlayDetected());
	Missing.Update(false, false, 0.1f);
	Missing.Update(true, false, ChangeTime / 2.f);
	TestEqual(TEXT("Idle takes the missing Alert's weight"), Missing.GetClipWeight(WasamiBossClip::Idle), 1.f, 1e-5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiMatronClipsTest, "Wasami.Matron.Clips",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiMatronClipsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("the path"), WasamiBossAnim::ClipPath(WasamiBossClip::Detected).ToString(),
		FString(TEXT("/Game/Wasami/Boss/A_WasamiBoss_Detected.A_WasamiBoss_Detected")));
	const TArray<float> Lengths = BossLengths();
	for (int32 Clip = 0; Clip < WasamiBossClip::Num; ++Clip)
	{
		const UAnimSequence* Sequence = Cast<UAnimSequence>(WasamiBossAnim::ClipPath(Clip).TryLoad());
		if (!TestNotNull(FString::Printf(TEXT("%s imported"), WasamiBossAnim::ClipNames[Clip]), Sequence))
		{
			continue;
		}
		TestEqual(FString::Printf(TEXT("%s's length"), WasamiBossAnim::ClipNames[Clip]), static_cast<double>(Sequence->GetPlayLength()), static_cast<double>(Lengths[Clip]), 1e-3);
		TestTrue(FString::Printf(TEXT("%s on the boss's skeleton"), WasamiBossAnim::ClipNames[Clip]),
			Sequence->GetSkeleton() && Sequence->GetSkeleton()->GetName() == TEXT("SK_WasamiBoss_Skeleton"));
		TestTrue(FString::Printf(TEXT("%s has spine_02 (the LookAt's bone)"), WasamiBossAnim::ClipNames[Clip]),
			Sequence->GetSkeleton() && Sequence->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(FName(WasamiBossAnim::LookAtBone)) != INDEX_NONE);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiMatronConesTest, "Wasami.Matron.Cones",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiMatronConesTest::RunTest(const FString& Parameters)
{
	const AWasamiViewcone* Long = GetDefault<AWasamiViewconeMatronLong>();
	const AWasamiViewcone* Short = GetDefault<AWasamiViewconeMatronShort>();
	TestEqual(TEXT("the long cone: 3000 cm"), Long->Length, 3000.f);
	TestEqual(TEXT("the short cone: 1350 cm"), Short->Length, 1350.f);
	for (const AWasamiViewcone* Cone : {Long, Short})
	{
		TestEqual(TEXT("35 degrees wide"), Cone->Angle, 35.f);
		TestFalse(TEXT("not turned on by initializing"), Cone->bAutoOn);
		TestFalse(TEXT("its dot hidden"), Cone->GetDot()->IsVisible());
		TestTrue(TEXT("its fan at 0 (the level places it)"), Cone->GetPlane()->GetRelativeLocation().IsZero());
		TestTrue(TEXT("its fan shown"), Cone->GetPlane()->IsVisible());
		TestTrue(TEXT("on the map"), Cone->ActorHasTag(TEXT("dd_minimap")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiMatronActorTest, "Wasami.Matron.Actor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiMatronActorTest::RunTest(const FString& Parameters)
{
	// The class.
	const AWasamiMatron* Defaults = GetDefault<AWasamiMatron>();
	TestTrue(TEXT("tagged Enemy"), Defaults->ActorHasTag(TEXT("Enemy")));
	TestEqual(TEXT("the boss's head at the long cone"), AWasamiMatron::MeshScale, (676.2613525390625 + 107.2183837890625) / 128.2, 1e-9);
	TestTrue(TEXT("about 6.1 times"), FMath::IsNearlyEqual(AWasamiMatron::MeshScale, 6.111, 1e-3));
	TestTrue(TEXT("the mesh drawn at that size"), Defaults->GetMesh()->GetRelativeScale3D().Equals(FVector(AWasamiMatron::MeshScale)));
	TestTrue(TEXT("the mesh has no collision"), Defaults->GetMesh()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	TestFalse(TEXT("nor is it on the navigation"), Defaults->GetMesh()->CanEverAffectNavigation());
	TestTrue(TEXT("the boss's animation"), Defaults->GetMesh()->AnimClass == UWasamiBossAnimInstance::StaticClass());
	const UBoxComponent* Box = Defaults->GetCloseArea();
	TestTrue(TEXT("CloseArea overlaps Pawn"), Box->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap);
	for (const ECollisionChannel Channel : {ECC_WorldStatic, ECC_WorldDynamic, ECC_Visibility, ECC_Camera, ECC_PhysicsBody, ECC_Vehicle, ECC_Destructible})
	{
		TestTrue(TEXT("and ignores the rest"), Box->GetCollisionResponseToChannel(Channel) == ECR_Ignore);
	}
	TestFalse(TEXT("CloseArea is not on the navigation"), Box->CanEverAffectNavigation());
	TestTrue(TEXT("the box's own size (the level scales it)"), Box->GetUnscaledBoxExtent().Equals(FVector(32.)));

	// A game world ticked by hand, as the sentry's test.
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	constexpr float Step = 0.0625f;
	float Now = 0.f;
	auto TickTo = [&Wrapper, &Now](float Time)
	{
		while (Now < Time - Step / 2.f)
		{
			Wrapper.TickTestWorld(Step);
			Now += Step;
		}
	};

	// The player (not possessed, so that it stays where it is put), away to the side.
	const FVector Away(0., 3000., 300.);
	ACharacter* Player = World->SpawnActor<ACharacter>(Away, FRotator::ZeroRotator);
	APlayerController* Controller = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the player"), Player) || !TestNotNull(TEXT("a controller"), Controller))
	{
		return false;
	}
	Controller->SetPawn(Player);

	// The Matron facing +X (her mesh's +Y), with CloseArea behind her, her cones over her looking ahead, and a sentry.
	AWasamiMatron* Matron = World->SpawnActor<AWasamiMatron>(FVector::ZeroVector, FRotator(0., -90., 0.));
	AWasamiViewcone* Long = World->SpawnActor<AWasamiViewconeMatronLong>(FVector(0., 0., 300.), FRotator::ZeroRotator);
	AWasamiViewcone* Short = World->SpawnActor<AWasamiViewconeMatronShort>(FVector(0., 0., 200.), FRotator(-20., 0., 0.));
	const FTransform SentryAt(FRotator::ZeroRotator, FVector(0., -3000., 500.));
	AWasamiEnemySentry* Sentry = World->SpawnActorDeferred<AWasamiEnemySentry>(AWasamiEnemySentry::StaticClass(), SentryAt,
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!TestNotNull(TEXT("the Matron"), Matron) || !TestNotNull(TEXT("the long cone"), Long) || !TestNotNull(TEXT("the short cone"), Short)
		|| !TestNotNull(TEXT("a sentry"), Sentry))
	{
		return false;
	}
	Sentry->bCanSpawn = true;
	Sentry->FinishSpawning(SentryAt);
	Sentry->GetCharacterMovement()->GravityScale = 0.f;
	const FVector Behind(-500., 0., 100.);
	Matron->GetCloseArea()->SetWorldLocation(Behind);
	Matron->GetCloseArea()->SetWorldScale3D(FVector(5.));
	Matron->LongCone = Long;
	Matron->ShortCone = Short;
	TestNotNull(TEXT("the boss's mesh"), Matron->GetMesh()->GetSkeletalMeshAsset());
	UWasamiBossAnimInstance* Anim = Cast<UWasamiBossAnimInstance>(Matron->GetMesh()->GetAnimInstance());
	if (!TestNotNull(TEXT("with its animation"), Anim))
	{
		return false;
	}
	TickTo(0.5f);
	TestFalse(TEXT("not switching before Activate"), Matron->IsActivated());
	TestFalse(TEXT("her cones not looking"), Long->IsInitialized() || Short->IsInitialized());
	TestEqual(TEXT("in Idle"), Anim->GetAnimState().GetClipWeight(WasamiBossClip::Idle), 1.f);

	// Activate at 0.5 s: her cones are hers and look; the first Switch (1.5 s) turns the long one on, seeing at 2.5 s. A
	// timer comes up to two ticks after its time, so the checks leave room.
	Matron->Activate();
	TestTrue(TEXT("Activate starts Switch"), Matron->IsActivated());
	TestTrue(TEXT("the long cone hers"), Long->GetOwner() == Matron);
	TestTrue(TEXT("the short cone hers"), Short->GetOwner() == Matron);
	TestTrue(TEXT("both looking"), Long->IsInitialized() && Short->IsInitialized());
	TickTo(1.4f);
	TestFalse(TEXT("bMode before the first Switch"), Matron->bMode);
	TickTo(1.9f);
	TestTrue(TEXT("the player away: bMode"), Matron->bMode);
	TickTo(2.4f);
	TestFalse(TEXT("the long cone a second on"), Long->IsOn());
	TickTo(2.9f);
	TestTrue(TEXT("the long cone on"), Long->IsOn());
	TestFalse(TEXT("the short cone off"), Short->IsOn());
	TestTrue(TEXT("Alert while the player is away"), Anim->GetAnimState().IsAlert());
	TickTo(3.6f);
	TestEqual(TEXT("in Alert"), Anim->GetAnimState().GetClipWeight(WasamiBossClip::Alert), 1.f);

	// The player into CloseArea (behind her): at the next Switch the short cone, and Idle.
	Player->SetActorLocation(Behind);
	TestTrue(TEXT("in CloseArea"), Matron->GetCloseArea()->IsOverlappingActor(Player));
	TickTo(4.9f);
	TestFalse(TEXT("not bMode"), Matron->bMode);
	TestFalse(TEXT("the long cone off"), Long->IsOn());
	TickTo(5.9f);
	TestTrue(TEXT("the short cone on"), Short->IsOn());
	TestFalse(TEXT("Idle while the player is by her"), Anim->GetAnimState().IsAlert());
	TestFalse(TEXT("not seen behind her"), Matron->bSpotted);

	// Switch turns only as bMode changes: the long cone turned on by hand stays on.
	Long->TurnOn();
	TickTo(7.9f);
	TestTrue(TEXT("Switch leaves the cones alone while bMode stays"), Long->IsOn() && Short->IsOn());
	Long->TurnOff();

	// The player away again: the long cone's turn once more.
	Player->SetActorLocation(Away);
	TickTo(8.9f);
	TestTrue(TEXT("bMode again"), Matron->bMode);
	TestFalse(TEXT("the short cone off"), Short->IsOn());
	TickTo(9.9f);
	TestTrue(TEXT("the long cone on again"), Long->IsOn());

	// Seen by the long cone: both cones gone, Detected, bSpotted; 1.0948 s on the sentries are told.
	Player->SetActorLocation(FVector(1500., 0., 300.));
	const TWeakObjectPtr<AWasamiViewcone> WeakLong(Long);
	const TWeakObjectPtr<AWasamiViewcone> WeakShort(Short);
	const float Placed = Now;
	while (!Matron->bSpotted && Now < Placed + 1.f)
	{
		TickTo(Now + Step);
	}
	const float Spotted = Now;
	TestTrue(TEXT("spotted within the sight's rate"), Matron->bSpotted && Spotted <= Placed + AWasamiViewcone::SightRateMax + Step);
	TestTrue(TEXT("the long cone destroyed"), !WeakLong.IsValid() || WeakLong->IsActorBeingDestroyed());
	TestTrue(TEXT("the short cone destroyed"), !WeakShort.IsValid() || WeakShort->IsActorBeingDestroyed());
	TestTrue(TEXT("Detected plays"), Anim->IsPlayingDetected());
	TestFalse(TEXT("the sentry not told yet"), Sentry->IsChasing());
	TickTo(Spotted + AWasamiMatron::ReinforcementTime - Step);
	TestFalse(TEXT("not before 1.0948 s"), Sentry->IsChasing());
	TickTo(Spotted + AWasamiMatron::ReinforcementTime + 2.f * Step);
	TestTrue(TEXT("the sentry chases"), Sentry->IsChasing());
	TestEqual(TEXT("Detected over everything"), Anim->GetAnimState().GetClipWeight(WasamiBossClip::Detected), 1.f);
	TestEqual(TEXT("turned to the player"), Anim->GetLookAtAlpha(), 1.f);

	// Once only; Switch goes on without her cones.
	const float DetectedTime = Anim->GetAnimState().GetClipTime(WasamiBossClip::Detected);
	IWasamiViewconeInterface::Execute_PlayerSpotted(Matron);
	TickTo(Now + Step);
	TestTrue(TEXT("a second Player Spotted does nothing"), Anim->GetAnimState().GetClipTime(WasamiBossClip::Detected) > DetectedTime);
	Player->SetActorLocation(Behind);
	TickTo(Now + 2.f);
	TestFalse(TEXT("Switch still sets bMode"), Matron->bMode);
	TickTo(Now + 6.f);
	TestEqual(TEXT("Detected held at its end"), Anim->GetAnimState().GetClipTime(WasamiBossClip::Detected), 156.f / 30.f, 1e-3f);
	return true;
}

#endif
