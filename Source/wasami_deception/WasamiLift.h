#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiLift.generated.h"

class UAudioComponent;
class UBoxComponent;
class USoundAttenuation;
class USoundBase;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiLiftPlayerOverlapSignature);

/**
 * What Dark Deception's two kinds of Zone 2 lift (pak_reference_2's Blueprints/06_Hospital/Lifts/Zone2) share: a floor
 * that rises Top Location above where it is placed and comes back down, with its sounds. BP_06_LiftBase_Corner is a
 * copy of BP_06_LiftBase in the original (an Actor of its own, not a child), so the two are the classes below and this
 * holds what the copies have in common.
 *
 * Every tick (ReceiveTick → Update Position), the floor (LiftMesh, which carries the boxes) moves at a constant speed
 * toward the height the class picks (GetTargetHeight): Top Location × 0.5 a second with a character on it (2 s up),
 * Top Location × 1.0 a second without (1 s). When it starts moving, the movement loop fades in over 0.5 s and the clunk
 * plays; when it stops, the loop fades out over 0.5 s and the clunk plays again (two DoOnce, the stopping one starting
 * closed).
 *
 * The meshes are not set by the class (assets under /Game/DD are never loaded from a constructor, WasamiAssets.h): the
 * level build places the lifts with the original's meshes and materials on LiftMesh and with each class's box sizes.
 */
UCLASS(Abstract)
class WASAMI_DECEPTION_API AWasamiLiftBase : public AActor
{
	GENERATED_BODY()

public:
	AWasamiLiftBase();

	/** Top Location: how far above the placed height the floor rises (both classes' default, and every placed lift's). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lift")
	float TopLocation = 535.f;

	/** IsMoving?: the floor has not reached the height it is going to. */
	UFUNCTION(BlueprintPure, Category = "Lift")
	bool IsMoving() const { return bIsMoving; }

	/** Character on Top?: a character (the player or an enemy) overlaps LiftCollisionOverlap. */
	UFUNCTION(BlueprintPure, Category = "Lift")
	bool IsCharacterOnTop() const;

	/** The floor's height over the placed one (LiftMesh's relative Z). */
	float GetHeight() const;

	/** Update Position: one step of the floor toward GetTargetHeight, then IsMoving?. Tick calls it. */
	void UpdatePosition(float DeltaSeconds);

	/** The movement loop is playing (the starting DoOnce fired last). */
	bool IsMovementSoundOn() const { return bMovementSoundOn; }

	UStaticMeshComponent* GetLiftMesh() const { return LiftMesh; }
	UBoxComponent* GetLiftCollision() const { return LiftCollision; }
	UBoxComponent* GetLiftCollisionOverlap() const { return LiftCollisionOverlap; }
	UBoxComponent* GetBottomCollision() const { return BottomCollision; }
	UBoxComponent* GetLiftCollision1() const { return LiftCollision1; }
	USceneComponent* GetMoveLocation() const { return MoveLocation; }
	UAudioComponent* GetAudio() const { return Audio; }
	UAudioComponent* GetMovementAudio() const { return MovementAudio; }

	/** The speeds, as parts of Top Location a second: with a character on the floor, and without. */
	static constexpr float SpeedWithCharacter = 0.5f;
	static constexpr float SpeedWithoutCharacter = 1.f;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Where Update Position takes the floor this tick: 0 (the placed height) or Top Location. */
	virtual float GetTargetHeight() const PURE_VIRTUAL(AWasamiLiftBase::GetTargetHeight, return 0.f;);

	UPROPERTY(VisibleAnywhere, Category = "Lift")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** LiftMesh: the floor that moves, without collision of its own (the boxes under it are its collision). */
	UPROPERTY(VisibleAnywhere, Category = "Lift")
	TObjectPtr<UStaticMeshComponent> LiftMesh;

	/** LiftCollision: the floor's solid box, sized by its scale (the level build writes each class's). */
	UPROPERTY(VisibleAnywhere, Category = "Lift")
	TObjectPtr<UBoxComponent> LiftCollision;

	/** LiftCollisionOverlap: the box just over the floor, where a character counts as on it. */
	UPROPERTY(VisibleAnywhere, Category = "Lift")
	TObjectPtr<UBoxComponent> LiftCollisionOverlap;

	/** BottomCollision: the solid box under the floor, "to prevent the player from being able to fall inside the lift". */
	UPROPERTY(VisibleAnywhere, Category = "Lift")
	TObjectPtr<UBoxComponent> BottomCollision;

	/** Move Location: a point 149.5 cm over the floor, where Zone 2's nurses walk to take the lift (AWasamiEnemyZone2). */
	UPROPERTY(VisibleAnywhere, Category = "Lift")
	TObjectPtr<USceneComponent> MoveLocation;

	/**
	 * LiftCollision1: a solid box at Top Location, where the floor stands at the top (the construction script puts it
	 * there). BP_06_Lift takes it away when play begins; the corner lift keeps it.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Lift")
	TObjectPtr<UBoxComponent> LiftCollision1;

	/** Audio: the clunk as the floor starts and stops (DD_TT_GarageLift_Down through MonkeyAttenuation). */
	UPROPERTY(VisibleAnywhere, Category = "Lift")
	TObjectPtr<UAudioComponent> Audio;

	/** MovementAudio: the loop while it moves (DD_TT_Lift_Loop at 0.7 and pitch 1.5, through 01_Lobby_Attenuation). */
	UPROPERTY(VisibleAnywhere, Category = "Lift")
	TObjectPtr<UAudioComponent> MovementAudio;

	UPROPERTY(EditAnywhere, Category = "Lift|Assets")
	TSoftObjectPtr<USoundBase> ClunkSound;

	UPROPERTY(EditAnywhere, Category = "Lift|Assets")
	TSoftObjectPtr<USoundAttenuation> ClunkAttenuation;

	UPROPERTY(EditAnywhere, Category = "Lift|Assets")
	TSoftObjectPtr<USoundBase> MovementSound;

	UPROPERTY(EditAnywhere, Category = "Lift|Assets")
	TSoftObjectPtr<USoundAttenuation> MovementAttenuation;

private:
	/** The floor's next height from Height (FInterpTo_Constant toward GetTargetHeight at the speed for Character on Top?). */
	float StepFrom(float Height, float DeltaSeconds) const;

	bool bIsMoving = false;
	bool bMovementSoundOn = false;
};

/**
 * Dark Deception's BP_06_Lift (a BP_06_LiftBase) and its children BP_06_Lift_03 (a long floor, eight in Zone 2) and
 * BP_06_Lift_04 (a wide one, two), which differ only in the mesh and the boxes' scale: the floor rises while a character
 * stands on it and comes back down when none does. LiftCollision1 is taken away when play begins.
 *
 * The player walking onto the floor (LiftCollisionOverlap's begin overlap) calls Player Overlap, which Zone 2's level
 * Blueprint binds on every lift to Bierce's remark on them (AWasamiZone2Flow's Setup Bierce Lift Quip).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiLift : public AWasamiLiftBase
{
	GENERATED_BODY()

public:
	/** Player Overlap: the player walked onto the floor. */
	UPROPERTY(BlueprintAssignable, Category = "Lift")
	FWasamiLiftPlayerOverlapSignature OnPlayerOverlap;

	/** LiftCollisionOverlap's begin overlap for that actor: Player Overlap when it is the player. The tests call it. */
	void NotifyOverlap(AActor* Other);

protected:
	virtual void BeginPlay() override;
	virtual float GetTargetHeight() const override;

private:
	UFUNCTION()
	void OnLiftCollisionOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};

/**
 * Dark Deception's BP_06_LiftBase_Corner (five in Zone 2's corners): a lift between the two floors of the maze that
 * waits on the player's floor (the top while the player stands higher than 610 cm, the bottom otherwise). The player
 * standing on it for a second (Double Check, 1 s after walking onto the actor, if still on LiftCollisionOverlap) sends it
 * to the other floor (Player Force Movement, GoUp?) with the clunk; stepping off the actor cancels that. LiftCollision1
 * stays at the top.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiCornerLift : public AWasamiLiftBase
{
	GENERATED_BODY()

public:
	/** Player Force Movement: the floor goes where GoUp? says, not to the player's floor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lift")
	bool bPlayerForceMovement = false;

	/** GoUp?: with Player Force Movement, the floor goes to the top (else to the bottom). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lift")
	bool bGoUp = false;

	/** What Floor is Player On?: the player (when there is one) stands higher than TopFloorHeight. */
	bool IsPlayerOnTopFloor() const;

	/** Double Check is waiting to run. */
	bool IsDoubleCheckPending() const;

	/** The actor's begin and end overlap (the original's ReceiveActorBeginOverlap / EndOverlap): only the player counts. */
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

	/** Double Check: the player still on LiftCollisionOverlap sends the floor to the other floor, with the clunk. */
	void DoubleCheck();

	/** What Floor is Player On?'s height, in world space, and Double Check's delay. */
	static constexpr float TopFloorHeight = 610.f;
	static constexpr float DoubleCheckDelay = 1.f;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual float GetTargetHeight() const override;

private:
	FTimerHandle DoubleCheckTimer;
};
