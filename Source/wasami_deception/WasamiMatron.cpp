#include "WasamiMatron.h"

#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiBossAnimInstance.h"
#include "WasamiEnemySentry.h"

AWasamiMatron::AWasamiMatron()
{
	Tags.Add(TEXT("Enemy"));

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	// SkeletalMesh: the original's SK_Matron at 5 with Matron_MiniBoss_AnimBP. It faces +Y, as SK_WasamiBoss does.
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(DefaultSceneRoot);
	SkeletalMesh->SetRelativeScale3D(FVector(MeshScale));
	SkeletalMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SkeletalMesh->SetCanEverAffectNavigation(false);
	SkeletalMesh->AnimClass = UWasamiBossAnimInstance::StaticClass();

	// CloseArea: Custom, overlapping Pawn and ignoring the rest (its AreaClass, NavArea_Obstacle, is the shape's default,
	// and an overlapping box is not on the navigation).
	CloseArea = CreateDefaultSubobject<UBoxComponent>(TEXT("CloseArea"));
	CloseArea->SetupAttachment(DefaultSceneRoot);
	CloseArea->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	CloseArea->SetCollisionResponseToAllChannels(ECR_Ignore);
	CloseArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CloseArea->SetCanEverAffectNavigation(false);

	MeshAsset = TSoftObjectPtr<USkeletalMesh>(WasamiAssets::Path(TEXT("/Game/Wasami/Boss/SK_WasamiBoss")));
}

void AWasamiMatron::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SkeletalMesh->SetSkeletalMeshAsset(MeshAsset.LoadSynchronous());
}

void AWasamiMatron::Activate()
{
	GetWorldTimerManager().SetTimer(SwitchTimer, this, &AWasamiMatron::Switch, SwitchRate, true);
	for (AWasamiViewcone* Cone : {LongCone.Get(), ShortCone.Get()})
	{
		if (IsValid(Cone))
		{
			Cone->SetOwner(this);
		}
	}
	for (AWasamiViewcone* Cone : {LongCone.Get(), ShortCone.Get()})
	{
		if (IsValid(Cone))
		{
			Cone->Initialize();
		}
	}
}

bool AWasamiMatron::IsActivated() const
{
	return GetWorldTimerManager().IsTimerActive(SwitchTimer);
}

void AWasamiMatron::Switch()
{
	const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	bMode = !CloseArea->IsOverlappingActor(Player);
	// Once her cones are destroyed the original's calls on them do nothing.
	AWasamiViewcone* On = bMode ? LongCone.Get() : ShortCone.Get();
	AWasamiViewcone* Off = bMode ? ShortCone.Get() : LongCone.Get();
	bool& bTurnClosed = bMode ? bLongTurnClosed : bShortTurnClosed;
	if (bTurnClosed)
	{
		return;
	}
	bTurnClosed = true;
	if (IsValid(On))
	{
		On->TurnOn();
	}
	if (IsValid(Off))
	{
		Off->TurnOff();
	}
	(bMode ? bShortTurnClosed : bLongTurnClosed) = false;
}

void AWasamiMatron::PlayerSpotted_Implementation()
{
	if (bSpottedClosed)
	{
		return;
	}
	bSpottedClosed = true;
	for (AWasamiViewcone* Cone : {LongCone.Get(), ShortCone.Get()})
	{
		if (IsValid(Cone))
		{
			Cone->Destroy();
		}
	}
	// PlayMontage (the Detected montage at rate 1 from 0): only its Notify Begin is bound.
	if (UWasamiBossAnimInstance* Anim = Cast<UWasamiBossAnimInstance>(SkeletalMesh->GetAnimInstance()))
	{
		Anim->PlayDetected();
	}
	GetWorldTimerManager().SetTimer(SentryTimer, this, &AWasamiMatron::CallSentries, ReinforcementTime, false);
	bSpotted = true;
}

void AWasamiMatron::CallSentries()
{
	for (TActorIterator<AWasamiEnemySentry> It(GetWorld()); It; ++It)
	{
		IWasamiViewconeInterface::Execute_PlayerSpotted(*It);
	}
}
