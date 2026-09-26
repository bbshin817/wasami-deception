#pragma once

#include "Camera/CameraModifier.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Scene.h"
#include "GameFramework/Actor.h"
#include "Math/InterpCurve.h"
#include "WasamiCameraAnim.generated.h"

class ACameraActor;
class ALevelSequenceActor;
class APlayerCameraManager;

/** A float track of a UE4 CameraAnim: the property it animates and its Matinee curve, keys and tangents as saved. */
USTRUCT(BlueprintType)
struct FWasamiCameraAnimFloatTrack
{
	GENERATED_BODY()

	/** The property as the original names it ('CameraComponent.PostProcessSettings.AutoExposureBias'). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Anim")
	FString PropertyName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Anim")
	FInterpCurveFloat Curve;
};

/** A linear colour track of a UE4 CameraAnim ('CameraComponent.PostProcessSettings.SceneColorTint'). */
USTRUCT(BlueprintType)
struct FWasamiCameraAnimColorTrack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Anim")
	FString PropertyName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Anim")
	FInterpCurveLinearColor Curve;
};

/**
 * One of Dark Deception's CameraAnims (UE4's UCameraAnim, which UE 5 no longer has), copied by the pipeline from the
 * export: its length, the post-process settings its camera starts from and their weight, and the tracks that animate
 * those settings and the field of view. UWasamiCameraAnimModifier plays it.
 */
UCLASS(BlueprintType)
class WASAMI_DECEPTION_API UWasamiCameraAnim : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Writes every post-process track's value at Time into Settings (the tracks leave the bOverride flags alone). */
	void ApplyPostProcessTracks(float Time, FPostProcessSettings& Settings) const;

	/** The field of view track ('CameraComponent.FieldOfView'), or null when the anim has none. */
	const FWasamiCameraAnimFloatTrack* FindFieldOfViewTrack() const;

	/** AnimLength (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Anim")
	float AnimLength = 3.f;

	/**
	 * BaseFOV, as the export has it. The field of view track does not count from it: the original classic build, watched
	 * playing CameraAnim_Teleport (BaseFOV 137.24), widens from the player's FOV instead of narrowing to 43°.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Anim")
	float BaseFOV = 90.f;

	/** BasePostProcessSettings: what the anim's camera starts with, override flags included. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Anim")
	FPostProcessSettings BasePostProcessSettings;

	/** BasePostProcessBlendWeight: the settings count for nothing at 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Anim")
	float BasePostProcessBlendWeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Anim")
	TArray<FWasamiCameraAnimFloatTrack> FloatTracks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Anim")
	TArray<FWasamiCameraAnimColorTrack> ColorTracks;

	/**
	 * The Matinee move track's six axes, keyed in seconds and in the order UInterpTrackMove makes them: the three
	 * translations, then the rotation's X, Y and Z. Empty for an anim whose camera stays at the origin (the powers').
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Anim")
	TArray<FInterpCurveFloat> MoveCurves;

	/** How many curves a move track has, one per axis. */
	static constexpr int32 MoveCurveCount = 6;

	/** Whether the anim has a move track to offset a camera by. */
	bool HasMoveTrack() const { return MoveCurves.Num() == MoveCurveCount; }

	/**
	 * Where the move track has put the camera at Time, in the camera's own space: the three translations, and the
	 * rotation as UInterpTrackMove reads its axes (an euler it turns into FRotator(Y, Z, X)). Identity without a
	 * move track.
	 */
	FTransform EvalMove(float Time) const;

	/**
	 * The move track at Time as a change from its first key, which is what UE4 put on the view: a CameraAnim's
	 * bRelativeToInitialTransform is true unless set otherwise (the capture scene's CameraAnim_Nurse_01 does not
	 * set it), and APlayerCameraManager then applied the anim camera's transform relative to its initial one.
	 * Identity at the start.
	 */
	FTransform EvalRelativeMove(float Time) const;
};

/**
 * The timing of a playing CameraAnim, as UE4's UCameraAnimInst kept it: the time in the anim, the linear blend in and
 * out (the weight is the smaller of the two), the end of a non-looping anim, and a Duration that includes the blend
 * out. UE 5.8's successor (CameraAnimationCameraModifier.cpp) blends the same way.
 */
struct WASAMI_DECEPTION_API FWasamiCameraAnimPlayback
{
	void Start(float InAnimLength, float InRate, float InScale, float InBlendInTime, float InBlendOutTime, bool bInLoop, float Duration);

	/** Moves on by DeltaTime and works out Weight; the anim may finish here. */
	void Advance(float DeltaTime);

	/** Starts the blend out, or ends at once when bImmediate or when there is no blend out. */
	void Stop(bool bImmediate);

	float AnimLength = 0.f;
	float PlayRate = 1.f;
	float Scale = 1.f;
	float BlendInTime = 0.f;
	float BlendOutTime = 0.f;
	bool bLoop = false;

	float CurTime = 0.f;
	float CurBlendInTime = 0.f;
	float CurBlendOutTime = 0.f;
	/** Time left until the Duration's blend out starts (0 = no Duration). */
	float RemainingTime = 0.f;
	bool bBlendingIn = false;
	bool bBlendingOut = false;
	bool bFinished = true;
	/** The blend weight times the scale, for this frame. */
	float Weight = 0.f;
};

USTRUCT()
struct FWasamiCameraAnimInstance
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UWasamiCameraAnim> Anim;

	int32 Handle = 0;
	FWasamiCameraAnimPlayback Playback;
	/** The field of view track's value when the anim started: the track adds only its change from there. */
	float InitialFOV = 0.f;
};

/**
 * Plays UWasamiCameraAnims on a player's camera, in place of UE4's APlayerCameraManager::PlayCameraAnim. Each frame
 * every anim's post-process settings go to the camera manager at the anim's weight, under the view target's own
 * (VTBlendOrder_Base, where UE 4 put camera anims and where UE 5's successor still does by default), and its field of
 * view track adds its change since the start, times the weight, to the view's FOV (UE4's bRelativeToInitialFOV, which
 * the exports leave at its default).
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiCameraAnimModifier : public UCameraModifier
{
	GENERATED_BODY()

public:
	/** The camera manager's modifier, added the first time it is asked for. */
	static UWasamiCameraAnimModifier* Get(APlayerCameraManager* CameraManager);

	/** PlayCameraAnim(Anim, Rate, Scale, BlendInTime, BlendOutTime, bLoop, bRandomStartTime false, Duration). */
	int32 Play(UWasamiCameraAnim* Anim, float Rate, float Scale, float BlendInTime, float BlendOutTime, bool bLoop, float Duration);

	/** UCameraAnimInst::Stop: blends out, or ends at once with bImmediate. An ended handle does nothing. */
	void Stop(int32 Handle, bool bImmediate);

	bool IsPlaying(int32 Handle) const;

	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

	/** The view's FOV with a field of view track's value added as its change from InitialFOV: 5–170°, as UE4 kept it. */
	static float AddFieldOfView(float ViewFOV, float TrackFOV, float InitialFOV, float Weight);

private:
	UPROPERTY(Transient)
	TArray<FWasamiCameraAnimInstance> Instances;

	int32 NextHandle = 1;
};

/**
 * What the original's MovieSceneCameraAnimTrack does to a camera a sequence binds, which UE 5.8's Sequencer no longer
 * has: while the scene's sequence is inside the track's section, the CameraAnim's move track offsets the view the
 * camera gives, in the camera's own space (UE4's FMovieSceneAdditiveCameraAnimationTrackExecutionToken →
 * FCameraAnimationHelper::ApplyOffset). dd_sequence places one of these for the capture scene's CameraAnim_Nurse_01
 * and fills it in; UWasamiCameraAnimOffsetModifier applies it.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiCameraAnimOffset : public AActor
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** The anim whose move track is the offset. */
	UPROPERTY(EditAnywhere, Category = "Camera Anim")
	TObjectPtr<UWasamiCameraAnim> Anim;

	/** The camera the original's track is on; the offset counts only while the view looks through it. */
	UPROPERTY(EditAnywhere, Category = "Camera Anim")
	TObjectPtr<ACameraActor> Camera;

	/** The actor that plays the scene; its player's time says where in the section we are. */
	UPROPERTY(EditAnywhere, Category = "Camera Anim")
	TObjectPtr<ALevelSequenceActor> Sequence;

	/** The section's start in the sequence (s), which is also where the anim's own time counts from. */
	UPROPERTY(EditAnywhere, Category = "Camera Anim")
	float StartTime = 0.f;

	/** The section's end in the sequence (s); the view is the camera's own outside the section. */
	UPROPERTY(EditAnywhere, Category = "Camera Anim")
	float EndTime = 0.f;

	/** Whether a sequence time falls in the section. */
	bool IsInSection(float Time) const { return Time >= StartTime && Time <= EndTime; }

	/** The offset the anim has this frame, or false when the scene is not playing that part of it. */
	bool CurrentOffset(FTransform& OutOffset) const;
};

/**
 * Applies the offsets of the AWasamiCameraAnimOffsets that are running to the player's view, where UE4's Sequencer
 * applied a camera anim's.
 *
 * UE4 handed the offset to the bound camera's component (UCameraComponent::AddAdditiveOffset) together with the
 * camera shakes the same sequence played, all accumulated into the one additive offset. UE 5.8 keeps that accumulator
 * for shakes alone (FAccumulatedShake in MovieSceneCameraShakeSystem.cpp), and it clears the component's offset every
 * frame a shake section is open - which is every frame from 22.13 s in the capture scene, where the original's shake
 * starts on the same camera. So the anim's offset goes on the view instead, from a camera modifier, which runs on
 * what GetCameraView returned: the shake's offset is already in it and both end up on the view, as they did in UE4.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiCameraAnimOffsetModifier : public UCameraModifier
{
	GENERATED_BODY()

public:
	/** The camera manager's modifier, added the first time it is asked for. */
	static UWasamiCameraAnimOffsetModifier* Get(APlayerCameraManager* CameraManager);

	void Add(AWasamiCameraAnimOffset* Offset);
	void Remove(AWasamiCameraAnimOffset* Offset);

	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

	/** The view with an offset applied in its own space (FCameraAnimationHelper::ApplyOffset). */
	static void ApplyOffset(const FTransform& Offset, FMinimalViewInfo& InOutPOV);

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AWasamiCameraAnimOffset>> Offsets;
};
