#include "WasamiTelepathyTracker.h"

#include "Components/WidgetComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "WasamiTelepathyTrackerWidget.h"

namespace
{
	// BP_TelepathyTracker (pak_reference_2): MapRangeUnclamped(distance, 0, 10000, 0.5, 0.1), and Remove's Delay.
	constexpr float NearDistance = 0.f;
	constexpr float FarDistance = 10000.f;
	constexpr float NearSize = 0.5f;
	constexpr float FarSize = 0.10000000149011612f;
	constexpr float RemoveDelay = 0.5f;
}

AWasamiTelepathyTracker::AWasamiTelepathyTracker()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = SceneRoot;

	// The export sets the space and the class; its window visibility (Visible) is UE 4.24's default, which only a
	// world-space widget's virtual window uses.
	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	Widget->SetupAttachment(SceneRoot);
	Widget->SetWidgetSpace(EWidgetSpace::Screen);
	Widget->SetWidgetClass(UWasamiTelepathyTrackerWidget::StaticClass());
}

float AWasamiTelepathyTracker::SizeForDistance(float Distance)
{
	return FMath::GetMappedRangeValueUnclamped(FVector2f(NearDistance, FarDistance), FVector2f(NearSize, FarSize), Distance);
}

void AWasamiTelepathyTracker::BeginPlay()
{
	// The component makes its widget in its own BeginPlay, before the actor's.
	Super::BeginPlay();
	WidgetReference = Cast<UWasamiTelepathyTrackerWidget>(Widget->GetUserWidgetObject());
	bGateOpen = true;
}

void AWasamiTelepathyTracker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RemoveTimer);
	Super::EndPlay(EndPlayReason);
}

void AWasamiTelepathyTracker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bGateOpen)
	{
		Update();
	}
}

void AWasamiTelepathyTracker::Update()
{
	if (!IsValid(Actor))
	{
		Remove();
		return;
	}
	// Onto the enemy's origin (its capsule's centre), then the size by the distance to the player (0 without one).
	SetActorLocation(Actor->GetActorLocation());
	const float Distance = static_cast<float>(GetDistanceTo(UGameplayStatics::GetPlayerCharacter(this, 0)));
	if (WidgetReference)
	{
		WidgetReference->SetSize(SizeForDistance(Distance));
	}
}

void AWasamiTelepathyTracker::Remove()
{
	bGateOpen = false;
	if (WidgetReference)
	{
		WidgetReference->Remove();
	}
	// A Delay: a second Remove while it counts does not restart it.
	FTimerManager& Timers = GetWorldTimerManager();
	if (!Timers.IsTimerActive(RemoveTimer))
	{
		Timers.SetTimer(RemoveTimer, this, &AWasamiTelepathyTracker::DestroyAfterRemove, RemoveDelay, false);
	}
}
