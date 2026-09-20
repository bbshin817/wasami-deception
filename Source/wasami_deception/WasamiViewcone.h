#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/Interface.h"
#include "WasamiViewcone.generated.h"

class UMaterialInterface;
class UStaticMeshComponent;

UINTERFACE(BlueprintType)
class UWasamiViewconeInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * What a view cone tells the actor it is a child of, after the original's BPI_06_Viewcone (pak_reference_2): the sentry
 * nurses (AWasamiEnemySentry) and the Matron (item 11). The defaults do nothing.
 */
class WASAMI_DECEPTION_API IWasamiViewconeInterface
{
	GENERATED_BODY()

public:
	/** Start Looking: the cone is turning on (it sees a second later). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Viewcone")
	void StartLooking();

	/** Stop Looking: the cone turned off. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Viewcone")
	void StopLooking();

	/** Player Spotted: the cone saw the player. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Viewcone")
	void PlayerSpotted();

protected:
	virtual void StartLooking_Implementation() {}
	virtual void StopLooking_Implementation() {}
	virtual void PlayerSpotted_Implementation() {}
};

/**
 * Dark Deception's BP_06_Miniboss_viewcone (pak_reference_2's Blueprints/06_Hospital/Miniboss), the sight of the
 * hospital's miniboss corridor, carried as a child actor. Initialize (the carrier's Activate) runs Update Sight on a
 * looping timer of 0.3 to 0.5 s: the first waits Offset seconds, then each spots the player once (Player Spotted on the
 * actor it is a child of and on its owner) when Player Inside Cone? and Player In Full View?, and the first also
 * finishes initializing (Initialize Finished) and, with bAutoOn?, turns the cone on. Turn On tells the carrier Start
 * Looking and sees a second later; Turn Off tells it Stop Looking and is blind at once.
 *
 * Its two planes are the original's marks for the tablet's map, 1000 cm over the cone once play begins: Plane, the
 * cone's fan (map_enemy_search_Mat, its Opacity eased by the Fade In timeline as the cone turns on and off), and
 * Plane1, a dot (0_DotCircle_Mat). Neither is ever drawn, as in the original: its map capture takes every view cone in
 * Show Only, but the capture has Translucency off and both materials are unlit and translucent, so not one of them
 * reaches the map. Here the cone carries no dd_minimap tag, which is what keeps them out of the capture, and the
 * planes stay bVisibleInSceneCaptureOnly so they never show in the world either (the original hides them over the
 * ceiling instead). The materials are loaded when play begins (WasamiAssets.h).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiViewcone : public AActor
{
	GENERATED_BODY()

public:
	AWasamiViewcone();

	/** Initialize's looping timer on Update Sight: RandomFloatInRange(0.3, 0.5), drawn once. */
	static constexpr float SightRateMin = 0.3f;
	static constexpr float SightRateMax = 0.5f;
	/** Turn On's Delay before the cone sees. */
	static constexpr float TurnOnDelay = 1.f;
	/** The Fade In timeline: 0.5 s of its Visibility track (0 to 1, cubic, auto tangents). */
	static constexpr float FadeInLength = 0.5f;
	/** ReceiveBeginPlay's K2_AddLocalOffset on Plane (after a Delay of 0: the next tick). */
	static const FVector PlaneLift;
	/** The class's Plane and Plane1. */
	static const FVector PlaneLocation;
	static const FVector DotLocation;

	/** Initialize: Update Sight from now on (the carrier sets Offset first). */
	UFUNCTION(BlueprintCallable, Category = "Viewcone")
	void Initialize();

	/** Whether Initialize has started Update Sight. */
	UFUNCTION(BlueprintPure, Category = "Viewcone")
	bool IsInitialized() const;

	/**
	 * Update Sight: a Delay of Offset (dropped while one waits), then Offset 0; the player spotted once when inside the
	 * cone and in full view; Initialize Finished once, and Turn On with bAutoOn?.
	 */
	void UpdateSight();

	/** Player Inside Cone?: on, the player nearer than Length and less than Angle degrees off its front. */
	UFUNCTION(BlueprintPure, Category = "Viewcone")
	bool PlayerInsideCone() const;

	/**
	 * Player In Full View?: on, a complex Camera trace from it to the player, past itself and the actor it is a child of,
	 * hits the player no farther than Length, and Player Inside Cone?.
	 */
	UFUNCTION(BlueprintPure, Category = "Viewcone")
	bool PlayerInFullView() const;

	/** Turn On: the carrier's Start Looking, and a second on (a Delay: dropped while one waits) on, and fading in. */
	UFUNCTION(BlueprintCallable, Category = "Viewcone")
	void TurnOn();

	/** Turn Off: the carrier's Stop Looking, off, and fading out. */
	UFUNCTION(BlueprintCallable, Category = "Viewcone")
	void TurnOff();

	/** bOn: the cone sees. */
	UFUNCTION(BlueprintPure, Category = "Viewcone")
	bool IsOn() const { return bOn; }

	/** The Fade In timeline's value (Plane's Opacity). */
	UFUNCTION(BlueprintPure, Category = "Viewcone")
	float GetFade() const;

	UStaticMeshComponent* GetPlane() const { return Plane; }
	UStaticMeshComponent* GetDot() const { return Plane1; }

	/** How far it sees (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewcone")
	float Length = 1000.f;

	/** How far off its front it sees (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewcone")
	float Angle = 45.f;

	/** bAutoOn?: turned on as it finishes initializing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewcone")
	bool bAutoOn = true;

	/** Offset: how long the first Update Sight waits (the carrier's Activate sets it); 0 from then on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewcone")
	float Offset = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Initialize Finished: after the first Update Sight's wait (empty here). */
	virtual void InitializeFinished() {}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Viewcone")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Viewcone")
	TObjectPtr<USceneComponent> Scene;

	/** The cone's fan on the map. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Viewcone")
	TObjectPtr<UStaticMeshComponent> Plane;

	/** The dot on the map. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Viewcone")
	TObjectPtr<UStaticMeshComponent> Plane1;

	UPROPERTY(EditDefaultsOnly, Category = "Viewcone")
	TSoftObjectPtr<UMaterialInterface> FanMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Viewcone")
	TSoftObjectPtr<UMaterialInterface> DotMaterial;

private:
	/** ReceiveBeginPlay after its Delay of 0: Plane's Opacity 0, and Plane lifted. */
	void BeginPlayNextTick();
	/** Update Sight after its Delay. */
	void LookForPlayer();
	/** Turn On after its Delay: on, and Fade In played. */
	void SeeAfterTurnOn();
	/** Player Spotted on the actor it is a child of, then on its owner (the same carrier: its own DoOnce takes one). */
	void TellPlayerSpotted();
	/** Fade In's Update Func: Plane's Opacity. */
	void UpdateFade();

	bool bOn = false;
	// Update Sight's DoOnces: the player spotted, and Initialize Finished.
	bool bSpottedClosed = false;
	bool bInitializeFinished = false;
	FTimerHandle SightTimer;
	FTimerHandle SightDelayTimer;
	FTimerHandle TurnOnTimer;
	// The Fade In timeline: its position (s) and which way it plays (1 Play, -1 Reverse, 0 stopped).
	float FadePosition = 0.f;
	int32 FadeDirection = 0;
};

/**
 * BP_06_Miniboss_viewcone_Nurse: the sentry nurses' cone, 1500 cm long and 20 degrees wide, its fan stretched along it.
 * Once initialized it turns every 10 s, off and on in turn (so blind from the first turn for 10 s, and seeing again a
 * second after the next).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiViewconeNurse : public AWasamiViewcone
{
	GENERATED_BODY()

public:
	AWasamiViewconeNurse();

	/** Initialize Finished's looping timer on Turn. */
	static constexpr float TurnRate = 10.f;
	/** The class's Plane (its InheritableComponentHandler's). */
	static const FVector NursePlaneLocation;
	static const FVector NursePlaneScale;

	/** Turn: a flip-flop, Turn Off then Turn On. */
	void Turn();

protected:
	virtual void InitializeFinished() override;

private:
	bool bTurnedOff = false;
	FTimerHandle TurnTimer;
};

/**
 * BP_06_Miniboss_viewcone_Matron_Long: the Matron's sight while the player is away from her desk, 3000 cm long and 35
 * degrees wide. It does not turn on as it finishes initializing: the Matron's Switch turns it and the short one on and
 * off in turn. Its dot is hidden; its fan (at the class's 0) is placed by the level.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiViewconeMatronLong : public AWasamiViewcone
{
	GENERATED_BODY()

public:
	AWasamiViewconeMatronLong();
};

/** BP_06_Miniboss_viewcone_Matron_Short: the Matron's sight while the player is by her desk, 1350 cm long, 35 degrees wide. */
UCLASS()
class WASAMI_DECEPTION_API AWasamiViewconeMatronShort : public AWasamiViewcone
{
	GENERATED_BODY()

public:
	AWasamiViewconeMatronShort();
};
