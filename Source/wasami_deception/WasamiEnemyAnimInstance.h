#pragma once

#include "AlphaBlend.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "CoreMinimal.h"
#include "WasamiEnemyAnimInstance.generated.h"

class UAnimSequence;

/**
 * The enemy's clips, by index into the lengths FWasamiEnemyAnimState is given: the locomotion's and the stun's first,
 * then the ones played once. The asset of each is /Game/Wasami/Enemy/A_WasamiEnemy_<name> (WasamiEnemyAnim::ClipNames,
 * the importer's dd_enemy.ROLES).
 */
namespace WasamiEnemyClip
{
	enum : int32
	{
		Idle,
		IdleAlert,
		Walk,
		Run,
		RunNightmare,
		StunLoop,
		StunRecover,
		Capture1,
		Capture2,
		Capture3,
		ChasePickUp,
		ChaseCharge,
		ChaseVaultRoll,
		ChaseVaultLand,
		ChaseRunFast,
		ChaseSlide,
		BeHitFlyUp,
		KnockDown,
		PushUpToIdle,
		Num
	};
}

namespace WasamiEnemyAnim
{
	/** The clips' names in WasamiEnemyClip's order ('Idle_Alert'), WasamiEnemyClip::Num of them. */
	extern WASAMI_DECEPTION_API const TCHAR* const ClipNames[WasamiEnemyClip::Num];

	/** The clip called Name, or INDEX_NONE. */
	WASAMI_DECEPTION_API int32 FindClip(FName Name);

	/** '/Game/Wasami/Enemy/A_WasamiEnemy_<name>.A_WasamiEnemy_<name>'. */
	WASAMI_DECEPTION_API FSoftObjectPath ClipPath(int32 Clip);

	// The original's nurse_idle1_Skeleton_AnimBlueprint (pak_reference_2): the root's Blend Poses by bool on bStunned,
	// the Idle ↔ Skating crossfades on Speed = VSize(GetVelocity()), Skating's run above 400 and the idle's alert pose.
	inline constexpr float StunBlendTime = 0.25f;
	inline constexpr float MoveSpeed = 5.f;
	inline constexpr float StartMovingTime = 0.5f;
	inline constexpr EAlphaBlendOption StartMovingBlend = EAlphaBlendOption::Sinusoidal;
	// Skating → Stop Skating (0.25 s, ExpOut). We have no stopping clip, so this crossfade goes to Idle.
	inline constexpr float StopMovingTime = 0.25f;
	inline constexpr EAlphaBlendOption StopMovingBlend = EAlphaBlendOption::ExpOut;
	inline constexpr float RunSpeed = 400.f;
	inline constexpr float RunBlendTime = 0.25f;
	inline constexpr float AlertBlendTime = 1.f;
	// TODO(仮): the original has one run; the Nightmare run (the chase after every shard) switches in like its run.
	inline constexpr float NightmareBlendTime = 0.25f;
	// An AnimMontage's default blends, for what plays once.
	inline constexpr float OnceBlendTime = 0.25f;

	// TODO(仮): the play rate follows the speed so the feet slide less (the original skates at rate 1). The speed of
	// each clip's planted foot at the mesh's own size, as PIE measured it (the item 4's step 4: 134, 446, 496 cm/s at
	// rate 1), grown with the enemy's mesh (AWasamiEnemy::MeshScale) where the rate is worked out, and the WebGL
	// version's limits (walk 0.5–2, run 0.6–1.8). The patrol's 350 cm/s walk then wants 1.94.
	inline constexpr float WalkStrideSpeed = 133.f;
	inline constexpr float RunStrideSpeed = 450.f;
	inline constexpr float NightmareStrideSpeed = 500.f;
	inline constexpr float WalkRateMin = 0.5f;
	inline constexpr float WalkRateMax = 2.f;
	inline constexpr float RunRateMin = 0.6f;
	inline constexpr float RunRateMax = 1.8f;
}

/**
 * A Blend Poses by bool with a linear blend (FAnimNode_BlendListBase): the true side's weight moves toward the side
 * asked for at 1 / BlendTime per second, and the first update after a reset snaps to it.
 */
struct WASAMI_DECEPTION_API FWasamiBoolBlend
{
	void Update(bool bNewValue, float BlendTime, float DeltaSeconds);
	void Reset() { bStarted = false; }

	float Weight = 0.f;
	bool bValue = false;
	bool bStarted = false;
};

/**
 * A two-state machine's crossfade (FAnimNode_StateMachine's standard blend): the state entered goes from the weight it
 * had to 1 over the transition's time, along the transition's curve.
 */
struct WASAMI_DECEPTION_API FWasamiStateBlend
{
	/** Starts the crossfade to B (or to A); nothing happens when that state is already the current one. */
	void Enter(bool bToB, float Duration, EAlphaBlendOption Option);
	void Advance(float DeltaSeconds);

	float WeightB = 0.f;
	bool bInB = false;
	float StartWeight = 1.f;
	float Elapsed = 0.f;
	float Duration = 0.f;
	EAlphaBlendOption Option = EAlphaBlendOption::Linear;
};

/**
 * The stun: Stun_Loop loops, and Stun_Recover (whose first key is the loop's) plays so that it ends when the stun does.
 * The loop starts at the phase that brings its end round exactly when the recovery starts, so the two join seamlessly;
 * a stun shorter than the recovery starts the recovery part way through.
 */
struct WASAMI_DECEPTION_API FWasamiStunPlayback
{
	void Start(float Duration, float InLoopLength, float InRecoverLength);
	void Advance(float DeltaSeconds) { Elapsed += DeltaSeconds; }

	bool IsRecovering() const { return Elapsed >= RecoverStart; }
	/** The time in Stun_Loop, or in Stun_Recover once recovering (held at its end). */
	float GetClipTime() const;

	float LoopLength = 0.f;
	float RecoverLength = 0.f;
	float Elapsed = 0.f;
	float RecoverStart = 0.f;
	float LoopStart = 0.f;
	float RecoverTimeStart = 0.f;
};

/** A clip played once over the rest (an AnimMontage in a full-body slot): blends in, then out as it ends or is stopped. */
struct WASAMI_DECEPTION_API FWasamiOncePlayback
{
	void Advance(float DeltaSeconds);
	void Stop(float InBlendOut);
	bool IsFinished() const { return bStopping && Weight <= 0.f; }

	int32 Clip = INDEX_NONE;
	float Length = 0.f;
	float Time = 0.f;
	float PlayRate = 1.f;
	float BlendIn = 0.f;
	float BlendOut = 0.f;
	float Weight = 0.f;
	bool bStopping = false;
};

/** What the enemy tells its animation each frame. */
struct FWasamiEnemyAnimInputs
{
	float Speed = 0.f;
	bool bStunned = false;
	/** How long a stun that starts now lasts (the enemy's). */
	float StunDuration = 0.f;
	bool bAggressiveIdle = false;
	bool bNightmare = false;
};

/** A clip at a time and weight, one of the poses the frame blends. */
struct FWasamiEnemyAnimSample
{
	int32 Clip = INDEX_NONE;
	float Time = 0.f;
	float Weight = 0.f;
	bool bLoop = false;
};

/**
 * The enemy's animation without the engine: which clips play, at what times and weights. The original ABP's tree —
 * Blend by bStunned (the stun | the locomotion: Idle (Idle | Idle_Alert) ↔ Moving (Walk | Run (Run | Run_Nightmare)))
 * — under a full-body slot that plays clips once. A branch whose weight is 0 does not move on, and a state entered from
 * nothing starts its clips from 0, as in the ABP; the locomotion moves on under the stun and what plays once too, so
 * it is in step with the speed when they end.
 */
struct WASAMI_DECEPTION_API FWasamiEnemyAnimState
{
	/** The clips' lengths in WasamiEnemyClip's order; a length of 0 is a missing clip, which never plays. */
	void Init(TArrayView<const float> InLengths);
	void Update(const FWasamiEnemyAnimInputs& Inputs, float DeltaSeconds);

	/** Plays Clip once over the rest; what was playing once blends out over BlendIn. False for a missing clip. */
	bool PlayOnce(int32 Clip, float PlayRate, float BlendIn, float BlendOut);
	/** Blends out what plays once over BlendOut (one already blending out keeps the shorter of the two). */
	void StopOnce(float BlendOut);
	/** Something plays once and has not started to blend out. */
	bool IsPlayingOnce() const;

	/** The poses to blend, with weights that add up to 1 (none before the first update). */
	void GetSamples(TArray<FWasamiEnemyAnimSample>& OutSamples) const;

	float GetLength(int32 Clip) const { return Lengths.IsValidIndex(Clip) ? Lengths[Clip] : 0.f; }
	float GetClipTime(int32 Clip) const { return ClipTimes.IsValidIndex(Clip) ? ClipTimes[Clip] : 0.f; }
	float GetClipRate(int32 Clip) const { return ClipRates.IsValidIndex(Clip) ? ClipRates[Clip] : 0.f; }
	/** The weight of a clip of the locomotion or the stun, or of what plays once, in the blend. */
	float GetClipWeight(int32 Clip) const;

	TArray<float> Lengths;
	TArray<float> ClipTimes;
	TArray<float> ClipRates;
	bool bStarted = false;
	FWasamiBoolBlend Stun;
	FWasamiStunPlayback StunPlayback;
	FWasamiStateBlend Moving;
	FWasamiBoolBlend Alert;
	FWasamiBoolBlend Running;
	FWasamiBoolBlend Nightmare;
	TArray<FWasamiOncePlayback> Once;

private:
	void AdvanceClip(int32 Clip, float Rate, float DeltaSeconds);
	/** How much of the frame what plays once takes, and the sum of its weights. */
	float GetOnceWeight(float* OutSum = nullptr) const;
};

/** Hands the frame's samples to the worker thread, which blends the clips' poses. */
USTRUCT()
struct FWasamiEnemyAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FWasamiEnemyAnimInstanceProxy() = default;
	explicit FWasamiEnemyAnimInstanceProxy(UAnimInstance* InAnimInstance) : FAnimInstanceProxy(InAnimInstance) {}

protected:
	virtual void PreEvaluateAnimation(UAnimInstance* InAnimInstance) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	struct FSample
	{
		const UAnimSequence* Sequence = nullptr;
		float Time = 0.f;
		float Weight = 0.f;
		bool bLoop = false;
	};

	TArray<FSample, TInlineAllocator<8>> Samples;
};

/**
 * The enemy Wasami's animation, after Dark Deception's nurse's ABP (nurse_idle1_Skeleton_AnimBlueprint): a native
 * anim instance whose proxy samples the imported clips (/Game/Wasami/Enemy/A_WasamiEnemy_*) and blends them. The
 * flags are read from an AWasamiEnemy owner every frame (another owner sets them); the speed is the owner's velocity.
 * The clips load when a game world starts it (never at the editor's start up, WasamiAssets.h), so in the editor's
 * level the enemy shows its reference pose.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** The owner's State is Stun (the ABP's bStunned). Setting it starts the stun's clips for StunDuration. */
	UPROPERTY(BlueprintReadWrite, Category = "Enemy")
	bool bStunned = false;

	/**
	 * How long a stun that starts now lasts: BP_06_ReaperNurse's Make Choice waits 17.0 s before it goes back to Patrol
	 * (an AWasamiEnemy owner gives what is left of its stun).
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Enemy")
	float StunDuration = 17.f;

	/** The sentry's idle (BP_06_ReaperNurse_Sentry's bAggressiveIdle). */
	UPROPERTY(BlueprintReadWrite, Category = "Enemy")
	bool bAggressiveIdle = false;

	/** The chase after every shard runs with Run_Nightmare. */
	UPROPERTY(BlueprintReadWrite, Category = "Enemy")
	bool bNightmare = false;

	/** The owner's speed this frame (cm/s). */
	UPROPERTY(BlueprintReadOnly, Category = "Enemy")
	float Speed = 0.f;

	/**
	 * Plays the clip called Clip ('Chase_Slide', WasamiEnemyAnim::ClipNames) once over everything else, at PlayRate,
	 * blending in and out. The owner keeps moving. Returns how long it plays (s), or 0 when there is no such clip.
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	float PlayOnce(FName Clip, float PlayRate = 1.0f, float BlendIn = 0.25f, float BlendOut = 0.25f);

	/** Blends out what plays once. */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void StopOnce(float BlendOut = 0.25f);

	/** What PlayOnce started still plays (false once it blends out). */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsPlayingOnce() const { return AnimState.IsPlayingOnce(); }

	/** The clip playing with the most weight, and its time (for the checks in PIE). */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	FName GetMainClip(float& OutTime, float& OutWeight) const;

	const FWasamiEnemyAnimState& GetAnimState() const { return AnimState; }

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

private:
	friend struct FWasamiEnemyAnimInstanceProxy;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAnimSequence>> Clips;

	FWasamiEnemyAnimState AnimState;
	TArray<FWasamiEnemyAnimSample> FrameSamples;
};
