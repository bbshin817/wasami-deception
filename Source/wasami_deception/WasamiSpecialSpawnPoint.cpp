#include "WasamiSpecialSpawnPoint.h"

#include "Components/MaterialBillboardComponent.h"
#include "Materials/MaterialInterface.h"
#include "WasamiAssets.h"

AWasamiSpecialSpawnPoint::AWasamiSpecialSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	MaterialBillboard = CreateDefaultSubobject<UMaterialBillboardComponent>(TEXT("MaterialBillboard"));
	MaterialBillboard->SetHiddenInGame(true);
	RootComponent = MaterialBillboard;
}

void AWasamiSpecialSpawnPoint::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// One element in world space, without the distance curves.
	MaterialBillboard->SetElements({});
	MaterialBillboard->AddElement(BillboardMaterial.LoadSynchronous(), nullptr, false, BillboardSize, BillboardSize, nullptr);
}

AWasamiPowerOrbSpawnPoint::AWasamiPowerOrbSpawnPoint()
{
	BillboardMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/Fords_Materials/m_crystal_Inst3")));
}

AWasamiBonusShardSpawnPoint::AWasamiBonusShardSpawnPoint()
{
	BillboardMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Engine/EngineDebugMaterials/VertexColorViewMode_RedOnly")));
}
