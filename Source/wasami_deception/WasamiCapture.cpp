#include "WasamiCapture.h"

#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiEnemy.h"
#include "WasamiEnemyAnimInstance.h"
#include "WasamiGameInstance.h"
#include "WasamiGameMode.h"
#include "WasamiPlayerCharacter.h"

namespace
{
	// The monkey's feet in 01_Hotel (MonkeyJumpscare's InterpTrackMove_1 at t = 0), its camera (InterpTrackMove_2 at
	// t = 0) and the ceiling light above it.
	const FVector HotelMonkey(4737.9833984375, 1072.7213134765625, 6917.716796875);
	const FVector HotelCamera(4832.646484375, 1075.6009521484375, 7107.3759765625);
	const FVector HotelLight(4777.72900390625, 1072.5234375, 7202.3505859375);

	// Each Matinee's length and its InterpTrackFade's keys (0 → 1, bPersistFade), in the order the clips pair with them:
	// MonkeyJumpscare, MonkeyJumpscare2, MonkeyJumpscare3.
	struct FMatineeFade
	{
		float Length;
		float Start;
		float End;
	};
	constexpr FMatineeFade MatineeFades[AWasamiCapture::NumChoices] = {
		{2.121222972869873f, 1.7005259990692139f, 1.769968032836914f},
		{2.464282989501953f, 1.920689582824707f, 2.051270008087158f},
		{3.068389654159546f, 2.6005260944366455f, 2.769968032836914f},
	};

	// The room: a 20 m cube of black planes around the mark, its floor at the Wasami's feet, reaching 15 m behind it
	// (Capture_2 starts some 7.75 m back).
	const FVector RoomCentre(-500., 0., 1000.);
	constexpr double RoomHalfSize = 1000.;
	// Engine's Plane is 100 cm across.
	constexpr double PlaneSize = 100.;
	const FVector WallNormals[] = {
		FVector(0., 0., 1.), FVector(0., 0., -1.), FVector(1., 0., 0.), FVector(-1., 0., 0.), FVector(0., 1., 0.),
		FVector(0., -1., 0.),
	};

	// The enemy's mesh as the room's Wasami wears it: facing +X, grown as the enemy's.
	FTransform BodyPlacement()
	{
		return FTransform(FRotator(0., AWasamiEnemy::MeshYaw, 0.), FVector::ZeroVector, FVector(AWasamiEnemy::MeshScale));
	}
}

const FVector AWasamiCapture::RoomLocation(0., 0., 50000.);
const double AWasamiCapture::MonkeyTop = 65.78780364990234 * 4.;
const double AWasamiCapture::WasamiTop = 170. * AWasamiEnemy::MeshScale;
const double AWasamiCapture::SceneScale = AWasamiCapture::WasamiTop / AWasamiCapture::MonkeyTop;
const FVector AWasamiCapture::HotelCameraOffset = HotelCamera - HotelMonkey;
const FVector AWasamiCapture::HotelLightOffset = HotelLight - HotelMonkey;
const double AWasamiCapture::MonkeyHeadBase = 34.261745931581764 * 4.;
const double AWasamiCapture::MonkeyHeadTop = 58.8946292245342 * 4.;
const double AWasamiCapture::FrameScale = AWasamiCapture::WasamiTop / (AWasamiCapture::MonkeyHeadTop - AWasamiCapture::MonkeyHeadBase);
const FVector AWasamiCapture::CameraOffset(
	AWasamiCapture::HotelCameraOffset.X * AWasamiCapture::FrameScale,
	AWasamiCapture::HotelCameraOffset.Y * AWasamiCapture::FrameScale,
	(AWasamiCapture::HotelCameraOffset.Z - AWasamiCapture::MonkeyHeadBase) * AWasamiCapture::FrameScale);
const FColor AWasamiCapture::LightColor(142, 236, 255, 255);

AWasamiCapture::AWasamiCapture()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetRelativeTransform(BodyPlacement());
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	// JumpscareCam, where the Matinee's camera is at t = 0, looking back at the Wasami.
	View = CreateDefaultSubobject<UCameraComponent>(TEXT("View"));
	View->SetupAttachment(Root);
	View->SetRelativeLocationAndRotation(CameraOffset, FRotator(0., 180., 0.));
	View->SetFieldOfView(FieldOfView);

	// ceilinglights_80 (a stationary light in the hotel; this room is spawned, so it is movable).
	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetRelativeLocation(HotelLightOffset * SceneScale);
	Light->SetIntensityUnits(ELightUnits::Unitless);
	Light->SetIntensity(LightIntensity);
	Light->SetAttenuationRadius(LightRadius);
	Light->SetSourceRadius(LightSourceRadius);
	Light->SetLightFColor(LightColor);

	// jumpscareblock and its fellows: black unlit planes without shadows, here facing in on every side.
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(WallNormals); ++Index)
	{
		UStaticMeshComponent* Wall = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Wall%d"), Index));
		Wall->SetupAttachment(Root);
		const FVector& Normal = WallNormals[Index];
		Wall->SetRelativeLocationAndRotation(RoomCentre - Normal * RoomHalfSize, FRotationMatrix::MakeFromZ(Normal).Rotator());
		Wall->SetRelativeScale3D(FVector(2. * RoomHalfSize / PlaneSize, 2. * RoomHalfSize / PlaneSize, 1.));
		Wall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Wall->SetCastShadow(false);
		Walls.Add(Wall);
	}

	ShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Blueprints/Main/JumpscareShake")));
	BodyMesh = TSoftObjectPtr<USkeletalMesh>(WasamiAssets::Path(TEXT("/Game/Wasami/Enemy/SK_WasamiEnemy")));
	WallMesh = TSoftObjectPtr<UStaticMesh>(WasamiAssets::Path(TEXT("/Engine/BasicShapes/Plane")));
	WallMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Engine/EngineDebugMaterials/BlackUnlitMaterial")));
}

void AWasamiCapture::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Body->SetSkeletalMeshAsset(BodyMesh.LoadSynchronous());
	UStaticMesh* Plane = WallMesh.LoadSynchronous();
	UMaterialInterface* Black = WallMaterial.LoadSynchronous();
	for (UStaticMeshComponent* Wall : Walls)
	{
		Wall->SetStaticMesh(Plane);
		Wall->SetMaterial(0, Black);
	}
}

float AWasamiCapture::FadeStartShare(int32 InChoice)
{
	const FMatineeFade& Fade = MatineeFades[FMath::Clamp(InChoice, 0, NumChoices - 1)];
	return Fade.Start / Fade.Length;
}

float AWasamiCapture::FadeEndShare(int32 InChoice)
{
	const FMatineeFade& Fade = MatineeFades[FMath::Clamp(InChoice, 0, NumChoices - 1)];
	return Fade.End / Fade.Length;
}

AWasamiCapture* AWasamiCapture::StartCapture(const UObject* WorldContextObject, AActor* Cause, int32 InChoice)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	AWasamiGameMode* GameMode = World ? World->GetAuthGameMode<AWasamiGameMode>() : nullptr;
	if (!GameMode || !GameMode->IsDeathOpen() || TActorIterator<AWasamiCapture>(World))
	{
		return nullptr;
	}
	if (InChoice < 0 || InChoice >= NumChoices)
	{
		UWasamiGameInstance* Instance = GameMode->GetWasamiGameInstance();
		InChoice = Instance ? Instance->TakeCaptureChoice() : FMath::RandRange(0, NumChoices - 1);
	}
	AWasamiCapture* Room = World->SpawnActor<AWasamiCapture>(StaticClass(), FTransform(RoomLocation));
	if (Room)
	{
		Room->Start(GameMode, Cause, InChoice);
	}
	return Room;
}

void AWasamiCapture::Start(AWasamiGameMode* InMode, AActor* Cause, int32 InChoice)
{
	Mode = InMode;
	CauseActor = Cause;
	Choice = FMath::Clamp(InChoice, 0, NumChoices - 1);

	// The clip, and its fade at the paired Matinee's share of it. A clip that carries the Wasami forward starts as far
	// back as it goes by the fade, so that the pelvis is then where it began at the mark.
	Clip = Cast<UAnimSequence>(WasamiEnemyAnim::ClipPath(WasamiEnemyClip::Capture1 + Choice).TryLoad());
	const float Length = Clip ? Clip->GetPlayLength() : DeathDelay;
	FadeStart = Length * FadeStartShare(Choice);
	FadeDuration = Length * (FadeEndShare(Choice) - FadeStartShare(Choice));
	if (Clip)
	{
		FVector Travel = WasamiEnemyAnim::GetRootTransform(*Clip, FadeStart).GetLocation()
			- WasamiEnemyAnim::GetRootTransform(*Clip, 0.).GetLocation();
		Travel.Z = 0.;
		Body->SetRelativeLocation(-BodyPlacement().TransformVector(Travel));
		Body->PlayAnimation(Clip, false);
	}

	// DisableInput(the player's controller) on the player character, Put Down Tablet, and the Matinee's director cut
	// to its camera.
	APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
	if (ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		Player->DisableInput(Controller);
		if (AWasamiPlayerCharacter* WasamiPlayer = Cast<AWasamiPlayerCharacter>(Player))
		{
			WasamiPlayer->PutDownTablet();
		}
	}
	if (Controller)
	{
		Controller->SetViewTarget(this);
		if (const TSubclassOf<UCameraShakeBase> Shake = ShakeClass.LoadSynchronous())
		{
			Controller->ClientStartCameraShake(Shake, ShakeScale);
		}
	}

	FTimerManager& Timers = GetWorldTimerManager();
	Timers.SetTimer(FadeTimer, this, &AWasamiCapture::StartFade, FMath::Max(FadeStart, KINDA_SMALL_NUMBER), false);
	Timers.SetTimer(DeathTimer, this, &AWasamiCapture::EndCapture, DeathDelay, false);
}

void AWasamiCapture::StartFade()
{
	// TODO(仮): the Matinee's fade eases (auto-clamped keys); the camera manager's is linear. Held black until the
	// level opens again.
	if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APlayerCameraManager* Camera = Controller->PlayerCameraManager)
		{
			Camera->StartCameraFade(0.f, 1.f, FadeDuration, FLinearColor::Black, false, true);
		}
	}
}

void AWasamiCapture::EndCapture()
{
	// Delay 3.5 → the zone's DeathEvent (the death screen, the pause). The enemy that caught the player is gone by now
	// (the capture removes every enemy), which reads as no cause: not the player's doing either way.
	if (AWasamiGameMode* GameMode = Mode.Get())
	{
		GameMode->DeathEvent(CauseActor.Get());
	}
}
