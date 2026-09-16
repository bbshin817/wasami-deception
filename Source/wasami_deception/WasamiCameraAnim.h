#pragma once

#include "Camera/CameraModifier.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Scene.h"
#include "Math/InterpCurve.h"
#include "WasamiCameraAnim.generated.h"

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
 * those settings. UWasamiCameraAnimModifier plays it.
 */
UCLASS(BlueprintType)
class WASAMI_DECEPTION_API UWasamiCameraAnim : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Writes every post-process track's value at Time into Settings (the tracks leave the bOverride flags alone). */
	void ApplyPostProcessTracks(float Time, FPostProcessSettings& Settings) const;

	/** AnimLength (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Anim")
	float AnimLength = 3.f;

	/** BaseFOV: the FOV the anim's camera starts from. */
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
};

/**
 * Plays UWasamiCameraAnims on a player's camera, in place of UE4's APlayerCameraManager::PlayCameraAnim. Each frame
 * every anim's post-process settings go to the camera manager at the anim's weight, under the view target's own
 * (VTBlendOrder_Base, where UE 4 put camera anims and where UE 5's successor still does by default).
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

private:
	UPROPERTY(Transient)
	TArray<FWasamiCameraAnimInstance> Instances;

	int32 NextHandle = 1;
};
