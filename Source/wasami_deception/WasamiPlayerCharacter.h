#pragma once

#include "CoreMinimal.h"
#include "Curves/RichCurve.h"
#include "GameFramework/Character.h"
#include "WasamiPlayerCharacter.generated.h"

class AWasamiGameMode;
class UCameraComponent;
class UCameraShakeBase;
class UInputAction;
class UInputMappingContext;
class USceneCaptureComponent2D;
class USoundBase;
class USpringArmComponent;
class UStaticMeshComponent;
class UWidgetComponent;
struct FInputActionValue;

/**
 * The player, after Dark Deception's BP_DD_PlayerCharacter (pak_reference): a capsule of radius 50 whose camera sits on
 * a zero-length spring arm 95 cm over its centre with rotation lag, walking at 300 and sprinting at 600 cm/s, the
 * camera's horizontal FOV following the speed, the walk / run head bob shakes, the 180° turn and the speed boost.
 * It also holds the tablet: the plate in front of the camera, its screen (UWasamiTabletWidget) and the scene capture
 * that draws the minimap.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AWasamiPlayerCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Whether the sprint is on: Shift held, or latched by a press with bToggleSprint. */
	UFUNCTION(BlueprintPure, Category = "Player")
	bool IsSprintOn() const { return bToggleSprint ? bSprintLatch : bSprintHeld; }

	UFUNCTION(BlueprintPure, Category = "Player|Boost")
	bool IsBoosting() const { return BoostTimeLeft > 0.f; }

	/** The speed boost's charge: 0 on use, rising to 1 when it can be used again. */
	UFUNCTION(BlueprintPure, Category = "Player|Boost")
	float GetBoostCharge() const;

	/** The right socket's `Percent`: 1 → 0 while the boost lasts, 0 → 1 over the cooldown that follows. */
	UFUNCTION(BlueprintPure, Category = "Player|Boost")
	float GetBoostSocketPercent() const;

	/** Whether the tablet is up (the original's isTabletUp?). */
	UFUNCTION(BlueprintPure, Category = "Player|Tablet")
	bool IsTabletUp() const { return bTabletUp; }

	/** Space raises and lowers the tablet (the original's Toggle Tablet). */
	UFUNCTION(BlueprintCallable, Category = "Player|Tablet")
	void ToggleTablet();

	/** Z switches the minimap between OrthoWidth 4000 and 10000 (the original's Resize Map). */
	UFUNCTION(BlueprintCallable, Category = "Player|Tablet")
	void ResizeMap();

	/** Walking Speed (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Movement")
	float WalkingSpeed = 300.f;

	/** Sprinting Speed (cm/s); Shift gives it whichever way the player moves. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Movement")
	float SprintingSpeed = 600.f;

	/** Both speeds while the speed boost lasts (cm/s: the original's 870 without upgrades). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Boost")
	float BoostSpeed = 870.f;

	/** Seconds the speed boost lasts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Boost")
	float BoostDuration = 6.75f;

	/** Seconds after the boost ends before it can be used again (its cooldown starts on use and includes the boost). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Boost")
	float BoostCooldown = 8.5f;

	/** The OPTIONS' TOGGLE SPRINT. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Options")
	bool bToggleSprint = false;

	/** The OPTIONS' MOUSE SENSITIVITY: a multiplier on the mouse axes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Options")
	float MouseSensitivity = 1.f;

	/** The OPTIONS' INVERTED Y AXIS. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Options")
	bool bInvertY = false;

	/** The OPTIONS' HEAD BOBBING. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Options")
	bool bHeadBob = true;

	/** The original's CanMove?: moving, looking, sprinting and the tablet are off while it is false. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
	bool bCanMove = true;

	/** The original's Can Use Tablet?. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Tablet")
	bool bCanUseTablet = true;

	/** What the shard count on the screen counts; empty until the shards exist, and the screen then shows 0. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Tablet")
	TSubclassOf<AActor> ShardActorClass;

	/** FOV Multiplier: the camera's FOV (°, horizontal) at FOVSpeedRange.X cm/s and below, rising linearly to FastFOV. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Camera")
	float BaseFOV = 90.f;

	/** The FOV (°) at FOVSpeedRange.Y cm/s and above. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Camera")
	float FastFOV = 115.f;

	/** The speeds (cm/s) BaseFOV and FastFOV map to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Camera")
	FVector2D FOVSpeedRange = FVector2D(300.f, 900.f);

	/** FInterpTo speed of the FOV, applied on every tick of the original's 0.001 s timer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Camera")
	float FOVInterpSpeed = 0.5f;

	/** Head bob while walking: the original's BP_DD_PlayerCharacter_WalkShake (built under /Game/DD by WasamiDDTools). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Camera")
	TSubclassOf<UCameraShakeBase> WalkShakeClass;

	/** Head bob while sprinting: BP_DD_PlayerCharacter_RunShake. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Camera")
	TSubclassOf<UCameraShakeBase> RunShakeClass;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
	TObjectPtr<UCameraComponent> Camera;

	/** The plate itself (tablet_new_pCube2), a child of the camera as the original's is. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Tablet")
	TObjectPtr<UStaticMeshComponent> Tablet;

	/** UMG_Tablet on the plate's face. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Tablet")
	TObjectPtr<UWidgetComponent> TabletScreen;

	/** Draws the level's map plane and the shards into T_NewMap from straight above the player. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Tablet")
	TObjectPtr<USceneCaptureComponent2D> MinimapCapture;

private:
	void CreateInput();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void SprintPressed();
	void SprintReleased();
	void TurnAround();
	void UseBoost();
	void ApplySpeed();
	void ApplyTabletInterp(float Value);
	void UpdateTablet(float DeltaSeconds);
	void UpdateTabletScreen();
	void RefreshMinimapContents();
	void UpdateFOV();
	void UpdateHeadBob();
	void StopHeadBob();

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> InputContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> TurnAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> BoostAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> TabletAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ResizeMapAction;

	/** 05_Tablet_Woosh_v2_1 (raising), 05_Tablet_Woosh_v1_1 (lowering) and UI_Select_V3 (the map's resize). */
	UPROPERTY()
	TObjectPtr<USoundBase> TabletUpSound;

	UPROPERTY()
	TObjectPtr<USoundBase> TabletDownSound;

	UPROPERTY()
	TObjectPtr<USoundBase> ResizeMapSound;

	UPROPERTY(Transient)
	TObjectPtr<AWasamiGameMode> WasamiGameMode;

	/** TabletInterp (0.5 s, raising) and Timeline_1 (0.3 s, lowering), built from the original's curves. */
	FRichCurve TabletRaiseCurve;
	FRichCurve TabletLowerCurve;
	FTimerHandle TabletScreenTimer;

	FTimerHandle FOVTimer;
	float TabletTime = 0.f;
	float TabletInterp = 0.f;
	int32 ShardsLeft = 0;
	bool bTabletUp = false;
	bool bTabletMoving = false;
	bool bMapZoomedOut = false;
	float BoostTimeLeft = 0.f;
	float BoostCooldownLeft = 0.f;
	bool bSprintHeld = false;
	bool bSprintLatch = false;
	bool bBobSprint = false;
	bool bBobStarted = false;
};
