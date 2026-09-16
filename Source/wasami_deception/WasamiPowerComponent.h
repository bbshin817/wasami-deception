#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "WasamiPowerTypes.h"
#include "WasamiPowerComponent.generated.h"

class AWasamiPlayerCharacter;
class UCameraShakeBase;
class USoundBase;
class UWasamiCameraAnim;
class AWasamiTeleportAim;
class AWasamiPrimalPower;
class AWasamiTelepathyPower;
class AWasamiVanishPower;
class UWasamiSpeedBoostWidget;
class UWasamiVanishWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWasamiPowerUsedSignature, EWasamiPower, Power);

/**
 * The tablet's powers, after Dark Deception's BP_DD_PlayerCharacter, BP_Powers and UMG_TabletPowers (pak_reference_2),
 * which each hold a part of it for the one player: the unlocked powers and the two sockets that point into them, Q / E
 * to use a socket and 1 / 2 to cycle it, each power's gauge on the tablet, the values of the upgrade level, the reset
 * on death, and the powers themselves (the speed boost, the teleport with its aim, AWasamiTeleportAim, the telepathy,
 * AWasamiTelepathyPower, Primal Fear, AWasamiPrimalPower, and Vanish, AWasamiVanishPower). The tablet's screen only
 * shows what this holds.
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

	/** A left click: confirms the teleport's aim, when there is one (the original's aim takes the click itself). */
	UFUNCTION(BlueprintCallable, Category = "Powers")
	void ConfirmTeleport();

	/** The mouse wheel (±1 a notch): moves the teleport's aim, when there is one. */
	UFUNCTION(BlueprintCallable, Category = "Powers")
	void AdjustTeleportDistance(float AxisValue);

	/** The teleport's aim while one is out (between Q / E and the move, or the take-back). */
	UFUNCTION(BlueprintPure, Category = "Powers")
	AWasamiTeleportAim* GetTeleportAim() const;

	/**
	 * Use Power: with Can Use Tablet? and Has Input, bounces the socket, then uses its power if it can be used and no
	 * power was used in the last 0.5 s. A power that cannot be used does nothing else and makes no sound, except that
	 * the side a teleport is aiming from takes the teleport back.
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

	/** Teleport_Mode_Entered: the teleport starts aiming. */
	UPROPERTY(EditAnywhere, Category = "Powers|Assets")
	TSoftObjectPtr<USoundBase> TeleportAimSound;

	/** BP_Power_Teleport: the aim the teleport spawns. */
	UPROPERTY(EditAnywhere, Category = "Powers|Assets")
	TSubclassOf<AWasamiTeleportAim> TeleportAimClass;

	/** Telepathy: the telepathy starts. */
	UPROPERTY(EditAnywhere, Category = "Powers|Assets")
	TSoftObjectPtr<USoundBase> TelepathySound;

	/** Teleport_Mode_Entered: the telepathy ends. */
	UPROPERTY(EditAnywhere, Category = "Powers|Assets")
	TSoftObjectPtr<USoundBase> TelepathyEndSound;

	/** BP_CameraShake_Streak: the telepathy starts. */
	UPROPERTY(EditAnywhere, Category = "Powers|Assets")
	TSoftClassPtr<UCameraShakeBase> TelepathyShakeClass;

	/** BP_Telepathy: what the telepathy spawns. */
	UPROPERTY(EditAnywhere, Category = "Powers|Assets")
	TSubclassOf<AWasamiTelepathyPower> TelepathyPowerClass;

	/** BP_PrimalPower: what Primal Fear spawns. */
	UPROPERTY(EditAnywhere, Category = "Powers|Assets")
	TSubclassOf<AWasamiPrimalPower> PrimalPowerClass;

	/** BP_VanishPower: what Vanish spawns. */
	UPROPERTY(EditAnywhere, Category = "Powers|Assets")
	TSubclassOf<AWasamiVanishPower> VanishPowerClass;

	/** UMG_Vanish: the purple vignette from a Vanish until the power is ready again. */
	UPROPERTY(EditAnywhere, Category = "Powers|Assets")
	TSubclassOf<UWasamiVanishWidget> VanishWidgetClass;

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

	void UseTeleport(bool bLeft);
	/** UsedTeleport: the aim's Used (and the end of a take-back): the side cycles again and the cooldown starts. */
	UFUNCTION()
	void UsedTeleport();
	void RefillTeleport();
	/** Reset Teleport with BP_Powers' Stop Teleport Timeline: a take-back, or the reset on death. */
	void ResetTeleport();

	void UseTelepathy();
	/** After the telepathy's time: its end sound, and the cooldown starts. */
	void EndTelepathy();
	/** The end of the cooldown (the Gate before it always lets it through). */
	void RefillTelepathy() { Refill(EWasamiPower::Telepathy); }

	void UsePrimal();
	/** 0.06 s after a use: the cooldown starts. */
	void StartPrimalCooldown();
	/** The end of the cooldown (the Gate before it always lets it through). */
	void RefillPrimal() { Refill(EWasamiPower::PrimalFear); }

	void UseVanish();
	/** 15 s after a use: the capsule blocks the camera channel again and the cooldown starts. */
	void EndVanish();
	/** The end of the cooldown (the Gate before it always lets it through), and Reset Vanish: the power is ready again
	 * and the widget goes. */
	void RefillVanish();

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

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedTeleportAimSound;

	/** What the teleport's aim uses, held from the start. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> LoadedTeleportAimAssets;

	/** The spawned BP_Power_Teleport (the original keeps the spawn's return value; a destroyed one stays in it). */
	UPROPERTY(Transient)
	TObjectPtr<AWasamiTeleportAim> TeleportAim;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedTelepathySound;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedTelepathyEndSound;

	UPROPERTY(Transient)
	TSubclassOf<UCameraShakeBase> LoadedTelepathyShake;

	/** What the telepathy's markers show, held from the start. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> LoadedTelepathyAssets;

	/** What Primal Fear's actor uses, held from the start. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> LoadedPrimalAssets;

	/** What Vanish's actor and widget use, held from the start. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> LoadedVanishAssets;

	/** Vanish Widget: the last UMG_Vanish made (it stays here after it is removed, as in the original). */
	UPROPERTY(Transient)
	TObjectPtr<UWasamiVanishWidget> VanishWidget;

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

	/** CurrentSide: the teleport was used from the left socket. */
	bool bTeleportLeft = false;
	/** The Gate before the teleport's cooldown delay: a use opens it, a take-back closes it. */
	bool bTeleportGateOpen = false;
	/** DoOnce_5 (its refill): starts closed and opens on a use. */
	bool bTeleportRefillOpen = false;
	FTimerHandle TeleportRefillTimer;

	FTimerHandle TelepathyEndTimer;
	FTimerHandle TelepathyRefillTimer;

	FTimerHandle PrimalCooldownTimer;
	FTimerHandle PrimalRefillTimer;

	FTimerHandle VanishEndTimer;
	FTimerHandle VanishRefillTimer;
};
