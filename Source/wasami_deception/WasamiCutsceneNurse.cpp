#include "WasamiCutsceneNurse.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "WasamiAssets.h"
#include "WasamiEnemy.h"

AWasamiCutsceneNurse::AWasamiCutsceneNurse()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	// 'SkeletalMesh' is what BP_06_Nurse_Cutscene calls its mesh, so Zone 1's bindings find it by that name.
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(DefaultSceneRoot);
	SkeletalMesh->SetRelativeScale3D(FVector(AWasamiEnemy::MeshScale));
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMesh->SetGenerateOverlapEvents(false);
	// The cut scenes animate it while the player looks at it from anywhere in the room.
	SkeletalMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	MeshAsset = TSoftObjectPtr<USkeletalMesh>(WasamiAssets::Path(TEXT("/Game/Wasami/Enemy/SK_WasamiEnemy")));
}

void AWasamiCutsceneNurse::SetUnderCapsule(bool bInUnderCapsule)
{
	bUnderCapsule = bInUnderCapsule;
	SkeletalMesh->SetRelativeLocationAndRotation(
		bUnderCapsule ? FVector(AWasamiEnemy::MeshX, AWasamiEnemy::MeshY, AWasamiEnemy::MeshZ) : FVector::ZeroVector,
		FRotator(0., bUnderCapsule ? AWasamiEnemy::MeshYaw : 0., 0.));
}

void AWasamiCutsceneNurse::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SkeletalMesh->SetSkeletalMeshAsset(MeshAsset.LoadSynchronous());
	SetUnderCapsule(bUnderCapsule);
}
