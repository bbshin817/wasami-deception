#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "WasamiPowerTypes.h"
#include "WasamiPowerComponent.generated.h"

class AWasamiPlayerCharacter;
class UCameraShakeBase;
class USoundBase;
class UWasamiCameraAnim;
class UWasamiSpeedBoostWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWasamiPowerUsedSignature, EWasamiPower, Power);

/**
 * The tablet's powers, after Dark Deception's BP_DD_PlayerCharacter, BP_Powers and UMG_TabletPowers (pak_reference_2),
 * which each hold a part of it for the one player: the unlocked powers and the two sockets that point into them, Q / E
 * to use a socket and 1 / 2 to cycle it, each power's gauge on the tablet, the values of the upgrade level, the reset
 * on death, and the speed boost itself. The tablet's screen only shows what this holds.
 */
UCLASS(ClassGroup = (Wasami), meta = (BlueprintSpawnableComponent))
class WASAMI_DECEPTION_API UWasamiPowerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWasamiPowerComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Q / E (Use Power Left / Right): with Can Interact?, at most once every 0.5 s per key, uses that socket. */
	void UsePowerLeftPressed() { UsePowerKey(true); }
	void UsePowerRightPressed() { UsePowerKey(false); }

	/** 1 / 2 (Cycle Power Left / Right): only with the tablet up, the socket moves to the next unlocked power. */
	UFUNCTION(BlueprintCallable, Category = "Powers")
	void CyclePower(bool bLeft);
	void CyclePowerLeft() { CyclePower(true); }
	void CyclePowerRight() { CyclePower(false); }

	/**
	 * Use Power: with Can Use Tablet? and Has Input, bounces the socket, then uses its power if it can be used and no
	 * power was used in the last 0.5 s. A power that cannot be used does nothing else and makes no sound.
	 */
	UFUNCTION(BlueprintCallable, Category = "Powers")
	void UsePower(bool bLeft);

	/** BP_Powers' Reset All Powers, which the death screen calls before the respawn. */
	UFUNCTION(BlueprintCallable, Category = "Powers")
	void ResetPowers();

	/** The power a socket shows (None when the socket points past the unlocked powers). */
	UFUNCTION(BlueprintPure, Category = "Powers")
	EWasamiPower GetSocketPower(bool bLeft) const;

	/** The power's icon `Percent`: 1 when full; it empties while the power lasts and fills over its cooldown. */
	UFUNCTION(BlueprintPure, Category = "Powers")
	float GetGaugePercent(EWasamiPower Power) const;

	UFUNCTION(BlueprintPure, Category = "Powers")
	bool IsPowerAvailable(EWasamiPower Power) const;

	/** BP_DD_Functions' Is Player Using Power ?: whether the power is in Active Powers. */
	UFUNCTION(BlueprintPure, Category = "Powers")
	bool IsUsingPower(EWasamiPower Power) const { return ActivePowers.Contains(Power); }

	/** Whether any power is unlocked (the tablet hides its sockets otherwise). */
	UFUNCTION(BlueprintPure, Category = "Powers")
	bool HasPowers() const { return Powers.Num() > 0; }

	/** Get Power Upgrade Level: the same level for every power here. */
	UFUNCTION(BlueprintPure, Category = "Powers")
	int32 GetUpgradeLevel(EWasamiPower Power) const { return FMath::Clamp(UpgradeLevel, 0, FWasamiPowerTuning::MaxLevel); }

	/** The values of the power's upgrade level. */
	const FWasamiPowerTuning& GetTuning(EWasamiPower Power) const { return FWasamiPowerTuning::ForLevel(GetUpgradeLevel(Power)); }

	/** UsedPower: a power is being used (after the checks, before its own work). */
	UPROPERTY(BlueprintAssignable, Category = "Powers")
	FWasamiPowerUsedSignature OnPowerUsed;

	/** The unlocked powers in their order (the original removes the locked ones from all six; all are unlocked here). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Powers")
	TArray<EWasamiPower> UnlockedPowers;

	/** The upgrade level of every power (the original saves one per power; the user fixed them at the top, 5). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Powers", meta = (ClampMin = "0", ClampMax = "5"))
	int32 UpgradeLevel = FWasamiPowerTuning::MaxLevel;

	/** power_refilled: a power can be used again. */
	UPROPERTY(EditAnywhere, Category = "Powers|Assets")
	TSoftObjectPtr<USoundBase> RefillSound;

	/** UI_Select_V3: a socket cycles. */
	UPROPERTY(EditAnywhere, Category = "Powers|Assets")
	TSoftObjectPtr<USoundBase> CycleSound;

	/** Shard_Streak_Milestone_V5: the speed boost starts. */
	UPROPERTY(EditAnywhere, Category = "Powers|Assets")
	TSoftObjectPtr<USoundBase> BoostSound;

	/** BP_CameraShake_Streak: the speed boost starts. */
	UPROPERTY(EditAnywhere, Category = "Powers|Assets")
	TSoftClassPtr<UCameraShakeBase> BoostShakeClass;

	/** CameraAnim_SpeedBoost: the view turns red while the speed boost lasts. */
	UPROPERTY(EditAnywhere, Category = "Powers|Assets")
	TSoftObjectPtr<UWasamiCameraAnim> BoostCameraAnim;

	/** UMG_SpeedBoost: the speed lines and the vignette while the speed boost lasts. */
	UPROPERTY(EditAnywhere, Category = "Powers|Assets")
	TSubclassOf<UWasamiSpeedBoostWidget> BoostWidgetClass;

protected:
	virtual void BeginPlay() override;

private:
	AWasamiPlayerCharacter* GetPlayer() const;
	void UsePowerKey(bool bLeft);
	void ReopenLeftKey() { bLeftKeyClosed = false; }
	void ReopenRightKey() { bRightKeyClosed = false; }
	void ReopenUse() { bUseClosed = false; }

	/** A Blueprint Delay: counts down and calls Callback, unless that delay is already counting (then nothing). */
	void Delay(FTimerHandle& Handle, float Seconds, void (UWasamiPowerComponent::*Callback)());

	/** Array_Find(Power, {P, !bAvailable}) and Array_Set on it with bAvailable. */
	void SetPowerAvailable(EWasamiPower Power, bool bAvailable);
	/** The end of a cooldown: power_refilled, and the power can be used again. */
	void Refill(EWasamiPower Power);
	FWasamiPowerGauge& Gauge(EWasamiPower Power) { return Gauges[static_cast<int32>(Power)]; }

	void UseSpeedBoost();
	void EndSpeedBoost();
	void RefillSpeedBoost();
	/** Sprinting Effects: the FX's camera shake follows the speed. */
	void UpdateSprintingEffects();

	UPROPERTY(Transient)
	TArray<FWasamiPowerSlot> Powers;

	/** Active Powers: the powers taking effect right now. */
	UPROPERTY(Transient)
	TArray<EWasamiPower> ActivePowers;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedRefillSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedCycleSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedBoostSound;

	UPROPERTY(Transient)
	TSubclassOf<UCameraShakeBase> LoadedBoostShake;

	UPROPERTY(Transient)
	TObjectPtr<UWasamiCameraAnim> LoadedBoostCameraAnim;

	/** What the boost's widget shows, held from the start. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> LoadedBoostWidgetAssets;

	/** The UMG_SpeedBoost on the screen (the original's CallFunc_Create_ReturnValue). */
	UPROPERTY(Transient)
	TObjectPtr<UWasamiSpeedBoostWidget> BoostWidget;

	FWasamiPowerGauge Gauges[WasamiPowerCount];
	int32 LeftIndex = 0;
	int32 RightIndex = 0;
	/** Can Cycle Left? / Right?: false on the side a teleport is aiming from. */
	bool bCanCycleLeft = true;
	bool bCanCycleRight = true;

	/** The DoOnce nodes behind Q, E and Use Power, and the delays that open them again. */
	bool bLeftKeyClosed = false;
	bool bRightKeyClosed = false;
	bool bUseClosed = false;
	FTimerHandle LeftKeyTimer;
	FTimerHandle RightKeyTimer;
	FTimerHandle UseTimer;

	/** The speed boost's DoOnce_3 (its end) and DoOnce_4 (its refill): both start closed and open on a use. */
	bool bBoostEndOpen = false;
	bool bBoostRefillOpen = false;
	FTimerHandle BoostEndTimer;
	FTimerHandle BoostRefillTimer;
	FTimerHandle SprintingEffectsTimer;
	/** CallFunc_PlayCameraAnim_ReturnValue: the boost's camera anim, which the reset stops. */
	int32 BoostCameraAnimHandle = 0;
};
