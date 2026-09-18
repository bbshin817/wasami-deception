#pragma once

#include "CoreMinimal.h"
#include "WasamiEnemy.h"
#include "WasamiEnemy06Chase.generated.h"

class UAudioComponent;
class UCameraShakeBase;
class UParticleSystem;
class USoundBase;

/**
 * The nurses of Zone 1's parking lot, after Dark Deception's BP_06_ReaperNurse_06_Chase (pak_reference_2), a
 * BP_06_ReaperNurse: Spawn Nurses_06 spawns two as the player comes to 06_Start. Its tick replaces the nurse's: while
 * stunned it only runs the stun (the same DoOnce: it stops and is back 17 s on), and otherwise it chases the player every
 * frame (Chase Player), so it never loses them; Make Choice still runs every half second as the nurse's does. While
 * bAttackDoor (the level's 06_DoorsLock sets it, and clears it as the doors break in 25 s on) it also stabs at the doors
 * again and again: the needle's montage, then 0 to 0.5 s, then the next; at the montage's notify, one time in two, Hit FX
 * (the dust, the slam and the shake). Its Chasing always answers true. The pill throw and the invisibility are empty.
 *
 * The nurse's montage (ReaperNurse_Needle_Attack_NoSound_Montage) is the nurse's own, so the stab plays Wasami's
 * Chase_Charge in its place (enemy-wasami-motions.md); the times (the notify, the end) are the montage's.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiEnemy06Chase : public AWasamiEnemy
{
	GENERATED_BODY()

public:
	AWasamiEnemy06Chase();

	// ReaperNurse_Needle_Attack_NoSound_Montage: its length, its notify (PlayMontageNotify, OnNotifyBegin), and its blend
	// out, which starts BlendOutTriggerTime before the end and takes the montage's default 0.25 s; OnCompleted comes as
	// the blend out ends.
	static constexpr float DoorAttackLength = 0.9666666388511658f;
	static constexpr float DoorHitTime = 0.41319915652275085f;
	static constexpr float DoorAttackBlendOutTrigger = 0.5f;
	static constexpr float DoorAttackBlendOut = 0.25f;
	static constexpr float DoorAttackSeconds = DoorAttackLength - DoorAttackBlendOutTrigger + DoorAttackBlendOut;
	/** OnCompleted's RandomFloatInRange(0, this) Delay before the next stab. */
	static constexpr float DoorAttackMaxWait = 0.5f;
	/** OnNotifyBegin's RandomBoolWithWeight. */
	static constexpr float HitFXChance = 0.5f;
	/** Hit FX: P_06_NurseDoorHit at the ParticleSystem component (this far in front of the capsule), at this scale. */
	static constexpr float HitFXForward = 230.f;
	static constexpr float HitFXScale = 0.5f;
	/** Hit FX's PlayWorldCameraShake: full at the nurse, gone this far away. */
	static constexpr float HitShakeRadius = 3000.f;
	/** Wasami's clip in place of the needle's montage. */
	static const FName DoorAttackClip;

	virtual void Tick(float DeltaSeconds) override;

	virtual bool IsChasing() const override { return true; }

	virtual float GetTimeToStunStart() const override { return 0.f; }

	/** Hit FX: the dust in front of it, Audio (a slam) and the shake about it. */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void HitFX();

	/** How many stabs at the doors it has started (for the checks). */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	int32 GetDoorAttacks() const { return DoorAttacks; }

	/** The level's 06_DoorsLock sets it, and the doors broken in clear it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	bool bAttackDoor = false;

	/** Door Location: Spawn Nurses_06 gives it the level's DoorLocation; nothing of the nurse's reads it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ExposeOnSpawn = true))
	TObjectPtr<AActor> DoorLocation;

protected:
	virtual void BeginPlay() override;

	/** Audio: 20-Elevator_Slams (not auto-activated), played by Hit FX. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UAudioComponent> Audio;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	TSoftObjectPtr<USoundBase> DoorHitSound;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	TSoftObjectPtr<UParticleSystem> DoorHitParticle;

	/** 01_Hotel_Lobby_ElevatorShakeStop. */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	TSoftClassPtr<UCameraShakeBase> DoorHitShakeClass;

private:
	/** The DoOnce around the montage: plays the stab and times its notify and its end. */
	void StartDoorAttack();
	void OnDoorAttackNotify();
	void OnDoorAttackCompleted();

	bool bDoorAttackClosed = false;
	int32 DoorAttacks = 0;
	FTimerHandle DoorHitTimer;
	FTimerHandle DoorAttackTimer;
};
