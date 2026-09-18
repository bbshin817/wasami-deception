#pragma once

#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiEnemyAnimInstance.h"
#include "WasamiGarageLift.generated.h"

class UAnimSequence;
class UAudioComponent;
class UBoxComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class USoundAttenuation;
class USoundBase;

/**
 * Dark Deception's BP_06_GarageLift (pak_reference_2's Blueprints/06_Hospital/Lifts/Garage): a car lift whose platform
 * rises about 3 m while the player stands on it and drops back when the player steps off. Two stand in Zone 2's garage.
 *
 * The lift is a skeletal mesh (hospital_garage_lift_anim at scale 30) with the platform's boxes on its bone joint4, so
 * the animation carries them: its anim instance (UWasamiGarageLiftAnimInstance, the original's ABP) reads Player
 * Overlapping? every update and plays hospital_garage_lift_anim_Anim up while it is true, and the platform's rising and
 * falling play Audio (DD_TT_GarageLift_Up) and Audio1 (DD_TT_GarageLift_Down).
 *
 * The mesh and sounds are soft references loaded as the components register and in BeginPlay (assets under /Game/DD
 * are never loaded from a constructor, WasamiAssets.h).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiGarageLift : public AActor
{
	GENERATED_BODY()

public:
	AWasamiGarageLift();

	/** Player Overlapping?: the player character overlaps Overlap Box. The anim instance reads it every update. */
	UFUNCTION(BlueprintPure, Category = "Garage Lift")
	virtual bool IsPlayerOverlapping() const;

	USkeletalMeshComponent* GetSkeletalMesh() const { return SkeletalMesh; }
	UBoxComponent* GetBox() const { return Box; }
	UBoxComponent* GetOverlapBox() const { return OverlapBox; }
	UAudioComponent* GetAudio() const { return Audio; }
	UAudioComponent* GetAudio1() const { return Audio1; }

	/** The bone the platform's boxes hang from. */
	static const FName PlatformBone;

protected:
	virtual void PreRegisterAllComponents() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Garage Lift")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** SkeletalMesh: the lift at scale 30, without collision of its own, animated by UWasamiGarageLiftAnimInstance. */
	UPROPERTY(VisibleAnywhere, Category = "Garage Lift")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	/** Box: the platform's solid box on joint4, which blocks pawns only. */
	UPROPERTY(VisibleAnywhere, Category = "Garage Lift")
	TObjectPtr<UBoxComponent> Box;

	/** Overlap Box: the box on joint4, over the platform, where the player counts as on it. */
	UPROPERTY(VisibleAnywhere, Category = "Garage Lift")
	TObjectPtr<UBoxComponent> OverlapBox;

	/** Audio: the rise (DD_TT_GarageLift_Up through 01_Lobby_Attenuation). */
	UPROPERTY(VisibleAnywhere, Category = "Garage Lift")
	TObjectPtr<UAudioComponent> Audio;

	/** Audio1: the fall (DD_TT_GarageLift_Down through 01_Lobby_Attenuation). */
	UPROPERTY(VisibleAnywhere, Category = "Garage Lift")
	TObjectPtr<UAudioComponent> Audio1;

	UPROPERTY(EditAnywhere, Category = "Garage Lift|Assets")
	TSoftObjectPtr<USkeletalMesh> MeshAsset;

	UPROPERTY(EditAnywhere, Category = "Garage Lift|Assets")
	TSoftObjectPtr<USoundBase> UpSound;

	UPROPERTY(EditAnywhere, Category = "Garage Lift|Assets")
	TSoftObjectPtr<USoundBase> DownSound;

	UPROPERTY(EditAnywhere, Category = "Garage Lift|Assets")
	TSoftObjectPtr<USoundAttenuation> SoundAttenuation;
};

/**
 * Dark Deception's BP_06_GarageLift_Zone1_Special: the garage lift in Zone 1's car park, which does not rise while a
 * nurse is near (Zone 1's level Blueprint, AWasamiZone1Flow, sets NurseNear for good when a nurse enters TriggerVolume_1).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiGarageLiftZone1Special : public AWasamiGarageLift
{
	GENERATED_BODY()

public:
	/** NurseNear: the lift counts nobody on it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Lift")
	bool bNurseNear = false;

	virtual bool IsPlayerOverlapping() const override;
};

/** Hands the frame's two times and the rise's weight to the worker thread, which blends the animation's poses. */
USTRUCT()
struct FWasamiGarageLiftAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FWasamiGarageLiftAnimInstanceProxy() = default;
	explicit FWasamiGarageLiftAnimInstanceProxy(UAnimInstance* InAnimInstance) : FAnimInstanceProxy(InAnimInstance) {}

protected:
	virtual void PreEvaluateAnimation(UAnimInstance* InAnimInstance) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	const UAnimSequence* Sequence = nullptr;
	float PlayerOnTime = 0.f;
	float PlayerOnWeight = 0.f;
};

/**
 * The garage lift's animation, after Dark Deception's hospital_garage_lift_anim_Skeleton_AnimBlueprint: a native anim
 * instance with the ABP's one state machine. Default holds the animation's first frame (a sequence player at play rate
 * 0); PlayerOn plays it once at play rate 1 and holds its last frame. PlayerOn? (the owner's Player Overlapping?,
 * written every update) enters PlayerOn and its negation leaves it, each over a 0.2 s HermiteCubic crossfade, so the
 * platform drops in 0.2 s. PlayerOn starts from the first frame again only when it enters with no weight left (the state
 * machine initialises a state that is not already active). Entering PlayerOn plays the lift's Audio; leaving it fades
 * Audio out over 0.25 s and plays Audio1 (the state's start and end notifies, Player On Event and Player Off Event).
 *
 * The animation loads when a game world starts it (never at the editor's start up, WasamiAssets.h), so in the editor's
 * level the lift shows its reference pose.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiGarageLiftAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** PlayerOn?: the owner's Player Overlapping? this update. */
	UPROPERTY(BlueprintReadOnly, Category = "Garage Lift")
	bool bPlayerOn = false;

	/** The state machine's current state is PlayerOn. */
	bool IsInPlayerOn() const { return StateBlend.bInB; }
	/** PlayerOn's weight in the pose (the rest is Default's first frame). */
	float GetPlayerOnWeight() const { return StateBlend.WeightB; }
	/** PlayerOn's sequence player's time (s). */
	float GetPlayerOnTime() const { return PlayerOnTime; }
	/** The animation's length (s), or 0 without it. */
	float GetLength() const;

	/** The transitions' crossfade (both ways). */
	static constexpr float CrossfadeDuration = 0.2f;
	/** Player Off Event's fade of Audio. */
	static constexpr float UpFadeOutSeconds = 0.25f;

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

private:
	friend struct FWasamiGarageLiftAnimInstanceProxy;

	/** AnimNotify_Player On Event: the owner's Audio plays. */
	void PlayerOnEvent();
	/** AnimNotify_Player Off Event: the owner's Audio fades out and Audio1 plays. */
	void PlayerOffEvent();

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> Sequence;

	FWasamiStateBlend StateBlend;
	float PlayerOnTime = 0.f;
	/** The state machine's bSkipFirstUpdateTransition: the first update takes no transition. */
	bool bUpdated = false;
};
