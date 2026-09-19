#include "WasamiFakeUseActor.h"

#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "WasamiAssets.h"

namespace
{
	const FName FakeUseInteractTag(TEXT("interact"));

	// BP_FakeUseActor_06_HospitalZone1_Elevator's Scene_GEN_VARIABLE and Audio1_GEN_VARIABLE (pak_reference_2).
	const FVector FakeElevatorSceneLocation(305., 70., -145.);
	const FVector FakeElevatorAudioLocation(0., 0., 160.);
}

AWasamiFakeUseActor::AWasamiFakeUseActor()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	// Box_GEN_VARIABLE: Custom on UBoxComponent's defaults (a WorldDynamic object), query and physics, ignoring the
	// engine's object channels (pawns among them); the trace channels, Visibility and Camera, keep their defaults
	// (block: the original's list holds only what differs from them), so the look's trace stops on it. Out of the
	// navigation (its NavArea_Obstacle then counts for nothing).
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetupAttachment(DefaultSceneRoot);
	Box->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	Box->SetCollisionObjectType(ECC_WorldDynamic);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Box->SetCollisionResponseToChannels(FCollisionResponseContainer::GetDefaultResponseContainer());
	for (const ECollisionChannel Ignored : {ECC_WorldStatic, ECC_WorldDynamic, ECC_Pawn, ECC_PhysicsBody, ECC_Vehicle,
			ECC_Destructible})
	{
		Box->SetCollisionResponseToChannel(Ignored, ECR_Ignore);
	}
	Box->SetCanEverAffectNavigation(false);
	Box->ComponentTags.Add(FakeUseInteractTag);
}

void AWasamiFakeUseActor::BeginPlay()
{
	Super::BeginPlay();
	// ReceiveBeginPlay: if bInactive, Box.SetCollisionEnabled(NoCollision).
	if (bInactive)
	{
		Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AWasamiFakeUseActor::Activate()
{
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AWasamiFakeUseActor::InteractWithObject_Implementation(AActor* Interactee)
{
	// InteractWithObject's DoOnce → broadcast Used → Used Event.
	if (bUsed)
	{
		return;
	}
	bUsed = true;
	OnUsed.Broadcast();
	UsedEvent();
}

void AWasamiFakeUseActor::UsedEvent()
{
	Destroy();
}

void AWasamiFakeUseSequencePlayer::UsedEvent()
{
	// Used Event: Sequence.GetSequencePlayer().Play() (the parent's is not called: the actor stays).
	ULevelSequencePlayer* Player = Sequence ? Sequence->GetSequencePlayer() : nullptr;
	if (!Player)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no level sequence to play"), *GetName());
		return;
	}
	Player->Play();
}

AWasamiFakeUseElevator::AWasamiFakeUseElevator()
{
	// The ActorSequence, played once.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	Scene->SetupAttachment(DefaultSceneRoot);
	Scene->SetRelativeLocation(FakeElevatorSceneLocation);

	// StaticMesh_GEN_VARIABLE and StaticMesh1_GEN_VARIABLE: UStaticMeshComponent's BlockAllDynamic, out of the
	// navigation.
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(Scene);
	StaticMesh->SetCanEverAffectNavigation(false);

	StaticMesh1 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh1"));
	StaticMesh1->SetupAttachment(Scene);
	StaticMesh1->SetCanEverAffectNavigation(false);

	Audio1 = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio1"));
	Audio1->SetupAttachment(Scene);
	Audio1->SetRelativeLocation(FakeElevatorAudioLocation);
	Audio1->bAutoActivate = false;

	DoorsSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/06_Hospital/DD_TT_Elevator_Doors_Open")));
	DoorsAttenuation = TSoftObjectPtr<USoundAttenuation>(WasamiAssets::Path(TEXT("/Game/DD/Audio/01_Hotel/01_Lobby_Attenuation")));
}

float AWasamiFakeUseElevator::EvaluateDoors(float Seconds)
{
	// The two keys of each transform track's x (0 and ±145), cubic with the flat tangents UE works out for the ends:
	// held before the first and after the last.
	const float T = FMath::Clamp((Seconds - DoorsStart) / (DoorsEnd - DoorsStart), 0.f, 1.f);
	return T * T * (3.f - 2.f * T);
}

void AWasamiFakeUseElevator::BeginPlay()
{
	Super::BeginPlay();
	USoundBase* Sound = DoorsSound.LoadSynchronous();
	USoundAttenuation* Attenuation = DoorsAttenuation.LoadSynchronous();
	LoadedAssets.Add(Sound);
	LoadedAssets.Add(Attenuation);
	Audio1->SetSound(Sound);
	Audio1->AttenuationSettings = Attenuation;
}

void AWasamiFakeUseElevator::UsedEvent()
{
	// Used Event: ActorSequence.SequencePlayer.Play(), then Audio1.Play(0) (the parent's is not called).
	SequencePosition = 0.f;
	bPlaying = true;
	ApplyDoors();
	SetActorTickEnabled(true);
	Audio1->Play(0.f);
}

void AWasamiFakeUseElevator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bPlaying)
	{
		SetActorTickEnabled(false);
		return;
	}
	// The sequence runs to its end and stops; its sections keep their state.
	SequencePosition = FMath::Min(SequencePosition + DeltaSeconds, SequenceLength);
	ApplyDoors();
	if (SequencePosition >= SequenceLength)
	{
		bPlaying = false;
		SetActorTickEnabled(false);
	}
}

void AWasamiFakeUseElevator::ApplyDoors()
{
	// The tracks set the whole relative transform: x on the curve, the rest at their keys (0, and a scale of 1).
	const float Offset = DoorTravel * EvaluateDoors(SequencePosition);
	StaticMesh1->SetRelativeTransform(FTransform(FVector(Offset, 0., 0.)));
	StaticMesh->SetRelativeTransform(FTransform(FVector(-Offset, 0., 0.)));
}
