#include "WasamiCollectable.h"

#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/RichCurve.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiCollectablesWidget.h"
#include "WasamiGameMode.h"
#include "WasamiSaveGame.h"

namespace
{
	// BP_Collectable's SCS templates (pak_reference_2). The box keeps UBoxComponent's 32 cm extent and is sized by its
	// scale; like the other overlap boxes it is out of the navigation (its AreaClass, NavArea_Obstacle, blocks nothing).
	const FVector CollectableBoxLocation(0., 0., 36.43724060058594);
	const FVector CollectableBoxScale(1.2296168804168701, 1.2997432947158813, 1.);
	const FVector CollectableMeshLocation(0., 0., 36.43724060058594);
	constexpr double CollectableMeshScale = 60.;

	// PointLight_GEN_VARIABLE (UE 4.24's point light otherwise; its intensity is unitless).
	const FVector CollectableLightLocation(0., 0., 35.76698303222656);
	constexpr float CollectableLightIntensity = 1000.f;
	constexpr float CollectableLightAttenuationRadius = 250.f;
	constexpr float CollectableLightSoftSourceRadius = 2000.f;

	// Bounce__UpdateFunc: the mesh's relative location MakeVector(0, 0, Lerp(35, 40, v)) and rotation
	// MakeRotator(0, 0, Lerp(0, 7, v)) (roll, pitch, yaw: the yaw).
	constexpr float BounceLowHeight = 35.f;
	constexpr float BounceHighHeight = 40.f;
	constexpr float BounceMaxYaw = 7.f;

	/** Bounce's NewTrack_0 (CurveFloat_0): 0, 1, 0 at 0, 2.5 and 5 s, cubic, with the flat tangents UE worked out. */
	FRichCurve MakeCollectableBounceCurve()
	{
		FRichCurve Curve;
		for (const FVector2f& Each : {FVector2f(0.f, 0.f), FVector2f(2.5f, 1.f), FVector2f(5.f, 0.f)})
		{
			FRichCurveKey& Key = Curve.GetKey(Curve.AddKey(Each.X, Each.Y));
			Key.InterpMode = RCIM_Cubic;
			Key.TangentMode = RCTM_Break;
			Key.ArriveTangent = 0.f;
			Key.LeaveTangent = 0.f;
		}
		return Curve;
	}
}

AWasamiCollectable::AWasamiCollectable()
{
	// Bounce, a timeline that loops for as long as the file is there.
	PrimaryActorTick.bCanEverTick = true;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	DefaultSceneRoot->SetMobility(EComponentMobility::Movable);
	RootComponent = DefaultSceneRoot;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetupAttachment(DefaultSceneRoot);
	Box->SetRelativeLocation(CollectableBoxLocation);
	Box->SetRelativeScale3D(CollectableBoxScale);
	Box->SetCanEverAffectNavigation(false);
	Box->OnComponentBeginOverlap.AddDynamic(this, &AWasamiCollectable::OnBoxBeginOverlap);

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(DefaultSceneRoot);
	StaticMesh->SetMobility(EComponentMobility::Movable);
	StaticMesh->SetRelativeLocation(CollectableMeshLocation);
	StaticMesh->SetRelativeScale3D(FVector(CollectableMeshScale));
	StaticMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	StaticMesh->SetCanEverAffectNavigation(false);

	PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	PointLight->SetupAttachment(DefaultSceneRoot);
	PointLight->SetRelativeLocation(CollectableLightLocation);
	PointLight->SetMobility(EComponentMobility::Movable);
	PointLight->IntensityUnits = ELightUnits::Unitless;
	PointLight->Intensity = CollectableLightIntensity;
	PointLight->AttenuationRadius = CollectableLightAttenuationRadius;
	PointLight->SoftSourceRadius = CollectableLightSoftSourceRadius;
	PointLight->CastShadows = false;
	PointLight->VolumetricScatteringIntensity = 0.f;

	PickupSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/SharedGameplay/Bierce_Secret_Files_Pickup")));
	FileMesh = TSoftObjectPtr<UStaticMesh>(WasamiAssets::Path(TEXT("/Game/DD/Meshes/Shared/secret_file")));
}

void AWasamiCollectable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	StaticMesh->SetStaticMesh(FileMesh.LoadSynchronous());
}

float AWasamiCollectable::EvaluateBounce(float Seconds)
{
	static const FRichCurve Curve = MakeCollectableBounceCurve();
	return Curve.Eval(FMath::Clamp(Seconds, 0.f, BounceLength));
}

float AWasamiCollectable::BounceHeight(float Value)
{
	return FMath::Lerp(BounceLowHeight, BounceHighHeight, Value);
}

float AWasamiCollectable::BounceYaw(float Value)
{
	return FMath::Lerp(0.f, BounceMaxYaw, Value);
}

void AWasamiCollectable::BeginPlay()
{
	Super::BeginPlay();
	LoadedAssets.Add(PickupSound.LoadSynchronous());
	UWasamiCollectablesWidget::LoadAssets(LoadedAssets);

	// ReceiveBeginPlay: Bounce.SetPlayRate(3) and Play, then Delay(0.2) and the save's check.
	BouncePosition = 0.f;
	GetWorldTimerManager().SetTimer(SaveCheckTimer, this, &AWasamiCollectable::CheckSave, SaveCheckDelay, false);
}

void AWasamiCollectable::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// The looping timeline wraps its position round its length.
	BouncePosition = FMath::Fmod(BouncePosition + DeltaSeconds * BouncePlayRate, BounceLength);
	ApplyBounce();
}

void AWasamiCollectable::ApplyBounce()
{
	const float Value = EvaluateBounce(BouncePosition);
	StaticMesh->SetRelativeLocationAndRotation(FVector(0., 0., BounceHeight(Value)), FRotator(0., BounceYaw(Value), 0.));
}

void AWasamiCollectable::CheckSave()
{
	// Array_Contains(the game mode's Struct Save's entry for the level's Secrets, ID).
	const AWasamiGameMode* Mode = GetWorld()->GetAuthGameMode<AWasamiGameMode>();
	const UWasamiSaveGame* Save = Mode ? Mode->GetSave() : nullptr;
	if (Save && Save->Hospital.Secrets.Contains(ID))
	{
		Destroy();
	}
}

void AWasamiCollectable::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor == UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		Collect();
	}
}

void AWasamiCollectable::Collect()
{
	// Collect's DoOnce.
	if (bTaken)
	{
		return;
	}
	bTaken = true;

	// CreateSound2D(Sound 2D, 0.85, 1, 0, None, False, True).Play(0): a world's sound, so it outlives the file.
	if (USoundBase* Sound = PickupSound.LoadSynchronous())
	{
		if (UAudioComponent* Voice = UGameplayStatics::CreateSound2D(this, Sound, PickupVolume, PickupPitch, 0.f, nullptr, false, true))
		{
			Voice->Play(0.f);
		}
	}
	// Create(Self, UMG_Collectables_C, None).AddToPlayerScreen(0).
	UWasamiCollectablesWidget::Show(this);
	Unlock();
	Destroy();
}

void AWasamiCollectable::Unlock()
{
	AWasamiGameMode* Mode = GetWorld()->GetAuthGameMode<AWasamiGameMode>();
	UWasamiSaveGame* Held = Mode ? Mode->GetSave() : nullptr;
	const FString& Slot = Mode ? Mode->SaveSlotName : UWasamiSaveGame::SlotName;
	// ForEachLoop over Collectables: LoadGameFromSlot('SaveSlot') cast to BP_DD_SaveGame (failing: the loop's next),
	// the SwitchEnum on Type (UWasamiSaveGame::Unlock) and SaveGameToSlot after Art Gallery's and Sound's. The loop's
	// end writes the last one read again, as it is, and is left out.
	for (const FWasamiCollectableEntry& Entry : Collectables)
	{
		UWasamiSaveGame* Stored = Cast<UWasamiSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, UWasamiSaveGame::UserIndex));
		if (!Stored)
		{
			continue;
		}
		if (Stored->Unlock(Entry))
		{
			UGameplayStatics::SaveGameToSlot(Stored, Slot, UWasamiSaveGame::UserIndex);
		}
		if (Held)
		{
			Held->Unlock(Entry);
		}
	}
	// Array_AddUnique(the Struct Save's entry for the level's Secrets, ID).
	if (Held)
	{
		Held->Hospital.Secrets.AddUnique(ID);
	}
}
