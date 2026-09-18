#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WasamiEnemyInterface.h"
#include "WasamiEnemy.generated.h"

class USkeletalMesh;
class UWasamiEnemyAnimInstance;

/**
 * The enemy Wasami, after Dark Deception's hospital nurse BP_06_ReaperNurse and its parent BP_DD_Character_Base
 * (pak_reference_2): a character with the Enemy tag and the enemy interface, the nurse's capsule, movement and mesh
 * placement, this game's Wasami (SK_WasamiEnemy) with UWasamiEnemyAnimInstance, and the nurse's stun. The AI (the
 * chase, the patrol and what it decides every half second) is not made yet; Make Choice only runs the stun.
 *
 * The stun, as the nurse's: Set State only sets State. The decision timer (Make Choice, every 0.5 s from BeginPlay)
 * that finds State == Stun stops the movement at once, waits 17 s and sets State back to Patrol, once (a DoOnce: a
 * stun sent while it waits changes nothing). The animation reads State == Stun every frame, as the nurse's ABP does.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiEnemy : public ACharacter, public IWasamiEnemyInterface
{
	GENERATED_BODY()

public:
	AWasamiEnemy();

	// The nurse's CDO and its components'.
	static constexpr float CapsuleRadius = 34.f;
	static constexpr float CapsuleHalfHeight = 118.05821990966797f;
	static constexpr float MaxSpeed = 800.f;
	static constexpr float TurnRate = 300.f;
	static constexpr double MeshX = -6.216194742592052e-05;
	static constexpr double MeshY = -0.00021553380065597594;
	static constexpr double MeshZ = -117.84394073486328;
	static constexpr double MeshYaw = -90.0001220703125;
	// This game's scale on the mesh, the same on X, Y and Z (the user's request, 2026-09-18: the nurse's height). The
	// top of the head, bind pose to bind pose: the nurse's nurse_idle1 has Nurse_TopOfHead_AuxSHJnt 229.05135 cm up
	// (its cap reaches 246.35), SK_WasamiEnemy its head_end 168.52719 cm up (its hair 170.0). Its feet stand on the
	// mesh's origin, so they stay where the nurse's are.
	static constexpr double NurseHeadTop = 229.05135;
	static constexpr double WasamiHeadTop = 168.52719;
	static constexpr double MeshScale = NurseHeadTop / WasamiHeadTop;
	// Make Choice's timer (K2_SetTimer, looping) and the stun's Delay.
	static constexpr float DecisionInterval = 0.5f;
	static constexpr float StunSeconds = 17.f;

	/**
	 * Spawns one whose capsule centre is at Location, turned to Yaw, with CanSpawn set as the hospital's level script
	 * spawns its nurses (a PIE check can call it from Python).
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy", meta = (WorldContext = "WorldContextObject"))
	static AWasamiEnemy* SpawnEnemy(const UObject* WorldContextObject, FVector Location, float Yaw = 0.0f, bool bSentry = false);

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Set Walk State: the movement's top speed is Normal Speed when bNormal, else Skate Speed. */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void SetWalkState(bool bNormal);

	/** The State Set State last set (the interface's Get State reads Patrol, as the nurse's does). */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	EWasamiEnemyState GetCurrentState() const { return State; }

	/** State is Stun: the ABP's bStunned. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsStunned() const { return State == EWasamiEnemyState::Stun; }

	/** Make Choice has started the stun and its 17 s run. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsStunRunning() const { return bStunRunning; }

	/**
	 * How long until the stun sets State back to Patrol: what is left of the 17 s, or, before the next decision starts
	 * them, the time to that decision and the 17 s. 0 when not stunned.
	 */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetStunTimeLeft() const;

	/** The mesh's animation (PlayOnce and the rest), or null before it starts. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	UWasamiEnemyAnimInstance* GetEnemyAnim() const;

	/** The base's CanSpawn: false destroys the enemy as it begins play. The level's script sets it on its spawns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ExposeOnSpawn = true))
	bool bCanSpawn = false;

	/** The sentry's idle (BP_06_ReaperNurse_Sentry). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ExposeOnSpawn = true))
	bool bAggressiveIdle = false;

	/** The chase after every shard runs with Run_Nightmare. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	bool bNightmare = false;

	/** Set Walk State's speeds (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float NormalSpeed = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float SkateSpeed = 800.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy")
	bool bNormalWalk = false;

	/** The nurse's Seen Player Recently (its Chasing); Player Vanish clears it. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Enemy")
	bool bSeenPlayerRecently = false;

protected:
	virtual void BeginPlay() override;

	virtual void SetState_Implementation(EWasamiEnemyState NewState, bool bByOrb) override;
	// The nurse's Get State always answers Patrol (it does not read State), and its No Telepathy false.
	virtual EWasamiEnemyState GetState_Implementation() const override { return EWasamiEnemyState::Patrol; }
	virtual void PlayerVanish_Implementation() override { bSeenPlayerRecently = false; }
	virtual bool NoTelepathy_Implementation() const override { return false; }

	/** Every DecisionInterval: the stun when State is Stun. */
	void MakeChoice();
	void EndStun();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy")
	EWasamiEnemyState State = EWasamiEnemyState::Patrol;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	TSoftObjectPtr<USkeletalMesh> MeshAsset;

private:
	FTimerHandle DecisionTimer;
	FTimerHandle StunTimer;
	bool bStunRunning = false;
};
