#pragma once

#include "CoreMinimal.h"
#include "Curves/RichCurve.h"
#include "GameFramework/Character.h"
#include "WasamiPlayerCharacter.generated.h"

class AWasamiArrowPointer;
class AWasamiGameMode;
class UCameraComponent;
class UChildActorComponent;
class UCameraShakeBase;
class UMaterialInterface;
class UInputAction;
class UInputMappingContext;
class USceneCaptureComponent2D;
class USoundBase;
class UPostProcessComponent;
class USpringArmComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextureRenderTarget2D;
class UWasamiChameleonComponent;
class UWasamiInteractWidget;
class UWasamiPowerComponent;
class UWasamiSettingsSaveGame;
class UWasamiTabletWidget;
class UWidgetComponent;
struct FInputActionValue;

/**
 * The player, after Dark Deception's BP_DD_PlayerCharacter (pak_reference): a capsule of radius 50 whose camera sits on
 * a zero-length spring arm 95 cm over its centre with rotation lag, walking at 300 and sprinting at 600 cm/s, the
 * camera's horizontal FOV following the speed, the walk / run head bob shakes and the 180° turn. It also holds the
 * tablet: the plate in front of the camera, its screen (UWasamiTabletWidget), the scene capture that draws the
 * minimap and the arrow on the map (AWasamiArrowPointer), the tablet's powers (UWasamiPowerComponent), and the post-process effects the powers switch on
 * (UWasamiChameleonComponent, the original's Chameleon FX). What it looks at within 200 cm it can use with the left
 * click (IWasamiInteractable), and the hand on the screen (UWasamiInteractWidget) says when.
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

	/** Sprinting? set false (a portal's way in, BP_00_Teleport): the sprint off, held or latched, and the walk's speed. */
	UFUNCTION(BlueprintCallable, Category = "Player|Movement")
	void StopSprinting();

	/**
	 * What the death before this carried over (UWasamiGameInstance::RememberPlayerState): the tablet already up, with
	 * none of Toggle Tablet's woosh or its rise, and the sprint already on. BeginPlay calls it with what the game
	 * instance carried.
	 */
	UFUNCTION(BlueprintCallable, Category = "Player")
	void RestoreState(bool bInTabletUp, bool bInSprintOn);

	/** Writes Walking Speed and Sprinting Speed (the speed boost sets both) and applies the one in use. */
	UFUNCTION(BlueprintCallable, Category = "Player|Movement")
	void SetMoveSpeeds(float Walking, float Sprinting);

	/** Whether the tablet is up (the original's isTabletUp?). */
	UFUNCTION(BlueprintPure, Category = "Player|Tablet")
	bool IsTabletUp() const { return bTabletUp; }

	/**
	 * Interact (F) pressed. The original's player has no use for it; the things that take it (a door break's lock) take
	 * the key themselves (AutoReceiveInput, without consuming it). Here they listen to this, which the key fires.
	 */
	FSimpleMulticastDelegate OnInteract;

	/** Fires OnInteract (F calls it; the debug command and the tests call it directly). */
	void InteractPressed();

	/**
	 * Esc pressed (the old version's InpActEvt_Escape, @7758): CreateAndAddWidget(UMG_Pause, 5), unless the game is
	 * paused (the original's key binding does not execute then); the menu's UI-only input keeps Esc from coming again.
	 * Wasami.Pause calls it (Esc stops a play session in the editor).
	 */
	void EscapePressed();

	/** How far ahead of the camera the player looks for something to use (the original's 200 cm traces). */
	static constexpr float InteractDistance = 200.f;

	/**
	 * Interact (Secondary) pressed (the left click; the tests call it directly): with Can Interact?, traces
	 * InteractDistance ahead of the camera on Visibility and calls InteractWithObject(this) on the actor it hits if that
	 * is IWasamiInteractable. The hit is kept for the release.
	 */
	void InteractSecondaryPressed();

	/** Interact (Secondary) released: StopInteractWithObject on the actor the last press hit, whatever Can Interact? is. */
	void InteractSecondaryReleased();

	/** The trace both of the above and the tick make: Visibility, simple collision, ignoring the player. */
	bool TraceInteract(FHitResult& OutHit) const;

	/**
	 * The tick's look for the hand (Tick calls it; the tests call it directly): with Can Interact?, shows the hand
	 * (SelfHitTestInvisible) while the trace hits a component tagged interact and collapses it otherwise. Without Can
	 * Interact? the hand stays as it was.
	 */
	void UpdateInteractWidget();

	/** UMG_Interact: the hand, made at BeginPlay (on the screen when the world has a game viewport). */
	UWasamiInteractWidget* GetInteractWidget() const { return InteractWidget; }

	/** Space raises and lowers the tablet (the original's Toggle Tablet). */
	UFUNCTION(BlueprintCallable, Category = "Player|Tablet")
	void ToggleTablet();

	/** Put Down Tablet: lowers the tablet if it is up (the woosh and the lowering), whatever the player may do. */
	UFUNCTION(BlueprintCallable, Category = "Player|Tablet")
	void PutDownTablet();

	/** Z switches the minimap between OrthoWidth 4000 and 10000 (the original's Resize Map). */
	UFUNCTION(BlueprintCallable, Category = "Player|Tablet")
	void ResizeMap();

	/** Whether Z has the map zoomed out (the original's mapZoomedOut?). */
	bool IsMapZoomedOut() const { return bMapZoomedOut; }

	/**
	 * Add To Map: every actor of ActorClass in the level now joins what the minimap shows, until Remove From Map (the
	 * bonus shard's reveal adds the enemies' classes). An actor of the class that comes later needs another call.
	 */
	UFUNCTION(BlueprintCallable, Category = "Player|Tablet")
	void AddToMap(TSubclassOf<AActor> ActorClass);

	/** Remove From Map: every actor of ActorClass in the level now leaves what Add To Map put on the minimap. */
	UFUNCTION(BlueprintCallable, Category = "Player|Tablet")
	void RemoveFromMap(TSubclassOf<AActor> ActorClass);

	/** Whether the minimap's capture shows Actor (as of its last refresh). */
	bool IsOnMap(const AActor* Actor) const;

	/** The screen on the tablet, once the widget component has made it. */
	UWasamiTabletWidget* GetTabletScreen() const;

	/** The plate itself, where PlaceTablet has put it this frame. */
	UStaticMeshComponent* GetTablet() const { return Tablet; }


	/** The map's arrow, once the child actor component has made it. */
	AWasamiArrowPointer* GetArrowPointer() const;

	UCameraComponent* GetCamera() const { return Camera; }

	UWasamiPowerComponent* GetPowers() const { return Powers; }

	/** The original's Chameleon FX. */
	UWasamiChameleonComponent* GetChameleon() const { return Chameleon; }

	/** The cutscene bars' volume, made at BeginPlay. */
	UPostProcessComponent* GetCutsceneBars() const { return CutsceneBars; }

	/** Walking Speed (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Movement")
	float WalkingSpeed = 300.f;

	/** Sprinting Speed (cm/s); Shift gives it whichever way the player moves. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Movement")
	float SprintingSpeed = 600.f;

	/**
	 * Takes the OPTIONS' TOGGLE SPRINT, MOUSE SENSITIVITY (UWasamiSettingsSaveGame::PlayerSensitivityFor), INVERTED Y
	 * AXIS and HEAD BOBBING, which the original reads from the game mode's settings each time it uses them: at BeginPlay
	 * and when the OPTIONS are saved.
	 */
	void ApplySettings(const UWasamiSettingsSaveGame& Settings);

	/**
	 * Set Up Mouse Smoothing: the spring arm's rotation lag from MOUSE SMOOTHING. Only the OPTIONS' SAVE & EXIT calls it
	 * (the level starts at the spring arm's own 20, as in the original).
	 */
	void SetUpMouseSmoothing(const UWasamiSettingsSaveGame& Settings);

	/** The OPTIONS' TOGGLE SPRINT (on, as its default). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Options")
	bool bToggleSprint = true;

	/** The OPTIONS' MOUSE SENSITIVITY: a multiplier on the mouse axes (1 for the setting's default of 0.5). */
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

	/** The original's Has Input (its scripted scenes turn it off): the powers need it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
	bool bHasInput = true;

	/** The original's Can Interact?: Q and E, the left click's use and the hand need it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
	bool bCanInteract = true;

	/** The original's Can Use Tablet?: the tablet and the powers need it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Tablet")
	bool bCanUseTablet = true;

	/** What the shard count on the screen counts and the minimap shows (AWasamiShard); with none, the screen shows 0. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Tablet")
	TSubclassOf<AActor> ShardActorClass;

	/**
	 * The classes whose every actor the minimap always shows besides the map plane, the arrow and the shards (the
	 * original's list also has BP_PowerOrb and BP_BonusShard): AWasamiPowerOrb and AWasamiBonusShard.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Tablet")
	TArray<TSubclassOf<AActor>> MinimapActorClasses;

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
	UPROPERTY(EditAnywhere, Category = "Player|Camera")
	TSoftClassPtr<UCameraShakeBase> WalkShakeClass;

	/** Head bob while sprinting: BP_DD_PlayerCharacter_RunShake. */
	UPROPERTY(EditAnywhere, Category = "Player|Camera")
	TSoftClassPtr<UCameraShakeBase> RunShakeClass;

	/** tablet_new_pCube2: the plate. */
	UPROPERTY(EditAnywhere, Category = "Player|Tablet")
	TSoftObjectPtr<UStaticMesh> TabletMesh;

	/**
	 * MM_CutsceneBars_Inst: the black bars over a cutscene, which read Mat_ParameterCol's Cutscene Bars
	 * (UWasamiCutsceneWidget animates it). The original hangs the same instance on DD_PlayerController's PostProcess at
	 * a weight of 1; this game has no player controller of its own, so the player carries it — an unbound volume is
	 * the same either way, and both live as long as the level does.
	 */
	UPROPERTY(EditAnywhere, Category = "Player|Tablet")
	TSoftObjectPtr<UMaterialInterface> CutsceneBarsMaterial;

	/** T_NewMap: what the minimap's scene capture draws into. */
	UPROPERTY(EditAnywhere, Category = "Player|Tablet")
	TSoftObjectPtr<UTextureRenderTarget2D> MinimapTarget;

	/** 05_Tablet_Woosh_v2_1 (raising), 05_Tablet_Woosh_v1_1 (lowering) and UI_Select_V3 (the map's resize). */
	UPROPERTY(EditAnywhere, Category = "Player|Tablet")
	TSoftObjectPtr<USoundBase> TabletUpSound;

	UPROPERTY(EditAnywhere, Category = "Player|Tablet")
	TSoftObjectPtr<USoundBase> TabletDownSound;

	UPROPERTY(EditAnywhere, Category = "Player|Tablet")
	TSoftObjectPtr<USoundBase> ResizeMapSound;

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

	/** Draws the level's map plane, the shards and the arrow into T_NewMap from straight above the player. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Tablet")
	TObjectPtr<USceneCaptureComponent2D> MinimapCapture;

	/** BP_ArrowPointer: the map's arrow, a child actor 2000 cm over the mesh, (5, 5, 1) (AWasamiArrowPointer). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Tablet")
	TObjectPtr<UChildActorComponent> ArrowPointer;

	/** The tablet's powers: its sockets, Q / E / 1 / 2, the gauges and the speed boost. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Powers")
	TObjectPtr<UWasamiPowerComponent> Powers;

	/** FX: the Chameleon's unbound post-process, which the speed boost shakes. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Powers")
	TObjectPtr<UWasamiChameleonComponent> Chameleon;

	/** PostProcess: DD_PlayerController's unbound volume carrying the cutscene bars (BeginPlay makes it). */
	UPROPERTY(Transient)
	TObjectPtr<UPostProcessComponent> CutsceneBars;

private:
	void CreateInput();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void SprintPressed();
	void SprintReleased();
	void TurnAround();
	void LeftMousePressed();
	void LeftMouseReleased();
	void MouseWheel(const FInputActionValue& Value);
	void ApplySpeed();
	void UpdateRestoredSprint();
	static bool IsSprintKeyDown();
	void ApplyTabletInterp(float Value);
	void PlaceTablet();
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
	TObjectPtr<UInputAction> UsePowerLeftAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> UsePowerRightAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> CyclePowerLeftAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> CyclePowerRightAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> TabletAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ResizeMapAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InteractAction;

	/** The left mouse button and the wheel, which the original's teleport aim takes straight from the keys. */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LeftMouseAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MouseWheelAction;

	/** Esc, which opens the pause menu. */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> EscapeAction;

	/** The sounds and shakes above, loaded at BeginPlay. */
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedTabletUpSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedTabletDownSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedResizeMapSound;

	UPROPERTY(Transient)
	TSubclassOf<UCameraShakeBase> LoadedWalkShake;

	UPROPERTY(Transient)
	TSubclassOf<UCameraShakeBase> LoadedRunShake;

	UPROPERTY(Transient)
	TObjectPtr<AWasamiGameMode> WasamiGameMode;

	UPROPERTY(Transient)
	TObjectPtr<UWasamiInteractWidget> InteractWidget;

	/** The actor the last Interact (Secondary) press's trace hit (null if it hit nothing). */
	TWeakObjectPtr<AActor> InteractHitActor;

	/** What Add To Map put on the minimap (the original adds them to the capture's Show Only, which is remade here). */
	TArray<TWeakObjectPtr<AActor>> MapAddedActors;

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
	bool bSprintHeld = false;
	bool bSprintLatch = false;
	/** Whether bSprintHeld came from RestoreState and no press or release of the key has confirmed it yet. */
	bool bSprintRestored = false;
	bool bBobSprint = false;
	bool bBobStarted = false;
};
