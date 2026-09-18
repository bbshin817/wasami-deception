#include "WasamiDoorBreak.h"

#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiPlayerCharacter.h"
#include "WasamiSwitchboxWidget.h"

namespace
{
	// BP_06_Hospital_DoorBreak (pak_reference_2): Box1's and Widget's templates.
	const FVector DoorBreakBoxExtent(100., 150., 100.);
	const FVector DoorBreakScale(0.5, 0.5, 0.5);
	const FVector2D DoorBreakWidgetSize(64., 64.);

	// Finished's PlaySound2D (volume, pitch); the press's PlaySoundAtLocation is at 1 and 1.
	constexpr float SlamVolume = 0.5f;
	constexpr float SlamPitch = 2.f;
	constexpr float LockpickedVolume = 2.f;
	constexpr float LockpickedPitch = 2.f;
}

AWasamiDoorBreak::AWasamiDoorBreak()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	// Widget_GEN_VARIABLE: screen space, 64 × 64, hidden. Its half scale does nothing in screen space; its window
	// visibility (Visible) is UE 4.24's default, which only a world-space widget's virtual window uses.
	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	Widget->SetupAttachment(DefaultSceneRoot);
	Widget->SetWidgetSpace(EWidgetSpace::Screen);
	Widget->SetWidgetClass(UWasamiSwitchboxWidget::StaticClass());
	Widget->SetDrawSize(DoorBreakWidgetSize);
	Widget->SetRelativeScale3D(DoorBreakScale);
	Widget->SetVisibility(false);

	// Box1_GEN_VARIABLE: a UBoxComponent at its defaults (OverlapAllDynamic, hidden in game) with its extent and half
	// scale, not generating overlaps. Its AreaClass (NavArea_Obstacle) is left out: a box that blocks nothing is not
	// part of the navigation.
	Box1 = CreateDefaultSubobject<UBoxComponent>(TEXT("Box1"));
	Box1->SetupAttachment(DefaultSceneRoot);
	Box1->SetBoxExtent(DoorBreakBoxExtent, false);
	Box1->SetRelativeScale3D(DoorBreakScale);
	Box1->SetGenerateOverlapEvents(false);

	// The class's AutoReceiveInput (Player0, priority 5) is how its Interact reaches it; here the player passes it on.
	LockpickingSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/06_Hospital/Lockpicking/SFX_06_Lockpicking")));
	LockpickingAttenuation = TSoftObjectPtr<USoundAttenuation>(WasamiAssets::Path(TEXT("/Game/DD/Audio/01_Hotel/01_Lobby_Attenuation")));
	SlamSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/07_FunPlace/Press_Slam_02")));
	LockpickedSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/06_Hospital/Lockpicking/SFX_06_Lockpicked")));
}

void AWasamiDoorBreak::BeginPlay()
{
	// The widget component makes its widget in its own BeginPlay, before the actor's.
	Super::BeginPlay();
	LoadedLockpickingSound = LockpickingSound.LoadSynchronous();
	LoadedLockpickingAttenuation = LockpickingAttenuation.LoadSynchronous();
	LoadedSlamSound = SlamSound.LoadSynchronous();
	LoadedLockpickedSound = LockpickedSound.LoadSynchronous();

	Box1->OnComponentBeginOverlap.AddDynamic(this, &AWasamiDoorBreak::OnBoxBeginOverlap);
	Box1->OnComponentEndOverlap.AddDynamic(this, &AWasamiDoorBreak::OnBoxEndOverlap);

	// ReceiveBeginPlay: UI Widget, its Finished bound to Finished, and its Progress Speed.
	UIWidget = Cast<UWasamiSwitchboxWidget>(Widget->GetUserWidgetObject());
	if (UIWidget)
	{
		UIWidget->Finished.AddDynamic(this, &AWasamiDoorBreak::Finished);
		UIWidget->ProgressSpeed = ProgressSpeed;
	}

	// The player is placed before play begins.
	if (AWasamiPlayerCharacter* Player = Cast<AWasamiPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		InteractSource = Player;
		InteractHandle = Player->OnInteract.AddUObject(this, &AWasamiDoorBreak::Interact);
	}
}

void AWasamiDoorBreak::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AWasamiPlayerCharacter* Player = InteractSource.Get())
	{
		Player->OnInteract.Remove(InteractHandle);
	}
	GetWorldTimerManager().ClearTimer(RemoveWidgetTimer);
	Super::EndPlay(EndPlayReason);
}

void AWasamiDoorBreak::EnableSwitch()
{
	if (IsValid(Box1))
	{
		Box1->SetGenerateOverlapEvents(true);
	}
}

void AWasamiDoorBreak::Disable()
{
	if (IsValid(Widget))
	{
		Widget->DestroyComponent();
	}
	if (IsValid(Box1))
	{
		Box1->DestroyComponent();
	}
}

void AWasamiDoorBreak::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor == UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		NotifyPlayerOverlap(true);
	}
}

void AWasamiDoorBreak::OnBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor == UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		NotifyPlayerOverlap(false);
	}
}

void AWasamiDoorBreak::NotifyPlayerOverlap(bool bBegin)
{
	bInRange = bBegin;
	if (IsValid(Widget))
	{
		Widget->SetVisibility(bBegin);
	}
	// Walking out empties the lock: its Progress and its ring.
	if (!bBegin && UIWidget)
	{
		UIWidget->Progress = 0.f;
		UIWidget->SetProgress(0.f);
	}
}

void AWasamiDoorBreak::Interact()
{
	if (!bInRange)
	{
		return;
	}
	if (UIWidget)
	{
		UIWidget->InteractEvent();
	}
	UGameplayStatics::PlaySoundAtLocation(this, LoadedLockpickingSound, GetActorLocation(), FRotator::ZeroRotator, 1.f, 1.f,
		0.f, LoadedLockpickingAttenuation);
}

void AWasamiDoorBreak::Finished()
{
	UGameplayStatics::PlaySound2D(this, LoadedSlamSound, SlamVolume, SlamPitch);
	UGameplayStatics::PlaySound2D(this, LoadedLockpickedSound, LockpickedVolume, LockpickedPitch);
	// Destroying the box ends its overlap with the player, which hides and empties the lock; the lock is then shown
	// again for its Completed.
	if (IsValid(Box1))
	{
		Box1->DestroyComponent();
	}
	bInRange = false;
	if (IsValid(Widget))
	{
		Widget->SetVisibility(true);
	}
	// The Delay is set before Finished Event fires (the event's handlers run while it counts).
	GetWorldTimerManager().SetTimer(RemoveWidgetTimer, this, &AWasamiDoorBreak::RemoveWidget, RemoveWidgetDelay, false);
	FinishedEvent.Broadcast();
}

void AWasamiDoorBreak::RemoveWidget()
{
	if (IsValid(Widget))
	{
		Widget->DestroyComponent();
	}
}
