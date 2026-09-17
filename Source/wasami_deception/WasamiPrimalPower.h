#pragma once

#include "CoreMinimal.h"
#include "WasamiPowerBurst.h"
#include "WasamiPrimalPower.generated.h"

class UCameraShakeBase;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USoundBase;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Primal Fear, after Dark Deception's BP_PrimalPower (pak_reference_2). The power spawns it 50 m under the player with
 * the upgrade level's Range; it moves onto the player, sounds Stun_Wave_Attack_New_04, shakes the camera, stuns once
 * every enemy within Range (through walls), and over its 2 s timeline a red sphere around the spot swells to Range and
 * fades while the screen flashes white and red.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiPrimalPower : public AWasamiPowerBurst
{
	GENERATED_BODY()

public:
	AWasamiPrimalPower();

	/**
	 * The stun: every actor with a pawn-type body within Radius of Center that implements the enemy interface gets
	 * Set State(Stun, bByOrb false), once. Returns how many did.
	 */
	UFUNCTION(BlueprintCallable, Category = "Primal Fear", meta = (WorldContext = "WorldContextObject"))
	static int32 StunEnemies(const UObject* WorldContextObject, FVector Center, float Radius);

	/** Loads what the power uses into Out, so that a spawn waits on nothing. */
	static void LoadAssets(TArray<TObjectPtr<UObject>>& Out);

	/** The timeline's tracks. */
	static const FRichCurve& GrowthCurve();
	static const FRichCurve& DesaturationCurve();
	static const FRichCurve& OpacityCurve();
	static const FRichCurve& PrimalFadeCurve();

	UStaticMeshComponent* GetSphere() const { return Sphere; }
	UMaterialInstanceDynamic* GetMaterialInstance() const { return MaterialInstance; }

	/** Range (cm): the power sets it by the upgrade level before the spawn finishes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Primal Fear", meta = (ExposeOnSpawn = "true"))
	float Range = 1500.f;

	/** /Engine/BasicShapes/Sphere (50 cm across its radius). */
	UPROPERTY(EditAnywhere, Category = "Primal Fear|Assets")
	TSoftObjectPtr<UStaticMesh> SphereMesh;

	/** M_05_Primal: the sphere's red, with the Desaturation and Opacity the timeline drives (its graph is an estimate). */
	UPROPERTY(EditAnywhere, Category = "Primal Fear|Assets")
	TSoftObjectPtr<UMaterialInterface> SphereMaterial;

	/** Stun_Wave_Attack_New_04. */
	UPROPERTY(EditAnywhere, Category = "Primal Fear|Assets")
	TSoftObjectPtr<USoundBase> WaveSound;

	/** 01_Hotel_Lobby_ElevatorShakeStop, played at a scale of 25. */
	UPROPERTY(EditAnywhere, Category = "Primal Fear|Assets")
	TSoftClassPtr<UCameraShakeBase> ShakeClass;

protected:
	virtual void StartPower() override;
	virtual void UpdateTimeline(float Position) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Primal Fear")
	TObjectPtr<UStaticMeshComponent> Sphere;

private:
	/** Material Instance: the sphere's dynamic instance of M_05_Primal. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;
};
