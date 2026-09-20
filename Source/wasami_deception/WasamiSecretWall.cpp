#include "WasamiSecretWall.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiVoice.h"

namespace
{
	// BP_03_SecretWall1's StaticMesh_GEN_VARIABLE (pak_reference_2): at 100, tagged interact, not generating overlaps;
	// UStaticMeshComponent's BlockAllDynamic otherwise. BP_07_Zone1_SecretWall takes it out of the navigation (its
	// material, M_07_TP_Stonewall_01, is the level's to set, as the placed wall's own).
	constexpr double SecretWallMeshScale = 100.;
	const FName SecretWallInteractTag(TEXT("interact"));
}

AWasamiSecretWall::AWasamiSecretWall()
{
	// Move Up, a timeline played once.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	DefaultSceneRoot->SetMobility(EComponentMobility::Movable);
	RootComponent = DefaultSceneRoot;

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(DefaultSceneRoot);
	StaticMesh->SetMobility(EComponentMobility::Movable);
	StaticMesh->SetRelativeScale3D(FVector(SecretWallMeshScale));
	StaticMesh->SetGenerateOverlapEvents(false);
	StaticMesh->SetCanEverAffectNavigation(false);
	StaticMesh->ComponentTags.Add(SecretWallInteractTag);

	SlideSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/02_School/Sliding_Wall")));
	SlideAttenuation = TSoftObjectPtr<USoundAttenuation>(WasamiAssets::Path(TEXT("/Game/DD/Audio/01_Hotel/01_Lobby_Attenuation")));
}

float AWasamiSecretWall::EvaluateMoveUp(float Seconds)
{
	// CurveFloat_0: (0, 0) and (3, 1), linear; held at 1 after its last key.
	return FMath::Clamp(Seconds / MoveUpCurveLength, 0.f, 1.f);
}

void AWasamiSecretWall::BeginPlay()
{
	Super::BeginPlay();
	LoadedAssets.Add(SlideSound.LoadSynchronous());
	LoadedAssets.Add(SlideAttenuation.LoadSynchronous());

	// ReceiveBeginPlay: OG Height = the actor's z; InterpHeight = Height + the actor's z.
	OGHeight = static_cast<float>(GetActorLocation().Z);
	InterpHeight = Height + OGHeight;
}

void AWasamiSecretWall::InteractWithObject_Implementation(AActor* Interactee)
{
	// InteractWithObject's DoOnce.
	if (bUsed)
	{
		return;
	}
	bUsed = true;

	// Array_Clear(StaticMesh.ComponentTags) (twice); PlaySoundAtLocation(Sliding_Wall, the actor, 0.65, 1, 0,
	// 01_Lobby_Attenuation); Move Up.SetPlayRate(0.7) and PlayFromStart.
	StaticMesh->ComponentTags.Empty();
	UGameplayStatics::PlaySoundAtLocation(this, SlideSound.LoadSynchronous(), GetActorLocation(), FRotator::ZeroRotator,
		SlideVolume, SlidePitch, 0.f, SlideAttenuation.LoadSynchronous());
	MoveUpPosition = 0.f;
	bMoving = true;
	ApplyMoveUp();
	SetActorTickEnabled(true);

	// Not the original's: the WebGL version's best, half a second after a secret door opens (its record 06). The
	// original says nothing here; its Bierce speaks over the secret files instead (BP_Collectable, record 18).
	GetWorldTimerManager().SetTimer(VoiceTimer, this, &AWasamiSecretWall::SayBest, VoiceDelay, false);
}

void AWasamiSecretWall::SayBest()
{
	WasamiVoice::Say(this, EWasamiVoice::Best);
}

void AWasamiSecretWall::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bMoving)
	{
		SetActorTickEnabled(false);
		return;
	}
	// The timeline runs to its length and stops there (its Finished does nothing).
	MoveUpPosition = FMath::Min(MoveUpPosition + DeltaSeconds * MoveUpPlayRate, MoveUpLength);
	ApplyMoveUp();
	if (MoveUpPosition >= MoveUpLength)
	{
		bMoving = false;
		SetActorTickEnabled(false);
	}
}

void AWasamiSecretWall::ApplyMoveUp()
{
	// Move Up__UpdateFunc: SetActorLocation(x, y, Lerp(OG Height, InterpHeight, Alpha)), not swept.
	FVector Location = GetActorLocation();
	Location.Z = FMath::Lerp(OGHeight, InterpHeight, EvaluateMoveUp(MoveUpPosition));
	SetActorLocation(Location);
}
