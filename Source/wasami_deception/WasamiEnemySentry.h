#pragma once

#include "CoreMinimal.h"
#include "WasamiEnemy.h"
#include "WasamiViewcone.h"
#include "WasamiEnemySentry.generated.h"

class UChildActorComponent;

/**
 * The nurses on watch over Zone 2's miniboss corridor, after Dark Deception's BP_06_ReaperNurse_Sentry (pak_reference_2),
 * a BP_06_ReaperNurse the level places six of, up high (CanSpawn, bAggressiveIdle). Its BeginPlay is empty, so it
 * stands without deciding until its cone spots the player. The level's Activate MiniBoss Enemies (Miniboss Transition)
 * calls Activate on every one: its Offset to its cone (Viewcone, a BP_06_Miniboss_viewcone_Nurse on its capsule, 72 cm
 * up and 20 degrees down), which then starts looking. Player Spotted, once: the cone destroyed, Chasing true for good,
 * a launch toward Jump Down Spot (400 cm/s level, 500 up) and the nurse's BeginPlay, so it decides as a nurse from then
 * on. Start and Stop Looking (the cone turning on and off) set bVarIdle.
 *
 * Left out: the 06_NurseAlert achievement Player Spotted sets (achievements are not in this game); the tick's gate, which
 * lets the nurse's tick (the skating sound) run once spotted; Initial Yaw (its construction script keeps the yaw, and
 * nothing reads it) and Update Rotation (nothing calls it).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiEnemySentry : public AWasamiEnemy, public IWasamiViewconeInterface
{
	GENERATED_BODY()

public:
	AWasamiEnemySentry();

	/** Viewcone_GEN_VARIABLE (on CollisionCylinder). */
	static const FVector ViewconeLocation;
	static const FRotator ViewconeRotation;
	/** Player Spotted's LaunchCharacter: the way to Jump Down Spot times this, and this up (both overriding). */
	static constexpr double JumpSpeed = 400.;
	static constexpr double JumpUpSpeed = 500.;

	/** Activate: the cone's Offset from its own, then the cone's Initialize. */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void Activate();

	/** Its cone while it has one (null once spotted). */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	AWasamiViewcone* GetViewcone() const;

	USceneComponent* GetJumpDownSpot() const { return JumpDownSpot; }

	/** Chasing answers its own Chasing (true once spotted, and for good), not Seen Player Recently. */
	virtual bool IsChasing() const override { return bChasing; }

	/**
	 * Before it spots the player its Make Choice never runs (BeginNurse is empty), so a stun set then never ends: the
	 * original's sentry keeps State at Stun and holds the stun's pose until it spots the player and Make Choice, its
	 * first decision, starts the 17 s.
	 */
	virtual float GetTimeToStunStart() const override
	{
		return bChasing ? Super::GetTimeToStunStart() : IndefiniteStunSeconds;
	}

	/** Offset: how long its cone waits before it first looks (10 s for three of the six). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float Offset = 0.f;

	/** Chasing: set as the player is spotted. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy")
	bool bChasing = false;

	/**
	 * The nurse's bVarIdle: false as its cone turns on (Start Looking), true as it turns off (Stop Looking). The ABP's
	 * second pair of alert idles reads it; this game's animation has one alert idle, so nothing does here.
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy")
	bool bVarIdle = false;

protected:
	/** BeginPlay's nurse part: the sentry's BeginPlay is empty (Player Spotted runs the nurse's). */
	virtual void BeginNurse() override {}

	virtual void StartLooking_Implementation() override { bVarIdle = false; }
	virtual void StopLooking_Implementation() override { bVarIdle = true; }
	virtual void PlayerSpotted_Implementation() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UChildActorComponent> Viewcone;

	/** Jump Down Spot: where it leaps to when spotting the player (each placed one moves it). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<USceneComponent> JumpDownSpot;

private:
	// Player Spotted's DoOnce.
	bool bSpottedClosed = false;
};
