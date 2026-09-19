#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiSpecialSpawnPoint.generated.h"

class UMaterialBillboardComponent;
class UMaterialInterface;

/**
 * A place a power orb or a bonus shard can move to, after Dark Deception's BP_PowerOrbSpawnPoint and
 * BP_BonusShardSpawnPoint (pak_reference_2): nothing but a 32 cm material billboard, hidden in game, as the root. The
 * pickup takes the point's actor location.
 */
UCLASS(Abstract)
class WASAMI_DECEPTION_API AWasamiSpecialSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AWasamiSpecialSpawnPoint();

	virtual void OnConstruction(const FTransform& Transform) override;

	UMaterialBillboardComponent* GetBillboard() const { return MaterialBillboard; }

	/** The billboard's material. */
	UPROPERTY(EditAnywhere, Category = "Spawn Point|Assets")
	TSoftObjectPtr<UMaterialInterface> BillboardMaterial;

	/** The billboard's size (cm, in the world). */
	static constexpr float BillboardSize = 32.f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawn Point")
	TObjectPtr<UMaterialBillboardComponent> MaterialBillboard;
};

/** BP_PowerOrbSpawnPoint: its billboard is the orb's crystal, m_crystal_Inst3. */
UCLASS()
class WASAMI_DECEPTION_API AWasamiPowerOrbSpawnPoint : public AWasamiSpecialSpawnPoint
{
	GENERATED_BODY()

public:
	AWasamiPowerOrbSpawnPoint();
};

/** BP_BonusShardSpawnPoint: its billboard is the engine's VertexColorViewMode_RedOnly. */
UCLASS()
class WASAMI_DECEPTION_API AWasamiBonusShardSpawnPoint : public AWasamiSpecialSpawnPoint
{
	GENERATED_BODY()

public:
	AWasamiBonusShardSpawnPoint();
};
