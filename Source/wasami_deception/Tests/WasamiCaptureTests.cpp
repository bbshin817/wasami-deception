#include "Misc/AutomationTest.h"
#include "../WasamiCapture.h"
#include "../WasamiEnemy.h"
#include "../WasamiEnemyAnimInstance.h"
#include "../WasamiGameInstance.h"
#include "../WasamiGameMode.h"
#include "../WasamiPlayerCharacter.h"
#include "../WasamiSaveGame.h"
#include "../WasamiVoice.h"
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
#include "Sound/SoundBase.h"
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
	TestEqual(TEXT("the game instance's bag gives every capture once in as many captures"), Taken.Num(), AWasamiCapture::NumChoices);
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
	TestEqual(TEXT("Capture_2 black at MonkeyJumpscare3's share"), AWasamiCapture::FadeEndShare(1), 2.7699680f / 3.0683897f, 1e-5f);
	TestEqual(TEXT("Capture_3 fades at MonkeyJumpscare2's share"), AWasamiCapture::FadeStartShare(2), 1.9206896f / 2.4642830f, 1e-5f);
	TestEqual(TEXT("its length"), AWasamiCapture::MatineeLength(2), 2.4642830f, 1e-5f);

	// The InterpTrackFade's two auto-clamped keys: flat at both ends, half way at the middle, and steepest there.
	TestEqual(TEXT("the fade opens clear"), AWasamiCapture::FadeCurve(0.f), 0.f);
	TestEqual(TEXT("and ends black"), AWasamiCapture::FadeCurve(1.f), 1.f);
	TestEqual(TEXT("half way at the middle"), AWasamiCapture::FadeCurve(0.5f), 0.5f, 1e-6f);
	TestEqual(TEXT("flat where it starts"), AWasamiCapture::FadeCurve(0.02f), 3.f * 0.02f * 0.02f - 2.f * 0.02f * 0.02f * 0.02f, 1e-6f);
	TestTrue(TEXT("slower than a straight line at first"), AWasamiCapture::FadeCurve(0.25f) < 0.25f - 0.05f);
	TestTrue(TEXT("and faster than one past the middle"), AWasamiCapture::FadeCurve(0.75f) > 0.75f + 0.05f);
	TestTrue(TEXT("never running back"), AWasamiCapture::FadeCurve(0.4f) < AWasamiCapture::FadeCurve(0.6f));
	TestEqual(TEXT("clear before it opens"), AWasamiCapture::FadeCurve(-1.f), 0.f);
	TestEqual(TEXT("black after it is through"), AWasamiCapture::FadeCurve(2.f), 1.f);
	TestEqual(TEXT("the face after the Gold Watcher's 0.2, 0.85 and 0.1 s"), AWasamiCapture::FaceDeathDelay, 1.15f, 1e-6f);

	struct FCase
	{
		int32 Choice;
		const TCHAR* Clip;
		double MinBack;
		double MaxBack;
		bool bLooping;
	};
	// Where the pelvis starts: Capture_1 flips where it stands; Capture_2 slides in from some 4.8 m, Capture_3 walks
	// in from some 1.3 m; the face runs in from 2.5 m.
	const FCase Cases[] = {
		{0, TEXT("A_WasamiEnemy_Capture_1"), -50., 50., false},
		{1, TEXT("A_WasamiEnemy_Capture_2"), 300., 600., false},
		{2, TEXT("A_WasamiEnemy_Capture_3"), 80., 250., false},
		{AWasamiCapture::FaceChoice, TEXT("A_WasamiEnemy_Run"), 200., 270., true},
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
		TestTrue(What + (Case.bLooping ? TEXT("over and over") : TEXT("once")), Single && Single->IsPlaying() && Single->IsLooping() == Case.bLooping);
		TestEqual(What + TEXT("at a standstill, moved on by the scene's time"), Single ? Single->GetPlayRate() : -1.f, 0.f);
		const double Back = -Room->GetActorTransform().InverseTransformPosition(Body->GetSocketLocation(TEXT("pelvis"))).X;
		TestTrue(What + FString::Printf(TEXT("started %.0f cm back"), Back), Back >= Case.MinBack && Back <= Case.MaxBack);
		TestEqual(What + TEXT("not far to the side"), Body->GetRelativeLocation().Y, 0., 50.);
		TestTrue(What + TEXT("facing +X"), Body->GetComponentTransform().TransformVectorNoScale(FVector::RightVector).Equals(FVector::ForwardVector, 1e-3));

		const float Length = Playing ? CastChecked<UAnimSequence>(Playing)->GetPlayLength() : 0.f;
		if (Case.Choice == AWasamiCapture::FaceChoice)
		{
			// Black at once with the death screen.
			TestEqual(What + TEXT("black at 1.15 s"), Room->GetFadeStart(), AWasamiCapture::FaceDeathDelay);
			TestEqual(What + TEXT("at once"), Room->GetFadeDuration(), 0.f);
			TestEqual(What + TEXT("the death screen with it"), Room->GetDeathDelay(), AWasamiCapture::FaceDeathDelay);
		}
		else
		{
			// The clip from where it starts to the paired Matinee's share of it, on the scene's time (sooner than at
			// rate 1), the Matinee's start to its fade over it.
			TestEqual(What + TEXT("where the clip starts"), Room->GetClipStart(), Room->ClipStarts[Case.Choice]);
			const float Span = Length * AWasamiCapture::FadeStartShare(Case.Choice) - Room->GetClipStart();
			TestEqual(What + TEXT("fade start on the scene's time"), Room->SceneTime(Room->GetFadeStart()), Span, 1e-3f);
			TestTrue(What + TEXT("sooner than at rate 1"), Room->GetFadeStart() < Span - 0.3f);
			TestEqual(What + TEXT("the Matinee's fade then"), Room->SceneTime(Room->GetFadeStart()) * Room->GetMatineeRate(),
				AWasamiCapture::FadeStartShare(Case.Choice) * AWasamiCapture::MatineeLength(Case.Choice), 1e-3f);
			TestEqual(What + TEXT("fade end on the scene's time"), Room->SceneTime(Room->GetFadeStart() + Room->GetFadeDuration()),
				Span * AWasamiCapture::FadeEndShare(Case.Choice) / AWasamiCapture::FadeStartShare(Case.Choice), 1e-3f);
			TestEqual(What + TEXT("the death screen 3.5 s on"), Room->GetDeathDelay(), AWasamiCapture::DeathDelay);
		}
		TestTrue(What + TEXT("fade before the death screen"), Room->GetFadeStart() + Room->GetFadeDuration() <= Room->GetDeathDelay());

		// The death screen on time (the test world's timers go off a step late).
		constexpr float Step = 0.1f;
		for (float Time = 0.f; Time < Room->GetDeathDelay() - 0.15f; Time += Step)
		{
			Wrapper.TickTestWorld(Step);
		}
		TestTrue(What + TEXT("alive until then"), Mode->IsDeathOpen());
		Wrapper.TickTestWorld(0.2f);
		Wrapper.TickTestWorld(0.1f);
		TestFalse(What + TEXT("dead then"), Mode->IsDeathOpen());
		Room->Destroy();
		Mode->Destroy();
	}

	// The room itself.
	AWasamiCapture* Room = World->SpawnActor<AWasamiCapture>(AWasamiCapture::StaticClass(), FTransform(AWasamiCapture::RoomLocation));
	const UCameraComponent* View = Room->GetView();
	TestTrue(TEXT("the camera where the hotel's is"), View->GetRelativeLocation().Equals(AWasamiCapture::CameraOffset, 1e-3));
	TestTrue(TEXT("and where it starts"), Room->CameraStart.Equals(AWasamiCapture::CameraOffset, 1e-9));
	// The hotel frames the monkey's 98.5 cm head from 94.7 cm, level with 0.534 of it; the room frames the Wasami's
	// upper 115.5 cm the same way.
	TestEqual(TEXT("the frame's scale"), AWasamiCapture::FrameScale, AWasamiCapture::WasamiTop / 2. / 98.5315, 1e-4);
	TestTrue(TEXT("some 111 cm in front, 3 cm to the side, 177 cm up"), View->GetRelativeLocation().Equals(FVector(110.99, 3.38, 177.21), 0.05));
	TestTrue(TEXT("looking back at the Wasami"), View->GetForwardVector().Equals(FVector::BackwardVector, 1e-4));
	TestEqual(TEXT("its field of view"), View->FieldOfView, 90.f);
	// ceilinglights_80 in a room shrunk to SceneScale: its lengths at that scale and its strength at the square of it,
	// which lays the same light on the Wasami as the hotel's does on the monkey.
	const UPointLightComponent* Light = Room->GetLight();
	TestEqual(TEXT("the room's scale"), AWasamiCapture::SceneScale, 0.878024, 1e-6);
	TestEqual(TEXT("the ceiling light's intensity"), Light->Intensity, 1500.f * 0.878024f * 0.878024f, 0.1f);
	TestTrue(TEXT("unitless"), Light->IntensityUnits == ELightUnits::Unitless);
	TestEqual(TEXT("its radius"), Light->AttenuationRadius, 500.f * 0.878024f, 0.01f);
	TestEqual(TEXT("its source radius"), Light->SourceRadius, 24.715225f * 0.878024f, 0.01f);
	TestTrue(TEXT("its colour"), Light->LightColor == FColor(255, 236, 142, 255));
	TestTrue(TEXT("above the Wasami"), Light->GetRelativeLocation().Z > AWasamiCapture::WasamiTop);

	// jumpscarelight and _5 on the eyes: 15 and 15 cm on a head a third of the monkey's, so the reach goes by that
	// and the strength by its square.
	TestEqual(TEXT("the head's scale"), AWasamiCapture::FaceScale, 0.334672, 1e-6);
	TestEqual(TEXT("two lights on the face"), Room->GetFaceLights().Num(), AWasamiCapture::NumFaceLights);
	for (int32 Index = 0; Index < Room->GetFaceLights().Num(); ++Index)
	{
		const UPointLightComponent* Eye = Room->GetFaceLights()[Index];
		TestFalse(TEXT("the face's light casts no shadow"), Eye->CastShadows);
		TestTrue(TEXT("the face's light in the ceiling light's colour"), Eye->LightColor == FColor(255, 236, 142, 255));
		TestTrue(TEXT("unitless"), Eye->IntensityUnits == ELightUnits::Unitless);
		TestEqual(TEXT("its reach at the head's scale"), Eye->AttenuationRadius, 15.f * 0.334672f, 1e-3f);
		TestEqual(TEXT("its strength at the square of it"), Eye->Intensity, 15.f * 0.334672f * 0.334672f, 1e-3f);
		// Where the eye sockets are in the mesh's own frame, however the head bone it rides is turned.
		const FVector At = Room->GetBody()->GetComponentTransform().InverseTransformPosition(Eye->GetComponentLocation());
		TestTrue(TEXT("on the face where the monkey's are"), At.Equals(AWasamiCapture::FaceLightPlaces[Index], 0.05));
		TestTrue(TEXT("outside the face it lights"), At.Y > 11.4 && At.Y < 12.1);
	}

	// Six black planes facing in, the camera and the slide's start inside them.
	TestEqual(TEXT("six planes"), Room->GetWalls().Num(), 6);
	const FVector Inside = Room->GetActorLocation() + FVector(-500., 0., 1000.);
	for (const UStaticMeshComponent* Plane : Room->GetWalls())
	{
		TestTrue(TEXT("facing in"), FVector::DotProduct(Plane->GetUpVector(), Inside - Plane->GetComponentLocation()) > 0.);
		TestTrue(TEXT("black"), Plane->GetMaterial(0) && Plane->GetMaterial(0)->GetName() == TEXT("M_WasamiCaptureBlack"));
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

		// Restore State: what the death carried, back on the player the level makes again (the roadmap's 38). The tablet
		// is up at once, where Toggle Tablet would start its rise at the bottom, and the sprint is on at its speed.
		Player->SetMoveSpeeds(300.f, 600.f);
		const FVector Stowed = Player->GetTablet()->GetComponentLocation();
		Player->RestoreState(true, true);
		TestTrue(TEXT("the tablet up again"), Player->IsTabletUp());
		TestTrue(TEXT("and up at once, with no rise left to play"),
			FVector::DotProduct(Player->GetTablet()->GetComponentLocation() - Stowed, Player->GetActorUpVector()) > 30.);
		TestTrue(TEXT("the sprint on, held"), Player->IsSprintOn());
		TestEqual(TEXT("at Sprinting Speed"), Player->GetCharacterMovement()->MaxWalkSpeed, Player->SprintingSpeed);
		Player->StopSprinting();
		Player->bToggleSprint = true;
		Player->RestoreState(true, true);
		TestTrue(TEXT("the sprint on, latched, with TOGGLE SPRINT"), Player->IsSprintOn());
		Player->StopSprinting();
		Player->bToggleSprint = false;
		Player->RestoreState(false, false);
		TestFalse(TEXT("nothing carried leaves the sprint off"), Player->IsSprintOn());
		TestTrue(TEXT("and does not put the tablet down"), Player->IsTabletUp());
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiCaptureCameraTest, "Wasami.Capture.Camera",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiCaptureCameraTest::RunTest(const FString& Parameters)
{
	// The scene's time: StartRate times as fast at first, easing back to 1, and its inverse.
	const AWasamiCapture* Defaults = GetDefault<AWasamiCapture>();
	TestEqual(TEXT("the scene starts with the capture"), Defaults->SceneTime(0.f), 0.f);
	TestEqual(TEXT("at StartRate at first"), Defaults->SceneTime(1e-3f) / 1e-3f, Defaults->StartRate, 0.01f);
	const float Rate = (Defaults->SceneTime(10.001f) - Defaults->SceneTime(10.f)) / 1e-3f;
	TestEqual(TEXT("at 1 later"), Rate, 1.f, 0.01f);
	TestEqual(TEXT("ahead by (StartRate - 1) x RateEaseTime in the end"), Defaults->SceneTime(10.f) - 10.f,
		(Defaults->StartRate - 1.f) * Defaults->RateEaseTime, 1e-3f);
	for (const float Time : {0.1f, 0.5f, 1.3f, 2.9f})
	{
		TestEqual(*FString::Printf(TEXT("the time back from the scene's at %.1f s"), Time), Defaults->RealTime(Defaults->SceneTime(Time)), Time, 1e-4f);
	}

	// The hotel's camera keys as saved, MonkeyJumpscare3's hold filled with MonkeyJumpscare's sway.
	FVector Location;
	FRotator Rotation;
	for (int32 Choice = 0; Choice < AWasamiCapture::NumHotelChoices; ++Choice)
	{
		AWasamiCapture::EvaluateHotelCamera(Choice, 0.f, Location, Rotation);
		TestTrue(TEXT("JumpscareCam where it starts"), Location.Equals(FVector(4832.646484375, 1075.6009521484375, 7107.3759765625), 1e-3));
		TestTrue(TEXT("looking along -X"), Rotation.Vector().Equals(FVector::BackwardVector, 1e-4));
	}
	AWasamiCapture::EvaluateHotelCamera(0, 0.46474358439445496f, Location, Rotation);
	TestTrue(TEXT("MonkeyJumpscare's key at 0.465 s"), Location.Equals(FVector(4857.294921875, 1075.6009521484375, 7127.689453125), 1e-2));
	TestTrue(TEXT("its roll, pitch and yaw"), Rotation.Equals(FRotator(-4.011479377746582, -180.61866760253906, 8.773558616638184), 1e-3));
	AWasamiCapture::EvaluateHotelCamera(2, 1.0456f, Location, Rotation);
	TestTrue(TEXT("MonkeyJumpscare2 down on the floor at 1.046 s"), FMath::IsNearlyEqual(Location.Z, 6950.666, 0.5));
	TestEqual(TEXT("looking up 51.7°"), Rotation.Pitch, 51.682, 0.01);
	AWasamiCapture::EvaluateHotelCamera(1, 0.6721904f, Location, Rotation);
	TestTrue(TEXT("MonkeyJumpscare3 with MonkeyJumpscare's 0.465 s key stretched to 0.672 s"),
		Location.Equals(FVector(4857.294921875, 1075.6009521484375, 7127.689453125), 1e-2));
	AWasamiCapture::EvaluateHotelCamera(1, 2.93878173828125f, Location, Rotation);
	TestTrue(TEXT("and its own lunge at the end"), Location.Equals(FVector(4587.26416015625, 1072.32763671875, 7085.09912109375), 1e-2));

	// JumpscareCam's focus pull: the middle of the band the Matinee holds sharp, grown by FrameScale.
	TestEqual(TEXT("the focus out at the far wall at first"), AWasamiCapture::HotelFocalDistance(0.f), 502.593f, 0.01f);
	TestEqual(TEXT("the near edge held to 0.128 s while the band closes in"),
		AWasamiCapture::HotelFocalDistance(AWasamiCapture::DofFocalDepthTime * 0.5f), 364.408f, 0.01f);
	TestEqual(TEXT("the near edge still where it was at the end of its hold"),
		AWasamiCapture::HotelFocalDistance(AWasamiCapture::DofFocalNearHold), 342.545f, 0.01f);
	TestEqual(TEXT("on the Wasami by 0.2315 s"), AWasamiCapture::HotelFocalDistance(AWasamiCapture::DofFocalDepthTime), 70.349f, 0.01f);
	TestEqual(TEXT("and held there for the rest of the Matinee"), AWasamiCapture::HotelFocalDistance(3.f), 70.349f, 0.01f);
	TestEqual(TEXT("held before it starts too"), AWasamiCapture::HotelFocalDistance(-1.f), 502.593f, 0.01f);
	bool bPullsIn = true;
	for (float Time = 0.f; Time < AWasamiCapture::DofFocalDepthTime; Time += 0.005f)
	{
		bPullsIn &= AWasamiCapture::HotelFocalDistance(Time) > AWasamiCapture::HotelFocalDistance(Time + 0.005f);
	}
	TestTrue(TEXT("pulling in the whole way, never back out"), bPullsIn);

	// 03_Watcher_Kill3 from rest: 82 cm on, 29 cm up, looking 25° down.
	FVector Move;
	FRotator Turn;
	AWasamiCapture::EvaluateWatcherCamera(0.f, Move, Turn);
	TestTrue(TEXT("the watcher's camera at rest at first"), Move.IsNearlyZero(1e-3) && Turn.IsNearlyZero(1e-3));
	AWasamiCapture::EvaluateWatcherCamera(5.f, Move, Turn);
	TestTrue(TEXT("pushed in at the end"), Move.Equals(FVector(81.668, 1.877, 29.035), 0.01));
	TestEqual(TEXT("looking down"), Turn.Pitch, -25.127, 0.01);

	// A room playing each: the camera sways from where it starts and follows the Wasami; the face's rushes in.
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	constexpr float Step = 1.f / 60.f;
	for (int32 Choice = 0; Choice < AWasamiCapture::NumChoices; ++Choice)
	{
		AWasamiCapture* Room = World->SpawnActor<AWasamiCapture>(AWasamiCapture::StaticClass(), FTransform(AWasamiCapture::RoomLocation));
		if (!TestNotNull(TEXT("the room"), Room))
		{
			return false;
		}
		Room->Start(nullptr, nullptr, Choice);
		const FString What = FString::Printf(TEXT("capture %d: "), Choice);
		const UCameraComponent* View = Room->GetView();
		const FVector StartLocation = View->GetRelativeLocation();
		const FQuat StartRotation = View->GetRelativeRotation().Quaternion();
		const bool bFace = Choice == AWasamiCapture::FaceChoice;
		TestTrue(What + TEXT("the camera where it starts"), StartLocation.Equals(bFace ? Room->FaceCameraOffset : Room->CameraStart, 1e-3));
		TestTrue(What + TEXT("looking back at the Wasami"), View->GetForwardVector().X < -0.8);
		TestEqual(What + TEXT("its field of view"), View->FieldOfView, bFace ? AWasamiCapture::WatcherFieldOfView : AWasamiCapture::FieldOfView);
		// The hotel's depth of field on the camera itself; the watcher's kill has none.
		const FPostProcessSettings& Post = View->PostProcessSettings;
		TestTrue(What + TEXT("the depth of field is the hotel's alone"), (Post.bOverride_DepthOfFieldFocalDistance != 0) == !bFace);
		if (!bFace)
		{
			TestTrue(What + TEXT("PostProcessVolume_1's Fstop"), Post.bOverride_DepthOfFieldFstop != 0
				&& FMath::IsNearlyEqual(Post.DepthOfFieldFstop, AWasamiCapture::DofFstop));
			TestEqual(What + TEXT("focused out where the Matinee starts"), Post.DepthOfFieldFocalDistance,
				AWasamiCapture::HotelFocalDistance(0.f), 0.01f);
		}
		const float FocusAtStart = Post.DepthOfFieldFocalDistance;

		double MostTurn = 0.;
		double MostMove = 0.;
		double LeastX = TNumericLimits<double>::Max();
		bool bInside = true;
		bool bKeptAway = true;
		bool bEyesRide = true;
		// The eye lights hang off the head bone, so they keep their distance from it through the whole clip.
		TArray<double> EyeFromHead;
		for (const UPointLightComponent* Eye : Room->GetFaceLights())
		{
			EyeFromHead.Add((Eye->GetComponentLocation() - Room->GetBody()->GetSocketLocation(TEXT("head"))).Size());
		}
		const float Until = FMath::Min(Room->GetFadeStart(), 1.1f);
		for (float Time = 0.f; Time < Until; Time += Step)
		{
			Wrapper.TickTestWorld(Step);
			const FVector At = View->GetRelativeLocation();
			MostTurn = FMath::Max(MostTurn, FMath::RadiansToDegrees(View->GetRelativeRotation().Quaternion().AngularDistance(StartRotation)));
			MostMove = FMath::Max(MostMove, (At - StartLocation).Size());
			LeastX = FMath::Min(LeastX, At.X);
			bInside &= At.Z > 0. && At.X > -1500. && At.X < 500. && FMath::Abs(At.Y) < 1000.;
			const FVector HeadAt = Room->GetBody()->GetSocketLocation(TEXT("head"));
			const FVector Head = Room->GetActorTransform().InverseTransformPosition(HeadAt);
			bKeptAway &= At.X >= Head.X + 20.;
			for (int32 Eye = 0; Eye < Room->GetFaceLights().Num(); ++Eye)
			{
				bEyesRide &= FMath::IsNearlyEqual((Room->GetFaceLights()[Eye]->GetComponentLocation() - HeadAt).Size(), EyeFromHead[Eye], 1.);
			}
		}
		TestTrue(What + FString::Printf(TEXT("turned %.1f° from where it started"), MostTurn), MostTurn > 5.);
		TestTrue(What + FString::Printf(TEXT("moved %.0f cm"), MostMove), MostMove > 20.);
		TestTrue(What + TEXT("inside the room"), bInside);
		TestTrue(What + TEXT("never into the Wasami's head"), bKeptAway);
		if (bFace)
		{
			TestTrue(What + TEXT("the Wasami there by then"), FMath::Abs(Room->GetBody()->GetRelativeLocation().X) < 5.);
			TestTrue(What + FString::Printf(TEXT("the camera pushed %.0f cm in at the face"), Room->FaceCameraOffset.X - LeastX),
				Room->FaceCameraOffset.X - LeastX > 40.);
		}
		TestTrue(What + TEXT("the eye lights ride the head"), bEyesRide);
		TestTrue(What + FString::Printf(TEXT("as far from it as the bind pose puts them (%.1f cm)"), EyeFromHead.Num() > 0 ? EyeFromHead[0] : 0.),
			EyeFromHead.Num() == AWasamiCapture::NumFaceLights && EyeFromHead[0] > 20. && EyeFromHead[0] < 35.);
		if (!bFace)
		{
			// The pull is through long before the fade (0.2315 s of the Matinee, sooner still on the scene's time).
			TestEqual(What + TEXT("the focus pulled in on the Wasami by then"), Post.DepthOfFieldFocalDistance,
				AWasamiCapture::HotelFocalDistance(AWasamiCapture::DofFocalDepthTime), 0.01f);
			TestTrue(What + FString::Printf(TEXT("from %.0f cm out"), FocusAtStart), FocusAtStart > Post.DepthOfFieldFocalDistance + 100.f);
		}
		Room->Destroy();
	}

	// MonkeyJumpscare2's camera (Capture_3's) ends up on the floor.
	AWasamiCapture* Floor = World->SpawnActor<AWasamiCapture>(AWasamiCapture::StaticClass(), FTransform(AWasamiCapture::RoomLocation));
	Floor->Start(nullptr, nullptr, 2);
	for (float Time = 0.f; Time < Floor->GetFadeStart(); Time += Step)
	{
		Wrapper.TickTestWorld(Step);
	}
	TestTrue(TEXT("capture 2: down on the floor by the fade"), Floor->GetView()->GetRelativeLocation().Z < 60.);
	Floor->Destroy();

	// The black comes on over the fade on the Matinee's curve, where the camera manager's own fade is a straight line.
	AWasamiCapture* Fading = World->SpawnActor<AWasamiCapture>(AWasamiCapture::StaticClass(), FTransform(AWasamiCapture::RoomLocation));
	Fading->Start(nullptr, nullptr, 0);
	const float Fade = Fading->GetFadeDuration();
	if (TestTrue(TEXT("the fade takes a while"), Fade > 0.f))
	{
		// Up to just before the fade in coarse steps, then 64 to a fade's length through it.
		constexpr float Coarse = 0.02f;
		for (float Time = 0.f; Time + Coarse < Fading->GetFadeStart(); Time += Coarse)
		{
			Wrapper.TickTestWorld(Coarse);
		}
		TestEqual(TEXT("the screen clear until the fade"), Fading->GetFadeAmount(), 0.f);

		const float Fine = Fade / 64.f;
		TArray<float> Amounts;
		while (Amounts.Num() < 192 && (Amounts.Num() == 0 || Amounts.Last() < 1.f))
		{
			Wrapper.TickTestWorld(Fine);
			Amounts.Add(Fading->GetFadeAmount());
		}
		const int32 First = Amounts.IndexOfByPredicate([](float Amount) { return Amount > 0.f; });
		const int32 Black = Amounts.IndexOfByPredicate([](float Amount) { return Amount >= 1.f; });
		if (TestTrue(TEXT("black in the end"), First != INDEX_NONE && Black > First))
		{
			const int32 Span = Black - First;
			TestTrue(FString::Printf(TEXT("over the fade's own length (%d of 64 steps)"), Span), Span >= 60 && Span <= 68);
			bool bForward = true;
			for (int32 Index = First; Index < Black; ++Index)
			{
				bForward &= Amounts[Index] <= Amounts[Index + 1] && Amounts[Index] <= 1.f;
			}
			TestTrue(TEXT("never running back"), bForward);
			// 3a² - 2a³ against the straight line: 0.156 at a quarter through, 0.844 at three.
			TestTrue(FString::Printf(TEXT("easing in (%.3f a quarter through)"), Amounts[First + Span / 4]),
				Amounts[First + Span / 4] < 0.21f);
			TestTrue(FString::Printf(TEXT("and out (%.3f at three quarters)"), Amounts[First + 3 * Span / 4]),
				Amounts[First + 3 * Span / 4] > 0.79f);
			TestEqual(TEXT("half way at the middle"), Amounts[First + Span / 2], 0.5f, 0.05f);
		}
		// Held black from there.
		Wrapper.TickTestWorld(0.1f);
		TestEqual(TEXT("and held black"), Fading->GetFadeAmount(), 1.f);
	}
	Fading->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiCaptureVoiceTest, "Wasami.Capture.Voice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiCaptureVoiceTest::RunTest(const FString& Parameters)
{
	// One voice a capture in place of the original's own waves: the hotel's three cry You where their Matinees open
	// with Evil_Monkey_Scream, and the face says Over where the Gold Watcher laughs with the grab.
	const AWasamiCapture* Defaults = GetDefault<AWasamiCapture>();
	for (int32 Choice = 0; Choice < AWasamiCapture::NumHotelChoices; ++Choice)
	{
		const FString What = FString::Printf(TEXT("capture %d: "), Choice);
		TestEqual(What + TEXT("as the Matinee opens"), AWasamiCapture::VoiceTime(Choice), 0.f);
		const USoundBase* Voice = Defaults->GetVoice(Choice);
		TestTrue(What + TEXT("Wasami_You"), Voice && Voice->GetName() == TEXT("Wasami_You"));
	}
	TestEqual(TEXT("the face's with the grab"), AWasamiCapture::VoiceTime(AWasamiCapture::FaceChoice), AWasamiCapture::WatcherAnimDelay);
	const USoundBase* Face = Defaults->GetVoice(AWasamiCapture::FaceChoice);
	TestTrue(TEXT("Wasami_Over"), Face && Face->GetName() == TEXT("Wasami_Over"));
	// Over is said and done before the death screen; You runs past it, but all that is left of it by then is the tail
	// of its reverb (the progress of 2026-09-22), and PlaySound2D goes on through the pause either way.
	TestTrue(TEXT("the face's is over before the death screen"),
		AWasamiCapture::VoiceTime(AWasamiCapture::FaceChoice) + WasamiVoice::Seconds(EWasamiVoice::Over) < AWasamiCapture::FaceDeathDelay);

	// A room playing each: the hotel's cries with the capture, the face's waits on its timer.
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	constexpr float Step = 1.f / 60.f;
	for (int32 Choice = 0; Choice < AWasamiCapture::NumChoices; ++Choice)
	{
		AWasamiCapture* Room = World->SpawnActor<AWasamiCapture>(AWasamiCapture::StaticClass(), FTransform(AWasamiCapture::RoomLocation));
		if (!TestNotNull(TEXT("the room"), Room))
		{
			return false;
		}
		Room->Start(nullptr, nullptr, Choice);
		const FString What = FString::Printf(TEXT("capture %d: "), Choice);
		const bool bFace = Choice == AWasamiCapture::FaceChoice;
		TestEqual(What + TEXT("the voice cried or waiting"), Room->GetVoiceDelay(),
			bFace ? AWasamiCapture::WatcherAnimDelay : 0.f, Step);
		for (float Time = 0.f; Time < AWasamiCapture::FaceDeathDelay; Time += Step)
		{
			Wrapper.TickTestWorld(Step);
		}
		TestEqual(What + TEXT("cried by 1.15 s"), Room->GetVoiceDelay(), 0.f);
		Room->Destroy();
	}
	return true;
}

#endif
