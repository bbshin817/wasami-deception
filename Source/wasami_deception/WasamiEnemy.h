#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Navigation/PathFollowingComponent.h"
#include "WasamiEnemyAnimInstance.h"
#include "WasamiEnemyInterface.h"
#include "WasamiEnemy.generated.h"

class UMaterialInterface;
class USkeletalMesh;
class USphereComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UWasamiEnemyAnimInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiEnemyCloseBy);

/**
 * 追跡中のランダムの動き (the item 26, the user's instruction of 2026-09-18): while the enemy chases, a chance to play
 * one of the six chase clips (Chase_PickUp to Chase_Slide) comes every 6 to 10 s — about every eight, the frequency
 * asked for — and each chance draws one of them, never the one played before. The enemy's decisions (Make Choice, every
 * half second) move the wait on; a chance it cannot take is either let go (Skip: something else plays once) or left
 * open until the way ahead clears.
 */
struct WASAMI_DECEPTION_API FWasamiChaseVariations
{
	/** The gap between two chances (s), drawn anew for each. */
	static constexpr float MinGap = 6.f;
	static constexpr float MaxGap = 10.f;
	/** What the wait is while no chase asks for chances. */
	static constexpr float NotChasing = -1.f;

	/** Starts the draws: the same Seed makes the same chances (0 is the seed, as FRandomStream takes it). */
	void Init(int32 Seed);

	/**
	 * Moves the wait on by Seconds. Answers the clip a chance has come for (a WasamiEnemyClip from ChasePickUp to
	 * ChaseSlide), or INDEX_NONE. The clip is drawn at each answer, so a chance left open — the caller plays nothing
	 * and calls neither Played nor Skip — may come out as another one. A chase that has just begun waits a whole gap
	 * before its first chance, and may open with any of the six.
	 */
	int32 Advance(bool bChasing, float Seconds);

	/** The clip Advance answered was played: the next chance is a gap away, and the next draw goes around this clip. */
	void Played(int32 Clip);

	/** The chance is let go without playing anything: the next one is a gap away. */
	void Skip() { Wait = DrawGap(); }

	/** What is left until the next chance (s): 0 while one stands open, NotChasing while there is no chase. */
	float GetWait() const { return Wait; }
	/** The clip the last chance played, or INDEX_NONE (also before the first chance of a chase). */
	int32 GetLast() const { return Last; }

private:
	float DrawGap() { return Random.FRandRange(MinGap, MaxGap); }
	/** One of the six at random, never Last. */
	int32 DrawClip();

	float Wait = NotChasing;
	int32 Last = INDEX_NONE;
	FRandomStream Random;
};

/**
 * The enemy Wasami, after Dark Deception's hospital nurse BP_06_ReaperNurse and its parent BP_DD_Character_Base
 * (pak_reference_2): a character with the Enemy tag and the enemy interface, the nurse's capsule, movement and mesh
 * placement, this game's Wasami (SK_WasamiEnemy) with UWasamiEnemyAnimInstance, and the nurse's AI: no behaviour
 * tree, only what Make Choice decides every 0.5 s from BeginPlay and the engine's AI MoveTo.
 *
 * Make Choice, as the nurse's: when State == Stun, it stops the movement at once (which also aborts the path the AI
 * follows), waits 17 s and sets State back to Patrol, once (a DoOnce: a stun sent while it waits changes nothing);
 * Set State only sets State, and the animation reads State == Stun every frame, as the nurse's ABP does. Otherwise it
 * first acts on Seen Player Recently (Chase Player and a retriggerable 3 s delay that forgets the player, or Not Seeing
 * Player), then sets it when Can See Player. Every decision asks for a new move, which aborts the one before and fails
 * its proxy: the next decision's move clears the Point Of Interest, and each one draws the random point again.
 *
 * Its Sphere catches the player: the nurse's BeginOverlap (@9818) removes every enemy and runs Jumpscare Handle; here
 * the capture (AWasamiCapture, the hotel's Death Event) plays it with a Wasami of its own.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiEnemy : public ACharacter, public IWasamiEnemyInterface
{
	GENERATED_BODY()

public:
	AWasamiEnemy();

	// The nurse's CDO and its components', but the speeds (below).
	static constexpr float CapsuleRadius = 34.f;
	static constexpr float CapsuleHalfHeight = 118.05821990966797f;
	static constexpr float MaxSpeed = 430.f;
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
	// Can See Player: the player within this angle (degrees) of its front, at any distance.
	static constexpr float ViewAngle = 100.f;
	// The RetriggerableDelay after Chase Player that forgets the player.
	static constexpr float ForgetSeconds = 3.f;
	// Generate Random Point's radius around the player (or itself when there is none).
	static constexpr float RandomPointRadius = 3000.f;
	// The AI MoveTo acceptance radii: to the player, to the Point Of Interest, to the random point.
	static constexpr float ChaseAcceptance = 5.f;
	static constexpr float PointOfInterestAcceptance = 5.f;
	static constexpr float RandomPointAcceptance = 50.f;
	// Sphere's radius (on CollisionCylinder, at its centre).
	static constexpr float SphereRadius = 54.92805862426758f;

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
	 * How long until the stun sets State back to Patrol: what is left of the 17 s, or, before they start, the time until
	 * they do (GetTimeToStunStart) and the 17 s. 0 when not stunned.
	 */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetStunTimeLeft() const;

	/** How long a stun set now waits before its 17 s start: to the next decision (the 06 nurse's next tick: none). */
	virtual float GetTimeToStunStart() const;

	/** The mesh's animation (PlayOnce and the rest), or null before it starts. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	UWasamiEnemyAnimInstance* GetEnemyAnim() const;

	/** The Sphere that catches the player. */
	USphereComponent* GetSphere() const { return Sphere; }

	/** 追跡中のランダムの動き (the item 26): the chances of the chase it is in, and what the last one played. */
	const FWasamiChaseVariations& GetChaseVariations() const { return ChaseVariations; }

	/** Whether a chase variation may play now: the parking lot's nurses do not while they stab at the doors. */
	virtual bool CanPlayChaseVariation() const { return true; }

	/**
	 * The way straight ahead of it is clear over Distance (cm), by a NavMesh ray from where it stands. The engine's ray
	 * is blocked by default, so without navigation data nothing is clear and no variation plays.
	 */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsWayAheadClear(float Distance) const;

	/** The map's mark (the nurse's StaticMesh), which the minimap shows only while a bonus shard's reveal adds it. */
	UStaticMeshComponent* GetMapMark() const { return MapMark; }

	/** The base's CanSpawn: false destroys the enemy as it begins play. The level's script sets it on its spawns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ExposeOnSpawn = true))
	bool bCanSpawn = false;

	/** The sentry's idle (BP_06_ReaperNurse_Sentry). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (ExposeOnSpawn = true))
	bool bAggressiveIdle = false;

	/** The chase after every shard runs with Run_Nightmare. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	bool bNightmare = false;

	/**
	 * Set Walk State's speeds (cm/s). Not the nurse's 350 and 800 (2026-09-19, the user: the enemy was too fast): the
	 * Murder Monkey's (BP_Monkey's Walk Speed 200 and Run Speed 430), as the WebGL version, so the player's walk (300) is
	 * caught and the sprint (600) gets away. MaxSpeed (the CDO's MaxWalkSpeed) is the chase's.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float NormalSpeed = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float SkateSpeed = 430.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy")
	bool bNormalWalk = false;

	/** The nurse's Seen Player Recently (its Chasing); Player Vanish clears it. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Enemy")
	bool bSeenPlayerRecently = false;

	/** Where Chase Player last saw the player; Not Seeing Player walks there, and zero once that move ends. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Enemy")
	FVector PointOfInterest = FVector::ZeroVector;

	/** The point Generate Random Point drew, which Not Seeing Player walks to when there is no Point Of Interest. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy")
	FVector RandomPoint = FVector::ZeroVector;

	/**
	 * CloseBy: sent with the first Chase Player after the enemy last forgot the player (the nurse's Detected line goes
	 * with it). The Zone 1 level's Setup Nurse Bierce Quips listens.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FWasamiEnemyCloseBy OnCloseBy;

	/** Chasing: Seen Player Recently. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	virtual bool IsChasing() const { return bSeenPlayerRecently; }

	/** Can See Player: the player within ViewAngle of its front, and a Camera trace from it hits the player first. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool CanSeePlayer() const;

	/** Chase Player: sees the player, keeps where it is, runs and moves to Player Target. */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void ChasePlayer();

	/** Not Seeing Player: walks to the Point Of Interest, or else to the random point. */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void NotSeeingPlayer();

	/** Generate Random Point: a point reachable from the player (or itself) within RandomPointRadius. */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void GenerateRandomPoint();

	/** Player Target: what Chase Player moves to (the player; the Zone 2 nurse a lift when on another floor). */
	virtual AActor* GetPlayerTarget() const;

	/** Random Point Destination: where Not Seeing Player goes without a Point Of Interest. */
	virtual FVector GetRandomPointDestination() const { return RandomPoint; }

protected:
	virtual void BeginPlay() override;

	/**
	 * The nurse's ReceiveBeginPlay (and its base's): the CanSpawn check, Generate Random Point and Make Choice every
	 * DecisionInterval. BeginPlay runs it; the sentry's is empty, and its Player Spotted runs this one.
	 */
	virtual void BeginNurse();

	virtual bool Chasing_Implementation() const override { return IsChasing(); }

	virtual void SetState_Implementation(EWasamiEnemyState NewState, bool bByOrb) override;
	// The nurse's Get State always answers Patrol (it does not read State), and its No Telepathy false.
	virtual EWasamiEnemyState GetState_Implementation() const override { return EWasamiEnemyState::Patrol; }
	virtual void PlayerVanish_Implementation() override { bSeenPlayerRecently = false; }
	virtual bool NoTelepathy_Implementation() const override { return false; }

	/** Every DecisionInterval: the stun, or the chase and what it has seen, and then a chase variation's chance. */
	void MakeChoice();
	/**
	 * 追跡中のランダムの動き (the item 26): moves the chances on by a decision. A chance that has come plays its clip
	 * over the chase, at the rate that keeps its feet with the chase's speed (WasamiEnemyAnim::ChaseVariationRate), when
	 * nothing else plays once, this enemy may (CanPlayChaseVariation) and the way ahead is clear for as far as the body
	 * moves while it plays (the clips are in-place forms: the movement is never stopped, so the body carries on at the
	 * chase's speed). A chance whose way is blocked stays open; the rest are let go.
	 */
	void UpdateChaseVariation();
	/**
	 * The stun's DoOnce: stops the movement and sets State back to Patrol StunSeconds on, unless it already runs. Make
	 * Choice starts it, and the 06 nurse's tick (whose own DoOnce does the same; the first to start ends the stun).
	 */
	void StartStun();
	void EndStun();
	/** The retriggerable delay's end: Seen Player Recently false and Reset Detection. */
	void ForgetPlayer();
	/** Reset Detection: the next Chase Player sends CloseBy again. */
	void ResetDetection() { bDetectionClosed = false; }

	/** Not Seeing Player's moves: to the random point (either way, a new one), to the Point Of Interest (then zero). */
	UFUNCTION()
	void OnRandomPointMoveEnded(EPathFollowingResult::Type MovementResult);
	UFUNCTION()
	void OnPointOfInterestMoveEnded(EPathFollowingResult::Type MovementResult);

	/**
	 * The Sphere's BeginOverlap: the player, while State is not Stun, is caught, once (a DoOnce): the capture starts and
	 * every enemy is removed, this one too.
	 */
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy")
	EWasamiEnemyState State = EWasamiEnemyState::Patrol;

	/** Sphere: overlaps pawns only (Custom: the rest ignored), and catches the player. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<USphereComponent> Sphere;

	/**
	 * StaticMesh: the engine's plane in M_Enemy, 10 m over the capsule's centre at (2.52, 2.52, 10), no shadow. The nurse
	 * has it on its mesh; the Wasami mesh is scaled, so it is on the capsule where the nurse's mesh puts it.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UStaticMeshComponent> MapMark;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	TSoftObjectPtr<USkeletalMesh> MeshAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	TSoftObjectPtr<UStaticMesh> MapMarkMesh;

	/** M_Enemy. */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	TSoftObjectPtr<UMaterialInterface> MapMarkMaterial;

private:
	FTimerHandle DecisionTimer;
	FTimerHandle StunTimer;
	FTimerHandle ForgetTimer;
	bool bStunRunning = false;
	// Chase Player's DoOnce around CloseBy.
	bool bDetectionClosed = false;
	// The Sphere's DoOnce around the capture.
	bool bCatchClosed = false;
	FWasamiChaseVariations ChaseVariations;
};
