#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiTeleportAim.generated.h"

class AWasamiPlayerCharacter;
class UAudioComponent;
class UCameraShakeBase;
class UDecalComponent;
class UMaterialInterface;
class UParticleSystem;
class UParticleSystemComponent;
class USoundBase;
class USpringArmComponent;
class UWasamiCameraAnim;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiTeleportUsedSignature);

/**
 * The teleport's aim, after Dark Deception's BP_Power_Teleport (pak_reference, the version the user chose for the
 * teleport). The power spawns it 50 m under the player; every tick it traces 500 cm straight down from Distance in
 * front of the player for the Teleport object channel (the levels' teleport zones) and moves its spring arm onto the
 * hit, and the decal on the arm (with the slashes and sparks over it) trails after it once the arm's lag comes on.
 * The mouse wheel sets the distance and a
 * left click confirms, playing CameraAnim_Teleport: 0.12 s later the player is swept to the decal's spot, and the aim
 * reports Used and goes away.
 * The player forwards the wheel and the click (the original's actor takes them itself, without consuming them).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiTeleportAim : public AActor
{
	GENERATED_BODY()

public:
	AWasamiTeleportAim();

	virtual void Tick(float DeltaSeconds) override;

	/** The mouse wheel (MouseWheelAxis, ±1 a notch): Alpha moves by a tenth of it and Distance follows. */
	UFUNCTION(BlueprintCallable, Category = "Teleport")
	void AdjustDistance(float AxisValue);

	/**
	 * A left click: the player's destination becomes the decal's spot raised 125 cm, and the first click starts the
	 * move (a second click before the move only changes where it goes).
	 */
	UFUNCTION(BlueprintCallable, Category = "Teleport")
	void Confirm();

	/** Distance for an Alpha: Lerp(250, MaxDistance, Alpha). */
	static float DistanceFor(float Alpha, float MaxDistance);

	/** Alpha after a wheel's AxisValue: Clamp(Alpha + AxisValue / 10, 0, 1). */
	static float StepAlpha(float Alpha, float AxisValue);

	/** Loads what the aim uses into Out, so that a spawn waits on nothing. */
	static void LoadAssets(TArray<TObjectPtr<UObject>>& Out);

	USpringArmComponent* GetSpringArm() const { return SpringArm; }
	UDecalComponent* GetDecal() const { return Decal; }
	UParticleSystemComponent* GetParticleSystem() const { return ParticleSystem; }

	/** Used: the player has been moved; the aim is destroyed right after. */
	UPROPERTY(BlueprintAssignable, Category = "Teleport")
	FWasamiTeleportUsedSignature OnUsed;

	/** Max Distance (cm): the power sets it by the upgrade level before the spawn finishes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport", meta = (ExposeOnSpawn = "true"))
	float MaxDistance = 1000.f;

	/** How far in front of the player the trace starts (cm). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Teleport")
	float Distance = 1000.f;

	/** Where Distance lies between 250 and MaxDistance; every aim starts at 0.6. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Teleport")
	float Alpha = 0.6f;

	/** Where the player's capsule is put (set by the clicks). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Teleport")
	FVector Location = FVector::ZeroVector;

	/** DD_LVL2_07_Teleport_Aiming_Loop_1227: loops while the aim lasts. */
	UPROPERTY(EditAnywhere, Category = "Teleport|Assets")
	TSoftObjectPtr<USoundBase> AimingLoopSound;

	/** Teleport_Committed: the move. */
	UPROPERTY(EditAnywhere, Category = "Teleport|Assets")
	TSoftObjectPtr<USoundBase> CommittedSound;

	/** BP_CameraShake_Streak: the move. */
	UPROPERTY(EditAnywhere, Category = "Teleport|Assets")
	TSoftClassPtr<UCameraShakeBase> CommittedShakeClass;

	/** CameraAnim_Teleport (pak_reference): the click; the view widens, flashes white and red, and settles in 0.5 s. */
	UPROPERTY(EditAnywhere, Category = "Teleport|Assets")
	TSoftObjectPtr<UWasamiCameraAnim> ConfirmCameraAnim;

	/** M_Decal_Teleport: the mark's glow (its graph is an estimate: the cook dropped the original's). */
	UPROPERTY(EditAnywhere, Category = "Teleport|Assets")
	TSoftObjectPtr<UMaterialInterface> DecalMaterial;

	/** P_ky_cutter2: the turning slashes and the rising red sparks over the mark. */
	UPROPERTY(EditAnywhere, Category = "Teleport|Assets")
	TSoftObjectPtr<UParticleSystem> AimParticles;

protected:
	virtual void BeginPlay() override;
	/** A destroyed aim's pending delays go with it (a take-back before the move cancels the move). */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport")
	TObjectPtr<USpringArmComponent> SpringArm;

	/** The mark on the floor, on the arm's end. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport")
	TObjectPtr<UDecalComponent> Decal;

	/** The slashes and sparks, on the decal; it starts with the aim (auto-activated as the template is set). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport")
	TObjectPtr<UParticleSystemComponent> ParticleSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport")
	TObjectPtr<UAudioComponent> Audio;

private:
	/** The player the aim works for (the original's GetPlayerCharacter(0)). */
	AWasamiPlayerCharacter* GetPlayer() const;
	void EnableLag();
	void Commit();

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedCommittedSound;

	UPROPERTY(Transient)
	TSubclassOf<UCameraShakeBase> LoadedCommittedShake;

	UPROPERTY(Transient)
	TObjectPtr<UWasamiCameraAnim> LoadedConfirmCameraAnim;

	FTimerHandle LagTimer;
	FTimerHandle CommitTimer;
	/** The DoOnce around the move. */
	bool bConfirmed = false;
};
