#include "Misc/AutomationTest.h"
#include "../WasamiGarageLift.h"
#include "../WasamiPlayerCharacter.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Seconds of play in ticks of 0.05 s at most. */
	void AdvanceGarageLiftWorld(FTestWorldWrapper& Wrapper, float Seconds)
	{
		Wrapper.TickTestWorld(0.f);
		for (float Left = Seconds; Left > 1e-4f; Left -= 0.05f)
		{
			Wrapper.TickTestWorld(FMath::Min(Left, 0.05f));
		}
	}

	/** The platform's height: its box, on joint4. */
	double PlatformHeight(const AWasamiGarageLift* Lift)
	{
		return Lift->GetBox()->GetComponentLocation().Z;
	}

	/** Puts the player just over the platform, falling, or away from it, still. */
	void PutPlayer(AWasamiPlayerCharacter* Player, const AWasamiGarageLift* Lift, bool bOn)
	{
		UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
		if (bOn)
		{
			Player->SetActorLocation(Lift->GetOverlapBox()->GetComponentLocation() + FVector(0., 0., 60.), false, nullptr, ETeleportType::TeleportPhysics);
			Movement->SetMovementMode(MOVE_Falling);
		}
		else
		{
			Movement->SetMovementMode(MOVE_None);
			Player->SetActorLocation(FVector(5000., 0., 100.), false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiGarageLiftTest, "Wasami.GarageLift.Actor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiGarageLiftTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiGarageLift* Lift = World->SpawnActor<AWasamiGarageLift>(FVector::ZeroVector, FRotator::ZeroRotator);
	AWasamiPlayerCharacter* Player = World->SpawnActor<AWasamiPlayerCharacter>(FVector(5000., 0., 100.), FRotator::ZeroRotator);
	// The first player controller's character without possessing it; its physics run as a controlled character's.
	APlayerController* Controller = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the lift"), Lift) || !TestNotNull(TEXT("the player"), Player) || !TestNotNull(TEXT("a controller"), Controller))
	{
		return false;
	}
	Controller->SetPawn(Player);
	Player->GetCharacterMovement()->bRunPhysicsWithNoController = true;
	PutPlayer(Player, Lift, false);
	if (!TestTrue(TEXT("the player is the player"), UGameplayStatics::GetPlayerCharacter(World, 0) == Player)
		|| !TestNotNull(TEXT("the mesh (WasamiDDTools.import_dd_gimmicks makes it)"), Lift->GetSkeletalMesh()->GetSkeletalMeshAsset()))
	{
		return false;
	}
	const UWasamiGarageLiftAnimInstance* Anim = Cast<UWasamiGarageLiftAnimInstance>(Lift->GetSkeletalMesh()->GetAnimInstance());
	if (!TestNotNull(TEXT("its anim instance"), Anim))
	{
		return false;
	}

	// BP_06_GarageLift's parts: the mesh at 30 without collision, the boxes on joint4 (195 × 262.5 cm), sounds that wait.
	TestTrue(TEXT("the mesh at 30, no collision"), Lift->GetSkeletalMesh()->GetRelativeScale3D().Equals(FVector(30.))
		&& Lift->GetSkeletalMesh()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	TestEqual(TEXT("the animation, 2.4583 s"), Anim->GetLength(), 2.4583f, 1e-3f);
	const UBoxComponent* Box = Lift->GetBox();
	const UBoxComponent* OverlapBox = Lift->GetOverlapBox();
	TestTrue(TEXT("the platform's box on joint4"), Box->GetAttachParent() == Lift->GetSkeletalMesh()
		&& Box->GetAttachSocketName() == AWasamiGarageLift::PlatformBone
		&& OverlapBox->GetAttachParent() == Lift->GetSkeletalMesh() && OverlapBox->GetAttachSocketName() == AWasamiGarageLift::PlatformBone);
	TestTrue(TEXT("195 × 262.5 × 18.3 cm"), Box->GetScaledBoxExtent().Equals(FVector(195.016, 262.492, 9.152), 0.05));
	TestTrue(TEXT("it blocks pawns only, in queries and physics"), Box->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics
		&& Box->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block && Box->GetCollisionResponseToChannel(ECC_WorldStatic) == ECR_Ignore
		&& Box->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Ignore);
	TestTrue(TEXT("the box over it overlaps pawns, 137 cm tall"), OverlapBox->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap
		&& OverlapBox->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Ignore
		&& OverlapBox->GetScaledBoxExtent().Equals(FVector(195.016, 262.493, 68.417), 0.05));
	TestTrue(TEXT("Up and Down wait"), !Lift->GetAudio()->bAutoActivate && !Lift->GetAudio1()->bAutoActivate);
	TestTrue(TEXT("Audio is the rise, Audio1 the fall"), Lift->GetAudio()->Sound && Lift->GetAudio()->Sound->GetName() == TEXT("DD_TT_GarageLift_Up")
		&& Lift->GetAudio1()->Sound && Lift->GetAudio1()->Sound->GetName() == TEXT("DD_TT_GarageLift_Down"));

	// Nobody on it: Default, the first frame.
	AdvanceGarageLiftWorld(Wrapper, 0.3f);
	const double Low = PlatformHeight(Lift);
	TestTrue(TEXT("the platform low"), Low < 20.);
	TestFalse(TEXT("in Default"), Anim->IsInPlayerOn() || Lift->IsPlayerOverlapping());
	TestTrue(TEXT("the box over the platform"), OverlapBox->GetComponentLocation().Z > Low + 50.);

	// The player on it: PlayerOn plays the rise once (2.46 s) and holds it, carrying the player.
	PutPlayer(Player, Lift, true);
	AdvanceGarageLiftWorld(Wrapper, 0.3f);
	TestTrue(TEXT("the player on it"), Lift->IsPlayerOverlapping());
	TestTrue(TEXT("in PlayerOn"), Anim->IsInPlayerOn() && Anim->GetPlayerOnWeight() >= 1.f);
	AdvanceGarageLiftWorld(Wrapper, 0.9f);
	const double Middle = PlatformHeight(Lift);
	TestTrue(TEXT("rising"), Middle > Low + 30. && Middle < Low + 290.);
	AdvanceGarageLiftWorld(Wrapper, 1.6f);
	const double High = PlatformHeight(Lift);
	TestEqual(TEXT("up 307 cm"), High - Low, 307.2, 5.);
	TestEqual(TEXT("held at the end"), Anim->GetPlayerOnTime(), Anim->GetLength(), 1e-3f);
	TestTrue(TEXT("it carried the player"), Player->GetActorLocation().Z > High + 60. && Lift->IsPlayerOverlapping());

	// Off it for 0.1 s: halfway through the 0.2 s crossfade; back on, PlayerOn goes on from where it was.
	PutPlayer(Player, Lift, false);
	AdvanceGarageLiftWorld(Wrapper, 0.1f);
	TestFalse(TEXT("back to Default"), Anim->IsInPlayerOn());
	TestTrue(TEXT("crossfading"), Anim->GetPlayerOnWeight() > 0.f && Anim->GetPlayerOnWeight() < 1.f);
	PutPlayer(Player, Lift, true);
	AdvanceGarageLiftWorld(Wrapper, 0.3f);
	TestTrue(TEXT("PlayerOn again, still at its end"), Anim->IsInPlayerOn()
		&& FMath::IsNearlyEqual(Anim->GetPlayerOnTime(), Anim->GetLength(), 1e-3f));
	TestEqual(TEXT("up"), PlatformHeight(Lift), High, 5.);

	// Off it: down in 0.2 s. Back on once Default has all the weight: the rise starts over.
	PutPlayer(Player, Lift, false);
	AdvanceGarageLiftWorld(Wrapper, 0.3f);
	TestTrue(TEXT("Default"), !Anim->IsInPlayerOn() && Anim->GetPlayerOnWeight() <= 0.f);
	TestEqual(TEXT("down in 0.2 s"), PlatformHeight(Lift), Low, 1.);
	PutPlayer(Player, Lift, true);
	AdvanceGarageLiftWorld(Wrapper, 0.3f);
	TestTrue(TEXT("the rise starts over"), Anim->IsInPlayerOn() && Anim->GetPlayerOnTime() < 0.5f);

	// BP_06_GarageLift_Zone1_Special: nobody counts on it while a nurse is near.
	AWasamiGarageLiftZone1Special* Special = World->SpawnActor<AWasamiGarageLiftZone1Special>(FVector(2000., 0., 0.), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Zone 1's lift"), Special))
	{
		return false;
	}
	PutPlayer(Player, Special, true);
	AdvanceGarageLiftWorld(Wrapper, 0.3f);
	TestTrue(TEXT("the player on Zone 1's lift"), Special->IsPlayerOverlapping());
	Special->bNurseNear = true;
	TestFalse(TEXT("not with a nurse near"), Special->IsPlayerOverlapping());
	AdvanceGarageLiftWorld(Wrapper, 0.1f);
	const UWasamiGarageLiftAnimInstance* SpecialAnim = Cast<UWasamiGarageLiftAnimInstance>(Special->GetSkeletalMesh()->GetAnimInstance());
	TestTrue(TEXT("it goes back to Default"), SpecialAnim && !SpecialAnim->IsInPlayerOn());
	return true;
}

#endif
