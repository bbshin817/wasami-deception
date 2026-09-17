#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "WasamiChameleonComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPostProcessComponent;

/**
 * The player's FX, after the Chameleon actor Dark Deception's BP_DD_PlayerCharacter carries as a child
 * (pak_reference_2's /Game/ThirdParty/Chameleon/Chameleon, a post-process effects pack): an unbound post-process
 * volume that, on every tick, holds a material instance for each of its effects that is switched on. The player only
 * ever switches on the Camera Shake, during a speed boost; the Radial Blur's width is written too, but its switch is
 * never set, so it is not made here.
 *
 * UPostProcessComponent cannot be derived from outside the engine (MinimalAPI), so this component makes one on the
 * owner at BeginPlay and fills it.
 */
UCLASS(ClassGroup = (Wasami), meta = (BlueprintSpawnableComponent))
class WASAMI_DECEPTION_API UWasamiChameleonComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWasamiChameleonComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Camera Shake: the picture shakes while it is on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon")
	bool bCameraShake = false;

	/** Camera Shake Power: how far the picture moves, as a part of the screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon")
	float CameraShakePower = 0.01f;

	/** Camera Shake Frequency. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chameleon")
	float CameraShakeFrequency = 10.f;

	/** M_CameraShake (the pack's graph is cooked away; the pipeline's M_DD_ChameleonCameraShake is estimated). */
	UPROPERTY(EditAnywhere, Category = "Chameleon|Assets")
	TSoftObjectPtr<UMaterialInterface> CameraShakeMaterial;

protected:
	virtual void BeginPlay() override;

private:
	/** InternalPP: the unbound volume the effects go into. */
	UPROPERTY(Transient)
	TObjectPtr<UPostProcessComponent> Volume;

	/** iCameraShake: the instance the effect's settings go into. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CameraShakeInstance;
};
