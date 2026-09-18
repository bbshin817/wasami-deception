#include "WasamiDoubleDoors.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/RichCurve.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiPlayerCharacter.h"

namespace
{
	// BP_06_DoubleDoors (pak_reference_2): the SCS templates. The boxes keep UBoxComponent's 32 cm extent and are sized
	// by their scale.
	const FVector DoorsStaticMeshLocation(-200., 0., 0.);
	const FVector DoorsStaticMesh1Location(200., 0., 0.);
	const FVector DoorsFrontEnterLocation(0., -150., 50.);
	const FVector DoorsFrontEnterScale(6.258716106414795, 2.8199589252471924, 1.);
	const FVector DoorsBackEnterLocation(0., 150., 50.);
	const FVector DoorsBackEnterScale(6.258716106414795, 2.972306966781616, 1.);
	const FVector DoorsLeaveLocation(0., 0., 50.);
	const FVector DoorsLeaveScale(6.258716106414795, 12.065208435058594, 1.);

	// PlaySoundAtLocation's volume and pitch: Open Door's, the closing timeline's Sound key's, Locked Sound's.
	constexpr float DoorsOpenVolume = 0.5f;
	constexpr float DoorsOpenPitch = 1.2f;
	constexpr float DoorsCloseVolume = 0.5f;
	constexpr float DoorsClosePitch = 1.f;
	constexpr float DoorsLockedVolume = 0.5f;
	constexpr float DoorsLockedPitch = 1.5f;

	/** Timeline_0's and Timeline_1's Float track (CurveFloat_0_1, CurveFloat_0_1_3: the same keys), with UE's tangents. */
	FRichCurve MakeDoorsSwingCurve()
	{
		struct FKey
		{
			float Time;
			float Value;
			float Tangent;
			ERichCurveInterpMode Interp;
		};
		const FKey Keys[] = {
			{0.f, 0.f, 0.f, RCIM_Cubic},
			{0.5519317984580994f, 0.9510974884033203f, 1.5182865858078003f, RCIM_Cubic},
			{0.6852617859840393f, 1.0404237508773804f, 0.1091407760977745f, RCIM_Cubic},
			{1.f, 1.f, 0.f, RCIM_Linear},
		};
		FRichCurve Curve;
		for (const FKey& Each : Keys)
		{
			FRichCurveKey& Key = Curve.GetKey(Curve.AddKey(Each.Time, Each.Value));
			Key.InterpMode = Each.Interp;
			Key.TangentMode = RCTM_Break;
			Key.ArriveTangent = Each.Tangent;
			Key.LeaveTangent = Each.Tangent;
		}
		return Curve;
	}
}

AWasamiDoubleDoors::AWasamiDoubleDoors()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// The class's Tags, which the player's look at what can be used reads.
	Tags.Add(TEXT("interact"));

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	// The doors: UStaticMeshComponent's defaults (BlockAllDynamic, movable), out of the navigation.
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(DefaultSceneRoot);
	StaticMesh->SetRelativeLocation(DoorsStaticMeshLocation);
	StaticMesh->SetCanEverAffectNavigation(false);

	StaticMesh1 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh1"));
	StaticMesh1->SetupAttachment(DefaultSceneRoot);
	StaticMesh1->SetRelativeLocation(DoorsStaticMesh1Location);
	StaticMesh1->SetCanEverAffectNavigation(false);

	// The boxes: UBoxComponent's defaults (OverlapAllDynamic, hidden in game), out of the navigation (their AreaClass,
	// NavArea_Obstacle, then does nothing).
	auto MakeBox = [this](const TCHAR* Name, const FVector& Location, const FVector& Scale)
	{
		UBoxComponent* Box = CreateDefaultSubobject<UBoxComponent>(Name);
		Box->SetupAttachment(DefaultSceneRoot);
		Box->SetRelativeLocation(Location);
		Box->SetRelativeScale3D(Scale);
		Box->SetCanEverAffectNavigation(false);
		return Box;
	};
	FrontEnter = MakeBox(TEXT("FrontEnter"), DoorsFrontEnterLocation, DoorsFrontEnterScale);
	BackEnter = MakeBox(TEXT("BackEnter"), DoorsBackEnterLocation, DoorsBackEnterScale);
	Leave = MakeBox(TEXT("Leave"), DoorsLeaveLocation, DoorsLeaveScale);

	OpenSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/06_Hospital/SFX_06_DoubleDoor_Open")));
	CloseSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/06_Hospital/SFX_06_DoubleDoor_Close")));
	SwingAttenuation = TSoftObjectPtr<USoundAttenuation>(WasamiAssets::Path(TEXT("/Game/DD/Audio/Misc/MonkeyAttenuation")));
	LockedDoorSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/01_Hotel/Locked_Door")));
	LockedAttenuation = TSoftObjectPtr<USoundAttenuation>(WasamiAssets::Path(TEXT("/Game/DD/Audio/01_Hotel/01_Lobby_Attenuation")));
}

float AWasamiDoubleDoors::EvaluateSwing(float Seconds)
{
	static const FRichCurve Curve = MakeDoorsSwingCurve();
	return Curve.Eval(FMath::Clamp(Seconds, 0.f, SwingLength));
}

void AWasamiDoubleDoors::BeginPlay()
{
	Super::BeginPlay();
	LoadedOpenSound = OpenSound.LoadSynchronous();
	LoadedCloseSound = CloseSound.LoadSynchronous();
	LoadedSwingAttenuation = SwingAttenuation.LoadSynchronous();
	LoadedLockedDoorSound = LockedDoorSound.LoadSynchronous();
	LoadedLockedAttenuation = LockedAttenuation.LoadSynchronous();

	FrontEnter->OnComponentBeginOverlap.AddDynamic(this, &AWasamiDoubleDoors::OnFrontEnterBeginOverlap);
	BackEnter->OnComponentBeginOverlap.AddDynamic(this, &AWasamiDoubleDoors::OnBackEnterBeginOverlap);
	Leave->OnComponentEndOverlap.AddDynamic(this, &AWasamiDoubleDoors::OnLeaveEndOverlap);
}

void AWasamiDoubleDoors::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(LockedSoundTimer);
	Super::EndPlay(EndPlayReason);
}

void AWasamiDoubleDoors::OnFrontEnterBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	NotifyFrontEnter(OtherActor);
}

void AWasamiDoubleDoors::OnBackEnterBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	NotifyBackEnter(OtherActor);
}

void AWasamiDoubleDoors::OnLeaveEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	NotifyLeave(OtherActor);
}

void AWasamiDoubleDoors::NotifyFrontEnter(AActor* Other)
{
	// @475: any character, the player or a nurse.
	LastFrontEnter = Other;
	if (Cast<ACharacter>(Other))
	{
		OpenFromFront();
	}
}

void AWasamiDoubleDoors::OpenFromFront()
{
	// @550.
	if (bLocked)
	{
		LockedSound();
		return;
	}
	if (bOpenFront || bOpenBack)
	{
		return;
	}
	bOpenFront = true;
	bAnimation = true;
	OpenDoor();
	OnOpen.Broadcast();
}

void AWasamiDoubleDoors::NotifyBackEnter(AActor* Other)
{
	// @1143.
	if (!Cast<ACharacter>(Other))
	{
		return;
	}
	if (bLocked)
	{
		LockedSound();
		return;
	}
	if (bOpenBack || bOpenFront)
	{
		return;
	}
	bOpenBack = true;
	bAnimation = false;
	OpenDoor();
	OnOpen.Broadcast();
}

void AWasamiDoubleDoors::NotifyLeave(AActor* Other)
{
	LastLeave = Other;
	CloseFromLeave(Other);
}

void AWasamiDoubleDoors::CloseFromLeave(AActor* Other)
{
	// @889: a character walked out and the player is not left inside.
	if (Cast<ACharacter>(Other) && !IsPlayerOverlapping())
	{
		CloseOpenSide();
	}
}

void AWasamiDoubleDoors::CloseOpenSide()
{
	// @1054: they swing shut the way they opened.
	if (!AnySideOpen())
	{
		return;
	}
	if (bOpenFront)
	{
		bOpenFront = false;
		bAnimation = true;
	}
	else
	{
		bOpenBack = false;
		bAnimation = false;
	}
	CloseDoor();
}

bool AWasamiDoubleDoors::IsPlayerOverlapping() const
{
	if (bLocked)
	{
		return false;
	}
	TArray<AActor*> Players;
	Leave->GetOverlappingActors(Players, AWasamiPlayerCharacter::StaticClass());
	return Players.Num() > 0;
}

void AWasamiDoubleDoors::Lock()
{
	// @1333 → @889 for the actor Leave's event last had.
	bLocked = true;
	CloseFromLeave(LastLeave.Get());
}

void AWasamiDoubleDoors::Unlock()
{
	// @1359 → @475 for the actor the front box's event last had.
	bLocked = false;
	if (Cast<ACharacter>(LastFrontEnter.Get()))
	{
		OpenFromFront();
	}
}

void AWasamiDoubleDoors::OpenFront()
{
	OpenFromFront();
}

void AWasamiDoubleDoors::ForceClose()
{
	CloseOpenSide();
}

void AWasamiDoubleDoors::UpdateAnimationSpeed(float Speed)
{
	PlayRate = Speed;
}

void AWasamiDoubleDoors::OpenDoor()
{
	UGameplayStatics::PlaySoundAtLocation(this, LoadedOpenSound, GetActorLocation(), FRotator::ZeroRotator, DoorsOpenVolume,
		DoorsOpenPitch, 0.f, LoadedSwingAttenuation);
	PlayFromStart(OpenTimeline, true);
}

void AWasamiDoubleDoors::CloseDoor()
{
	StaticMesh1->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PlayFromStart(CloseTimeline, false);
}

void AWasamiDoubleDoors::LockedSound()
{
	if (bLockedSoundClosed)
	{
		return;
	}
	bLockedSoundClosed = true;
	UGameplayStatics::PlaySoundAtLocation(this, LoadedLockedDoorSound, GetActorLocation(), FRotator::ZeroRotator,
		DoorsLockedVolume, DoorsLockedPitch, 0.f, LoadedLockedAttenuation);
	GetWorldTimerManager().SetTimer(LockedSoundTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		bLockedSoundClosed = false;
	}), LockedSoundDelay, false);
}

void AWasamiDoubleDoors::UpdateFunc(bool bOpen, float Alpha)
{
	const float Amount = bAnimation ? OpenAmount : -OpenAmount;
	ApplyUpdateValues(bOpen ? FMath::Lerp(0.f, Amount, Alpha) : FMath::Lerp(Amount, 0.f, Alpha));
}

void AWasamiDoubleDoors::ApplyUpdateValues(float Value)
{
	StaticMesh->SetRelativeRotation(FRotator(0., Value, 0.));
	StaticMesh1->SetRelativeRotation(FRotator(0., -Value, 0.));
}

void AWasamiDoubleDoors::PlayFromStart(FSwing& Swing, bool bOpen)
{
	// FTimeline::PlayFromStart: to the start without its events but with an update, then playing.
	Swing.Position = 0.f;
	Swing.bPlaying = true;
	UpdateFunc(bOpen, EvaluateSwing(0.f));
	SetActorTickEnabled(true);
}

void AWasamiDoubleDoors::TickSwing(FSwing& Swing, bool bOpen, float DeltaSeconds)
{
	if (!Swing.bPlaying)
	{
		return;
	}
	// FTimeline::TickTimeline: stops at the end; an event fires when its time is in [old, new), pushed a little past the
	// end on the last tick.
	const float OldPosition = Swing.Position;
	float NewPosition = OldPosition + DeltaSeconds * PlayRate;
	if (NewPosition > SwingLength)
	{
		NewPosition = SwingLength;
		Swing.bPlaying = false;
	}
	Swing.Position = NewPosition;
	if (!bOpen)
	{
		const float MaxTime = NewPosition == SwingLength ? NewPosition + KINDA_SMALL_NUMBER : NewPosition;
		if (CloseSoundTime >= OldPosition && CloseSoundTime < MaxTime)
		{
			UGameplayStatics::PlaySoundAtLocation(this, LoadedCloseSound, GetActorLocation(), FRotator::ZeroRotator,
				DoorsCloseVolume, DoorsClosePitch, 0.f, LoadedSwingAttenuation);
		}
	}
	UpdateFunc(bOpen, EvaluateSwing(NewPosition));
}

void AWasamiDoubleDoors::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Timeline_0 before Timeline_1: with both running, the closing one has the last word.
	TickSwing(OpenTimeline, true, DeltaSeconds);
	TickSwing(CloseTimeline, false, DeltaSeconds);
	if (!OpenTimeline.bPlaying && !CloseTimeline.bPlaying)
	{
		SetActorTickEnabled(false);
	}
}
