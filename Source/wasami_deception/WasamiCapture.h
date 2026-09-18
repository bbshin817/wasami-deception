#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiCapture.generated.h"

class AWasamiGameMode;
class UAnimSequence;
class UCameraComponent;
class UCameraShakeBase;
class UMaterialInterface;
class UPointLightComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * The player caught: a black room far above the level where the enemy Wasami plays one of its three capture clips at the
 * camera, after the Death Event of Dark Deception's hotel (01_Hotel's level Blueprint, pak_reference @63919), whose
 * monkey jumps at a camera of its own in one of three Matinees. Start does what that event does: the player's input
 * off, the tablet put down, the view cut to the room's camera (the Matinee's director cuts, nothing blends), a clip
 * picked by Random Integer In Range (No Repeat) and played once, JumpscareShake at 0.3, and 3.5 s later the game
 * mode's DeathEvent (the death screen and the pause). The Matinee's fade to black is copied at the same share of the
 * clip, and held. The room's camera does not move (its moves are item 24).
 *
 * The room is the hotel's jumpscare scene measured from the monkey's feet (the JumpscareMonkey, scale 4, and the first
 * Matinee at t = 0: its camera and the ceiling light above), shrunk by the monkey's height to the enemy's.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiCapture : public AActor
{
	GENERATED_BODY()

public:
	AWasamiCapture();

	/** How many capture clips there are to pick from (Capture_1 to Capture_3). */
	static constexpr int32 NumChoices = 3;

	/** The Death Event's Delay before the death screen. */
	static constexpr float DeathDelay = 3.5f;

	/** ClientPlayCameraShake(JumpscareShake, 0.3). */
	static constexpr float ShakeScale = 0.3f;

	/** Where the room is spawned: above the levels, far from anything. */
	static const FVector RoomLocation;

	/**
	 * The top of the monkey the hotel's camera frames (the highest point of Monkey.psk, 65.788, at the JumpscareMonkey's
	 * scale 4) and of the enemy Wasami (SK_WasamiEnemy's 170.0 at AWasamiEnemy::MeshScale): the room is the hotel's
	 * scene grown by their ratio.
	 */
	static const double MonkeyTop;
	static const double WasamiTop;
	static const double SceneScale;

	/**
	 * The hotel's first Matinee at t = 0, from the monkey's feet in its own frame (it faces +X, as the room's Wasami
	 * does): the camera (InterpTrackMove_2's first key, yaw 180) and the ceiling light above (ceilinglights_80).
	 */
	static const FVector HotelCameraOffset;
	static const FVector HotelLightOffset;

	/** The monkey's head in Monkey.psk's bind pose, from its Neck_Top joint to its Head_Top, at scale 4. */
	static const double MonkeyHeadBase;
	static const double MonkeyHeadTop;

	/**
	 * The room's camera from the Wasami's feet. TODO(仮): the hotel's camera frames the monkey's head (the camera level
	 * with it, 0.534 of the way up, as far away as 0.96 of its size), and the Wasami's clips move the whole body (a back
	 * flip, a slide along the floor, a walk), so this one frames the whole Wasami the same way: its offsets grown by
	 * WasamiTop / the monkey's head.
	 */
	static const double FrameScale;
	static const FVector CameraOffset;

	/** JumpscareCam's field of view (the CameraComponent's default; the Matinee's FOVAngle track has no keys). */
	static constexpr float FieldOfView = 90.f;

	/** ceilinglights_80: 1500 (Unitless), radius 500, source radius 24.715, colour (142, 236, 255). */
	static constexpr float LightIntensity = 1500.f;
	static constexpr float LightRadius = 500.f;
	static constexpr float LightSourceRadius = 24.715225219726562f;
	static const FColor LightColor;

	/**
	 * The fade to black of the Matinee Choice is paired with (TODO(仮): by length, MonkeyJumpscare with Capture_1,
	 * MonkeyJumpscare2 with Capture_2, MonkeyJumpscare3 with Capture_3): its InterpTrackFade's two keys as shares of the
	 * Matinee's length.
	 */
	static float FadeStartShare(int32 Choice);
	static float FadeEndShare(int32 Choice);

	/**
	 * Starts a capture unless the game mode's death is closed or one is already going: spawns the room and Starts it.
	 * Choice 0 to 2 forces a clip; any other takes the next from the game instance's bag. Returns the room, or null.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wasami|Capture", meta = (WorldContext = "WorldContextObject"))
	static AWasamiCapture* StartCapture(const UObject* WorldContextObject, AActor* Cause, int32 Choice = -1);

	/**
	 * The Death Event from Put Down Tablet on: plays the clip Choice (0 to 2), and calls Mode's DeathEvent(Cause) after
	 * DeathDelay. Without a player (the tests), only the room plays.
	 */
	void Start(AWasamiGameMode* InMode, AActor* Cause, int32 Choice);

	/** The clip playing (0 to 2), or INDEX_NONE before Start. */
	int32 GetChoice() const { return Choice; }

	/** The clip's fade to black: when it starts and how long it takes (s, from Start). */
	float GetFadeStart() const { return FadeStart; }
	float GetFadeDuration() const { return FadeDuration; }

	USkeletalMeshComponent* GetBody() const { return Body; }
	UCameraComponent* GetView() const { return View; }
	UPointLightComponent* GetLight() const { return Light; }
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetWalls() const { return Walls; }

	/** JumpscareShake (import_dd_camera_shakes). */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Capture")
	TSoftClassPtr<UCameraShakeBase> ShakeClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Capture")
	TSoftObjectPtr<USkeletalMesh> BodyMesh;

	/** jumpscareblock's plane and its BlackUnlitMaterial. */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Capture")
	TSoftObjectPtr<UStaticMesh> WallMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Capture")
	TSoftObjectPtr<UMaterialInterface> WallMaterial;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	void StartFade();
	void EndCapture();

	UPROPERTY(VisibleAnywhere, Category = "Wasami|Capture")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Wasami|Capture")
	TObjectPtr<USkeletalMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Wasami|Capture")
	TObjectPtr<UCameraComponent> View;

	UPROPERTY(VisibleAnywhere, Category = "Wasami|Capture")
	TObjectPtr<UPointLightComponent> Light;

	/** The black planes: floor, ceiling, back, front, left, right, each facing in. */
	UPROPERTY(VisibleAnywhere, Category = "Wasami|Capture")
	TArray<TObjectPtr<UStaticMeshComponent>> Walls;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> Clip;

	TWeakObjectPtr<AWasamiGameMode> Mode;
	TWeakObjectPtr<AActor> CauseActor;
	int32 Choice = INDEX_NONE;
	float FadeStart = 0.f;
	float FadeDuration = 0.f;
	FTimerHandle FadeTimer;
	FTimerHandle DeathTimer;
};
