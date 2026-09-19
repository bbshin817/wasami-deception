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
	const UPointLightComponent* Light = Room->GetLight();
	TestEqual(TEXT("the ceiling light's intensity"), Light->Intensity, 1500.f);
	TestTrue(TEXT("unitless"), Light->IntensityUnits == ELightUnits::Unitless);
	TestEqual(TEXT("its radius"), Light->AttenuationRadius, 500.f);
	TestTrue(TEXT("its colour"), Light->LightColor == FColor(255, 236, 142, 255));
	TestTrue(TEXT("above the Wasami"), Light->GetRelativeLocation().Z > AWasamiCapture::WasamiTop);
	const UPointLightComponent* FaceLight = Room->GetFaceLight();
	TestFalse(TEXT("the face's light casts no shadow"), FaceLight->CastShadows);
	TestTrue(TEXT("the face's light in the ceiling light's colour"), FaceLight->LightColor == FColor(255, 236, 142, 255));

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

		double MostTurn = 0.;
		double MostMove = 0.;
		double LeastX = TNumericLimits<double>::Max();
		bool bInside = true;
		bool bKeptAway = true;
		const float Until = FMath::Min(Room->GetFadeStart(), 1.1f);
		for (float Time = 0.f; Time < Until; Time += Step)
		{
			Wrapper.TickTestWorld(Step);
			const FVector At = View->GetRelativeLocation();
			MostTurn = FMath::Max(MostTurn, FMath::RadiansToDegrees(View->GetRelativeRotation().Quaternion().AngularDistance(StartRotation)));
			MostMove = FMath::Max(MostMove, (At - StartLocation).Size());
			LeastX = FMath::Min(LeastX, At.X);
			bInside &= At.Z > 0. && At.X > -1500. && At.X < 500. && FMath::Abs(At.Y) < 1000.;
			const FVector Head = Room->GetActorTransform().InverseTransformPosition(Room->GetBody()->GetSocketLocation(TEXT("head")));
			bKeptAway &= At.X >= Head.X + 20.;
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
		const FVector Head = Room->GetActorTransform().InverseTransformPosition(Room->GetBody()->GetSocketLocation(TEXT("head")));
		TestTrue(What + TEXT("the face's light by the head"), Room->GetFaceLight()->GetRelativeLocation().Equals(Head + Room->FaceLightOffset, 30.));
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
	return true;
}

#endif
