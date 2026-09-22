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
class USoundBase;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * The player caught: a black room far above the level where the enemy Wasami plays one of its capture clips at the
 * camera. Three of them follow the Death Event of Dark Deception's hotel (01_Hotel's level Blueprint, pak_reference
 * @63919), whose monkey jumps at a camera of its own in one of three Matinees: the player's input off, the tablet put
 * down, the view cut to the room's camera (the Matinee's director cuts, nothing blends), JumpscareShake at 0.3, the
 * Matinee's camera moves and its fade to black, and 3.5 s later the game mode's DeathEvent (the death screen and the
 * pause). The fourth is the manor's Gold Watcher (BP_03_Watcher's kill that grabs and laughs): its camera anim
 * 03_Watcher_Kill3 in front of the Wasami's face as it rushes in, a cut to black and DeathEvent at once 1.15 s on.
 *
 * The room is the hotel's jumpscare scene measured from the monkey's feet (the JumpscareMonkey, scale 4, and the first
 * Matinee at t = 0: its camera and the ceiling light above), shrunk by the monkey's height to the enemy's.
 *
 * Every clip plays on a scene time that starts StartRate times as fast and eases back to 1 (TODO(仮): the user's
 * "最初は高速で、イージングで徐々に等速に戻る"); the hotel's camera keys and fades run on it too.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiCapture : public AActor
{
	GENERATED_BODY()

public:
	AWasamiCapture();

	/** How many captures there are to pick from: Capture_1 to Capture_3 with the hotel's Matinees, and the face. */
	static constexpr int32 NumChoices = 4;

	/** The captures after the hotel's three Matinees (Capture_1 to Capture_3, the choices below FaceChoice). */
	static constexpr int32 NumHotelChoices = 3;

	/** The manor's Gold Watcher kill with the Wasami's face (the Run clip). */
	static constexpr int32 FaceChoice = 3;

	/** The hotel's Death Event: Delay 3.5 before the death screen. */
	static constexpr float DeathDelay = 3.5f;

	/** The hotel's ClientPlayCameraShake(JumpscareShake, 0.3). */
	static constexpr float ShakeScale = 0.3f;

	/**
	 * BP_03_Watcher's grab and laugh (@1474): 0.2 s after its catch, watcher_kill_grablaugh and 03_Watcher_Kill3 (rate
	 * 1, scale 1, no blends); 0.85 s on the axe's hit; 0.1 s on UMG_BlackFade_3 (black at once) and the level's Death
	 * Event, which puts the death screen up at once.
	 */
	static constexpr float WatcherAnimDelay = 0.2f;
	static constexpr float WatcherHitDelay = 0.85f;
	static constexpr float WatcherBlackDelay = 0.1f;
	static constexpr float FaceDeathDelay = WatcherAnimDelay + WatcherHitDelay + WatcherBlackDelay;

	/** The Gold Watcher's own Camera: 121.465 cm in front of its capsule's centre, FOV 75. */
	static constexpr double WatcherCameraForward = 121.46516418457031;
	static constexpr float WatcherFieldOfView = 75.f;

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
	 * The room's camera from the Wasami's feet at t = 0. TODO(仮): the hotel's camera frames the monkey's head (the
	 * camera level with it 0.534 of the way up, as far away as 0.96 of its size); this one frames the Wasami's upper
	 * half (WasamiTop / 2 up to WasamiTop) the same way, its offsets grown by that half over the monkey's head.
	 */
	static const double FrameScale;
	static const FVector CameraOffset;

	/** JumpscareCam's field of view (the CameraComponent's default; the Matinee's FOVAngle track has no keys). */
	static constexpr float FieldOfView = 90.f;

	/**
	 * JumpscareCam's depth of field, the same in all three Matinees (InterpTrackFloatProp_1 and _2 on its
	 * CameraComponent.PostProcessSettings): the focal distance holds at 142.943 cm to 0.128 s and falls to 10 by
	 * 0.2248 s, the focal region 571.429 to 100 by 0.2315 s — the near edge of the band that stays sharp, and how
	 * deep it is. The method is not the camera's own: 01_Hotel's bUnbound PostProcessVolume_1 lays DOFM_Gaussian and
	 * an Fstop of 4 over the whole level, and the Matinee's camera overrides neither.
	 */
	static constexpr float DofFocalNear = 142.94285583496094f;
	static constexpr float DofFocalNearEnd = 10.f;
	static constexpr float DofFocalNearHold = 0.12800943851470947f;
	static constexpr float DofFocalNearTime = 0.22483110427856445f;
	static constexpr float DofFocalDepth = 571.4285888671875f;
	static constexpr float DofFocalDepthEnd = 100.f;
	static constexpr float DofFocalDepthTime = 0.23150849342346191f;
	static constexpr float DofFstop = 4.f;

	/**
	 * Where the hotel choices' camera focuses MatineeTime in (cm from the camera, in this room's scale). UE 5.8 has no
	 * Gaussian depth of field off mobile (Scene.h keeps the focal region, the transition regions and the blur sizes
	 * under Lens|Mobile Depth of Field), so the two tracks go to the cinematic focal distance instead. TODO(仮): the
	 * middle of the band the original holds sharp (the near edge plus half the depth, 428.66 cm at the start and 60 cm
	 * once it is through), grown by FrameScale as this room's camera is. The original's blur is there to put the hotel
	 * room behind the monkey away; this room is black already, so what is left of it falls on the Wasami alone.
	 */
	static float HotelFocalDistance(float MatineeTime);

	/**
	 * ceilinglights_80: 1500 (Unitless), radius 500, source radius 24.715, colour (255, 236, 142), the export's
	 * [B, G, R, A] turned round. The room is the hotel's scene at SceneScale, so its lengths are put down at
	 * SceneScale and its strength at the square of it: a light that much nearer in a room that much smaller lays the
	 * same light on the Wasami as the hotel's does on the monkey (it falls as one over the distance squared).
	 */
	static constexpr float LightIntensity = 1500.f;
	static constexpr float LightRadius = 500.f;
	static constexpr float LightSourceRadius = 24.715225219726562f;
	static const FColor LightColor;

	/**
	 * jumpscarelight and jumpscarelight_5, the two the hotel's monkey wears on its eyes (no Matinee touches them):
	 * point lights on its Monkey_Head_TopSHJnt socket, 15 (Unitless) and 15 cm of reach each, no shadows, scaled 0.25
	 * against the monkey's 4 so they stand at world scale. In the monkey's mesh they sit (±17.7, 47.5, -34.1) from
	 * that joint at its scale 4 — across, forward and down — which is 3.8 cm out from its face, level with its eyes.
	 */
	static constexpr int32 NumFaceLights = 2;
	static constexpr float FaceLightIntensity = 15.f;
	static constexpr float FaceLightRadius = 15.f;

	/** SK_WasamiEnemy's head in its bind pose, from its head joint up to head_end, at AWasamiEnemy::MeshScale. */
	static const double WasamiHeadBase;
	static const double WasamiHeadTop;

	/**
	 * How big the Wasami's head is beside the monkey's (33.0 cm against 98.5). The eye lights' reach goes by it and
	 * their strength by the square of it, so their glow covers as much of this face as the original's does of the
	 * monkey's, as bright.
	 */
	static const double FaceScale;

	/**
	 * TODO(仮): where those two sit on the Wasami's face, in SK_WasamiEnemy's bind pose (its mesh's frame, +Y
	 * forward). No pair of joints carries the monkey's places over — its head_end leans 1.8 cm off the midline and
	 * the two faces are shaped differently — so the shape of the heads does it: across and up they are where the
	 * monkey's are in the box its own head fills (0.631 and 0.366 across, 0.511 and 0.517 up, both boxes taken from
	 * the head joint up); forward they stand 0.93 out of the face they land on (10.57 there), the monkey's own 0.945
	 * of clearance shrunk by the heads' ratio. That puts them on the eye sockets either side of the nose, which is as
	 * near the eyes as the model can say — they are painted in its one baked texture, and it has no eye joints.
	 */
	static const FVector FaceLightPlaces[NumFaceLights];

	/**
	 * The Matinee a hotel choice is paired with (0 MonkeyJumpscare, 1 MonkeyJumpscare2, 2 MonkeyJumpscare3). TODO(仮):
	 * MonkeyJumpscare with Capture_1 (the back flip); MonkeyJumpscare2, whose camera falls to the floor and looks up,
	 * with Capture_3 (the walk at the camera); MonkeyJumpscare3, whose camera stands, with Capture_2 (the slide, whose
	 * Wasami turns its back to the camera once up).
	 */
	static int32 MatineeFor(int32 Choice);

	/** The fade to black of the Matinee a hotel choice is paired with: its InterpTrackFade's two keys as shares of its length. */
	static float FadeStartShare(int32 Choice);
	static float FadeEndShare(int32 Choice);

	/** The length (InterpLength) of the Matinee a hotel choice is paired with. */
	static float MatineeLength(int32 Choice);

	/**
	 * The camera of the Matinee a hotel choice is paired with (MatineeFor) at MatineeTime, as the hotel has it (world,
	 * its Euler track as a rotator): MonkeyJumpscare's and MonkeyJumpscare2's NewCameraGroup keys as saved. TODO(仮):
	 * MonkeyJumpscare3's holds still from 0.204 s to 2.572 s while five monkeys jump in; one Wasami does not fill that,
	 * so its hold takes MonkeyJumpscare's sway (its keys from 0.204 s to 1.522 s, stretched 1.797 times).
	 */
	static void EvaluateHotelCamera(int32 Choice, float MatineeTime, FVector& OutLocation, FRotator& OutRotation);

	/** 03_Watcher_Kill3 at Time (its length 1.019 s; held after): the camera's move from its first key, and its turn. */
	static void EvaluateWatcherCamera(float Time, FVector& OutMove, FRotator& OutTurn);
	static const float WatcherKill3Length;

	/**
	 * Starts a capture unless the game mode's death is closed or one is already going: spawns the room and Starts it.
	 * Choice 0 to NumChoices - 1 forces one; any other takes the next from the game instance's bag. Returns the room,
	 * or null.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wasami|Capture", meta = (WorldContext = "WorldContextObject"))
	static AWasamiCapture* StartCapture(const UObject* WorldContextObject, AActor* Cause, int32 Choice = -1);

	/**
	 * The Death Event from Put Down Tablet on: plays the capture Choice (0 to NumChoices - 1), and calls Mode's
	 * DeathEvent(Cause) after DeathDelay (the face: FaceDeathDelay). Without a player (the tests), only the room plays.
	 */
	void Start(AWasamiGameMode* InMode, AActor* Cause, int32 Choice);

	/** The capture playing (0 to NumChoices - 1), or INDEX_NONE before Start. */
	int32 GetChoice() const { return Choice; }

	/** The fade to black: when it starts and how long it takes (s, from Start; the face cuts, 0 long). */
	float GetFadeStart() const { return FadeStart; }
	float GetFadeDuration() const { return FadeDuration; }

	/**
	 * How black the screen is Alpha of the way through the fade (0 to 1). The Matinees' InterpTrackFade holds two
	 * auto-clamped keys, 0 and 1, whose tangents both come out 0 (Matinee auto-sets its tangents with stationary
	 * endpoints), so the cubic between them is 3a² - 2a³ — not the camera manager's straight line.
	 */
	static float FadeCurve(float Alpha);

	/** How black the screen is now (0 to 1): 0 before the fade, 1 once it is through, and held there. */
	float GetFadeAmount() const { return FadeAmount; }

	/** Where the clip started (s into it), and how fast the Matinee's camera goes on the scene's time. */
	float GetClipStart() const { return ClipStart; }
	float GetMatineeRate() const { return MatineeRate; }

	/** When DeathEvent comes (s, from Start). */
	float GetDeathDelay() const { return Choice == FaceChoice ? FaceDeathDelay : DeathDelay; }

	/**
	 * How many sounds a capture plays, and when (s, from Start; every one at volume 1 and pitch 1, as the original has
	 * them). The hotel's three Matinees all open with InterpTrackSound_0's Evil_Monkey_Scream at t = 0; their other
	 * sound tracks are left out, having nothing here to go with — the knife stabs of MonkeyJumpscare2 (0.484 s) and
	 * MonkeyJumpscare3 (2.437 s), which the Wasami's clips do not make, and the four small screams MonkeyJumpscare3
	 * lays under the five monkeys that jump in with its own. The face follows BP_03_Watcher's kill (@1474):
	 * PlaySound2D(LIVING_STATUE_Laughter_05) with the grab at WatcherAnimDelay, PlaySound2D(Axe_Hit_03) after its
	 * WatcherHitDelay.
	 */
	static int32 NumSounds(int32 Choice);
	static float SoundTime(int32 Choice, int32 Index);

	/** The Index-th sound of the capture Choice, loaded, or null. */
	USoundBase* GetSound(int32 Choice, int32 Index) const;

	/** How long (s) until the Index-th sound is played; 0 once it has been, or before Start. */
	float GetSoundDelay(int32 Index) const;

	/** The scene time at Time (s, from Start): StartRate times as fast at first, easing back to 1 over RateEaseTime. */
	float SceneTime(float Time) const;

	/** The time (s, from Start) the scene reaches Scene. */
	float RealTime(float Scene) const;

	USkeletalMeshComponent* GetBody() const { return Body; }
	UCameraComponent* GetView() const { return View; }
	UPointLightComponent* GetLight() const { return Light; }
	const TArray<TObjectPtr<UPointLightComponent>>& GetFaceLights() const { return FaceLights; }
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetWalls() const { return Walls; }

	/** JumpscareShake (import_dd_camera_shakes). */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Capture")
	TSoftClassPtr<UCameraShakeBase> ShakeClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Capture")
	TSoftObjectPtr<USkeletalMesh> BodyMesh;

	/**
	 * jumpscareblock's plane, and M_WasamiCaptureBlack for its BlackUnlitMaterial: the engine's debug material
	 * is editor-only and never cooked, which left the walls checkered in a packaged build (import_wasami_enemy makes
	 * the same material -- unlit, emissive 0 -- in this game's content).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Capture")
	TSoftObjectPtr<UStaticMesh> WallMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Capture")
	TSoftObjectPtr<UMaterialInterface> WallMaterial;

	/**
	 * What the capture plays, all 2D: the listener follows the view, which is up in the room, and the original plays
	 * the watcher's two through PlaySound2D as well. The hotel Matinees' Evil_Monkey_Scream, and the Gold Watcher's
	 * LIVING_STATUE_Laughter_05 and Axe_Hit_03.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Capture")
	TSoftObjectPtr<USoundBase> ScreamSound;

	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Capture")
	TSoftObjectPtr<USoundBase> LaughSound;

	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Capture")
	TSoftObjectPtr<USoundBase> HitSound;

	/** TODO(仮): the scene's rate at first, and how long (s) it takes to ease back to 1 (exponentially). */
	UPROPERTY(EditAnywhere, Category = "Wasami|Capture")
	float StartRate = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Wasami|Capture")
	float RateEaseTime = 0.4f;

	/**
	 * TODO(仮): where each hotel clip starts (s), skipping the Wasami's approach from far off: Capture_2's slide from
	 * some 4.8 m before the mark (of 7.3 m by its fade), Capture_3's walk from 1.3 m (of 2.2 m).
	 */
	UPROPERTY(EditAnywhere, Category = "Wasami|Capture")
	TArray<float> ClipStarts;

	/** The hotel choices' camera at t = 0, from the Wasami's feet (CameraOffset). */
	UPROPERTY(EditAnywhere, Category = "Wasami|Capture")
	FVector CameraStart = FVector::ZeroVector;

	/**
	 * TODO(仮): the hotel choices' camera follows this bone of the Wasami (the victim's eyes follow it; the hotel's
	 * monkey stands where it is, the Wasami's clips flip, slide and walk): the turn from where the bone is when the
	 * Wasami stands at the mark (AimRest: neck_01 in Capture_1 at t = 0) to where it is, seen from CameraStart, catching
	 * up at AimSpeed (RInterpTo), on top of the Matinee's own turns. The camera keeps AimGap in front of it on X.
	 */
	UPROPERTY(EditAnywhere, Category = "Wasami|Capture")
	FName AimBone = TEXT("neck_01");

	UPROPERTY(EditAnywhere, Category = "Wasami|Capture")
	FVector AimRest = FVector(10.6, -3.4, 186.8);

	UPROPERTY(EditAnywhere, Category = "Wasami|Capture")
	float AimSpeed = 8.f;

	UPROPERTY(EditAnywhere, Category = "Wasami|Capture")
	float AimGap = 50.f;

	/**
	 * TODO(仮): the face's camera at t = 0 from the Wasami's feet (the Gold Watcher's is 121.465 cm in front of its
	 * capsule, level with its head; the Wasami's head runs at some 186 cm), looking back along -X as the watcher's does.
	 */
	UPROPERTY(EditAnywhere, Category = "Wasami|Capture")
	FVector FaceCameraOffset = FVector(WatcherCameraForward, 0., 186.);

	/** TODO(仮): the face's Wasami rushes in from FaceRushDistance back, the distance left falling by e every FaceRushTime. */
	UPROPERTY(EditAnywhere, Category = "Wasami|Capture")
	float FaceRushDistance = 250.f;

	UPROPERTY(EditAnywhere, Category = "Wasami|Capture")
	float FaceRushTime = 0.1f;

	/** TODO(仮): how near (cm, on X) the face's camera may come to the Wasami's head bone. */
	UPROPERTY(EditAnywhere, Category = "Wasami|Capture")
	float FaceGap = 40.f;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	/** Puts the clip, the Wasami and the camera where they are Time into the capture. */
	void UpdateScene(float Time, float DeltaSeconds);

	/** Puts the eye lights on the face, their places turned into the head bone's own frame by the bind pose. */
	void PlaceFaceLights();

	/** Plays the Index-th sound of the capture going on (the timers Start sets, and t = 0 at once). */
	void PlayCaptureSound(int32 Index);

	void StartFade();

	/** Puts the screen's black at Amount (0 to 1) and keeps it there, as the Matinee's bPersistFade does. */
	void ApplyFade(float Amount);

	void EndCapture();

	UPROPERTY(VisibleAnywhere, Category = "Wasami|Capture")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Wasami|Capture")
	TObjectPtr<USkeletalMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Wasami|Capture")
	TObjectPtr<UCameraComponent> View;

	UPROPERTY(VisibleAnywhere, Category = "Wasami|Capture")
	TObjectPtr<UPointLightComponent> Light;

	/** The eyes' two lights, on the head bone as the monkey's are on its own. */
	UPROPERTY(VisibleAnywhere, Category = "Wasami|Capture")
	TArray<TObjectPtr<UPointLightComponent>> FaceLights;

	/** The black planes: floor, ceiling, back, front, left, right, each facing in. */
	UPROPERTY(VisibleAnywhere, Category = "Wasami|Capture")
	TArray<TObjectPtr<UStaticMeshComponent>> Walls;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> Clip;

	TWeakObjectPtr<AWasamiGameMode> Mode;
	TWeakObjectPtr<AActor> CauseActor;
	int32 Choice = INDEX_NONE;
	float ClipLength = 0.f;
	float ClipStart = 0.f;
	float MatineeRate = 0.f;
	/** Where the clip's Wasami starts (the room's frame). */
	FVector BodyStart = FVector::ZeroVector;
	float Elapsed = 0.f;
	/** Where the camera's eyes look at the Wasami (from CameraStart), catching up. */
	FRotator Aim = FRotator::ZeroRotator;
	bool bAimSet = false;
	float FadeStart = 0.f;
	float FadeDuration = 0.f;
	/** The fade going on: how far into it (s) and how black the screen is now. */
	float FadeTime = 0.f;
	float FadeAmount = 0.f;
	bool bFading = false;
	FTimerHandle FadeTimer;
	FTimerHandle DeathTimer;
	/** One per sound of the capture going on (NumSounds); the ones due at t = 0 stay unset. */
	TArray<FTimerHandle> SoundTimers;
};
