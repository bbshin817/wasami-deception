#pragma once

#include "CoreMinimal.h"
#include "WasamiPowerBurst.h"
#include "WasamiSphereBurst.generated.h"

class UCameraShakeBase;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USoundBase;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * What Dark Deception's BP_PrimalPower and BP_StunCollectEffect (pak_reference_2) share over the burst's frame: a
 * sphere with a dynamic instance of M_05_Primal. On BeginPlay the actor moves onto the player, sounds
 * Stun_Wave_Attack_New_04 at the origin and shakes the camera with 01_Hotel_Lobby_ElevatorShakeStop at 25; over the
 * 2 s timeline the sphere's radius goes to Range × float while the material's Desaturation and Opacity follow their
 * tracks. The subclasses set Range, the tracks, the wave's pitch and the sphere's colour in their constructors.
 */
UCLASS(Abstract)
class WASAMI_DECEPTION_API AWasamiSphereBurst : public AWasamiPowerBurst
{
	GENERATED_BODY()

public:
	AWasamiSphereBurst();

	/** Loads what the class's defaults use into Out, so that a spawn waits on nothing. */
	void LoadDefaultAssets(TArray<TObjectPtr<UObject>>& Out) const;

	UStaticMeshComponent* GetSphere() const { return Sphere; }
	UMaterialInstanceDynamic* GetMaterialInstance() const { return MaterialInstance; }

	/** Range (cm): the sphere's radius at a float of 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sphere Burst", meta = (ExposeOnSpawn = "true"))
	float Range = 1500.f;

	/** /Engine/BasicShapes/Sphere (50 cm across its radius). */
	UPROPERTY(EditAnywhere, Category = "Sphere Burst|Assets")
	TSoftObjectPtr<UStaticMesh> SphereMesh;

	/** M_05_Primal: the sphere's red, with the Desaturation and Opacity the timeline drives (its graph is an estimate). */
	UPROPERTY(EditAnywhere, Category = "Sphere Burst|Assets")
	TSoftObjectPtr<UMaterialInterface> SphereMaterial;

	/** Stun_Wave_Attack_New_04. */
	UPROPERTY(EditAnywhere, Category = "Sphere Burst|Assets")
	TSoftObjectPtr<USoundBase> WaveSound;

	/** 01_Hotel_Lobby_ElevatorShakeStop, played at a scale of 25. */
	UPROPERTY(EditAnywhere, Category = "Sphere Burst|Assets")
	TSoftClassPtr<UCameraShakeBase> ShakeClass;

	/** PlaySoundAtLocation's pitch for the wave. */
	float GetWavePitch() const { return WavePitch; }

	/** The Color the material instance is given, if the class sets one (M_05_Primal's own is red). */
	const TOptional<FLinearColor>& GetSphereColor() const { return SphereColor; }

protected:
	/** The sphere's mesh and instance, the move onto the player, the wave and the shake. */
	virtual void StartPower() override;

	/** The sphere's scale, the volumes' weights and the material's parameters. */
	virtual void UpdateTimeline(float Position) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sphere Burst")
	TObjectPtr<UStaticMeshComponent> Sphere;

	/** The timeline's float (the sphere's radius over Range), desaturation and opacity tracks. */
	FRichCurve GrowthTrack;
	FRichCurve DesaturationTrack;
	FRichCurve OpacityTrack;

	float WavePitch = 1.f;
	TOptional<FLinearColor> SphereColor;

private:
	/** Material Instance: the sphere's dynamic instance of M_05_Primal. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;
};
