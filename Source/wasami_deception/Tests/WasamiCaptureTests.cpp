#include "Misc/AutomationTest.h"
#include "../WasamiCapture.h"
#include "../WasamiEnemy.h"
#include "../WasamiEnemyAnimInstance.h"
#include "../WasamiGameInstance.h"
#include "../WasamiGameMode.h"
#include "../WasamiPlayerCharacter.h"
#include "../WasamiSaveGame.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FString CaptureTestSlotName(TEXT("WasamiTest_CaptureSlot"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiCaptureNoRepeatTest, "Wasami.Capture.NoRepeat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiCaptureNoRepeatTest::RunTest(const FString& Parameters)
{
	// Random Integer In Range (No Repeat) over 0..2: every run of three calls gives each once.
	const FRandomStream Stream(1234);
	TArray<int32> Remaining;
	bool bStarted = false;
	bool bAllRuns = true;
	int32 Firsts[3] = {0, 0, 0};
	for (int32 Run = 0; Run < 60; ++Run)
	{
		TSet<int32> Seen;
		for (int32 Call = 0; Call < 3; ++Call)
		{
			const int32 Value = UWasamiGameInstance::TakeNoRepeat(Remaining, bStarted, 0, 2, Stream);
			bAllRuns &= Value >= 0 && Value <= 2 && !Seen.Contains(Value);
			Seen.Add(Value);
			if (Call == 0)
			{
				++Firsts[FMath::Clamp(Value, 0, 2)];
			}
		}
		bAllRuns &= Remaining.IsEmpty() && !bStarted;
	}
	TestTrue(TEXT("each run of three is 0, 1 and 2 in some order, the bag empty after it"), bAllRuns);
	TestTrue(TEXT("any may come first"), Firsts[0] > 0 && Firsts[1] > 0 && Firsts[2] > 0);

	// Half-way through a run the bag holds what is left.
	UWasamiGameInstance::TakeNoRepeat(Remaining, bStarted, 0, 2, Stream);
	TestTrue(TEXT("started"), bStarted);
	TestEqual(TEXT("two left"), Remaining.Num(), 2);
	TArray<int32> One;
	bool bOneStarted = false;
	TestEqual(TEXT("a range of one number gives it"), UWasamiGameInstance::TakeNoRepeat(One, bOneStarted, 4, 4, Stream), 4);

	UWasamiGameInstance* Instance = NewObject<UWasamiGameInstance>();
	TSet<int32> Taken;
	for (int32 Call = 0; Call < AWasamiCapture::NumChoices; ++Call)
	{
		Taken.Add(Instance->TakeCaptureChoice());
	}
	TestEqual(TEXT("the game instance's bag gives the three clips in three captures"), Taken.Num(), AWasamiCapture::NumChoices);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiCaptureRoomTest, "Wasami.Capture.Room",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiCaptureRoomTest::RunTest(const FString& Parameters)
{
	UGameplayStatics::DeleteGameInSlot(CaptureTestSlotName, UWasamiSaveGame::UserIndex);
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();

	// StartCapture goes through the world's game mode (the test world makes the project's), once at a time.
	if (AWasamiGameMode* Auth = World->GetAuthGameMode<AWasamiGameMode>())
	{
		AWasamiCapture* First = AWasamiCapture::StartCapture(World, nullptr, 2);
		TestTrue(TEXT("a capture with the game mode's death open"), First && First->GetChoice() == 2);
		TestNull(TEXT("not a second while one goes on"), AWasamiCapture::StartCapture(World, nullptr, 0));
		if (First)
		{
			First->Destroy();
		}
		Auth->DeathEvent(nullptr);
		TestNull(TEXT("none once the death is closed"), AWasamiCapture::StartCapture(World, nullptr, 0));
	}

	// The hotel's scene grown to the enemy: the monkey is 263 cm, the Wasami 231 cm.
	TestEqual(TEXT("the monkey's top"), AWasamiCapture::MonkeyTop, 263.1512, 1e-3);
	TestEqual(TEXT("the Wasami's top"), AWasamiCapture::WasamiTop, 170. * AWasamiEnemy::MeshScale, 1e-9);
	TestEqual(TEXT("the scale"), AWasamiCapture::SceneScale, 0.87802, 1e-4);

	TestEqual(TEXT("Capture_1 fades at MonkeyJumpscare's share"), AWasamiCapture::FadeStartShare(0), 1.7005260f / 2.1212230f, 1e-5f);
	TestEqual(TEXT("Capture_3 black at MonkeyJumpscare3's share"), AWasamiCapture::FadeEndShare(2), 2.7699680f / 3.0683897f, 1e-5f);

	struct FCase
	{
		int32 Choice;
		const TCHAR* Clip;
		double MinBack;
		double MaxBack;
	};
	// Capture_1 flips where it stands; Capture_2 slides some 5.7 m (grown to 7.7 m), Capture_3 walks some 2.1 m.
	const FCase Cases[] = {
		{0, TEXT("A_WasamiEnemy_Capture_1"), -50., 50.},
		{1, TEXT("A_WasamiEnemy_Capture_2"), 500., 900.},
		{2, TEXT("A_WasamiEnemy_Capture_3"), 150., 400.},
	};
	for (const FCase& Case : Cases)
	{
		AWasamiGameMode* Mode = World->SpawnActorDeferred<AWasamiGameMode>(AWasamiGameMode::StaticClass(), FTransform::Identity);
		Mode->SaveSlotName = CaptureTestSlotName;
		Mode->FinishSpawning(FTransform::Identity);
		AWasamiCapture* Room = World->SpawnActor<AWasamiCapture>(AWasamiCapture::StaticClass(), FTransform(AWasamiCapture::RoomLocation));
		if (!TestNotNull(TEXT("the room"), Room))
		{
			return false;
		}
		Room->Start(Mode, nullptr, Case.Choice);
		const FString What = FString::Printf(TEXT("clip %d: "), Case.Choice);
		TestEqual(What + TEXT("the choice"), Room->GetChoice(), Case.Choice);

		// The Wasami, facing the camera, playing its clip once from where it has to start.
		const USkeletalMeshComponent* Body = Room->GetBody();
		TestNotNull(What + TEXT("the mesh"), Body->GetSkeletalMeshAsset());
		const UAnimSingleNodeInstance* Single = Body->GetSingleNodeInstance();
		const UAnimationAsset* Playing = Single ? Single->GetAnimationAsset() : nullptr;
		TestTrue(What + TEXT("its clip"), Playing && Playing->GetName() == Case.Clip);
		TestTrue(What + TEXT("once"), Single && Single->IsPlaying() && !Single->IsLooping());
		const double Back = -Body->GetRelativeLocation().X;
		TestTrue(What + FString::Printf(TEXT("started %.0f cm back"), Back), Back >= Case.MinBack && Back <= Case.MaxBack);
		TestEqual(What + TEXT("not to the side"), Body->GetRelativeLocation().Y, 0., 30.);
		TestTrue(What + TEXT("facing +X"), Body->GetComponentTransform().TransformVectorNoScale(FVector::RightVector).Equals(FVector::ForwardVector, 1e-3));

		// The fade at the paired Matinee's share of the clip.
		const float Length = Playing ? CastChecked<UAnimSequence>(Playing)->GetPlayLength() : 0.f;
		TestEqual(What + TEXT("fade start"), Room->GetFadeStart(), Length * AWasamiCapture::FadeStartShare(Case.Choice), 1e-4f);
		TestTrue(What + TEXT("fade before the death screen"), Room->GetFadeStart() + Room->GetFadeDuration() < AWasamiCapture::DeathDelay);

		// The death screen 3.5 s on.
		constexpr float Step = 0.1f;
		for (float Time = 0.f; Time < AWasamiCapture::DeathDelay - 0.15f; Time += Step)
		{
			Wrapper.TickTestWorld(Step);
		}
		TestTrue(What + TEXT("alive until 3.5 s"), Mode->IsDeathOpen());
		Wrapper.TickTestWorld(0.2f);
		TestFalse(What + TEXT("dead at 3.5 s"), Mode->IsDeathOpen());
		Room->Destroy();
		Mode->Destroy();
	}

	// The room itself.
	AWasamiCapture* Room = World->SpawnActor<AWasamiCapture>(AWasamiCapture::StaticClass(), FTransform(AWasamiCapture::RoomLocation));
	const UCameraComponent* View = Room->GetView();
	TestTrue(TEXT("the camera where the hotel's is"), View->GetRelativeLocation().Equals(AWasamiCapture::CameraOffset, 1e-3));
	// The hotel frames the monkey's 98.5 cm head from 94.7 cm, level with 0.534 of it; the room frames the Wasami's
	// 231 cm the same way.
	TestEqual(TEXT("the frame's scale"), AWasamiCapture::FrameScale, AWasamiCapture::WasamiTop / 98.5315, 1e-4);
	TestTrue(TEXT("some 222 cm in front, 7 cm to the side, 123 cm up"), View->GetRelativeLocation().Equals(FVector(222.0, 6.75, 123.37), 0.05));
	TestTrue(TEXT("looking back at the Wasami"), View->GetForwardVector().Equals(FVector::BackwardVector, 1e-4));
	TestEqual(TEXT("its field of view"), View->FieldOfView, 90.f);
	const UPointLightComponent* Light = Room->GetLight();
	TestEqual(TEXT("the ceiling light's intensity"), Light->Intensity, 1500.f);
	TestTrue(TEXT("unitless"), Light->IntensityUnits == ELightUnits::Unitless);
	TestEqual(TEXT("its radius"), Light->AttenuationRadius, 500.f);
	TestTrue(TEXT("its colour"), Light->LightColor == FColor(142, 236, 255, 255));
	TestTrue(TEXT("above the Wasami"), Light->GetRelativeLocation().Z > AWasamiCapture::WasamiTop);

	// Six black planes facing in, the camera and the slide's start inside them.
	TestEqual(TEXT("six planes"), Room->GetWalls().Num(), 6);
	const FVector Inside = Room->GetActorLocation() + FVector(-500., 0., 1000.);
	for (const UStaticMeshComponent* Plane : Room->GetWalls())
	{
		TestTrue(TEXT("facing in"), FVector::DotProduct(Plane->GetUpVector(), Inside - Plane->GetComponentLocation()) > 0.);
		TestTrue(TEXT("black"), Plane->GetMaterial(0) && Plane->GetMaterial(0)->GetName() == TEXT("BlackUnlitMaterial"));
		TestFalse(TEXT("no shadow"), Plane->CastShadow);
		const FVector Up = Plane->GetUpVector();
		const double Wall = FVector::DotProduct(Plane->GetComponentLocation() - Room->GetActorLocation(), Up);
		TestTrue(TEXT("the camera inside"), FVector::DotProduct(View->GetRelativeLocation(), Up) > Wall);
		TestTrue(TEXT("the slide's start inside"), FVector::DotProduct(FVector(-900., 0., 100.), Up) > Wall);
	}

	// Put Down Tablet: lowers a raised tablet, and does nothing to a lowered one.
	AWasamiPlayerCharacter* Player = World->SpawnActor<AWasamiPlayerCharacter>(AWasamiPlayerCharacter::StaticClass(), FTransform(FVector(0., 0., 200.)));
	if (TestNotNull(TEXT("a player"), Player))
	{
		Player->ToggleTablet();
		TestTrue(TEXT("raised"), Player->IsTabletUp());
		Player->PutDownTablet();
		TestFalse(TEXT("put down"), Player->IsTabletUp());
		Player->PutDownTablet();
		TestFalse(TEXT("still down"), Player->IsTabletUp());
	}

	UGameplayStatics::DeleteGameInSlot(CaptureTestSlotName, UWasamiSaveGame::UserIndex);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiCaptureCatchTest, "Wasami.Capture.Catch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiCaptureCatchTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	if (!TestNotNull(TEXT("the project's game mode"), World->GetAuthGameMode<AWasamiGameMode>()))
	{
		return false;
	}

	// The player: possessed by the first player controller (the capture disables its input), and moved by teleports,
	// which update the overlaps without sweeping. The world is not ticked.
	const FVector Away(0., 5000., 500.);
	ACharacter* Player = World->SpawnActor<ACharacter>(Away, FRotator::ZeroRotator);
	APlayerController* Controller = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the player"), Player) || !TestNotNull(TEXT("a controller"), Controller))
	{
		return false;
	}
	Controller->Possess(Player);
	if (!TestTrue(TEXT("the player is the player"), UGameplayStatics::GetPlayerCharacter(World, 0) == Player))
	{
		return false;
	}
	ACharacter* Walker = World->SpawnActor<ACharacter>(FVector(0., -5000., 500.), FRotator::ZeroRotator);

	// Two enemies far apart, floating.
	AWasamiEnemy* Stunned = AWasamiEnemy::SpawnEnemy(World, FVector(0., 0., 500.), 0.f);
	AWasamiEnemy* Awake = AWasamiEnemy::SpawnEnemy(World, FVector(2000., 0., 500.), 0.f);
	if (!TestNotNull(TEXT("an enemy"), Stunned) || !TestNotNull(TEXT("another"), Awake) || !TestNotNull(TEXT("a walker"), Walker))
	{
		return false;
	}
	Stunned->GetCharacterMovement()->GravityScale = 0.f;
	Awake->GetCharacterMovement()->GravityScale = 0.f;
	auto CaptureCount = [World]()
	{
		int32 Count = 0;
		for (TActorIterator<AWasamiCapture> It(World); It; ++It)
		{
			++Count;
		}
		return Count;
	};
	// Inside a Sphere (54.9 cm) with the player's capsule (34 cm), clear of the enemy's (34 cm).
	const FVector Beside(80., 0., 0.);

	// Not an enemy's own capsule, not another pawn, not a stunned enemy.
	TestEqual(TEXT("nothing caught at the start"), CaptureCount(), 0);
	Walker->SetActorLocation(Awake->GetActorLocation() + Beside, false, nullptr, ETeleportType::TeleportPhysics);
	TestEqual(TEXT("another pawn is not caught"), CaptureCount(), 0);
	Walker->SetActorLocation(FVector(0., -5000., 500.), false, nullptr, ETeleportType::TeleportPhysics);
	IWasamiEnemyInterface::Execute_SetState(Stunned, EWasamiEnemyState::Stun, false);
	Player->SetActorLocation(Stunned->GetActorLocation() + Beside, false, nullptr, ETeleportType::TeleportPhysics);
	TestTrue(TEXT("the player is in the stunned one's Sphere"), Stunned->GetSphere()->IsOverlappingActor(Player));
	TestEqual(TEXT("a stunned enemy does not catch"), CaptureCount(), 0);
	TestTrue(TEXT("and stays"), IsValid(Stunned) && IsValid(Awake));
	Player->SetActorLocation(Away, false, nullptr, ETeleportType::TeleportPhysics);

	// The other catches: the capture, every enemy removed, itself too.
	Player->SetActorLocation(Awake->GetActorLocation() + Beside, false, nullptr, ETeleportType::TeleportPhysics);
	TestEqual(TEXT("an enemy that is not stunned catches the player"), CaptureCount(), 1);
	TestFalse(TEXT("the enemy that caught is removed"), IsValid(Awake));
	TestFalse(TEXT("and the other"), IsValid(Stunned));
	TestTrue(TEXT("the pawns stay"), IsValid(Player) && IsValid(Walker));
	for (TActorIterator<AWasamiCapture> It(World); It; ++It)
	{
		TestTrue(TEXT("the view in the room"), Controller->GetViewTarget() == *It);
		It->Destroy();
	}
	return true;
}

#endif
