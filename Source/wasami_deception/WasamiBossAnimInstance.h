#pragma once

#include "AlphaBlend.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "CoreMinimal.h"
#include "WasamiEnemyAnimInstance.h"
#include "WasamiBossAnimInstance.generated.h"

class UAnimSequence;

/** The boss's clips, by index into the lengths FWasamiBossAnimState is given (the importer's dd_boss.ROLES). */
namespace WasamiBossClip
{
	enum : int32
	{
		Idle,
		Alert,
		Detected,
		Num
	};
}

namespace WasamiBossAnim
{
	/** The clips' names in WasamiBossClip's order. */
	extern WASAMI_DECEPTION_API const TCHAR* const ClipNames[WasamiBossClip::Num];

	/** '/Game/Wasami/Boss/A_WasamiBoss_<name>.A_WasamiBoss_<name>'. */
	WASAMI_DECEPTION_API FSoftObjectPath ClipPath(int32 Clip);

	// The original's Matron_MiniBoss_AnimBP (pak_reference_2): Idle → Alert Transition (DD_Matron_Zone_02_Idle_Alert_Transition,
	// 0.8 s) on bAlert, crossfading over 0.5 s, then → Alert once less than 0.05 s of it is left, over 0.2 s; Alert → Idle
	// Transition (0.8 s) on NOT bAlert over 0.6 s, then → Idle the same way; HermiteCubic all. A transition state leads
	// only onward, so a change runs to its end before the next starts.
	inline constexpr float TransitionClipLength = 0.8f;
	inline constexpr float TransitionTimeLeft = 0.05f;
	inline constexpr float TransitionOutTime = 0.2f;
	// TODO(仮): the model has no transition clips, so a change is one crossfade to the other state, as long as the
	// original's whole change: its transition clip until 0.05 s is left, then the 0.2 s crossfade (0.95 s).
	inline constexpr float ChangeTime = TransitionClipLength - TransitionTimeLeft + TransitionOutTime;
	inline constexpr EAlphaBlendOption ChangeBlend = EAlphaBlendOption::HermiteCubic;
	// DD_Matron_Zone_02_Detected_Montage in DefaultSlot: blending in over the montage's default 0.25 s, Cubic, and
	// without an automatic blend out, so it stays on its last pose.
	inline constexpr float DetectedBlendIn = 0.25f;
	inline constexpr EAlphaBlendOption DetectedBlend = EAlphaBlendOption::Cubic;
	// The LookAt after the slot: spine_02 turned so that the mesh's +Y (not in the bone's space) points at the player,
	// at most 90 degrees; its alpha is bSpotted, blending in over 0.5 s (Cubic) and out at once.
	inline constexpr float LookAtBlendIn = 0.5f;
	inline constexpr EAlphaBlendOption LookAtBlend = EAlphaBlendOption::Cubic;
	inline constexpr float LookAtClamp = 90.f;
	inline const TCHAR* const LookAtBone = TEXT("spine_02");
}

/**
 * The boss's animation without the engine, after the Matron's ABP: the state machine Idle ↔ Alert (bAlert), under a
 * slot that plays Detected once and holds its last pose, and the LookAt's alpha. A state entered from nothing starts
 * its clip from 0; a branch whose weight is 0 does not move on. The first update takes no change
 * (bSkipFirstUpdateTransition), so it starts in Idle.
 */
struct WASAMI_DECEPTION_API FWasamiBossAnimState
{
	/** The clips' lengths in WasamiBossClip's order; a length of 0 is a missing clip, which never plays. */
	void Init(TArrayView<const float> InLengths);
	void Update(bool bAlert, bool bSpotted, float DeltaSeconds);

	/** The montage: Detected from its start, blending in. False for a missing clip. */
	bool PlayDetected();
	bool IsPlayingDetected() const { return bDetected; }

	/** The poses to blend, with weights that add up to 1 (none before the first update). */
	void GetSamples(TArray<FWasamiEnemyAnimSample>& OutSamples) const;

	float GetLength(int32 Clip) const { return Lengths.IsValidIndex(Clip) ? Lengths[Clip] : 0.f; }
	float GetClipTime(int32 Clip) const { return ClipTimes.IsValidIndex(Clip) ? ClipTimes[Clip] : 0.f; }
	float GetClipWeight(int32 Clip) const;
	/** Whether the state machine is in Alert (or changing to it). */
	bool IsAlert() const { return Change.bInB; }
	/** The LookAt's alpha. */
	float GetLookAtAlpha() const;

	TArray<float> Lengths;
	TArray<float> ClipTimes;
	bool bStarted = false;
	/** Idle (A) ↔ Alert (B). */
	FWasamiStateBlend Change;
	bool bDetected = false;
	/** How long Detected has blended in (s). */
	float DetectedElapsed = 0.f;
	/** The LookAt's alpha before its curve. */
	FWasamiBoolBlend LookAt;
};

/** Hands the frame's samples and the LookAt to the worker thread, which blends the poses and turns spine_02. */
USTRUCT()
struct FWasamiBossAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FWasamiBossAnimInstanceProxy() = default;
	explicit FWasamiBossAnimInstanceProxy(UAnimInstance* InAnimInstance) : FAnimInstanceProxy(InAnimInstance) {}

protected:
	virtual void PreEvaluateAnimation(UAnimInstance* InAnimInstance) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	/** The LookAt (FAnimNode_LookAt's with a location and no target bone: the location is in the world). */
	void LookAt(FPoseContext& Output) const;

	TArray<FWasamiPoseSample, TInlineAllocator<4>> Samples;
	float LookAtAlpha = 0.f;
	FVector LookAtLocation = FVector::ZeroVector;
};

/**
 * The boss Wasami's animation, after the Matron's Matron_MiniBoss_AnimBP (pak_reference_2): a native anim instance
 * whose proxy samples the imported clips (/Game/Wasami/Boss/A_WasamiBoss_*) and blends them. Each frame it reads, as the
 * ABP's event graph does, bAlert (the owning AWasamiMatron's bMode), bSpotted and the player's location (another owner
 * sets them). The clips load when a game world starts it, so in the editor's level the boss shows its reference pose.
 * The ABP's ModifyBone (the Matron's two saws scaled to 0) is left out: the boss has no such bones.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiBossAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** bAlert: Alert, else Idle. */
	UPROPERTY(BlueprintReadWrite, Category = "Boss")
	bool bAlert = false;

	/** bSpotted: the LookAt turns the boss to the player. */
	UPROPERTY(BlueprintReadWrite, Category = "Boss")
	bool bSpotted = false;

	/** Location: where the LookAt points (the ABP's default until the first update). */
	UPROPERTY(BlueprintReadWrite, Category = "Boss")
	FVector Location = FVector(100., 0., 0.);

	/** Plays Detected (the Detected montage), which stays on its last pose. Returns its length (s), or 0 without it. */
	UFUNCTION(BlueprintCallable, Category = "Boss")
	float PlayDetected();

	UFUNCTION(BlueprintPure, Category = "Boss")
	bool IsPlayingDetected() const { return AnimState.IsPlayingDetected(); }

	/** The clip playing with the most weight, and its time (for the checks in PIE). */
	UFUNCTION(BlueprintPure, Category = "Boss")
	FName GetMainClip(float& OutTime, float& OutWeight) const;

	/** The LookAt's alpha this frame. */
	UFUNCTION(BlueprintPure, Category = "Boss")
	float GetLookAtAlpha() const { return AnimState.GetLookAtAlpha(); }

	const FWasamiBossAnimState& GetAnimState() const { return AnimState; }

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

private:
	friend struct FWasamiBossAnimInstanceProxy;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAnimSequence>> Clips;

	FWasamiBossAnimState AnimState;
	TArray<FWasamiEnemyAnimSample> FrameSamples;
};
